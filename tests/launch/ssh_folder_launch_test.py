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
        onServer("rm -rf ecce-data; mkdir ecce-data", PORT2)
        with open(os.path.join(s.state, ".ECCE", "AddedServers"), "w") as h:
            h.write("ssh\tFolder\t%s\nssh\tBad\t%s\nssh\tOther\t%s\n" % (BASE, BAD, OTHER))
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

        ans = send("server roots")
        check("Folder (SSH) (not connected)" in ans and "Bad (SSH) (not connected)" in ans
              and ans.startswith("Local data"), "both SSH roots start not connected: %s" % ans)
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
        check(ans == "allowed", "copy to Local data is allowed (#890): %s" % ans)

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

        # Copy and move between trees (#890).
        def srvFiles(path, port=None):
            out = onServer("cd %s && find . -type f ! -name '.*' -printf '%%P %%s\\n' | sort" % path, port)
            return [l.strip() for l in out.splitlines() if l.strip()]

        def locFiles(path):
            res = []
            for r, d, f in os.walk(path):
                for n in f:
                    if not n.startswith("."):
                        res.append("%s %d" % (os.path.relpath(os.path.join(r, n), path),
                                              os.path.getsize(os.path.join(r, n))))
            return sorted(res)

        def formula(u):
            a = send("summary " + u)
            return a.split("Formula=")[1].split(" ")[0] if "Formula=" in a else a

        srcFiles = srvFiles(calc)
        srcFormula = formula(url)
        check(srcFiles and srcFormula not in ("", "<none>"), "the source calculation has files and a formula: %s" % srcFormula)
        ans = send("newproject lproj")
        lproj = ans.split(" ", 1)[1] if ans.startswith("ok ") else ""
        check(bool(lproj), "a local project: %s" % ans)
        ans = send("server xfer copy %s %s" % (url, lproj))
        check("\tok\t" in ans, "SSH -> Local copy: %s" % ans)
        lcalc = lproj.replace("file://", "") + "/" + name
        check(locFiles(lcalc) == srcFiles, "every file arrived with its size (%d files)" % len(srcFiles))
        check(os.path.exists(os.path.join(lcalc, ".ecce-meta")), "the .ecce-meta arrived")
        check(onServer("test -d %s && echo yes" % calc).strip() == "yes", "the SSH source is untouched")
        check(formula("file://" + lcalc) == srcFormula, "its summary opens from Local data: %s" % formula("file://" + lcalc))
        ans = send("server xfer copy %s %s" % (url, lproj))
        check("\tno\t" in ans and "already" in ans, "a name clash is refused: %s" % ans)
        check(not [x for x in os.listdir(lproj.replace("file://", "")) if x != name], "the refusal left no partial copy")

        ans = send("server newproject Folder pB")
        pB = ans.split(" ", 1)[1] if ans.startswith("ok ") else ""
        ans = send("server xfer copy file://%s %s" % (lcalc, pB))
        check("\tok\t" in ans, "Local -> SSH copy: %s" % ans)
        check(srvFiles("ecce-data/pB/" + name) == srcFiles, "every file is on the server with its size")
        check(formula(pB + "/" + name) == srcFormula, "its summary opens from the SSH tree")

        # An interrupted copy leaves the source intact and no partial target.
        ans = send("server newproject Other pI")
        pI = ans.split(" ", 1)[1] if ans.startswith("ok ") else ""
        send("server failafter 3")
        ans = send("server xfer copy file://%s %s" % (lcalc, pI))
        send("server failafter -1")
        check("\tno\t" in ans and "partial copy was removed" in ans, "a failure part-way is reported: %s" % ans)
        check(srvFiles("ecce-data/pI", PORT2) == [] and name not in onServer("ls ecce-data/pI", PORT2),
              "no partial target remains")
        check(locFiles(lcalc) == srcFiles, "the source is intact")
        send("server failafter 3")
        ans = send("server xfer move file://%s %s" % (lcalc, pI))
        send("server failafter -1")
        check("\tno\t" in ans and locFiles(lcalc) == srcFiles and name not in onServer("ls ecce-data/pI", PORT2),
              "a failed move keeps the source and leaves no target: %s" % ans)

        # A running calculation is not moved (copy is allowed).
        ans = send("setstate file://%s running" % lcalc)
        check(ans == "ok", "marked running: %s" % ans)
        ans = send("server newproject Other pguard")
        pg = ans.split(" ", 1)[1] if ans.startswith("ok ") else ""
        ans = send("server xfer move file://%s %s" % (lcalc, pg))
        check("\tno\t" in ans and "submitted or running" in ans, "moving a running calculation is refused: %s" % ans)
        check(os.path.isdir(lcalc) and not srvFiles("ecce-data/pguard", PORT2) , "the source stays and nothing arrived")
        ans = send("setstate file://%s completed" % lcalc)

        # Move Local -> SSH: the source goes only after the check.
        ans = send("server xfer move file://%s %s" % (lcalc, pg))
        check("\tok\t" in ans, "Local -> SSH move: %s" % ans)
        check(srvFiles("ecce-data/pguard/" + name, PORT2) == srcFiles, "the moved calculation is complete on the other server")
        check(not os.path.exists(lcalc), "the Local source is gone after the move")
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
