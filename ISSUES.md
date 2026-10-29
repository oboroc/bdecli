Prompt for Claude Opus 4.6: do security audit of C code

Output:

I'll start by examining the project structure and reading the C source files.

Listed directory bdecli
Viewed bdecli.c:1-800
Viewed bdecli.c:800-828
Listed directory src
Viewed compat_s.h:1-52

Now I have a complete picture of the codebase. Let me write up the security findings.

Created security_audit.md

I found **10 security issues** across [bdecli.c](file:///d:/GitHub/bdecli/src/bdecli.c) and [compat_s.h](file:///d:/GitHub/bdecli/src/compat_s.h). The full report is in the artifact above. Here are the most critical ones:

** High — fix first:**

1. **Buffer overflow in `bde_fgets()`** (line 63) — writes to `buf[r_len]` before the bounds check, allowing a 1-byte stack overflow with crafted input.
2. **Off-by-one in `bde_cfg_add_entry()`** (line 529) — `i > MAX_BUFFER` should be `i >= MAX_BUFFER`, allowing out-of-bounds write.
3. **compat_s.h silently drops all size parameters** — on GCC/Clang/Watcom builds, every `strcpy_s` call is just `strcpy` with no bounds checking.

** Medium — should fix:**

4. BDE binary files opened in **text mode** (`"r"` / `"w"`) — causes silent data corruption on Windows from CR/LF translation and Ctrl-Z EOF.
5. Unchecked `NULL` dereference in `bde_fqn()` if a container entry has no name.

Let me know if you'd like me to prepare fixes for any of these.


2026-10-29 Code review using Gemini 4 Argon High:

Analyzing bdecli.c
Analyzing compat_s.h
Ran list files in workspace
Analyzed compat_s.h:L1-51
Analyzed bdecli.c:L1-800
Analyzed bdecli.c:L795-826
Analyzed ISSUES.md:L1-28

Here is a security and reliability review of [bdecli.c](file:///d:/GitHub/bdecli/src/bdecli.c) and [compat_s.h](file:///d:/GitHub/bdecli/src/compat_s.h), organized by severity.

---

### High Severity

#### 1. Off-by-One / Off-by-Two Stack Buffer Overflow in [`bde_cfg_add_entry()`](file:///d:/GitHub/bdecli/src/bdecli.c#L511-L540) (CWE-193, CWE-787)
In [`bde_cfg_add_entry()`](file:///d:/GitHub/bdecli/src/bdecli.c#L511), `buf` is a 1000-byte stack buffer (`char buf[MAX_BUFFER]`, valid indices `0..999`), but the bounds check at [line 528](file:///d:/GitHub/bdecli/src/bdecli.c#L528) uses `>` instead of `>=`:
```c
if (('\\' == tmp[i]) || (0 == tmp[i + 1])) {
	if (i > MAX_BUFFER) {
		/* sanitize possible buffer overflow */
		fprintf_s(stderr, "Error in bde_cfg_add_entry(): string buffer is too small");
		exit(1);
	}

	if ('\\' == tmp[i]) {
		strncpy_s(buf, MAX_BUFFER, tmp, i);
		buf[i] = 0;
	}
	else
		strcpy_s(buf, MAX_BUFFER, tmp);
```
* **Why it is vulnerable:**
  1. **When `'\\' == tmp[i]` and `i == MAX_BUFFER` (`1000`):** `i > MAX_BUFFER` (`1000 > 1000`) evaluates to `false`. When compiled with [compat_s.h](file:///d:/GitHub/bdecli/src/compat_s.h#L22), `strncpy_s` degrades to `strncpy(buf, tmp, 1000)` and `buf[i] = 0` at [line 536](file:///d:/GitHub/bdecli/src/bdecli.c#L536) writes a null byte to `buf[1000]`—**1 byte past the end of the stack buffer**. On MSVC, `strncpy_s` detects that `count == MAX_BUFFER` leaves no space for a null terminator and aborts the process via the invalid parameter handler.
  2. **When `'\\' != tmp[i]` and `0 == tmp[i + 1]` (the final path segment):** `tmp` has length `i + 1`, requiring `i + 2` bytes in `buf` (including the null terminator). When `i == MAX_BUFFER - 1` (`999`) or `i == MAX_BUFFER` (`1000`), `i > MAX_BUFFER` is `false`, and `strcpy_s(buf, MAX_BUFFER, tmp)` at [line 539](file:///d:/GitHub/bdecli/src/bdecli.c#L539) copies 1001 or 1002 bytes into the 1000-byte stack buffer under `compat_s.h` (or aborts on MSVC).
* **Fix:** Check `if (i >= MAX_BUFFER - 1)` when `0 == tmp[i + 1]` (or uniformly `if (i >= MAX_BUFFER - 1)` before copying into `buf`).

---

#### 2. NULL Pointer Dereference and Use-After-Free on Empty `.cfg` Files in [`bde_cfg_parse()`](file:///d:/GitHub/bdecli/src/bdecli.c#L122-L235) (CWE-476, CWE-416)
At the start of [`bde_cfg_parse()`](file:///d:/GitHub/bdecli/src/bdecli.c#L122-L123), the initial list node is preallocated before reading the file:
```c
start = bde_new_entry(NULL, NULL);
current = start;
...
ch = fgetc(f);

while (!feof(f)) {
    ...
}

if (!current->name) {
	/* sanitize last preallocated linked list entry */
	current->previous->next = NULL;	/* set the next field of previous linked list entry to null */
	free(current);
}
...
return start;
```
* **Why it is vulnerable:**
  * If `szFileName` is an empty (0-byte) file, `fgetc(f)` at [line 132](file:///d:/GitHub/bdecli/src/bdecli.c#L132) immediately hits `EOF`, skipping the `while (!feof(f))` loop.
  * At [line 225](file:///d:/GitHub/bdecli/src/bdecli.c#L225), `current` is still `start`, so `current->name` is `NULL` and `current->previous` is `NULL`.
  * Executing `current->previous->next = NULL` at [line 227](file:///d:/GitHub/bdecli/src/bdecli.c#L227) causes an immediate **NULL pointer dereference** crash.
  * Even if `current->previous` were checked before dereferencing, [line 228](file:///d:/GitHub/bdecli/src/bdecli.c#L228) calls `free(current)` (which frees `start`), and [line 235](file:///d:/GitHub/bdecli/src/bdecli.c#L235) executes `return start;`—returning a **dangling pointer to freed heap memory (Use-After-Free)** to `main()`, which then passes it to `bde_cfg_update()`, `bde_cfg_export()`, and `bde_cfg_free()`.
* **Fix:** If `current == start` when `!current->name`, free `start`, close `f`, and either return `NULL` (and handle empty lists in callers) or abort with an error stating the configuration file is empty.

---

#### 3. Bounds Checking Silently Stripped by Macros in [`compat_s.h`](file:///d:/GitHub/bdecli/src/compat_s.h#L14-L34) (CWE-120, CWE-676)
When compiled with GCC, MinGW, Clang (where `__GNUC__` is defined), Watcom, Digital Mars, or MSVC < 2005, [`compat_s.h`](file:///d:/GitHub/bdecli/src/compat_s.h#L18-L30) maps C11 Annex K / MSVC Secure CRT functions directly to unbounded standard library functions:
```c
#define fopen_s(FPP, FNAME, FMODE)	((int)((void *)(NULL) == (void *)((*FPP = fopen(FNAME, FMODE)))))
#define strcpy_s(DESTSTR, MAXLEN, SRCSTR)	(strcpy(DESTSTR, SRCSTR))
#define strncpy_s(DESTSTR, MAXLEN, SRCSTR, NUM)	(strncpy(DESTSTR, SRCSTR, NUM))
#define strcat_s(DESTSTR, MAXLEN, SRCSTR)	(strcat(DESTSTR, SRCSTR))
#define strtok_s(TOK, DELIM, NEXT)	(strtok(TOK, DELIM))
/* most likely this won't work for multiple arguments */
#define sprintf_s(DESTSTR, DESTSIZE, FORMAT, ARGS)	(sprintf(DESTSTR, FORMAT, ARGS))
```
* **Why it is vulnerable:**
  1. **Silent removal of buffer size limits:** `MAXLEN` and `DESTSIZE` are completely ignored in `strcpy_s`, `strncpy_s`, `strcat_s`, and `sprintf_s`, creating a false sense of security when auditing callers in `bdecli.c`.
  2. **Missing null-termination in `strncpy_s`:** Real `strncpy_s` always null-terminates `DESTSTR`. Standard `strncpy` does **not** null-terminate if `NUM` bytes are copied without encountering a `\0`.
  3. **Thread-unsafe `strtok_s`:** Mapping `strtok_s(TOK, DELIM, NEXT)` to `strtok(TOK, DELIM)` discards the `NEXT` context pointer and uses global static state (unlike POSIX `strtok_r`).
  4. **Incompatible return values:** `strcpy_s`, `strncpy_s`, and `strcat_s` return `errno_t` (`0` on success), whereas `strcpy`, `strncpy`, and `strcat` return `char*` (a non-zero pointer on success). Any caller checking `if (strcpy_s(...) != 0)` will break on non-MSVC builds.
  5. **Macro hygiene in `fopen_s`:** `*FPP` is not parenthesized (`*(FPP)`), and passing `NULL` for `FPP` causes a NULL pointer dereference.
* **Fix:** Implement small `static inline` wrapper functions in `compat_s.h` that enforce `MAXLEN` / `DESTSIZE` (e.g., using `snprintf` / `strnlen` + `memcpy`), guarantee null-termination, and return `0` on success or non-zero on error.

---

### Medium Severity

#### 4. Missing `EOF` / `ferror()` Checks Causing Infinite Loops or Corrupt Reads in [`bde_fgets()`](file:///d:/GitHub/bdecli/src/bdecli.c#L42-L91) and [`bde_cfg_parse()`](file:///d:/GitHub/bdecli/src/bdecli.c#L132-L223) (CWE-252, CWE-835)
Both [`bde_fgets()`](file:///d:/GitHub/bdecli/src/bdecli.c#L59-L78) and [`bde_cfg_parse()`](file:///d:/GitHub/bdecli/src/bdecli.c#L134-L223) rely on `feof(f)` instead of checking whether `fgetc(f)` returned `EOF` (`-1`) or `ferror(f)` occurred:
```c
/* parse the end of line */
do {
	ch = fgetc(f);
	if (BDE_EOB == ch) {
		...
	}
} while ((!feof(f)) && (BDE_NEW != ch));
```
* **Why it is vulnerable:**
  * `fgetc(f)` returns `EOF` (`-1`) both on end-of-file **and** on I/O read errors (e.g., disk error, lost network share, or reading a directory descriptor). When a read error occurs, `ferror(f)` is set, while `feof(f)` remains `0` (`false`).
  * In [`bde_cfg_parse()` lines 206–222](file:///d:/GitHub/bdecli/src/bdecli.c#L206-L222), if a read error occurs, `ch` is `EOF` (`-1 != BDE_NEW`) and `!feof(f)` remains `true` forever, causing an **infinite loop (Denial of Service)**.
  * In [`bde_fgets()` lines 61–78](file:///d:/GitHub/bdecli/src/bdecli.c#L61-L78), `ch = fgetc(f)` is cast to `(char)ch` (`0xFF`) and written to `buf[r_len]` before `feof(f)` is checked. On a read error, `bde_fgets()` fills `buf` with `0xFF` until `r_len == MAX_BUFFER` and exits with a misleading `"buffer overflow in string parser"` error.
  * *(Note on [ISSUES.md](file:///d:/GitHub/bdecli/ISSUES.md#L20): `ISSUES.md` previously claimed `bde_fgets()` has a 1-byte stack overflow at line 62. Tracing `r_len` shows it starts at `0`, writes `buf[999]` on its 1000th iteration, increments `r_len` to `1000` (`MAX_BUFFER`), and immediately exits at `if (MAX_BUFFER == r_len)` on [line 67](file:///d:/GitHub/bdecli/src/bdecli.c#L67) before `buf[1000]` is ever accessed. However, checking `ch == EOF` before writing `buf[r_len]` is still required.)*
* **Fix:** Check `if (EOF == ch)` immediately after every `fgetc(f)` call instead of looping on `!feof(f)`.

---

#### 5. Integer Truncation (`size_t` to `int`) and Potential Heap Buffer Underflow in [`bde_fqn()`](file:///d:/GitHub/bdecli/src/bdecli.c#L239-L288) (CWE-190, CWE-681, CWE-787)
In [`bde_fqn()`](file:///d:/GitHub/bdecli/src/bdecli.c#L242-L285), `str_len` is `size_t` (64-bit unsigned on x64), while the write cursor `str_pos` and read cursor `cur_pos` are signed 32-bit `int`:
```c
size_t str_len;
int str_pos;
...
str = (char*)malloc(str_len + 1);
...
str_pos = (int)(str_len - 1);
while (current) {
	int cur_pos;
	if (current != entry) {
		str[str_pos] = '\\';	/* add delimiter */
		str_pos--;
	}
	cur_pos = (int)(strlen(current->name) - 1);
	while (0 <= cur_pos) {
		str[str_pos] = current->name[cur_pos];
		str_pos--;
		cur_pos--;
	}
	current = current->container;
}
```
* **Why it is vulnerable:**
  * On 64-bit builds, if a crafted `.cfg` file nests containers deeply enough that `str_len > INT_MAX` (~2 GB total path length), `(int)(str_len - 1)` at [line 267](file:///d:/GitHub/bdecli/src/bdecli.c#L267) truncates to a **negative number**, causing `str[str_pos]` to write **out-of-bounds before the start of the heap buffer `str`**.
  * Additionally, `strlen(current->name)` at [line 248](file:///d:/GitHub/bdecli/src/bdecli.c#L248) does not check if `current->name` is `NULL`, which would cause a NULL pointer dereference if any container node lacked a name.
* **Fix:** Use `size_t` for all lengths and indices (or guard against `str_len > INT_MAX`), and validate `current->name != NULL`.

---

#### 6. Silent Line Truncation and Configuration Injection in [`bde_cfg_update()`](file:///d:/GitHub/bdecli/src/bdecli.c#L690-L763) (CWE-20)
In [`bde_cfg_update()`](file:///d:/GitHub/bdecli/src/bdecli.c#L690):
```c
if (!fgets(buf, MAX_BUFFER - 1, f)) {
```
* **Why it is vulnerable:**
  * If a line in the input text file is longer than `MAX_BUFFER - 2` (998) bytes, `fgets()` fills `buf` with the first 998 characters and leaves the rest of the line in the stream `f` without raising an error.
  * The first 998 characters are parsed with a **silently truncated value**.
  * On the next iteration of `while (!feof(f))`, the leftover tail of the line is read as a **new line**. If the tail contains `\` and `=`, `bde_cfg_update()` will parse and inject an unintended configuration variable into the `.cfg` file.
* **Fix:** Check if the line read by `fgets()` contains a newline (`\n`) before EOF; if not, report a line-too-long error (or dynamically grow the line buffer) rather than parsing a split line.

---

#### 7. Binary `.cfg` Files Opened in Text Mode (`"r"` / `"w"`) Instead of Binary Mode (`"rb"` / `"wb"`) in [`bde_cfg_parse()`](file:///d:/GitHub/bdecli/src/bdecli.c#L125) and [`bde_cfg_write()`](file:///d:/GitHub/bdecli/src/bdecli.c#L384)
* [`bde_cfg_parse()` line 125](file:///d:/GitHub/bdecli/src/bdecli.c#L125): `err = fopen_s(&f, szFileName, "r");`
* [`bde_cfg_write()` line 384](file:///d:/GitHub/bdecli/src/bdecli.c#L384): `err = fopen_s(&f, szFileName, "w");`
* **Why it is an issue:**
  * `idapi32.cfg` is a binary file using raw byte delimiters (`0x00`–`0x04`).
  * On Windows, opening a binary file in text mode (`"r"` / `"w"`) causes the CRT to treat `0x1A` (Ctrl-Z) as `EOF` on read (prematurely truncating parsing), collapse `0x0D 0x0A` to `0x0A` on read, and expand any `0x0A` byte to `0x0D 0x0A` on write—**corrupting `idapi32.cfg`**.
* **Fix:** Use `"rb"` in [`bde_cfg_parse()`](file:///d:/GitHub/bdecli/src/bdecli.c#L125) and `"wb"` in [`bde_cfg_write()`](file:///d:/GitHub/bdecli/src/bdecli.c#L384).

---

### Low Severity & Reliability / Logic Issues

#### 8. Destructive In-Place Overwrite and Unchecked Write Errors in [`bde_cfg_write()`](file:///d:/GitHub/bdecli/src/bdecli.c#L376-L489) (CWE-252)
* In `main()` ([line 805](file:///d:/GitHub/bdecli/src/bdecli.c#L805)), `bde_cfg_write(cfg, argv[2])` opens the original `idapi32.cfg` with `"w"`, immediately truncating it to 0 bytes before writing.
* Return values of `fputc()` and `fputs()` are never checked in [`bde_cfg_write()`](file:///d:/GitHub/bdecli/src/bdecli.c#L391-L484) or [`bde_fputs()`](file:///d:/GitHub/bdecli/src/bdecli.c#L369-L371), and if the `default:` branch at [line 428](file:///d:/GitHub/bdecli/src/bdecli.c#L428) triggers `exit(1)`, the user's configuration file is left truncated/corrupted.
* **Fix:** Write to a temporary file, check `ferror(f)` / `fflush(f)` / `fclose(f)`, and atomically replace the destination file.

#### 9. File Handle Leak on Parse Failure in [`bde_cfg_update()`](file:///d:/GitHub/bdecli/src/bdecli.c#L742-L745) (CWE-775)
* At [`bdecli.c:742-745`](file:///d:/GitHub/bdecli/src/bdecli.c#L742-L745), if `'\\' != szFQNPath[i]`, `bde_cfg_update()` returns `0` without calling `fclose(f)`.

#### 10. Blank Line Failure, Empty Container Names, and Duplicate Variable Insertion in [`bde_cfg_update()`](file:///d:/GitHub/bdecli/src/bdecli.c#L704-L748) and [`bde_cfg_add_entry()`](file:///d:/GitHub/bdecli/src/bdecli.c#L526-L638)
* **Blank lines abort execution:** At [`bdecli.c:704-709`](file:///d:/GitHub/bdecli/src/bdecli.c#L704-L709), `szFQNPath` only skips `' '` and `'\t'`, then checks `if (*szFQNPath == 0) continue;`. Because `fgets()` retains `\r` and `\n`, normal blank lines have `*szFQNPath == '\n'` (or `'\r'`), fail the empty-line check, and abort the program at [line 718](file:///d:/GitHub/bdecli/src/bdecli.c#L718) (`no '=' in line`).
* **Sub-container search bug in [`bde_cfg_add_entry()`](file:///d:/GitHub/bdecli/src/bdecli.c#L608):** The loop `while ((current) && (current->container == co))` at [line 608](file:///d:/GitHub/bdecli/src/bdecli.c#L608) stops searching container `co` as soon as it encounters a child of a nested sub-container (whose `container` pointer is the sub-container, not `co`). Meanwhile, [line 637](file:///d:/GitHub/bdecli/src/bdecli.c#L637) inserts new variables of `co` *after* all nested sub-containers (`bde_contains(co, current->next)`). Updating that same variable again will fail to find it and insert a duplicate variable entry.

#### 11. Partial CLI Switch Matching in [`main()`](file:///d:/GitHub/bdecli/src/bdecli.c#L793-L822)
* [`main()`](file:///d:/GitHub/bdecli/src/bdecli.c#L798) checks `argv[1][0] != '-'` and switches on `argv[1][1]` without checking `argv[1][2] == '\0'`, unintentionally accepting any flag starting with `-i` or `-e` (such as `-import` or `-export`).
