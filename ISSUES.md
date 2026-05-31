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

**🔴 High — fix first:**
1. **Buffer overflow in `bde_fgets()`** (line 63) — writes to `buf[r_len]` before the bounds check, allowing a 1-byte stack overflow with crafted input.
2. **Off-by-one in `bde_cfg_add_entry()`** (line 529) — `i > MAX_BUFFER` should be `i >= MAX_BUFFER`, allowing out-of-bounds write.
3. **compat_s.h silently drops all size parameters** — on GCC/Clang/Watcom builds, every `strcpy_s` call is just `strcpy` with no bounds checking.

**🟠 Medium — should fix:**
4. BDE binary files opened in **text mode** (`"r"` / `"w"`) — causes silent data corruption on Windows from CR/LF translation and Ctrl-Z EOF.
5. Unchecked `NULL` dereference in `bde_fqn()` if a container entry has no name.

Let me know if you'd like me to prepare fixes for any of these.