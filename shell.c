#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 4096
#define MAX_ARGS 256

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

int main(void) {
    char line[MAX_LINE];

    while (1) {
        printf("shell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            // EOF (Ctrl-D)
            printf("\n");
            break;
        }

        // Tokenize
        char *args[MAX_ARGS];
        int argc = tokenize(line, args, MAX_ARGS);
        if (argc == 0) continue;

        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        // Special behavior: "countnames file1 file2 ..."
        // Spawn 1 child per file; if no files, spawn 1 child reading stdin.
        if (strcmp(args[0], "countnames") == 0 || strcmp(args[0], "./countnames") == 0) {
            int nfiles = argc - 1;
            int nchildren = (nfiles > 0) ? nfiles : 1;

            pid_t *pids = (pid_t *)calloc((size_t)nchildren, sizeof(pid_t));
            if (!pids) {
                perror("calloc");
                continue;
            }

            for (int i = 0; i < nchildren; i++) {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    // still wait for any already-forked children
                    nchildren = i;
                    break;
                }

                if (pid == 0) {
                    // child
                    if (nfiles > 0) {
                        char *cn_argv[3];
                        cn_argv[0] = (char *)"countnames";
                        cn_argv[1] = args[i + 1];  // ith file
                        cn_argv[2] = NULL;
                        execvp("./countnames", cn_argv);
                    } else {
                        // stdin mode
                        char *cn_argv[2];
                        cn_argv[0] = (char *)"countnames";
                        cn_argv[1] = NULL;
                        execvp("./countnames", cn_argv);
                    }

                    // exec only returns on error
                    perror("execvp ./countnames");
                    _exit(127);
                }

                // parent
                pids[i] = pid;
            }

            // Wait for all children (wait() returns *any* finished child)
            for (int i = 0; i < nchildren; i++) {
                int status;
                pid_t w = wait(&status);
                if (w < 0) {
                    perror("wait");
                    break;
                }

                // Optional: print child completion info
                // (You can remove this if you want the shell to be quieter.)
                if (WIFEXITED(status)) {
                    // printf("child %d exited with %d\n", (int)w, WEXITSTATUS(status));
                } else if (WIFSIGNALED(status)) {
                    // printf("child %d killed by signal %d\n", (int)w, WTERMSIG(status));
                }
            }

            free(pids);
            continue;
        }

        // Default behavior: run one command like a normal shell (single child)
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
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
        }
    }

    return 0;
}