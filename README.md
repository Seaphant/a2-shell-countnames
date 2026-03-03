# A2 — Shell with Parallel countnames

**Student name(s):** William Nguyen

---

## How to compile

From the project directory (where `shell.c` and `countnames.c` are):

```bash
gcc -o countnames countnames.c -Wall -Werror
gcc -o shell shell.c -Wall -Werror
```

This produces the executables `countnames` and `shell`. Both must be in the same directory so that the shell can run `./countnames`.

---

## How to run

1. Start the shell:
   ```bash
   ./shell
   ```

2. At the `shell>` prompt, run countnames with one or more files:
   ```bash
   ./countnames test/names1.txt test/names2.txt
   ```
   or
   ```bash
   countnames test/names1.txt test/names2.txt
   ```

3. Each child process writes its output to `PID.out` and `PID.err` (where PID is the child's process id). The number of `.out`/`.err` files equals the number of input files.

4. Type `exit` to quit the shell.

---

## Expected output for each test case

For all tests, run from within the shell: `./countnames test/<filename> ...` or with multiple files. Output appears in `PID.out` and `PID.err`; PIDs vary per run.

---

### Given test: `test/names1.txt` and `test/names2.txt`

**What it tests:** Two files processed in parallel; two child processes; empty-line warning.

**Command (from shell):** `./countnames test/names1.txt test/names2.txt`

**Expected:** Two pairs of files (e.g. `12345.out`, `12345.err`, `12346.out`, `12346.err`).

- **File 1 (names1.txt):**  
  - `.out`: `Tom Wu: 3`  
  - `.err`: `Warning - file test/names1.txt line 3 is empty.`

- **File 2 (names2.txt):**  
  - `.out`: `Jenn Xu: 2` and `Tom Wu: 1`  
  - `.err`: (empty)

**Exit code:** 0 for both children

---

### Given test: `test/names1.txt` `test/names2.txt` `test/names2.txt`

**What it tests:** Duplicate filename on command line; same file processed multiple times by separate children.

**Command (from shell):** `./countnames test/names1.txt test/names2.txt test/names2.txt`

**Expected:** Three pairs of PID.out/PID.err files. Two children process names2.txt (each produces same counts: Jenn Xu: 2, Tom Wu: 1); one child processes names1.txt (Tom Wu: 3).

**Exit code:** 0 for all children

---

### Given test: `test/names.txt`

**What it tests:** Multiple names, duplicates, empty lines; stderr warnings.

**Command (from shell):** `./countnames test/names.txt`

**Expected (in one PID.out):**
```
Dave Joe: 2
John Smith: 1
Nicky: 1
Yuan Cheng Chang: 3
```

**Expected (in same PID.err):**
```
Warning - file test/names.txt line 2 is empty.
Warning - file test/names.txt line 5 is empty.
```

**Exit code:** 0

---

### Given test: `test/testm1_empty_lines.txt`

**What it tests:** Empty lines in the middle of a file; per-line warnings.

**Command (from shell):** `./countnames test/testm1_empty_lines.txt`

**Expected (in PID.out):**
```
Alice: 2
Bob: 3
```

**Expected (in PID.err):**
```
Warning - file test/testm1_empty_lines.txt line 3 is empty.
```

**Exit code:** 0

---

### Custom test 1: `test/test_custom1_empty.txt`

**What it tests:** Completely empty file; no names to count.

**Command (from shell):** `./countnames test/test_custom1_empty.txt`

**Expected:** One PID.out (empty or no name lines), one PID.err (empty). Program exits 0.

**Exit code:** 0

---

### Custom test 2: `test/test_custom2_single_repeated.txt`

**What it tests:** Single distinct name repeated multiple times; trailing empty line.

**Command (from shell):** `./countnames test/test_custom2_single_repeated.txt`

**Expected (in PID.out):**
```
Alice: 5
```

**Expected (in PID.err):**
```
Warning - file test/test_custom2_single_repeated.txt line 6 is empty.
```

**Exit code:** 0

---

### Custom test 3: `test/test_custom3_leading_trailing_empty.txt`

**What it tests:** Empty lines at start and end of file; names in the middle.

**Command (from shell):** `./countnames test/test_custom3_leading_trailing_empty.txt`

**Expected (in PID.out):**
```
Bob: 2
Carol: 2
```

**Expected (in PID.err):**
```
Warning - file test/test_custom3_leading_trailing_empty.txt line 1 is empty.
Warning - file test/test_custom3_leading_trailing_empty.txt line 4 is empty.
Warning - file test/test_custom3_leading_trailing_empty.txt line 7 is empty.
```

**Exit code:** 0

---

### Error case: nonexistent file

**What it tests:** Child cannot open file; error written to PID.err; exit 1. Other children still succeed.

**Command (from shell):** `./countnames test/names1.txt test/nonexistent_xyz.txt test/names2.txt`

**Expected:** Three children. The child for `test/nonexistent_xyz.txt` writes to its PID.err:
```
error: cannot open file test/nonexistent_xyz.txt
```
and exits with code 1. The other two children complete normally with exit 0.

---

### Stdin mode (no filenames)

**What it tests:** countnames with no arguments reads from stdin; one child spawned.

**Command (from shell):**  
First run: `./countnames`  
Then type (or pipe) names and press Ctrl-D to signal EOF.

Or: `echo -e "Alice\nBob\nAlice" | ./shell` and type `./countnames` then paste input and Ctrl-D.

**Expected:** One PID.out with counts; one PID.err for any empty-line warnings. Exit 0.

---

# Lessons learned

- `fork()` creates a copy of the process; the child can use `execvp()` to replace its image with another program (e.g. countnames).
- To avoid bottlenecks: fork all children first, then call `wait()` in a loop. Children run in parallel; the parent waits for any to finish.
- Each child gets its own PID; redirecting stdout/stderr to `PID.out` and `PID.err` via `freopen()` lets each child write to unique files.
- `wait()` returns when any child exits; we must call it once per forked child to avoid zombies.
- Exit codes and signals: `WIFEXITED(status)` and `WEXITSTATUS(status)` decode how a child terminated.

---

# References

- Course A2 assignment description and Canvas/slides (processes, fork, exec, wait).
- [Process creation and waiting (slides)](https://docs.google.com/presentation/d/1tFAJHE88J3ylpWa9CZ5dhcAnSy1giA-YiUEUEIJej0Q/edit?folder=1HuyG3ez13YNEHILs7ky31jL4dIPhU4Sp#slide=id.g27d9461c96e_1_127)
- [strtok for line parsing (slides)](https://docs.google.com/presentation/d/11fwG5voPoGm-1KZAQ-JGaw1uSSSXVL5V8j7dFOuQ2E8/edit?slide=id.g278e77643e5_0_314)
- GNU parallel: https://www.gnu.org/software/parallel/
- man pages: fork(2), execvp(3), wait(2)

---

# Acknowledgements

- Course materials and embedded tutor.
- Class time and/or Discord.
- No direct help from others; used course materials and man pages.
