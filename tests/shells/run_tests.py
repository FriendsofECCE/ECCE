#!/usr/bin/env python3
"""
Does a long command's echo come back byte for byte from every shell ECCE
can end up talking to?  (#143, #69 Bug 2)

eccejobstore's initMon() sends the eccejobmonitor command over a pty and
waits, with no timeout, for exactly cmd + "\\r\\n".  A line editor that
redraws long lines (tcsh's editor wraps with " \\b"; bash's readline wraps
with "\\r" or scrolls with a leading "<"; zsh's zle and ksh's editors do
the same) means that never arrives and job monitoring hangs forever.
RCommand's init lines turn each editor off; this checks that they do.

Two tiers:

  1. A pty harness that plays RCommand's side of the conversation, with
     the init lines READ FROM RCommand.C (so the test cannot drift from
     the code), in the three shapes a session takes:
       local  -- "<sh> -fc 'echo +hi+ && <sh> -f'", RCommand("system")
                 and any machine that is this host;
       ssh    -- ssh with a remote command and no remote tty: the shell
                 has no tty, and the LOCAL pty does the echoing;
       hop    -- a front end or hop: ssh allocates a remote pty, the
                 user's LOGIN shell (any shell: zsh, mksh, ...) is
                 there, and RCommand types "<csh|bash> -i" into it.
  2. testRCommandEcho, linked against the real RCommand, run against each
     local csh/tcsh/bash (skipped if it has not been built).

Shells not present are skipped.  Extra directories to look for shells in
(an unpacked .deb, say) go in ECCE_TEST_SHELL_DIRS, colon-separated.

    --control   strip the editor-off commands from the init lines and
                expect FAILURES: proves the harness can see the fault.
    --driver P  path to testRCommandEcho (default: build-cmake's).
"""
import argparse
import os
import pty
import re
import select
import shutil
import signal
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
RCOMMAND = os.path.join(ROOT, "src", "comm", "rcommand", "RCommand.C")

CSH_FAMILY = ["csh", "bsd-csh", "tcsh"]
LOGIN_SHELLS = ["bash", "zsh", "tcsh", "bsd-csh", "mksh", "ksh93", "dash"]
EDITOR_OFF = ["; unset edit", "; set +o emacs; set +o vi"]

#  Shaped like initMon()'s command, long enough to wrap anything.  No
#  glob characters, since RCommand matches with glob.
LONG_CMD = ("echo ECCE_LONG_ECHO -configFile eccejobmonitor.conf -jobId 10"
            " -bookmark 0" +
            "".join(" -arg%02d /tmp/ecce_mca32/jobs/direct__da8YQu" % i
                    for i in range(7)) +
            "; echo eccejobmonitor_went_bye_bye")
DONE = b"eccejobmonitor_went_bye_bye\r\n"


def init_lines():
    """The csh and bash init lines, as the C++ string literals spell them.

    Every occurrence (constructor and hop()) must agree, or one path has
    been fixed and the other forgotten."""
    src = open(RCOMMAND).read()
    found = {}
    for key, start in (("csh", "unalias precmd"), ("bash", "unalias -a")):
        pat = r'expwrite\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+)\)'
        vals = set()
        for m in re.finditer(pat, src):
            text = "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(1)))
            text = bytes(text, "utf-8").decode("unicode_escape")
            if text.startswith(start):
                vals.add(text)
        if len(vals) != 1:
            sys.exit("RCommand.C has %d distinct %s init lines: %r"
                     % (len(vals), key, sorted(vals)))
        found[key] = vals.pop()
    return found


def find_shell(name, dirs):
    for d in dirs:
        p = os.path.join(d, name)
        if os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    return shutil.which(name)


def zsh_env(path, env, tmp):
    """An unpacked zsh cannot find zle.so in its compiled-in module path,
    and without zle it has no editor to test.  Point it at its own."""
    root = os.path.dirname(os.path.dirname(os.path.dirname(path)))
    mods = [os.path.join(dp, "..") for dp, dn, fn in
            os.walk(os.path.join(root, "usr", "lib")) if "zle.so" in fn]
    if mods and not path.startswith("/usr/"):
        zd = os.path.join(tmp, "zdot")
        os.makedirs(zd, exist_ok=True)
        with open(os.path.join(zd, ".zshenv"), "w") as f:
            f.write("module_path=(%s)\n" % os.path.realpath(mods[0]))
        env = dict(env, ZDOTDIR=zd)
    return env


class Pty:
    def __init__(self, argv, env):
        self.pid, self.fd = pty.fork()
        if self.pid == 0:
            try:
                os.execve(argv[0], argv, env)
            finally:
                os._exit(127)
        self.buf = b""

    def until(self, pred, timeout=5.0):
        end = time.time() + timeout
        while time.time() < end:
            if pred(self.buf):
                return True
            r, _, _ = select.select([self.fd], [], [], 0.05)
            if r:
                try:
                    b = os.read(self.fd, 65536)
                except OSError:
                    return pred(self.buf)
                if not b:
                    return pred(self.buf)
                self.buf += b
        return pred(self.buf)

    def send(self, line):
        self.buf = b""
        os.write(self.fd, line.encode() + b"\n")

    def close(self):
        try:
            os.kill(self.pid, signal.SIGKILL)
        except OSError:
            pass
        os.waitpid(self.pid, 0)
        os.close(self.fd)


