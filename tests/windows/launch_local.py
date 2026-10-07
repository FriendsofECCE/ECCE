#!/usr/bin/env python3
"""Local job end to end on Windows, with no GUI (#133).

launchjob create/launch -> gensub job script (sh) -> a stub "mopac" ->
eccejobmaster/eccejobstore/eccejobmonitor -> state on the local data folder.
Run from MSYS2 UCRT64 python (b.bat) on the VM:

    python3 tests/windows/launch_local.py complete|cancel [--build DIR]
"""
import argparse
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")
OUT = os.path.join(REPO, "tests", "parsers", "fixtures", "mopac", "ch4_force.out")
FINAL = ("completed", "loaded", "failed", "killed", "unsuccessful", "system_failure")


def say(t):
    print(t, flush=True)


class S:
    def __init__(self, build, state):
        self.build, self.state = build, state
        self.fail = []
        shutil.rmtree(state, ignore_errors=True)
        for d in (".ECCE", "tmp", "jobs"):
            os.makedirs(os.path.join(state, d))
        self.env = dict(os.environ)
        # WINTEST_HOME: an install tree (packaging/windows) instead of the checkout;
        # WINTEST_BASE_PATH: the PATH to start from, e.g. one without MSYS2's usr\bin.
        home = os.environ.get("WINTEST_HOME", REPO).replace("\\", "/")
        base_path = os.environ.get("WINTEST_BASE_PATH", os.environ["PATH"])
        self.env.update({
            "ECCE_HOME": home, "ECCE_REALUSERHOME": state,
            "ECCE_REALUSER": "andy", "HOST": "localhost",
            "ECCE_TMPDIR": os.path.join(state, "tmp"),
            "ECCE_LOCAL_DATA": os.path.join(state, "localdata"),
            "ECCE_SESSION_ID": "wintest",
            "PATH": os.pathsep.join(([os.environ["WINTEST_PATH"]]
                                     if "WINTEST_PATH" in os.environ else []) +
                                    [os.path.join(home, "scripts"),
                                     os.path.join(home, "scripts", "parsers"),
                                     build, base_path]),
        })
        for k in ("ECCE_TMPDIR", "ECCE_LOCAL_DATA"):
            self.env[k] = self.env[k].replace("\\", "/")
        if "NO_MESSAGING" in os.environ.get("WINTEST", ""):
            self.env["ECCE_NO_MESSAGING"] = "1"
        for k in ("ECCE_TRANSPORT", "DISPLAY"):
            self.env.pop(k, None)

    def check(self, ok, what):
        say("  %s %s" % ("ok  " if ok else "FAIL", what))
        if not ok:
            self.fail.append(what)
        return ok

    def drv(self, *a, timeout=180):
        r = subprocess.run([os.path.join(self.build, "launchjob.exe"),
                            "-pipe", os.devnull] + list(a), env=self.env,
                           cwd=self.build, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT, timeout=timeout)
        return r.returncode, r.stdout.decode("utf-8", "replace")

    def last(self, *a):
        rc, out = self.drv(*a)
        return out.strip().splitlines()[-1] if out.strip() else ""

    def wait(self, url, secs, want=FINAL):
        st, end = "", time.time() + secs
        while time.time() < end:
            st = self.last("state", url)
            if st in want:
                break
            time.sleep(1)
        return st


def stub(state, delay):
    path = os.path.join(state, "stubmopac")
    with open(path, "w", newline="\n") as h:
        h.write("#!/bin/sh\necho \"stub args: $*\" >> %s/stub.log\nsleep %d\n"
                "cat %s > mopac.out\n" % (state.replace("\\", "/"), delay,
                              OUT.replace("\\", "/")))
    return path.replace("\\", "/")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("case", choices=("complete", "cancel"))
    ap.add_argument("--build", default=os.path.join(REPO, "build"))
    ap.add_argument("--state", default=os.path.expanduser("~/launchtest"))
    a = ap.parse_args()
    # Forward slashes: paths reach sh scripts, where a backslash is an escape.
    s = S(os.path.abspath(a.build).replace("\\", "/"),
          os.path.abspath(a.state).replace("\\", "/"))
    st = s.state.replace("\\", "/")
    with open(os.path.join(st, ".ECCE", "CONFIG.localhost"), "w", newline="\n") as h:
        h.write("MOPAC: %s\n" % stub(st, 2 if a.case == "complete" else 300))
    url = s.userUrl = "file://%s/localdata/users/local" % st
    rc, out = s.drv("create", url, "wintest", "mopac_es", DECK, "mopac.mop",
                    "localhost", st + "/jobs", "andy")
    say(out)
    if not s.check(rc == 0, "calculation created"):
        return 1
    calc = out.strip().splitlines()[-1]
    rc, out = s.drv("launch", calc)
    say(out)
    s.check(rc == 0, "Launch ran to the end")
    jobid = s.last("jobid", calc)
    say("job id: " + jobid)
    if a.case == "complete":
        state = s.wait(calc, 120)
        s.check(state in ("completed", "loaded"), "state completed (%s)" % state)
        s.check("TE" in s.drv("props", calc)[1].split(), "TE property parsed")
    else:
        time.sleep(10)
        rc, out = s.drv("kill", calc)
        say(out)
        state = s.wait(calc, 120)
        s.check(state == "killed", "state killed (%s)" % state)
    say("")
    say("FAILED: " + "; ".join(s.fail) if s.fail else "PASSED")
    return 1 if s.fail else 0


if __name__ == "__main__":
    sys.exit(main())
