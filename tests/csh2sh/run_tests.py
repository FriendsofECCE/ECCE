#!/usr/bin/env python3
"""ecce-csh2sh and the csh rules it shares with gensub.

  cases/NAME.cfg   a CONFIG file with csh in it; header lines "# args: ..." add
                   options.  Beside it, written by --update and reviewed by hand:
                   NAME.expected (the converted file) and NAME.report (--check).
  run/NAME.csh     a csh snippet.  Converted, then run under dash and the original
                   under tcsh with the same environment; what they print and the
                   environment they leave behind must agree ("# env: A=1 B=2" lines
                   add variants).  Skipped without tcsh.

Also: backups and idempotence, every converted case embedded in a gensub job script
(dash -n; gensub refusing exactly what the report says is left), the launcher's
once-per-version run, and the rule list gensub and ecce-csh2sh share.

    tests/csh2sh/run_tests.py [--build DIR] [--update] [-v]

Exit 77 (CTest SKIP) without perl or dash.
"""

import argparse
import glob
import importlib.util
import os
import re
import shutil
import stat
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
C2S = os.path.join(REPO, "scripts", "ecce-csh2sh")
SKIP = 77

fails = []
count = 0


def say(t):
    print(t, flush=True)


def check(ok, what, detail=""):
    global count
    count += 1
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        fails.append(what)
        if detail:
            say("      " + detail.replace("\n", "\n      "))
    return ok


def run(argv, env=None, cwd=None, inp=None):
    r = subprocess.run(argv, env=env, cwd=cwd, input=inp, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
    return r.returncode, r.stdout.decode("utf-8", "replace")


def c2s(args, home=None, user=None):
    env = dict(os.environ)
    env["ECCE_HOME"] = home or ""
    if user:
        env["ECCE_REALUSERHOME"] = user
    return run(["perl", C2S] + args, env=env)


def twins(tmp):
    """The files the source cases refer to."""
    for rel in ("have.csh", "have.sh", "lonely.csh", "init/csh", "init/sh"):
        p = os.path.join(tmp, rel)
        os.makedirs(os.path.dirname(p), exist_ok=True)
        with open(p, "w") as h:
            h.write("# %s\n" % rel)


def read(p):
    with open(p) as h:
        return h.read()


def write(p, t):
    os.makedirs(os.path.dirname(p), exist_ok=True)
    with open(p, "w") as h:
        h.write(t)


def header_args(text):
    out = []
    for l in text.splitlines():
        m = re.match(r"#\s*args:\s*(.*)", l)
        if m:
            out += m.group(1).split()
    return out


# --- fixtures -----------------------------------------------------------------

def casesSuite(args):
    for cfg in sorted(glob.glob(os.path.join(HERE, "cases", "*.cfg"))):
        name = os.path.basename(cfg)[:-4]
        say("case %s" % name)
        tmp = tempfile.mkdtemp(prefix="csh2sh-")
        try:
            twins(tmp)
            src = read(cfg).replace("@T@", tmp)
            f = os.path.join(tmp, "CONFIG.case")
            write(f, src)
            extra = header_args(src)
            expect_out = os.path.join(HERE, "cases", name + ".expected")
            expect_rep = os.path.join(HERE, "cases", name + ".report")

            rc, rep = c2s(["--check", f] + extra)
            rep = "exit=%d\n%s" % (rc, rep.replace(tmp, "@T@"))
            check(read(f) == src, "--check leaves the file alone")
            check(not os.path.exists(f + ".csh-backup"), "--check writes no backup")

            rc2, conv = c2s(["--convert", f] + extra)
            new = read(f)
            conv_out = new.replace(tmp, "@T@")
            if args.update:
                write(expect_out, conv_out)
                write(expect_rep, rep)
            if os.path.exists(expect_out):
                check(conv_out == read(expect_out), "converted file is as expected",
                      "".join(__import__("difflib").unified_diff(
                          read(expect_out).splitlines(1), conv_out.splitlines(1), "expected", "got")))
                want_rep = read(expect_rep)
                check(rep == want_rep, "--check report is as expected",
                      "".join(__import__("difflib").unified_diff(
                          want_rep.splitlines(1), rep.splitlines(1), "expected", "got")))
            else:
                check(False, "no %s (run --update, then read it)" % expect_out)

            # Comments, blank lines and unrelated keys survive; so does the line count.
            check(len(new.splitlines()) == len(src.splitlines()), "same number of lines")
            for a, b in zip(src.splitlines(), new.splitlines()):
                if a.strip() == "" or a.lstrip().startswith("#"):
                    if a != b:
                        check(False, "comment or blank line changed: %r -> %r" % (a, b))
            changed = new != src
            check(changed == os.path.exists(f + ".csh-backup"),
                  "a backup exists exactly when the file changed")
            if changed:
                check(read(f + ".csh-backup") == src, "the backup is the original")
                check(stat.S_IMODE(os.stat(f).st_mode) == stat.S_IMODE(os.stat(f + ".csh-backup").st_mode),
                      "the backup has the file's mode")

            # Idempotent: a second run changes nothing and keeps the first backup.
            rc3, conv2 = c2s(["--convert", f] + extra)
            check(read(f) == new, "second --convert changes nothing")
            if changed:
                check(read(f + ".csh-backup") == src, "second run keeps the first backup")
            rc4, rep2 = c2s(["--check", f] + extra)
            check("would be converted" not in rep2, "nothing left to convert after --convert")

            # Embedded in a job script: gensub accepts it iff nothing is reported.
            gensubCase(name, new, rep2)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)


