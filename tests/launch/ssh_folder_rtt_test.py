#!/usr/bin/env python3
"""Round trips of one click in an SSH-folder tree (#1082).

Run by ssh_folder_rtt_test.sh (test sshd, private ~/.ssh).  A folder of 50
entries (8 calculations from tests/apps/fixtures, 20 plain folders, 21
files, and a project of 13 calculations) is shown twice in an Organizer:
with the batch cache and with ECCE_SFTP_NOBATCH=1.  Selecting the folder
and then the project must take at most 3 remote scripts each with the
cache, many more without, and the tree children, State cells and energy
cells must be identical.  Reads the count with `server rtt`.

Exit status 77 (CTest SKIP) without Xvfb or the built binaries.
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

failures = []
PORT = int(os.environ.get("ECCE_TEST_SSH_PORT", "2236"))
FOLDER = "/home/bashuser/ecce-rtt"
BASE = "sftp://bashuser@sshport:%d%s" % (PORT, FOLDER)
FIX = os.path.join(REPO, "tests", "apps", "fixtures")
CALCS = ["calc-water-opt", "calc-water-vib", "calc-water-freq-g16", "calc-water-smd-g16",
         "calc-orca-water-ccpvdz", "calc-polar-h2o-orca", "calc-polar-co2-g16", "calc-water-orca-nbo"]


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def onServer(cmd, stdin=None):
    r = subprocess.run(["ssh", "-p", str(PORT), "bashuser@sshport", cmd], stdin=stdin,
                       capture_output=True, text=True, timeout=120)
    if r.returncode:
        say("  ssh to the test sshd failed (%d): %s" % (r.returncode, r.stderr.strip()[-200:]))
    return r.stdout


def populate():
    onServer("rm -rf %s; mkdir -p %s/proj16" % (FOLDER, FOLDER))
    tar = subprocess.Popen(["tar", "-C", FIX, "-cf", "-"] + CALCS, stdout=subprocess.PIPE)
    onServer("cd %s && tar -xf -" % FOLDER, stdin=tar.stdout)
    tar.wait()
    # 13 calculations in the project: copies of the eight and five again.
    cmds = ["cd %s" % FOLDER]
    for i in range(13):
        cmds.append("cp -a %s proj16/c%02d-%s" % (CALCS[i % 8], i, CALCS[i % 8]))
    for i in range(20):
        cmds.append("mkdir dir%02d && echo x > dir%02d/f.txt" % (i, i))
    for i in range(21):
        cmds.append("echo data%d > file%02d.txt" % (i, i))
    onServer("; ".join(cmds))


def scenario(s, build, wrappers, control, display, label, extra_env):
    cmds = os.path.join(s.state, "commands-" + label)
    open(cmds, "w").close()
    olog = os.path.join(s.state, "organizer-%s.log" % label)
    genv = s.env({"DISPLAY": display.name, "PATH": wrappers + os.pathsep + s.env()["PATH"],
                  "ECCE_NATIVE_DIALOGS": "explicit", "ECCE_SSH_CONTROL_DIR": control,
                  "ECCE_ASKPASS": "/bin/false", "ECCE_TEST_ORGANIZER": cmds})
    genv.update(extra_env)
    org = subprocess.Popen([os.path.join(wrappers, "ecce-organizer")], env=genv,
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
                return "the Organizer ended"
            for t in open(olog, errors="replace").read().splitlines()[seen:]:
                if t.startswith(want):
                    return t[len(want):]
            time.sleep(0.05)
        return "no answer"

    r = {}
    try:
        send("server roots")
        r["connect"] = send("server connect Folder")
        r["roots"] = send("server roots")
        base = send("server rtt")
        r["login"] = int(base) if base.isdigit() else -1
        for step, url in (("folder", BASE + "/"), ("project", BASE + "/proj16")):
            n0 = int(send("server rtt"))
            r[step + "-select"] = send("select " + url)
            r[step + "-rtt"] = int(send("server rtt")) - n0
        r["children"] = send("treechildren " + BASE + "/")
        if r["children"] == "not in the tree":
            # What the tree holds, for the log of a failed run.
            r["bare-children"] = send("treechildren " + BASE)
            r["walk"] = send("walk")
            say("  organizer log tail:\n" + "\n".join(l for l in open(olog, errors="replace").read().splitlines()
                                                 if "SftpEDSI" in l or "rror" in l)[-3000:])
        r["states"] = send("tablestates")
        r["energies"] = send("tableenergies")
    finally:
        if org.poll() is None:
            os.killpg(org.pid, 15)
            org.wait()
    return r


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

    s = harness.Session(build, "sshrtt", {}, local=True, keep=args.keep)
    harness.link(os.path.join(build, "organizer"), os.path.join(s.home, "bin", "organizer"))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        os.environ.setdefault("ECCE_REALUSER", s.user())
        populate()
        with open(os.path.join(s.state, ".ECCE", "AddedServers"), "w") as h:
            h.write("ssh\tFolder\t%s\t0\n" % BASE)
        control = os.path.join(s.state, "cm")
        os.makedirs(control, mode=0o700, exist_ok=True)
        os.chmod(control, 0o700)
        display = xdisplay.Display().__enter__()
        old = scenario(s, build, wrappers, control, display, "nobatch", {"ECCE_SFTP_NOBATCH": "1"})
        new = scenario(s, build, wrappers, control, display, "batch", {})
        for k in sorted(new):
            say("  %-16s batch=%s  no-batch=%s" % (k, str(new[k])[:70], str(old[k])[:70]))
        check(new["connect"] == "connected", "the folder connects")
        for step in ("folder", "project"):
            check(0 <= new[step + "-rtt"] <= 3, "%s click: %d scripts with the cache (<= 3)" % (step, new[step + "-rtt"]))
            check(old[step + "-rtt"] > 10, "%s click: %d scripts without it" % (step, old[step + "-rtt"]))
        for k in ("children", "states", "energies"):
            check(new[k] == old[k], "%s identical with and without the cache" % k)
        check(new["children"].count(" ") >= 49, "the folder shows its 50 entries")
        check("|" in new["energies"], "the project table has energy cells")
    finally:
        s.services(False)
        if display:
            display.__exit__(None, None, None)
    say("FAIL: %d" % len(failures) if failures else "PASS")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
