#!/usr/bin/env python3
"""SSH-folder startup choices, server menus and launching (#767, #795).

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
import signal
import shlex
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
# The port is in the address (#789); Host sshport has no Port line.
PORT = int(os.environ.get("ECCE_TEST_SSH_PORT", "2232"))
PORT2 = int(os.environ.get("ECCE_TEST_SSH_PORT2", str(PORT + 1)))
BASE = "sftp://bashuser@sshport:%d/home/bashuser/ecce-data" % PORT
OTHER = "sftp://bashuser@sshport:%d/home/bashuser/ecce-data" % PORT2
BAD = "sftp://bashuser@sshbad/home/bashuser/ecce-data"


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def onServer(cmd, port=None):
    r = subprocess.run(["ssh", "-p", str(port or PORT), "bashuser@sshport", cmd], capture_output=True, text=True, timeout=60)
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
    # Leave a live monitor to check after Disconnect, even for this tiny job.
    slow_xtb = os.path.join(s.state, "slow-xtb")
    with open(slow_xtb, "w") as h:
        h.write('#!/bin/sh\nsleep 10\nexec %s "$@"\n' % shlex.quote(xtb))
    os.chmod(slow_xtb, 0o755)
    s.registerMachine("localhost", "Shell", {"xTB": slow_xtb})
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
        onServer("rm -rf ecce-data; mkdir ecce-data", PORT2)
        with open(os.path.join(s.state, ".ECCE", "AddedServers"), "w") as h:
            h.write("ssh\tBad\t%s\t0\nssh\tOther\t%s\t0\n" % (BAD, OTHER))
        # ECCE's own masters: one per route, so the port must tell them apart.
        control = os.path.join(s.state, "cm")
        os.makedirs(control, mode=0o700, exist_ok=True)
        os.chmod(control, 0o700)

        def drive(*argv):
            return s.run([os.path.join(s.home, "bin", "launchjob")] + list(argv),
                         cwd=os.path.join(s.home, "bin"))

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"],
                      "ECCE_NATIVE_DIALOGS": "explicit",
                      "ECCE_SSH_CONTROL_DIR": control,
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

        pictures = os.environ.get("RUNDIR", rundir)
        os.makedirs(pictures, exist_ok=True)
        ans = send("prefs data add ssh Folder sshport:%d bashuser /home/bashuser/ecce-data %s" %
                   (PORT, os.path.join(pictures, "add-server.png")))
        check(ans == "saved", "add SSH folder through the Add Server dialog: %s" % ans)
        send("server sync")
        ans = send("server roots")
        check("Folder (SSH) (not connected)" in ans and "Bad (SSH) (not connected)" in ans
              and ans.startswith("Local data"), "both SSH roots start not connected: %s" % ans)
        ans = send("server menu")
        check("[ ] Folder" in ans and "[ ] Other" in ans and
              "Add Server...;Manage Servers..." in ans, "toolbar lists remembered servers and ticks: %s" % ans)
        check(send("server startup Folder on") == "ok", "set Connect at start in Edit Server")
        os.killpg(org.pid, signal.SIGTERM)
        org.wait(20)
        open(cmds, "w").close()
        org = subprocess.Popen([os.path.join(wrappers, "ecce-organizer")],
                               env=dict(genv, ECCE_TEST_ORGANIZER=cmds),
                               cwd=os.path.join(s.home, "bin"), stdout=open(olog, "w"),
                               stderr=subprocess.STDOUT, start_new_session=True)
        ans = send("server menu")
        check("[x] Folder" in ans and "[ ] Other" in ans and "[ ] Bad" in ans,
              "restart connects only Connect-at-start entry: %s" % ans)
        check(send("server rootmenu Folder") == "Disconnect;Remove from list",
              "connected root offers Disconnect and Remove")
        check(send("server rootchoose Folder Disconnect") == "ok", "root Disconnect action")
        check("[ ] Folder" in send("server menu"), "disconnect clears toolbar tick")
        check("Folder (SSH) (not connected)" in send("server roots"), "disconnect folds root")
        check(send("server rootchoose Folder Connect") == "ok", "root Connect action")
        check("[x] Folder" in send("server menu"), "connect sets toolbar tick")
        check(send("server choose Folder") == "ok", "toolbar toggles server off")
        check("[ ] Folder" in send("server menu"), "toolbar toggle clears tick")
        check(send("server choose Other") == "ok", "toolbar connects second server")
        check("[x] Other" in send("server menu"), "second server tick set")
        check(send("server rootchoose Other Remove_from_list") == "ok", "root Remove action")
        check("Other" not in send("server menu") and "Other" not in send("server roots"),
              "Remove forgets entry in both menus and tree")
        # Restore the second entry for the existing cross-port launch checks.
        with open(os.path.join(s.state, ".ECCE", "AddedServers"), "a") as h:
            h.write("ssh\tOther\t%s\t0\n" % OTHER)
        send("server sync")
        ans = send("server menushot " + os.path.join(pictures, "servers-menu.png"))
        check(ans == "saved", "PNG of open Servers toolbar menu: %s" % ans)
        # A tree search past a root that is not connected (here: a URL in no
        # tree) used to recurse without end and crash the Organizer.
        ans = send("panelmessage file:///nowhere/p/c")
        check(ans == "not in the tree", "a search past the unconnected roots ends: %s" % ans)
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
        ans = send("server connect Other")
        check(ans == "connected", "the folder on the other port connects: %s" % ans)
        ans = send("server newproject Other p789")
        check(ans.startswith("ok sftp://bashuser@sshport:%d/" % PORT2), "project made on the other port: %s" % ans)
        one, two = onServer("ls ecce-data"), onServer("ls ecce-data", PORT2)
        check("p789" in two and "p767" not in two and "p789" not in one,
              "two ports on one host reach two servers, no shared master: %r / %r" % (one, two))
        socks = [f for f in os.listdir(control) if len(f) == 16 and "." not in f]
        say("  owned masters: %s" % socks)
        check(len(socks) >= 2, "one owned master per port")
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
                        user, "procs=1", "queue=normal", "nodes=1", "code=xTB", "xyz=" + xyz)
        url = out.strip().splitlines()[-1] if rc == 0 else None
        if not check(url and url.startswith("sftp"), "xtb calculation made in the folder: %s" % (url or out[-300:])):
            return 1
        rc, out = drive("state", url)
        check(out.strip().splitlines()[-1] == "ready", "it is ready: %s" % out.strip()[-40:])
        ans = send("server launchcheck " + url)
        check(ans == "allowed", "launch from the connected folder is allowed: %s" % ans)
        ans = send("launchselected " + url, timeout=240)
        check(ans.startswith("ok ") and "submitted" in ans, "Launch Selected submitted it: %s" % ans)
        rc, out = drive("state", url)
        check(out.strip().splitlines()[-1] in ("submitted", "running"),
              "job is still live before Disconnect: %s" % out.strip()[-40:])
        check(send("server disconnect Folder") == "ok", "disconnect after launch leaves monitor running")
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
        check("bashuser" in onServer("cat %s/.ecce-meta ecce-data/.ecce-meta 2>/dev/null; true" % calc),
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