_gensub = None


def gensubDriver():
    global _gensub
    if _gensub is None:
        spec = importlib.util.spec_from_file_location(
            "queues_run_tests", os.path.join(REPO, "tests", "queues", "run_tests.py"))
        sys.path.insert(0, os.path.join(REPO, "tests", "queues"))
        mod = importlib.util.module_from_spec(spec)
        sys.argv_saved = sys.argv
        spec.loader.exec_module(mod)
        _gensub = mod
    return _gensub


def gensubCase(name, cfgtext, report_after):
    q = gensubDriver()
    g = q.Gensub()
    try:
        # Only the snippets: the other keys of the fixture are not for this machine.
        g.config(cfgtext)
        left = "needs attention (" in report_after
        try:
            script = g.script("shell", "nwchem", q.PROFILES["basic"])
            if left:
                check(False, "%s: gensub accepted a file the report says still has csh" % name)
            else:
                check(True, "gensub generates a script from the converted file")
                for p in q.notShell(script):
                    check(False, p)
                if script:
                    res = subprocess.run(["dash", "-n"], input=script.encode(),
                                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                    check(res.returncode == 0, "dash -n accepts the generated job script",
                          res.stdout.decode())
        except RuntimeError as exc:
            msg = str(exc)
            if left:
                nlines = len(re.findall(r"^ +\S+, line \d+:", msg, re.M))
                reported = len(re.findall(r"^    line \d+ ", report_after.split("needs attention")[1], re.M)) \
                    if "needs attention (" in report_after else 0
                check(nlines >= 1, "gensub still refuses the lines left (%d named)" % nlines)
            else:
                check(False, "gensub refused a converted file: %s" % " ".join(msg.split())[:200])
    finally:
        g.close()


# --- run the converted snippet ---------------------------------------------------

def snippetBody(cfgtext):
    m = re.search(r"^setup \{\n(.*)^\}\n", cfgtext, re.S | re.M)
    return m.group(1)


def runSuite():
    tcsh = shutil.which("tcsh")
    dash = shutil.which("dash")
    for csh in sorted(glob.glob(os.path.join(HERE, "run", "*.csh"))):
        name = os.path.basename(csh)[:-4]
        say("run %s" % name)
        if not tcsh:
            say("  SKIP no tcsh")
            continue
        src = read(csh)
        variants = [m.group(1).split() for m in re.finditer(r"^# env:(.*)$", src, re.M)] or [[]]
        tmp = tempfile.mkdtemp(prefix="csh2sh-run-")
        try:
            cfg = os.path.join(tmp, "CONFIG.run")
            write(cfg, "setup {\n" + src + "}\n")
            rc, out = c2s(["--convert", cfg, "--assume-sh-twins"])
            conv = snippetBody(read(cfg))
            check("needs attention" not in out, "converted without leftovers", out)
            cshfile = os.path.join(tmp, "orig.csh")
            shfile = os.path.join(tmp, "conv.sh")
            write(cshfile, src)
            write(shfile, conv)
            # Hand-made inputs of the kind gensub's job script has already set.
            for var in variants:
                base = {"PATH": "/usr/bin:/bin", "HOME": tmp, "runDir": tmp, "totalprocs": "2",
                        "scratchDir": tmp + "/scr"}
                for kv in var:
                    k, _, v = kv.partition("=")
                    base[k] = v

                def trace(shell_argv, body, shell):
                    # empty script gives the shell's own variables; subtract them
                    outs = {}
                    for label, text in (("empty", ""), ("job", body)):
                        w = os.path.join(tmp, "w_" + label + shell)
                        write(w, text + "\necho @@ENV@@\nenv\n")
                        r = subprocess.run(shell_argv + [w], env=dict(base), cwd=tmp,
                                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
                        o = r.stdout.decode("utf-8", "replace")
                        pre, _, envtext = o.partition("@@ENV@@\n")
                        outs[label] = (pre, set(envtext.splitlines()))
                    return outs["job"][0], sorted(outs["job"][1] - outs["empty"][1])

                o1, e1 = trace([tcsh, "-f"], src, "csh")
                o2, e2 = trace([dash], conv, "sh")
                # export of an unset-before variable shows in both; drop shell noise
                noise = re.compile(r"^(?:_|PWD|OLDPWD|SHLVL)=")
                e1 = [x for x in e1 if not noise.match(x)]
                e2 = [x for x in e2 if not noise.match(x)]
                label = " ".join(var) or "empty environment"
                check(o1 == o2, "same output (%s)" % label, "csh:\n%s\nsh:\n%s" % (o1, o2))
                check(e1 == e2, "same environment (%s)" % label, "csh: %s\nsh:  %s" % (e1, e2))
        finally:
            shutil.rmtree(tmp, ignore_errors=True)


# --- the rule list shared with gensub ----------------------------------------------

def rulesSuite():
    say("shared rules")
    # gensub's own translate/refuse behaviour is covered by tests/queues; here that
    # both tools read one list and nothing parses csh twice.
    gensub = read(os.path.join(REPO, "scripts", "gensub"))
    check("CshToSh::snippet" in gensub, "gensub translates through CshToSh")
    check("@cshOnly" not in gensub, "gensub carries no second rule list")
    check("refusal" in read(os.path.join(REPO, "scripts", "CshToSh.pm")), "the rule list is in CshToSh.pm")
    r = subprocess.run(["perl", "-I", os.path.join(REPO, "scripts"), "-MCshToSh", "-e",
                        'my $r = CshToSh::snippet("setenv A b\\nif (-e x) then\\nendif\\n"); '
                        'print $r->{text}, "|", scalar(@{$r->{issues}}), "\\n"'],
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out = r.stdout.decode()
    check(out == "export A=b\nif (-e x) then\nendif\n|2\n",
          "basic mode (gensub) translates the plain forms and refuses the structured ones", out)


# --- notes beyond the cases ---------------------------------------------------------

def extraSuite():
    say("details")
    tmp = tempfile.mkdtemp(prefix="csh2sh-x-")
    try:
        # no newline at the end of the file
        f = os.path.join(tmp, "CONFIG.nonl")
        write(f, "setup {\nsetenv A b\n}\nwrapup { setenv C d }")
        c2s(["--convert", f])
        check(read(f) == "setup {\nexport A=b\n}\nwrapup { export C=d }", "no trailing newline is kept")
        # an existing backup is never overwritten without --force
        g = os.path.join(tmp, "CONFIG.bk")
        write(g, "setup {\nsetenv A b\n}\n")
        write(g + ".csh-backup", "OLD\n")
        rc, out = c2s(["--convert", g])
        check(rc == 2 and read(g) == "setup {\nsetenv A b\n}\n" and read(g + ".csh-backup") == "OLD\n",
              "an existing backup stops the conversion (exit 2, nothing changed)", out)
        rc, out = c2s(["--convert", "--force", g])
        check(rc == 0 and read(g) == "setup {\nexport A=b\n}\n" and read(g + ".csh-backup") == "setup {\nsetenv A b\n}\n",
              "--force replaces the backup", out)
        # exit codes of --check
        h = os.path.join(tmp, "CONFIG.clean")
        write(h, "setup {\nexport A=b\n}\n")
        check(c2s(["--check", h])[0] == 0, "--check: 0 for a clean file")
        i = os.path.join(tmp, "CONFIG.dirty")
        write(i, "setup {\nalias a b\n}\n")
        check(c2s(["--check", i])[0] == 1, "--check: 1 when a line needs attention")
        rc, out = c2s(["--convert", i])
        check(rc == 1 and not os.path.exists(i + ".csh-backup"),
              "--convert: 1 when lines are left, and no backup for an unchanged file", out)
        check(c2s(["--check", os.path.join(tmp, "absent")])[0] == 2, "an unreadable file is exit 2")
        check(c2s(["--check"])[0] == 2, "no target is exit 2")
        # a symlinked config is converted in place, not replaced by a file
        real = os.path.join(tmp, "real")
        write(real, "setup {\nsetenv A b\n}\n")
        link = os.path.join(tmp, "CONFIG.link")
        os.symlink(real, link)
        c2s(["--convert", link])
        check(os.path.islink(link) and read(real) == "setup {\nexport A=b\n}\n", "a symlinked file is converted in place")
        # --siteconfig: submit.site and CONFIG.*, never CONFIG-Examples or backups
        home = os.path.join(tmp, "home")
        write(home + "/siteconfig/submit.site", "setup {\nsetenv S 1\n}\n")
        write(home + "/siteconfig/CONFIG.m", "wrapup {\nsetenv W 1\n}\n")
        write(home + "/siteconfig/CONFIG.m.csh-backup", "wrapup {\nsetenv OLD 1\n}\n")
        write(home + "/siteconfig/CONFIG-Examples/CONFIG.x", "setup {\nsetenv X 1\n}\n")
        rc, out = c2s(["--check", "--siteconfig"], home=home)
        check(rc == 1 and "submit.site" in out and "CONFIG.m\n" in out and "CONFIG-Examples" not in out
              and "csh-backup\n" not in out.replace("original kept", ""),
              "--siteconfig covers submit.site and CONFIG.* only", out)
        rc, out = c2s(["--check", "--siteconfig", "--brief"], home=home)
        check("->" not in out and "write:" not in out and "submit.site" in out,
              "--brief names files, not lines", out)
        os.unlink(home + "/siteconfig/CONFIG.m.csh-backup")
        c2s(["--convert", "--siteconfig"], home=home)
        check(read(home + "/siteconfig/CONFIG-Examples/CONFIG.x") == "setup {\nsetenv X 1\n}\n",
              "CONFIG-Examples is left alone")
        check(read(home + "/siteconfig/submit.site") == "setup {\nexport S 1\n}\n".replace("export S 1", "export S=1"),
              "submit.site converted")
        check(c2s(["--check", "--siteconfig"], home=home)[0] == 0, "--check is clean after --convert --siteconfig")
        check(c2s(["--check", "--siteconfig"])[0] == 2, "--siteconfig without ECCE_HOME is exit 2")
        # The shipped site files are already sh.
        rc, out = c2s(["--check", "--siteconfig"], home=REPO)
        check(rc == 0, "the shipped siteconfig needs nothing", out)
        for ex in sorted(glob.glob(os.path.join(REPO, "siteconfig", "CONFIG-Examples", "CONFIG.*"))):
            rc, out = c2s(["--check", ex])
            check(rc == 0, "shipped example %s is sh" % os.path.basename(ex), out)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


# --- the launcher's once-per-version run -----------------------------------------------

def launcherSuite():
    say("first start (the ecce launcher)")
    tmp = tempfile.mkdtemp(prefix="csh2sh-launch-")
    try:
        launcher = os.path.join(tmp, "ecce")
        t = read(os.path.join(REPO, "packaging", "ecce.in"))
        t = t.replace("@ECCE_HOME_DIR@", os.path.join(tmp, "ecce-home")).replace("@ECCE_VERSION_RAW@", "v9.9.9-test")
        write(launcher, t)
        os.chmod(launcher, 0o755)
        home = os.path.join(tmp, "ecce-home")
        os.makedirs(home + "/siteconfig")
        os.symlink(os.path.join(REPO, "scripts"), home + "/scripts")
        # The launcher sources the session library from the install.
        os.makedirs(home + "/bin")
        os.symlink(os.path.join(REPO, "packaging", "gateway", "ecce-session-lib.sh"),
                   home + "/bin/ecce-session-lib.sh")
        stubs = os.path.join(tmp, "stubs")
        write(stubs + "/ecce-gateway", "#!/bin/sh\necho started >> \"$ECCE_STUB_LOG\"\n")
        os.chmod(stubs + "/ecce-gateway", 0o755)
        user = os.path.join(tmp, "user")
        write(user + "/.ECCE/CONFIG.m1", "setup {\nsetenv A b\nalias ll ls\n}\n")
        write(user + "/.ECCE/CONFIG.m2", "wrapup {\nexport OK=1\n}\n")
        write(user + "/.ECCE/CONFIG.m2.csh-backup", "wrapup {\nsetenv KEEP 1\n}\n")
        log = os.path.join(tmp, "stub.log")
        env = dict(os.environ, ECCE_HOME=home, ECCE_REALUSERHOME=user, HOME=user,
                   PATH=stubs + ":" + os.environ["PATH"], ECCE_STUB_LOG=log)
        env.pop("ECCE_REMOTE_SERVER", None)

        rc, out = run(["bash", launcher], env=env)
        check(rc == 0 and read(log) == "started\n", "the session starts", out)
        check(read(user + "/.ECCE/CONFIG.m1") == "setup {\nexport A=b\nalias ll ls\n}\n", "csh in CONFIG converted")
        check(read(user + "/.ECCE/CONFIG.m1.csh-backup") == "setup {\nsetenv A b\nalias ll ls\n}\n", "backup written")
        check(read(user + "/.ECCE/CONFIG.m2") == "wrapup {\nexport OK=1\n}\n", "an sh file is untouched")
        check(read(user + "/.ECCE/CONFIG.m2.csh-backup") == "wrapup {\nsetenv KEEP 1\n}\n", "an old backup is untouched")
        check(read(user + "/.ECCE/csh2sh-done") == "v9.9.9-test\n", "marker holds the version")
        note = read(user + "/.ECCE/csh2sh-notice") if os.path.exists(user + "/.ECCE/csh2sh-notice") else ""
        check("CONFIG.m1" in note and "CONFIG.m1.csh-backup" in note and "alias ll ls" in note
              and "write: the sh equivalent" in note,
              "a notice lists the file, the backup and the line left", note)
        # The gateway shows and removes the notice; simulate that and start again.
        os.unlink(user + "/.ECCE/csh2sh-notice")
        write(user + "/.ECCE/CONFIG.m3", "setup {\nsetenv LATER 1\n}\n")
        rc, out = run(["bash", launcher], env=env)
        check(read(log) == "started\nstarted\n", "second session starts")
        check(read(user + "/.ECCE/CONFIG.m3") == "setup {\nsetenv LATER 1\n}\n",
              "the second start converts nothing")
        check(not os.path.exists(user + "/.ECCE/csh2sh-notice") and not os.path.exists(user + "/.ECCE/CONFIG.m3.csh-backup"),
              "the second start leaves no notice and no backup")
        # A new version runs once more.
        write(launcher, read(launcher).replace("v9.9.9-test", "v9.9.10-test"))
        rc, out = run(["bash", launcher], env=env)
        check(read(user + "/.ECCE/CONFIG.m3") == "setup {\nexport LATER=1\n}\n", "a new version converts again")
        check(read(user + "/.ECCE/csh2sh-done") == "v9.9.10-test\n", "marker moves to the new version")
        # Nothing to convert: no notice, marker written.
        user2 = os.path.join(tmp, "user2")
        write(user2 + "/.ECCE/CONFIG.ok", "setup {\nexport A=1\n}\n")
        e2 = dict(env, ECCE_REALUSERHOME=user2, HOME=user2)
        rc, out = run(["bash", launcher], env=e2)
        check(not os.path.exists(user2 + "/.ECCE/csh2sh-notice")
              and read(user2 + "/.ECCE/csh2sh-done") == "v9.9.10-test\n",
              "a user with sh files gets no notice and a marker")
        # A user with no config files at all: no process, marker may be absent
        user3 = os.path.join(tmp, "user3")
        os.makedirs(user3 + "/.ECCE")
        e3 = dict(env, ECCE_REALUSERHOME=user3, HOME=user3)
        rc, out = run(["bash", launcher], env=e3)
        check(rc == 0, "a user with no CONFIG files starts", out)
        # cost of a start after the first: builtins only
        import time
        t0 = time.time()
        for _ in range(10):
            run(["bash", launcher], env=env)
        per = (time.time() - t0) / 10
        say("  note %.0f ms per start with the stub gateway (marker current)" % (per * 1000))
        check(per < 0.5, "a later start costs no noticeable time")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build")
    ap.add_argument("--update", action="store_true")
    ap.add_argument("-v", action="store_true")
    args = ap.parse_args()
    if not shutil.which("perl") or not shutil.which("dash"):
        say("SKIP: needs perl and dash")
        return SKIP
    casesSuite(args)
    runSuite()
    rulesSuite()
    extraSuite()
    launcherSuite()
    say("")
    say("%d checks, %d failed" % (count, len(fails)))
    for f in fails:
        say("  FAIL " + f)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