def prompt(buf):
    return buf.endswith(b"+go+")


def converse(p, dialect, inits):
    """RCommand's side, after the shell is up.  Returns None or a reason."""
    p.send(inits[dialect])
    if not p.until(prompt):
        return "no +go+ prompt after init (%r)" % p.buf[-80:]
    if dialect == "csh":
        p.send("unalias *")
        if not p.until(lambda b: b.endswith(b"\r\n+go+")):
            return "unalias * failed"
    p.send(LONG_CMD)
    p.until(lambda b: DONE + b"+go+" in b)
    if (LONG_CMD + "\r\n").encode() not in p.buf:
        i = p.buf.find(b"eccejobmonitor_went_bye_bye")
        return "echo not exact: %r" % p.buf[max(0, i - 60):i + 30]
    if DONE not in p.buf.split((LONG_CMD + "\r\n").encode(), 1)[1]:
        return "command did not run"
    return None


def scenario_local(sh, dialect, env, inits):
    p = Pty([sh, "-fc", "echo +hi+ && %s -f" % sh], env)
    try:
        if not p.until(lambda b: b"+hi+\r\n" in b):
            return "no +hi+"
        time.sleep(0.2)
        return converse(p, dialect, inits)
    finally:
        p.close()


def scenario_ssh(sh, dialect, env, inits):
    inner = sh + (" --noediting -i" if dialect == "bash" else " -i")
    relay = 'cat | setsid -w sh -c "echo +hi+ && %s" 2>&1 | cat' % inner
    p = Pty(["/bin/sh", "-c", relay], env)
    try:
        if not p.until(lambda b: b"+hi+\r\n" in b):
            return "no +hi+"
        time.sleep(0.3)
        return converse(p, dialect, inits)
    finally:
        p.close()


def wait_shell_ready(p, tries=10, per_try=0.5):
    """Mirrors RCommand::waitShellReady() (#143/#69 item 3): poll with a
    real probe instead of a fixed sleep, so a login shell's editor
    (tcsh, zsh, ksh93) flushing typed-ahead input during its own raw-
    mode setup can't discard what we send next -- we just see no probe
    echo and retry."""
    for _ in range(tries):
        p.send("echo ECCE_READY_''PROBE")
        #  A bare substring, not "\r\n...\r\n"-anchored: bash's bracketed-
        #  paste escapes and a shell's own prompt/echo quirks (bsd-csh
        #  prints its prompt directly against the output with no
        #  newline between) can land other bytes at that exact
        #  boundary. All that matters here is "did this shell just run
        #  our command", not the framing -- unlike the item-1 sentinel,
        #  nothing downstream parses what follows this match.
        if p.until(lambda b: b"ECCE_READY_PROBE" in b, timeout=per_try):
            return True
        #  ksh93 continuation-prompt flush before the next attempt.
        os.write(p.fd, b"\x03\n")
    return False


def scenario_hop(login, sh, dialect, env, inits):
    p = Pty([login, "-i"], env)
    try:
        p.until(lambda b: False, timeout=0.6)
        p.send(sh + " -i")
        if not wait_shell_ready(p):
            return "inner shell %s never became ready" % sh
        return converse(p, dialect, inits)
    finally:
        p.close()


def run_driver(driver, shells, base_env, tmp):
    results = []
    for dialect, sh in shells:
        #  RCommand classifies by basename now (#143/#69 item 3), not by
        #  an exact match against the literal string "bash" -- so drive
        #  it with the shell's own name/path directly.
        name = sh
        for term in (None, "xterm"):
            env = dict(base_env, ECCE_REALUSER=os.environ.get(
                "USER", "ecce"), HOME=tmp)
            env["PATH"] = os.path.dirname(sh) + ":" + env["PATH"]
            if term:
                env["TERM"] = term
            try:
                r = subprocess.run([driver, name], env=env, timeout=90,
                                   capture_output=True, text=True)
                out = (r.stdout.strip().splitlines() or ["(no output)"])[-1]
                ok = r.returncode == 0
            except subprocess.TimeoutExpired:
                out, ok = "TIMEOUT", False
            results.append(("driver", sh, term, None if ok else out))
    return results


