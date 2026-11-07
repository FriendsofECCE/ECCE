#!/usr/bin/env python3
"""ecce-move / ecce-rename: the command line asks the running Organizer to
move or rename calculations with its own move, and its #700 refusals.

Two calculations are moved into a project (the Organizer finds them at the
new URL and no longer at the old one); a running calculation, and the folder
holding it, are refused while the rest still moves (exit 1); one calculation
is renamed; name clashes, an unknown folder, a path outside the data and a
bad name are refused; without a session the commands say so (exit 2); wrong
usage exits 64.

    tests/launch/ecce_move_test.py [--build DIR] [--keep]

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

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
    mv = os.path.join(build, "ecce-move")
    rn = os.path.join(build, "ecce-rename")
    ann = os.path.join(build, "ecce-annotate")
    cp = os.path.join(build, "ecce-copy")
    for exe in (mv, rn, cp):
        if not os.access(exe, os.X_OK):
            say("FAIL: %s is not built" % exe)
            return 1
    import xdisplay

    s = harness.Session(build, "ecce-move", {"CP2K": "/bin/true"},
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

        def run(exe, *argv, env=None):
            r = subprocess.run([exe] + list(argv), env=s.env(env),
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               timeout=300)
            return r.returncode, r.stdout.decode("utf-8", "replace"), \
                r.stderr.decode("utf-8", "replace")

        #  No session, wrong usage.
        empty = tempfile.mkdtemp(prefix="ecce-move-nosession-")
        try:
            env = {"ECCE_SESSION_ID": "0123456789abcdef", "ECCE_REALUSERHOME": empty}
            rc, out, err = run(mv, "a", "b", env=env)
            check(rc == 2 and "ECCE is not running: start ECCE first" in err,
                  "ecce-move without a session: exit 2 (rc=%s %r)" % (rc, err))
            rc, out, err = run(rn, "a", "b", env=env)
            check(rc == 2, "ecce-rename without a session: exit 2 (rc=%s)" % rc)
        finally:
            shutil.rmtree(empty, ignore_errors=True)
        check(run(mv)[0] == 64 and run(mv, "one")[0] == 64,
              "ecce-move without a folder: usage, 64")
        check(run(rn, "one")[0] == 64 and run(rn, "a", "b", "c")[0] == 64,
              "ecce-rename without exactly two arguments: usage, 64")

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        olog = os.path.join(s.state, "organizer.log")
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
            env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
            cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=120):
            seen = len(open(olog, errors="replace").read().splitlines())
            with open(cmds, "a") as h:
                h.write(line + "\n")
            want = "ECCE_TEST_ORGANIZER: %s: " % line
            end = time.time() + timeout
            while time.time() < end:
                if org.poll() is not None:
                    return "the Organizer ended (%s)" % org.returncode
                for t in open(olog, errors="replace").read().splitlines()[seen:]:
                    if t.startswith(want):
                        return t[len(want):]
                time.sleep(0.5)
            return "no answer"

        check("selected=" in send("tree"), "the Organizer is up")
        home = s.localData() + "/users/local"
        for proj in ("dest", "busy"):
            check(send("newproject " + proj).startswith("ok"), "project %s made" % proj)

        def make(parent, name):
            url, out = (None, "")
            rc, out = s.driver("create", "file://" + parent, name, "cp2k_es", deck,
                               "cp2k.cp2kin", "localhost", rundir, s.user(),
                               "procs=1", "queue=normal", "nodes=1", "wall=0 1:0")
            if rc != 0:
                say(out)
                return None
            return out.strip().splitlines()[-1]

        urls = {}
        for name, parent in (("one", home), ("two", home), ("ren", home),
                             ("run", home + "/busy")):
            urls[name] = make(parent, name)
            check(urls[name] is not None, "calculation %s created" % name)
        say("urls: %r" % urls)
        if None in urls.values():
            return 1
        rc, out = s.launch(urls["run"])
        check(rc == 0, "run is launched")
        end = time.time() + 120
        while time.time() < end and s.state_of(urls["run"]) not in ("submitted", "running"):
            time.sleep(2)
        check(s.state_of(urls["run"]) in ("submitted", "running"), "run is active")

        def go(exe, *argv):
            #  The Organizer needs a moment to subscribe.
            end = time.time() + 120
            while True:
                rc, out, err = run(exe, *argv)
                if rc != 2 or time.time() > end:
                    return rc, out, err
                time.sleep(3)

        def known(url):
            """Does the Organizer find the node? (annotate answers per node)"""
            rc, out, err = run(ann, url, "probe")
            return ": annotated" in out

        dest = home + "/dest"
        P = lambda n: urls[n][len("file://"):]
        rc, out, err = go(mv, P("one"), P("two"), dest)
        say(out)
        check(rc == 0 and "one: moved to %s" % dest in out and
              "two: moved to %s" % dest in out,
              "two calculations moved: exit 0 and a line each (rc=%s)" % rc)
        check(os.path.isdir(dest + "/one") and not os.path.exists(P("one")),
              "the calculation is in the folder on disk")
        check(known("file://" + dest + "/one") and known(dest + "/two"),
              "the Organizer finds them at the new URL")
        check(not known(urls["one"]), "and no longer at the old one")

        #  ecce-copy (#890): a copy, also of a running calculation; the
        #  source stays and a name clash is refused.
        arch = home + "/arch"
        os.makedirs(arch, exist_ok=True)
        rc, out, err = go(cp, P("run"), arch)
        check(rc == 0 and "run: copied to %s" % arch in out and os.path.isdir(arch + "/run") and
              os.path.isdir(P("run")), "ecce-copy copies a running calculation, the source stays (rc=%s %r)" % (rc, out))
        rc, out, err = go(cp, P("run"), arch)
        check(rc == 1 and "not copied" in out, "a name clash is refused (rc=%s %r)" % (rc, out))
        check(run(cp, "x")[0] == 64, "ecce-copy without a folder: usage, 64")

        #  A running calculation, and the folder holding it, are refused.
        rc, out, err = go(mv, P("run"), P("ren"), dest)
        say(out)
        check(rc == 1 and "run: not moved: " in out and "submitted or running" in out and
              "ren: moved to" in out,
              "a running calculation is refused, the other moves (rc=%s)" % rc)
        check(os.path.isdir(P("run")) and os.path.isdir(dest + "/ren"),
              "the running one stayed, the other moved")
        rc, out, err = go(mv, home + "/busy", dest)
        check(rc == 1 and "busy: not moved: " in out and "run" in out,
              "a folder holding a running calculation is refused (rc=%s %r)" % (rc, out))
        rc, out, err = go(rn, P("run"), "run2")
        check(rc == 1 and "not renamed: " in out and os.path.isdir(P("run")),
              "a running calculation is not renamed (rc=%s)" % rc)

        #  Rename, and what is refused.
        rc, out, err = go(rn, dest + "/ren", "renamed")
        check(rc == 0 and "ren: renamed to renamed" in out and
              os.path.isdir(dest + "/renamed") and known(dest + "/renamed"),
              "rename: exit 0, on disk and in the Organizer (rc=%s %r)" % (rc, out))
        rc, out, err = go(rn, dest + "/renamed", "one")
        check(rc == 1 and "not renamed: " in out and os.path.isdir(dest + "/renamed"),
              "rename onto an existing name is refused (rc=%s %r)" % (rc, out))
        rc, out, err = go(rn, dest + "/renamed", "bad name!")
        check(rc == 1 and "not renamed" in out, "an invalid name is refused (rc=%s)" % rc)
        #  A clash on move: make a second "one" and try to move it in.
        urls["one2"] = make(home, "one")
        rc, out, err = go(mv, P("one2"), dest)
        check(rc == 1 and "one: not moved: " in out and os.path.isdir(P("one2")),
              "a name clash on move is refused, nothing overwritten (rc=%s %r)" % (rc, out))
        rc, out, err = go(mv, P("one2"), home + "/no-such-folder")
        check(rc == 1 and "not moved" in out, "an unknown folder is refused (rc=%s)" % rc)
        rc, out, err = go(mv, P("one2"), dest + "/one")
        check(rc == 1 and "not moved" in out, "a calculation is no target folder (rc=%s)" % rc)
        rc, out, err = go(mv, os.path.join(s.state, "decks"), dest)
        check(rc == 1 and "not inside the data folder" in out,
              "a path outside the data is refused (rc=%s)" % rc)
        rc, out, err = go(mv, "file:///etc", dest)
        check(rc == 1 and "not moved" in out and org.poll() is None,
              "a URL outside the data is refused and the Organizer survives (rc=%s)" % rc)
        check(send("tree").startswith("ok"), "the Organizer still answers")
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
