# A3 — Shell with Parallel countnames and Pipe Communication

**Student name(s):** William Nguyen

## Communication

Used **pipe** — each child gets its own pipe, writes NameCountData structs to it, parent reads when the child exits and adds to the totals.

## Compile

From the project folder:

```
gcc -o countnames countnames.c -Wall -Werror
gcc -o shell shell.c -Wall -Werror
```

Keep countnames and shell in the same directory.

## Run

1. `./shell`
2. At the prompt type something like `./countnames test/names1.txt test/names2.txt`
3. Parent prints the combined name counts to stdout
4. Each child still makes its own PID.out and PID.err
5. `exit` to quit

## Test cases

Run these from inside the shell.

### Test 1: names1 + names2

**Command:** `./countnames test/names1.txt test/names2.txt`

Two files, Tom Wu shows up in both. Parent should add the counts.

Expected output:
```
Jenn Xu: 2
Tom Wu: 4
```

### Test 2: Three files (one duplicated)

**Command:** `./countnames test/names1.txt test/names2.txt test/names2.txt`

Same file twice = two children process it. Tests parallel + aggregation.

Expected output:
```
Jenn Xu: 4
Tom Wu: 5
```

### Test 3: Single file

**Command:** `./countnames test/names.txt`

Multiple names, some empty lines (warnings go to .err).

Expected output:
```
Dave Joe: 2
John Smith: 1
Nicky: 1
Yuan Cheng Chang: 3
```

### Test 4: Empty line in middle

**Command:** `./countnames test/testm1_empty_lines.txt`

Expected output:
```
Alice: 2
Bob: 3
```

### Test 5: Empty file

**Command:** `./countnames test/test_custom1_empty.txt`

No names, so nothing printed. Child still exits 0.

### Test 6: One name repeated

**Command:** `./countnames test/test_custom2_single_repeated.txt`

Expected output:
```
Alice: 5
```

### Test 7: Leading/trailing empty lines

**Command:** `./countnames test/test_custom3_leading_trailing_empty.txt`

Expected output:
```
Bob: 2
Carol: 2
```

### Test 8: Nonexistent file mixed in

**Command:** `./countnames test/names1.txt test/nonexistent_xyz.txt test/names2.txt`

One child fails (writes to its .err), others still run. Parent aggregates from the ones that succeed.

Expected output:
```
Jenn Xu: 2
Tom Wu: 4
```

### Test 9: Stdin

**Command:** `./countnames` then type names, Ctrl-D when done

One child reads from stdin. Output is whatever you typed.

---

## Lessons learned

- Pipes let the child send data back to the parent. Parent creates it, child inherits the fds, writes, parent reads.
- One pipe per child avoids mixing up data when they run at the same time.
- wait() returns -1 when there's no more children, so loop until that.
- read() can return partial data so you need a loop to get everything.
- PID.out and PID.err still get created per child like in A2.

## References

- Assignment spec on Canvas
- Fork/exec/wait slides: https://docs.google.com/presentation/d/1tFAJHE88J3ylpWa9CZ5dhcAnSy1giA-YiUEUEIJej0Q/edit
- man pipe, man fork, man wait

## Acknowledgements

Course materials and slides.
