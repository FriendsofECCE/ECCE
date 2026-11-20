#!/usr/bin/env python3
"""Tools > Allocations Refresh with no live login (#1226).

Run by allocations_ssh_test.sh (test sshd, private ~/.ssh).  Two registered
machines with an allocation command and a profile account: `alloc` (key
login works) and `allocbad` (key refused).  Opening the window with no
connection must not log in ("not checked: ... press Refresh"); Refresh
(`allocations refresh`) must log in to `alloc` and fill its row, and leave
`allocbad` as "not checked: not logged in to sshbad...".

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
# projinfo's layout, so a working login shows a number in the row.
COMMAND = ("printf 'Information for project acct-1:\\nAllocations:\\n"
           "  sshtest:  30000 CPUhours/month, used 7.4%% the past 30 days\\n'")
NONE = ["", "", "", ""]


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def register(ue):
    with open(os.path.join(ue, "MyMachines"), "a") as h:
        for name, host in (("alloc", "sshtest"), ("allocbad", "sshbad")):
            h.write("%s\t%s\tLinux\tx86_64\ttest\t4:1\tssh\t:NWChem\tMN:RD:SD:UN:AA\n"
                    % (name, host))
    with open(os.path.join(ue, "MachPrefs"), "a") as h:
        for name in ("alloc", "allocbad"):
            h.write("%s:\tbashuser|ssh||/home/bashuser/ecce||acct-1|1|1|60|0|0|1\n" % name)
    for name in ("alloc", "allocbad"):
        with open(os.path.join(ue, "CONFIG." + name), "w") as h:
            h.write("allocationCommand: %s\n" % COMMAND)


def rows(log, seen):
    out = {}
    for t in open(log, errors="replace").read().splitlines()[seen:]:
        if t.startswith("ALLOC: "):
            f = t[7:].split("|")
            out[f[0]] = (f + NONE)[:4]
    return out


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

    s = harness.Session(build, "allocssh", {}, local=True, keep=args.keep)
    harness.link(os.path.join(build, "organizer"), os.path.join(s.home, "bin", "organizer"))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        os.environ.setdefault("ECCE_REALUSER", s.user())
        register(os.path.join(s.state, ".ECCE"))
        control = os.path.join(s.state, "cm")
        os.makedirs(control, mode=0o700, exist_ok=True)
        os.chmod(control, 0o700)
        display = xdisplay.Display().__enter__()
        cmds = os.path.join(s.state, "commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        # /bin/false: a password prompt would be an error; both logins use keys.
        genv = s.env({"DISPLAY": display.name, "PATH": wrappers + os.pathsep + s.env()["PATH"],
                      "ECCE_NATIVE_DIALOGS": "explicit", "ECCE_SSH_CONTROL_DIR": control,
                      "ECCE_ASKPASS": "/bin/false", "ECCE_TEST_ORGANIZER": cmds})
        org = subprocess.Popen([os.path.join(wrappers, "ecce-organizer")], env=genv,
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
                    return "the Organizer ended", {}
                for t in open(olog, errors="replace").read().splitlines()[seen:]:
                    if t.startswith(want):
                        return t[len(want):], rows(olog, seen)
                time.sleep(0.1)
            return "no answer", {}

        first, r1 = send("allocations")
        say("  open: %s %s" % (first, r1))
        check(set(r1) == {"alloc", "allocbad"}, "both machines have a row")
        a = r1.get("alloc", NONE)
        check(a[2].startswith("not checked: not logged in to sshtest") and a[3] == "",
              "opening the window does not log in (%s)" % a[2])
        check(not os.listdir(control), "no shared connection after opening the window")
        out, r2 = send("allocations refresh")
        say("  refresh: %s %s" % (out, r2))
        a = r2.get("alloc", NONE)
        check(a[1] == "acct-1" and "7.4" in a[2] and a[3] != "",
              "Refresh logs in and fills the row (%s | %s)" % (a[2], a[3]))
        b = r2.get("allocbad", NONE)
        check(b[2].startswith("not checked: not logged in to sshbad") and b[3] == "",
              "a refused login says why (%s)" % b[2])
    finally:
        if org and org.poll() is None:
            os.killpg(org.pid, 15)
            org.wait()
        s.services(False)
        if display:
            display.__exit__(None, None, None)
    say("FAIL: %d" % len(failures) if failures else "PASS")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