def run_driver_reject(driver, shells, base_env, tmp):
    """Anything that isn't csh/tcsh/bash must be refused outright, not
    silently treated as csh (#143/#69 item 3, narrowed 2026-09-28):
    RCommand's constructor sets p_errMessage and returns before
    p_connected is ever set, so isOpen() is false and testRCommandEcho
    exits 2 with "NO SESSION" -- never 0 or 1.

    Only checks for the rejection itself (exit 2, "NO SESSION"), not
    the specific "ECCE needs csh, tcsh or bash" wording RCommand's
    constructor actually sets: isOpen()'s existing "else" branch
    unconditionally overwrites p_errMessage with a generic "Failed to
    open remote shell ... (incorrect password?)" whenever p_connected
    is false, for EVERY early-return case in the constructor, not just
    this new one -- a pre-existing bug (predates this change, same
    shape as the CalcEd/ai.<code> "discarded error message" class in
    CLAUDE.md) that swallows the specific rejection text before
    commError() is ever read. Flagged for a decision, not fixed here:
    fixing isOpen() is a separate, wider change than this task's scope."""
    results = []
    for sh in shells:
        for term in (None, "xterm"):
            env = dict(base_env, ECCE_REALUSER=os.environ.get(
                "USER", "ecce"), HOME=tmp)
            env["PATH"] = os.path.dirname(sh) + ":" + env["PATH"]
            if term:
                env["TERM"] = term
            try:
                r = subprocess.run([driver, sh], env=env, timeout=90,
                                   capture_output=True, text=True)
                out = (r.stdout.strip().splitlines() or ["(no output)"])[-1]
                ok = r.returncode == 2 and "NO SESSION" in out
            except subprocess.TimeoutExpired:
                out, ok = "TIMEOUT", False
            results.append(("driver reject", sh, term, None if ok else out))
    return results


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--control", action="store_true")
    ap.add_argument("--driver",
                    default=os.path.join(ROOT, "build-cmake",
                                         "testRCommandEcho"))
    args = ap.parse_args()

    inits = init_lines()
    for key in inits:
        if not any(off in inits[key] for off in EDITOR_OFF):
            print("NOTE: %s init line has no editor-off command: %r"
                  % (key, inits[key]))
    if args.control:
        for key in inits:
            for off in EDITOR_OFF:
                inits[key] = inits[key].replace(off, "")

    dirs = [d for d in os.environ.get("ECCE_TEST_SHELL_DIRS",
                                      "").split(":") if d]
    tmp = tempfile.mkdtemp(prefix="ecce-shells-")
    base = {"PATH": "/usr/bin:/bin", "HOME": tmp, "LANG": "C.UTF-8"}

    targets = []          # (dialect, path): shells RCommand drives
    for name in CSH_FAMILY:
        p = find_shell(name, dirs)
        if p and os.path.realpath(p) not in [os.path.realpath(t[1])
                                             for t in targets]:
            targets.append(("csh", p))
    for p in {find_shell("bash", dirs)} - {None}:
        targets.append(("bash", p))
    logins = [p for p in (find_shell(n, dirs) for n in LOGIN_SHELLS) if p]
    for name in CSH_FAMILY + LOGIN_SHELLS:
        if not find_shell(name, dirs):
            print("SKIP %s: not found" % name)

    #  ECCE's LOCAL shell classification (#143/#69 item 3, narrowed
    #  2026-09-28) only recognises csh/tcsh/bash by basename; anything
    #  else must be refused outright rather than guessed at as csh
    #  syntax. Drive the real binary at each of these other spellings
    #  and expect a clean rejection, not a dialect match.
    reject_targets = []
    for name in ("sh", "dash", "ksh", "ksh93", "mksh", "zsh"):
        p = find_shell(name, dirs)
        if p and os.path.realpath(p) not in [os.path.realpath(t[1])
                                             for t in targets]:
            reject_targets.append(p)

    results = []
    for term in (None, "xterm"):
        env = dict(base, TERM=term) if term else dict(base)
        for dialect, sh in targets:
            results.append(("local", sh, term,
                            scenario_local(sh, dialect, env, inits)))
            results.append(("ssh", sh, term,
                            scenario_ssh(sh, dialect, env, inits)))
            for login in logins:
                lenv = zsh_env(login, env, tmp) if "zsh" in login else env
                results.append(("hop via " + login, sh, term,
                                scenario_hop(login, sh, dialect, lenv,
                                             inits)))

    if args.control:
        print("(control run: driver skipped, it uses the compiled code)")
    elif os.access(args.driver, os.X_OK):
        results += run_driver(args.driver, targets, base, tmp)
        results += run_driver_reject(args.driver, reject_targets, base, tmp)
    else:
        print("SKIP driver: %s not built" % args.driver)

    shutil.rmtree(tmp, ignore_errors=True)

    bad = 0
    for kind, sh, term, why in results:
        tag = "PASS" if why is None else "FAIL"
        bad += why is not None
        print("%s %-22s %-40s TERM=%-6s %s"
              % (tag, kind, sh, term or "unset", why or ""))
    print("%d checks, %d failed" % (len(results), bad))

    if args.control:
        #  With the editors left on, tcsh (if present) and bash must fail
        #  somewhere; if nothing fails the harness cannot see the bug.
        return 0 if bad else 1
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
