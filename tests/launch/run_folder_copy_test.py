#!/usr/bin/env python3
"""A remote job's run folder is a marked copy of the calculation (#1083).

Run by run_folder_copy_test.sh (test sshd, private ~/.ssh).  A MOPAC water
calculation in Local data is launched on an ssh machine (the sshd, which has
MOPAC); in its run folder there must be .ecce-meta with the copy markers,
Parameters/ and Inputs/ after the launch, and Props/ and the state completed
after the store.  An Organizer with that run folder's parent as an SSH folder
labels the copy "(copy of ..." and its TE equals the original's.

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
"""
import argparse
import os
import shutil
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402

failures = []
PORT = int(os.environ.get("ECCE_TEST_SSH_PORT", "2242"))
DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")
JOBS = "/home/bashuser/ecce-jobs"


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def onServer(cmd):
    r = subprocess.run(["ssh", "-p", str(PORT), "bashuser@sshport", cmd],
                       capture_output=True, text=True, timeout=60)
    if r.returncode:
        say("  ssh to the test sshd failed (%d): %s" % (r.returncode, r.stderr.strip()[-200:]))
    return r.stdout


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
    import xdisplay

    s = harness.Session(build, "runcopy", {"MOPAC": "/usr/bin/mopac"}, local=True, keep=args.keep)
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
    s.registerMachine("sshtest", "Shell", {"MOPAC": "/usr/bin/mopac"}, host="sshtest")
    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        os.environ.setdefault("ECCE_REALUSER", s.user())
        onServer("rm -rf %s; mkdir -p %s" % (JOBS, JOBS))
        name = "copy-%d" % int(time.time())
        rundir = "%s/%s" % (JOBS, name)
        rc, out = s.driver("create", s.userUrl(), name, "mopac_es", DECK, "mopac.mop",
                           "sshtest", rundir, "bashuser")
        url = out.strip().splitlines()[-1] if rc == 0 else None
        if not check(url and url.startswith("file://"), "calculation made in Local data: %s" % (url or out[-300:])):
            return 1
        rc, out = s.launch(url)
        say("\n".join("  | " + l for l in out.strip().splitlines()[-12:]))
        if not check(rc == 0, "Launch ran to the end"):
            return 1
        ran = [l.split(":", 1)[1].strip() for l in out.splitlines() if l.startswith("run directory:")]
        rundir = ran[-1] if ran else rundir
        say("  run directory: " + rundir)

        listing = onServer("ls -A %s" % rundir).split()
        check(".ecce-meta" in listing, "the run folder has .ecce-meta: %s" % listing)
        check("Parameters" in listing and "Inputs" in listing,
              "Parameters/ and Inputs/ are in the run folder after the launch")
        meta = onServer("cat %s/.ecce-meta" % rundir)
        check("copyOf" in meta and url in meta, "ecce:copyOf is the original's URL (%s)" % url)
        for key in ("copyOrigin", "copyOriginRoot", "copyMadeAt"):
            check(key in meta, "%s is stamped" % key)

        state = s.waitState(url, 240)
        check(state == "completed", "the job completed (%s)" % state)
        listing = onServer("ls -A %s" % rundir).split()
        for d in ("Parameters", "Inputs", "Props", "Outputs"):
            check(d in listing, "%s/ is in the run folder after the store" % d)
        props = onServer("ls %s/Props" % rundir).split()
        say("  properties in the run folder: " + " ".join(props))
        check("TE" in props, "TE is in the run folder's Props/")
        origTE = open(os.path.join(url[len("file://"):], "Props", "TE"), "rb").read() \
            if os.path.exists(os.path.join(url[len("file://"):], "Props", "TE")) else None
        check(origTE is not None, "the original has Props/TE")
        copyTE = onServer("cat %s/Props/TE" % rundir).encode()
        check(origTE is not None and copyTE.strip() == origTE.strip(),
              "the copy's TE file equals the original's")

        folder = "sftp://bashuser@sshport:%d%s" % (PORT, JOBS)
        copyUrl = folder + "/" + name
        rc, out = s.driver("state", copyUrl)
        check(out.strip().splitlines()[-1:] == ["completed"], "the copy's own state is completed: %s" % out.strip()[-60:])
        rc, out = s.driver("props", copyUrl)
        check("TE" in out.split(), "the copy's properties list TE: %s" % out.split())

        display = xdisplay.Display().__enter__()
        control = os.path.join(s.state, "cm")
        os.makedirs(control, mode=0o700, exist_ok=True)
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"],
                      "ECCE_NATIVE_DIALOGS": "explicit",
                      "ECCE_SSH_CONTROL_DIR": control,
                      "ECCE_ASKPASS": "/bin/false"})
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

        ans = send("prefs data add ssh Runs sshport:%d bashuser %s %s" % (PORT, JOBS, os.path.join(s.state, "add.png")))
        check(ans == "saved", "SSH folder of the run folders added: %s" % ans)
        send("server sync")
        ans = send("server connect Runs")
        check(ans == "connected", "the run folder's parent connects: %s" % ans)
        ans = send("treechildren " + folder)
        check(ans.startswith("ok ") and name in ans and "(copy of" in ans,
              "the Organizer labels the copy: %s" % ans)
        ans = send("summary " + copyUrl)
        say("  summary of the copy: " + ans[:160])
        check("Formula=" in ans, "the copy opens from the SSH tree")
    finally:
        if org is not None and org.poll() is None:
            os.killpg(org.pid, signal.SIGTERM)
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
