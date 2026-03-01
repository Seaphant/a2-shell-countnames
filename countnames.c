#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_DISTINCT 100
#define MAX_NAME_LEN 30
#define BUF_SIZE 1024

typedef struct {
    char name[MAX_NAME_LEN + 1];
    int count;
} NameCount;

static void trim_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
        len--;
    }
    if (len > 0 && s[len - 1] == '\r') {
        s[len - 1] = '\0';
    }
}

static int find_name(NameCount arr[], int n, const char *name) {
    for (int i = 0; i < n; i++) {
        if (strcmp(arr[i].name, name) == 0) return i;
    }
    return -1;
}

static int cmp_namecount(const void *a, const void *b) {
    const NameCount *x = (const NameCount *)a;
    const NameCount *y = (const NameCount *)b;
    return strcmp(x->name, y->name);
}

static void warn_empty(const char *filename, int lineNo) {
    if (filename != NULL) {
        fprintf(stderr, "Warning - file %s line %d is empty.\n", filename, lineNo);
    } else {
        fprintf(stderr, "Warning - line %d is empty.\n", lineNo);
    }
}

static void process_stream(FILE *fp, const char *filename,
                           NameCount counts[], int *numDistinct) {
    char buf[BUF_SIZE];
    int lineNo = 0;

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        lineNo++;
        trim_newline(buf);

        // Empty line only (spaces count as a name)
        if (strlen(buf) == 0) {
            warn_empty(filename, lineNo);
            continue;
        }

        // truncate to 30 chars
        if (strlen(buf) > MAX_NAME_LEN) {
            buf[MAX_NAME_LEN] = '\0';
        }

        int idx = find_name(counts, *numDistinct, buf);
        if (idx >= 0) {
            counts[idx].count++;
        } else if (*numDistinct < MAX_DISTINCT) {
            strcpy(counts[*numDistinct].name, buf);
            counts[*numDistinct].count = 1;
            (*numDistinct)++;
        }
        // If > MAX_DISTINCT distinct names appear, extra distinct names are ignored.
    }
}

static int redirect_to_pid_files(void) {
    pid_t pid = getpid();
    char outname[64], errname[64];

    snprintf(outname, sizeof(outname), "%d.out", (int)pid);
    snprintf(errname, sizeof(errname), "%d.err", (int)pid);

    FILE *out = freopen(outname, "w", stdout);
    if (!out) return 1;

    FILE *err = freopen(errname, "w", stderr);
    if (!err) return 1;

    // Optional: line-buffer stdout for nicer output while debugging
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    return 0;
}

int main(int argc, char *argv[]) {
    if (redirect_to_pid_files() != 0) {
        // If we can't open pid files, last resort:
        perror("redirect_to_pid_files");
        return 1;
    }

    NameCount counts[MAX_DISTINCT];
    int numDistinct = 0;

    if (argc == 1) {
        // stdin mode (still writes to PID.out / PID.err)
        process_stream(stdin, NULL, counts, &numDistinct);
    } else {
        // file mode (shell passes one filename per child; process_stream resets lineNo per call)
        for (int i = 1; i < argc; i++) {
            FILE *fp = fopen(argv[i], "r");
            if (fp == NULL) {
                fprintf(stderr, "error: cannot open file %s\n", argv[i]);
                return 1;
            }
            process_stream(fp, argv[i], counts, &numDistinct);
            fclose(fp);
        }
    }

    if (numDistinct == 0) return 0;

    qsort(counts, numDistinct, sizeof(NameCount), cmp_namecount);

    for (int i = 0; i < numDistinct; i++) {
        // format doesn’t matter as long as consistent; keeping your "name: count"
        printf("%s: %d\n", counts[i].name, counts[i].count);
    }

    return 0;
}