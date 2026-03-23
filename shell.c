#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 4096
#define MAX_ARGS 256
#define MAX_DISTINCT 300
#define PIPE_NAME_LEN 30

typedef struct {
    char name[PIPE_NAME_LEN + 1];
    int count;
} NameCountData;

static int tokenize(char *line, char *argv[], int max_args) {
    int argc = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok && argc < max_args - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    argv[argc] = NULL;
    return argc;
}

// lookup name in our aggregated results
static int agg_find(NameCountData agg[], int n, const char *name) {
    for (int i = 0; i < n; i++)
        if (strcmp(agg[i].name, name) == 0) return i;
    return -1;
}

// add to aggregation, merge count if name already exists
static int agg_add(NameCountData agg[], int *n, const char *name, int count) {
    int idx = agg_find(agg, *n, name);
    if (idx >= 0) {
        agg[idx].count += count;
        return *n;
    }
    if (*n >= MAX_DISTINCT) return *n;
    strncpy(agg[*n].name, name, PIPE_NAME_LEN);
    agg[*n].name[PIPE_NAME_LEN] = '\0';
    agg[*n].count = count;
    (*n)++;
    return *n;
}

// keep reading until we get full len bytes (pipes can return partial data)
static ssize_t read_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t n = read(fd, (char *)buf + total, len - total);
        if (n <= 0) return total;
        total += (size_t)n;
    }
    return (ssize_t)total;
}

// read what a child sent and merge into our totals
static void read_pipe_and_aggregate(int fd, NameCountData agg[], int *nagg) {
    int num_entries;
    if (read_all(fd, &num_entries, sizeof(int)) != (ssize_t)sizeof(int)) return;

    for (int i = 0; i < num_entries; i++) {
        NameCountData data;
        if (read_all(fd, &data, sizeof(data)) != (ssize_t)sizeof(data)) break;
        agg_add(agg, nagg, data.name, data.count);
    }
}

static int cmp_agg(const void *a, const void *b) {
    const NameCountData *x = (const NameCountData *)a;
    const NameCountData *y = (const NameCountData *)b;
    return strcmp(x->name, y->name);
}

int main(void) {
    char line[MAX_LINE];

    while (1) {
        printf("shell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        char *args[MAX_ARGS];
        int argc = tokenize(line, args, MAX_ARGS);
        if (argc == 0) continue;

        if (strcmp(args[0], "exit") == 0)
            break;

        // countnames: one child per file, each writes to its own pipe
        if (strcmp(args[0], "countnames") == 0 || strcmp(args[0], "./countnames") == 0) {
            int nfiles = argc - 1;
            int nchildren = (nfiles > 0) ? nfiles : 1;

            int (*pipes)[2] = (int (*)[2])malloc((size_t)nchildren * sizeof(int[2]));
            pid_t *pids = (pid_t *)calloc((size_t)nchildren, sizeof(pid_t));
            if (!pipes || !pids) {
                perror("malloc/calloc");
                free(pipes);
                free(pids);
                continue;
            }

            int pipe_ok = 1;
            for (int i = 0; i < nchildren; i++) {
                if (pipe(pipes[i]) < 0) {  // one pipe per child
                    perror("pipe");
                    for (int j = 0; j < i; j++) { close(pipes[j][0]); close(pipes[j][1]); }
                    free(pipes);
                    free(pids);
                    pipe_ok = 0;
                    break;
                }
            }
            if (!pipe_ok) continue;

            int n_pipes = nchildren;  // save this in case fork fails partway
            for (int i = 0; i < nchildren; i++) {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    nchildren = i;
                    break;
                }

                if (pid == 0) {
                    // child: close read ends, keep only our write end
                    for (int j = 0; j < nchildren; j++) {
                        close(pipes[j][0]);
                        if (j != i) close(pipes[j][1]);
                    }
                    char fd_str[16];
                    snprintf(fd_str, sizeof(fd_str), "%d", pipes[i][1]);

                    if (nfiles > 0) {
                        char *cn_argv[4];
                        cn_argv[0] = (char *)"countnames";
                        cn_argv[1] = args[i + 1];
                        cn_argv[2] = fd_str;
                        cn_argv[3] = NULL;
                        execvp("./countnames", cn_argv);
                    } else {
                        char *cn_argv[3];
                        cn_argv[0] = (char *)"countnames";
                        cn_argv[1] = fd_str;
                        cn_argv[2] = NULL;
                        execvp("./countnames", cn_argv);
                    }
                    perror("execvp ./countnames");
                    _exit(127);
                }
                pids[i] = pid;
            }

            // close our copies of write ends so we see EOF when kids exit
            for (int i = 0; i < n_pipes; i++)
                close(pipes[i][1]);

            // collect results from each child as they finish
            NameCountData agg[MAX_DISTINCT];
            int nagg = 0;
            memset(agg, 0, sizeof(agg));

            // wait returns -1 when no more kids left
            int status;
            pid_t w;
            while ((w = wait(&status)) != (pid_t)-1) {
                for (int i = 0; i < nchildren; i++) {
                    if (pids[i] == w) {  // found which child finished
                        read_pipe_and_aggregate(pipes[i][0], agg, &nagg);
                        break;
                    }
                }
            }

            // print combined totals
            if (nagg > 0) {
                qsort(agg, (size_t)nagg, sizeof(NameCountData), cmp_agg);
                for (int i = 0; i < nagg; i++)
                    printf("%s: %d\n", agg[i].name, agg[i].count);
            }

            // cleanup
            for (int i = 0; i < n_pipes; i++)
                close(pipes[i][0]);
            free(pipes);
            free(pids);
            continue;
        }

        // regular command
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }
        if (pid == 0) {
            execvp(args[0], args);
            perror("execvp");
            _exit(127);
        }
        int status;
        if (waitpid(pid, &status, 0) < 0)
            perror("waitpid");
    }
    return 0;
}
