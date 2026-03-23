#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_DISTINCT 100
#define MAX_NAME_LEN 30
#define BUF_SIZE 1024
#define PIPE_NAME_LEN 30

typedef struct {
    char name[MAX_NAME_LEN + 1];
    int count;
} NameCount;

/* Pipe protocol: used for parent-child communication (A3) */
typedef struct {
    char name[PIPE_NAME_LEN + 1];
    int count;
} NameCountData;

// strip newline/cr from end
static void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
        len--;
    }
    if (len > 0 && s[len - 1] == '\r')
        s[len - 1] = '\0';
}

static int find_name(NameCount arr[], int n, const char *name) {
    for (int i = 0; i < n; i++)
        if (strcmp(arr[i].name, name) == 0) return i;
    return -1;
}

static int cmp_namecount(const void *a, const void *b) {
    const NameCount *x = (const NameCount *)a;
    const NameCount *y = (const NameCount *)b;
    return strcmp(x->name, y->name);
}

static void warn_empty(const char *filename, int lineNo) {
    if (filename != NULL)
        fprintf(stderr, "Warning - file %s line %d is empty.\n", filename, lineNo);
    else
        fprintf(stderr, "Warning - line %d is empty.\n", lineNo);
}

static void process_stream(FILE *fp, const char *filename,
                          NameCount counts[], int *numDistinct) {
    char buf[BUF_SIZE];
    int lineNo = 0;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        lineNo++;
        trim_newline(buf);

        if (strlen(buf) == 0) {
            warn_empty(filename, lineNo);
            continue;
        }

        if (strlen(buf) > MAX_NAME_LEN)
            buf[MAX_NAME_LEN] = '\0';

        int idx = find_name(counts, *numDistinct, buf);
        if (idx >= 0) {
            counts[idx].count++;
        } else if (*numDistinct < MAX_DISTINCT) {
            strcpy(counts[*numDistinct].name, buf);
            counts[*numDistinct].count = 1;
            (*numDistinct)++;
        }
    }
}

/* Write name counts to pipe for parent aggregation (A3) */
static void write_results_to_pipe(int pipe_fd, NameCount counts[], int numDistinct) {
    ssize_t nw;
    nw = write(pipe_fd, &numDistinct, sizeof(int));
    if (nw != (ssize_t)sizeof(int)) return;

    for (int i = 0; i < numDistinct; i++) {
        NameCountData data;
        memset(&data, 0, sizeof(data));
        strncpy(data.name, counts[i].name, PIPE_NAME_LEN);
        data.name[PIPE_NAME_LEN] = '\0';
        data.count = counts[i].count;
        nw = write(pipe_fd, &data, sizeof(data));
        if (nw != (ssize_t)sizeof(data)) return;
    }
}

// redirect stdout/stderr to PID.out and PID.err
static int redirect_to_pid_files(void) {
    pid_t pid = getpid();
    char outname[64], errname[64];

    snprintf(outname, sizeof(outname), "%d.out", (int)pid);
    snprintf(errname, sizeof(errname), "%d.err", (int)pid);

    if (!freopen(outname, "w", stdout)) return 1;
    if (!freopen(errname, "w", stderr)) return 1;

    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    return 0;
}

int main(int argc, char *argv[]) {
    int pipe_fd = -1;
    int file_start = 1, file_end = argc;

    /* A3: when invoked by shell with pipe, argv is: countnames file pipe_fd */
    if (argc >= 3) {
        pipe_fd = atoi(argv[argc - 1]);
        file_end = argc - 1;  /* last arg is pipe fd, not a file */
    }

    if (redirect_to_pid_files() != 0) {
        perror("redirect_to_pid_files");
        return 1;
    }

    NameCount counts[MAX_DISTINCT];
    int numDistinct = 0;

    if (file_start >= file_end) {
        process_stream(stdin, NULL, counts, &numDistinct);
    } else {
        for (int i = file_start; i < file_end; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (fp == NULL) {
                fprintf(stderr, "error: cannot open file %s\n", argv[i]);
                if (pipe_fd >= 0) close(pipe_fd);
                return 1;
            }
            process_stream(fp, argv[i], counts, &numDistinct);
            fclose(fp);
        }
    }

    qsort(counts, numDistinct, sizeof(NameCount), cmp_namecount);

    /* A3: write to pipe for parent aggregation (even if empty, so parent doesn't block) */
    if (pipe_fd >= 0) {
        write_results_to_pipe(pipe_fd, counts, numDistinct);
        close(pipe_fd);
    }

    for (int i = 0; i < numDistinct; i++)
        printf("%s: %d\n", counts[i].name, counts[i].count);

    return 0;
}
