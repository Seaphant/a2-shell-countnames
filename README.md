# A3 — Shell with Parallel countnames and Pipe Communication

**Student name(s):** William Nguyen

## Communication Method

**Pipe** — One pipe per child process. Children write NameCountData structs to their pipe; parent reads and aggregates when each child finishes.

## How to compile

From project directory:

```
gcc -o countnames countnames.c -Wall -Werror
gcc -o shell shell.c -Wall -Werror
```

Keep countnames and shell in the same directory so the shell can find `./countnames`.

## How to run

1. Run `./shell`
2. At `shell>` prompt, type:
   - `./countnames test/names1.txt test/names2.txt`
   - or `countnames test/names1.txt test/names2.txt`
3. Parent prints aggregated name counts to **stdout** (your terminal)
4. Each child still creates `PID.out` and `PID.err` (one pair per file)
5. Type `exit` to quit

## Test cases

Run from inside the shell (`./shell` then enter commands).

### Test 1: `test/names1.txt` and `test/names2.txt`

**Command:** `./countnames test/names1.txt test/names2.txt`

**What it tests:** Two files with overlapping names; parent aggregates counts across children.

- Child 1 (names1): Tom Wu: 3
- Child 2 (names2): Jenn Xu: 2, Tom Wu: 1
- **Expected stdout from parent:**  
  Jenn Xu: 2  
  Tom Wu: 4

**Edge case:** Same name in multiple files — counts are summed.

---

### Test 2: `test/names1.txt` `test/names2.txt` `test/names2.txt`

**Command:** `./countnames test/names1.txt test/names2.txt test/names2.txt`

**What it tests:** Three files (one duplicated); three children run in parallel.

- Child 1: Tom Wu: 3
- Child 2 & 3 (same file): Jenn Xu: 2, Tom Wu: 1 each
- **Expected stdout from parent:**  
  Jenn Xu: 4  
  Tom Wu: 5

**Edge case:** Duplicate file argument — multiple children process same file.

---

### Test 3: `test/names.txt`

**Command:** `./countnames test/names.txt`

**What it tests:** Single file with multiple names and empty lines.

- **Expected stdout from parent:**  
  Dave Joe: 2  
  John Smith: 1  
  Nicky: 1  
  Yuan Cheng Chang: 3

**Edge case:** Empty lines produce warnings in `PID.err`.

---

### Test 4: `test/testm1_empty_lines.txt`

**Command:** `./countnames test/testm1_empty_lines.txt`

**What it tests:** File with empty line in middle.

- **Expected stdout from parent:**  
  Alice: 2  
  Bob: 3

**Edge case:** Empty line warning in `.err`.

---

### Test 5: `test/test_custom1_empty.txt`

**Command:** `./countnames test/test_custom1_empty.txt`

**What it tests:** Empty input file.

- **Expected stdout from parent:** (nothing — no names)

**Edge case:** Empty file — child returns 0, parent gets empty aggregation.

---

### Test 6: `test/test_custom2_single_repeated.txt`

**Command:** `./countnames test/test_custom2_single_repeated.txt`

**What it tests:** Single name repeated many times.

- **Expected stdout from parent:**  
  Alice: 5

**Edge case:** Single distinct name.

---

### Test 7: `test/test_custom3_leading_trailing_empty.txt`

**Command:** `./countnames test/test_custom3_leading_trailing_empty.txt`

**What it tests:** Empty lines at start, middle, and end.

- **Expected stdout from parent:**  
  Bob: 2  
  Carol: 2

**Edge case:** Multiple empty line warnings in `.err`.

---

### Test 8: Nonexistent file (mixed with valid files)

**Command:** `./countnames test/names1.txt test/nonexistent_xyz.txt test/names2.txt`

**What it tests:** One child fails to open file; others succeed.

- Child for `nonexistent_xyz.txt` writes error to its `.err` and exits 1
- Other two children succeed and send data
- **Expected stdout from parent:**  
  Jenn Xu: 2  
  Tom Wu: 4

**Edge case:** Partial failure — parent still aggregates from successful children.

---

### Test 9: Stdin (no args)

**Command:** `./countnames` then type names, Ctrl-D to end

**What it tests:** Single child reading from stdin.

- **Expected stdout from parent:** Counts for names you typed.

**Edge case:** Stdin mode with pipe communication.

---

## Lessons learned

- Pipes enable parent-child communication: parent creates pipe, child inherits fds, writes results, parent reads after child exits.
- One pipe per child avoids interleaved data when multiple children run in parallel.
- `wait()` returns -1 when no more children exist; loop until then to reap all.
- Use `read()` in a loop to handle partial reads from pipes.
- Children still create PID.out and PID.err; parent aggregates and prints to its stdout.

## References

- [Assignment 3 spec](https://sjsu.instructure.com/) (Canvas)
- [Fork/exec/wait slides](https://docs.google.com/presentation/d/1tFAJHE88J3ylpWa9CZ5dhcAnSy1giA-YiUEUEIJej0Q/edit?folder=1HuyG3ez13YNEHILs7ky31jL4dIPhU4Sp#slide=id.g27d9461c96e_1_127)
- `man pipe`, `man fork`, `man execvp`, `man wait`

## Acknowledgements

Course materials, assignment specification, and IPC sample code.
