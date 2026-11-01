#!/usr/bin/env python3
"""An added SSH folder as an Organizer tree, with launching (#767, b2).

Run by tests/launch/ssh_folder_test.sh, which starts the test sshd and gives
this process a private ~/.ssh (Host sshtest = the sshd with the test key,
Host sshbad = the same sshd with a key it does not know).  A local-mode
Organizer with both as `ssh` entries: the roots are "(not connected)"; the
bad one stays so and a launch from it is refused; the good one connects, a
project and calculation made there appear in the sshd's folder, a copy to
Local data is refused, and an xtb water calculation launched on localhost
stores its outputs and properties in the folder and nothing in Local data.

Exit status 77 (CTest SKIP) without xtb, Xvfb or the built binaries.
"""
import argparse
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402
import xtb_test  # noqa: E402

failures = []
BASE = "sftp://bashuser@sshtest/home/bashuser/ecce-data"
BAD = "sftp://bashuser@sshbad/home/bashuser/ecce-data"


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def onServer(cmd):
    r = subprocess.run(["ssh", "bashuser@sshtest", cmd], capture_output=True, text=True, timeout=60)
    if r.returncode:
        say("  ssh to the test sshd failed (%d): %s" % (r.returncode, r.stderr.strip()[-200:]))
    return r.stdout


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    xtb = shutil.which("xtb")
    if not xtb:
        harness.skip("xtb is not installed")
    if not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "sshfolder", {"xTB": xtb}, local=True, keep=args.keep)
    harness.link(os.path.join(build, "organizer"), os.path.join(s.home, "bin", "organizer"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    s.registerMachine("localhost", "Shell", {"xTB": xtb})
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    os.makedirs(work, exist_ok=True)
    os.makedirs(rundir, exist_ok=True)
    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        os.environ.setdefault("ECCE_REALUSER", s.user())
        user = s.user()
        onServer("rm -rf ecce-data; mkdir ecce-data")
        with open(os.path.join(s.state, ".ECCE", "AddedServers"), "w") as h:
            h.write("ssh\tFolder\t%s\nssh\tBad\t%s\n" % (BASE, BAD))

        def drive(*argv):
            return s.run([os.path.join(s.home, "bin", "launchjob")] + list(argv),
                         cwd=os.path.join(s.home, "bin"))

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"],
                      "ECCE_NATIVE_DIALOGS": "explicit",
                      "ECCE_ASKPASS": "/bin/false"})
        if os.environ.get("ECCE_TEST_PRELOAD"):   # e.g. a crash-backtrace library
            genv["LD_PRELOAD"] = os.environ["ECCE_TEST_PRELOAD"]
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen([os.path.join(wrappers, "ecce-organizer")],
                               env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
                               cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
                               stderr=subprocess.STDOUT, start_new_session=True)

        def send(line, timeout=300):
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

        ans = send("server roots")
        check("Folder (SSH) (not connected)" in ans and "Bad (SSH) (not connected)" in ans
              and ans.startswith("Local data"), "both SSH roots start not connected: %s" % ans)
        ans = send("server connect Bad")
        check(ans == "not connected", "a key the server does not know leaves it disconnected: %s" % ans)
        ans = send("server launchcheck %s/p/c" % BAD)
        check(ans == "refused", "a launch from it is refused: %s" % ans)
        ans = send("server connect Folder")
        check(ans == "connected", "the good folder connects: %s" % ans)
        ans = send("server roots")
        check("Folder (SSH) (not connected)" not in ans and "Bad (SSH) (not connected)" in ans,
              "only the good root is connected: %s" % ans)
        ans = send("server newproject Folder p767")
        check(ans.startswith("ok sftp://"), "project created: %s" % ans)
        ans = send("server newcalc Folder p767 c1")
        check(ans.startswith("ok sftp://"), "calculation created: %s" % ans)
        listing = onServer("ls -a ecce-data ecce-data/p767 ecce-data/p767/c1")
        check(".ecce-meta" in listing and "c1" in listing, "they are in the server's folder")
        ans = send("server copycheck %s/p767 file:///tmp" % BASE)
        check(ans.startswith("refused: Copying between Local data and a server"), "copy to Local data refused: %s" % ans)

        text, rc = xtb_test.deck(xtb_test.WATER, "Energy")
        deck = os.path.join(work, "w.xtbin")
        open(deck, "w").write(text)
        xyz = os.path.join(work, "w.xyz")
        with open(xyz, "w") as h:
            h.write("3\nwater\n")
            for a in xtb_test.WATER:
                h.write("%s %.6f %.6f %.6f\n" % a)
        name = "w-%d" % int(time.time())
        rc, out = drive("create", BASE, name, "xtb_es", deck, "xtb.xtbin", "localhost", rundir,
                        "bashuser", "procs=1", "queue=normal", "nodes=1", "code=xTB", "xyz=" + xyz)
        url = out.strip().splitlines()[-1] if rc == 0 else None
        if not check(url and url.startswith("sftp"), "xtb calculation made in the folder: %s" % (url or out[-300:])):
            return 1
        rc, out = drive("state", url)
        check(out.strip().splitlines()[-1] == "ready", "it is ready: %s" % out.strip()[-40:])
        ans = send("server launchcheck " + url)
        check(ans == "allowed", "launch from the connected folder is allowed: %s" % ans)
        ans = send("launchselected " + url, timeout=240)
        check(ans.startswith("ok ") and "submitted" in ans, "Launch Selected submitted it: %s" % ans)
        state = ""
        end = time.time() + 300
        while time.time() < end:
            rc, out = drive("state", url)
            state = out.strip().splitlines()[-1] if out.strip() else ""
            if state in ("completed", "failed", "unsuccessful", "system_failure"):
                break
            time.sleep(2)
        check(state == "completed", "it reached completed (last: %s)" % state)
        calc = "ecce-data/" + url.split("/ecce-data/")[1]
        check("xtb.xtbout" in onServer("ls " + calc + "/Outputs"), "the output is in the server's folder")
        props = onServer("ls " + calc + "/Props").split()
        say("  properties: " + " ".join(props))
        for key in ("TE", "HLGAP", "DIPOLE"):
            check(key in props, "%s is stored in the server's folder" % key)
        check("bashuser" in onServer("cat %s/.ecce-meta ecce-data/.ecce-meta 2>/dev/null" % calc),
              "the owner stamped is the ssh user")
        stray = [r for r, d, f in os.walk(s.localData()) if name in os.path.basename(r)]
        check(not stray, "nothing of it in Local data: %s" % stray)
    finally:
        if org is not None and org.poll() is None:
            org.terminate()
            try:
                org.wait(10)
            except Exception:
                org.kill()
        if display is not None:
            display.__exit__(None, None, None)
        s.stop()
    if failures:
        say("FAILED: " + "; ".join(failures))
        return 1
    say("ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
