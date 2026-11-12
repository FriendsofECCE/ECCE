#!/usr/bin/env python3
"""ecce-reparse: the command line asks the running Organizer to re-parse the
stored outputs of calculations, as Run Mgmt > Re-Parse Outputs does.

A MOPAC output is imported, its Props are deleted, and ecce-reparse (by path)
must print "name: done", exit 0 and bring the properties back; --json gives
the same as an array. A Ready calculation (no output) and an unknown path are
refused with a reason and exit 1. Without a session the command says so and
exits 2.

    tests/launch/ecce_reparse_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def main():
    import time
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    exe = os.path.join(build, "ecce-reparse")
    if not os.access(exe, os.X_OK):
        say("FAIL: ecce-reparse is not built in %s" % build)
        return 1
    import xdisplay

    s = harness.Session(build, "ecce-reparse", {"CP2K": "/bin/true"},
                        local=True, keep=args.keep)
    slow = os.path.join(s.state, "slow-cp2k")
    with open(slow, "w") as h:
        h.write("#!/bin/sh\nsleep 300\nexec /bin/true \"$@\"\n")
    os.chmod(slow, 0o755)
    harness.link(os.path.join(build, "organizer"),
                 os.path.join(s.home, "bin", "organizer"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    s.registerMachine("localhost", "Shell", {"CP2K": slow})
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    deck = os.path.join(work, "a.cp2kin")
    open(deck, "w").write("&GLOBAL\n  RUN_TYPE ENERGY\n&END GLOBAL\n")

    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        url, out = s.create("idle", "cp2k_es", deck, "cp2k.cp2kin", rundir,
                            extra=("procs=1", "queue=normal", "nodes=1",
                                   "wall=0 1:0"))
        if not check(url is not None, "calculation idle created"):
            say(out)
            return 1

        def run(*argv, **kw):
            r = subprocess.run([exe] + list(argv), env=s.env(kw.get("env")),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=900)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        #  No session: said so.
        empty = tempfile.mkdtemp(prefix="ecce-reparse-nosession-")
        try:
            rc, out, err = run(url, env={
                "ECCE_SESSION_ID": "0123456789abcdef",
                "ECCE_REALUSERHOME": empty})
        finally:
            shutil.rmtree(empty, ignore_errors=True)
        check(rc == 2 and "ECCE is not running: start ECCE first" in err,
              "without a session: exit 2 and the message (rc=%s %r)" % (rc, err))
        rc, out, err = run()
        check(rc == 64, "no argument: usage, exit 64 (rc=%s)" % rc)

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        olog = os.path.join(s.state, "organizer.log")
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        import sessionkey
        os.makedirs(os.path.join(s.state, ".ECCE", "waiting"), exist_ok=True)
        with open(os.path.join(s.state, ".ECCE", "waiting", "session"), "w") as h:
            h.write(sessionkey.key(genv.get("ECCE_SESSION_ID", ""), genv) + "\n")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def props_dirs():
            found = []
            for base, dirs, _f in os.walk(s.localData()):
                if "Props" in dirs:
                    found.append(os.path.join(base, "Props"))
                    dirs[:] = []
            return found

        #  Wait for the Organizer, then import a stored output.
        fixture = os.path.join(REPO, "tests", "parsers", "fixtures", "mopac",
                               "ch4_opt.out")
        with open(cmds, "a") as h:
            h.write("import %s\n" % fixture)
        end = time.time() + 240
        imported = None
        while time.time() < end and not imported:
            time.sleep(2)
            imported = [p for p in props_dirs() if os.listdir(p)]
        if not check(bool(imported), "the MOPAC output was imported with properties"):
            say(open(olog, errors="replace").read()[-2000:])
            return 1
        props = imported[0]
        calc = os.path.dirname(props)
        time.sleep(15)   # the import's own parsing has finished
        before = sorted(e for e in os.listdir(props) if not e.startswith("."))
        name = os.path.basename(calc)
        for entry in before:
            full = os.path.join(props, entry)
            if os.path.isdir(full):
                shutil.rmtree(full)
            elif os.path.exists(full):
                os.unlink(full)

        rc, out, err = run(calc, url, "file://" + os.path.join(s.state, "no-such-calc"))
        say(out)
        say(err)
        lines = out.splitlines()
        check(rc == 1, "exit 1 as some were refused (rc=%s)" % rc)
        check("%s: done" % name in lines, "'%s: done'" % name)
        check(any(l.startswith("idle: refused: ") for l in lines),
              "a Ready calculation is refused with a reason")
        check(any("refused: not a calculation in this session" in l for l in lines),
              "an unknown path is refused")
        check(sorted(e for e in os.listdir(props) if not e.startswith(".")) == before,
              "the properties are back (%s)" % sorted(os.listdir(props)))

        for entry in before:
            full = os.path.join(props, entry)
            if os.path.isdir(full):
                shutil.rmtree(full)
            elif os.path.exists(full):
                os.unlink(full)
        rc, out, err = run("--json", calc)
        say(out)
        try:
            data = json.loads(out)
        except ValueError:
            data = None
        check(rc == 0 and data == [{"name": name, "status": "done", "reason": ""}],
              "--json: one done object, exit 0 (rc=%s)" % rc)
        check(sorted(e for e in os.listdir(props) if not e.startswith(".")) == before, "the properties are back again")
    finally:
        if org and org.poll() is None:
            org.terminate()
        if display:
            display.__exit__(None, None, None)
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)

    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
