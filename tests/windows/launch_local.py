#!/usr/bin/env python3
"""Local job end to end on Windows, with no GUI (#133).

launchjob create/launch -> gensub job script (sh) -> a stub "mopac" ->
eccejobmaster/eccejobstore/eccejobmonitor -> state on the local data folder.
Run from MSYS2 UCRT64 python (b.bat) on the VM:

    python3 tests/windows/launch_local.py complete|cancel [--build DIR]
"""
import argparse
import glob
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
            "ECCE_SESSION_ID": "00c0ffee0000beef",
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


def procs():
    """MSYS process table rows (pid, ppid, pgid, winpid, command) from `ps -W -l`."""
    out = subprocess.run(["ps", "-W", "-l"], stdout=subprocess.PIPE).stdout
    rows = []
    for line in out.decode("utf-8", "replace").splitlines()[1:]:
        f = line.split()
        if f and not f[0].isdigit():
            f = f[1:]
        if len(f) >= 8 and f[0].isdigit():
            rows.append((f[0], f[1], f[2], f[3], " ".join(f[7:])))
    return rows


def broker(s, what):
    """ecce-broker-win start|stop for this session; the parsed broker file."""
    bash = os.path.join(s.env["ECCE_HOME"], "packaging", "windows", "ecce-broker-win")
    r = subprocess.run(["bash", bash, what], env=dict(s.env, ECCE_NO_MESSAGING=""),
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    say(r.stdout.decode("utf-8", "replace").strip())
    cfg = {}
    f = os.path.join(s.state, ".ECCE", "broker_%s_%s" % (s.env["HOST"], s.env["ECCE_SESSION_ID"]))
    if os.path.exists(f):
        for line in open(f):
            if "=" in line:
                k, v = line.strip().split("=", 1)
                cfg[k] = v
    return cfg


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
    cfg = {}
    sub = None
    if "NO_MESSAGING" not in os.environ.get("WINTEST", ""):
        cfg = broker(s, "start")
        if not s.check(cfg.get("port") and cfg.get("password"), "broker started, login in broker file"):
            return 1
        base = ["mosquitto_sub", "-h", cfg["host"], "-p", cfg["port"], "-t", "ecce/#", "-v"]
        s.sublog = os.path.join(st, "sub.log")
        sub = subprocess.Popen(base + ["-u", cfg["user"], "-P", cfg["password"]],
                               stdout=open(s.sublog, "w"), stderr=subprocess.STDOUT)
        anon = subprocess.run(base + ["-W", "5"], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
        say("anonymous subscriber: rc=%d %s" % (anon.returncode, anon.stdout.decode().strip()))
        s.check(anon.returncode != 0 and b"not authori" in anon.stdout.lower(),
                "subscription without the password is refused")
        # mosquitto_sub never exits on its own once subscribed: -d shows the SUBACK, the timeout ends it.
        try:
            bad = subprocess.run(base[:-3] + ["-t", "ecce/other/#", "-d", "-u", cfg["user"],
                                 "-P", cfg["password"]], stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, timeout=6).stdout
        except subprocess.TimeoutExpired as e:
            bad = e.stdout or b""
        bad = bad.decode("utf-8", "replace")
        say("another user's topic (ecce/other/#) with this login: %s" %
            " | ".join(l for l in bad.splitlines() if "SUBACK" in l or "Subscribed" in l))
        s.check("(128)" in bad or "Subscribed (mid: 1): 128" in bad,
                "subscription to another user's topic is denied by the ACL")
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
        grp = [p for p in procs() if p[2] == jobid]
        say("process group %s before cancel:" % jobid)
        for p in grp:
            say("  pid %s ppid %s pgid %s winpid %s %s" % p)
        s.check(len(grp) >= 2, "job's process group has its processes (%d)" % len(grp))
        rc, out = s.drv("kill", calc)
        say(out)
        state = s.wait(calc, 120)
        s.check(state == "killed", "state killed (%s)" % state)
        time.sleep(3)
        left = [p for p in procs() if p[2] == jobid or p[0] == jobid]
        say("process group %s after cancel: %d processes" % (jobid, len(left)))
        for p in left:
            say("  pid %s ppid %s pgid %s winpid %s %s" % p)
        s.check(not left, "no process of the job left")
        say("all sleep/stub processes now: %s" % [p for p in procs() if "sleep" in p[4] or "stub" in p[4]])
        for f in glob.glob(os.path.join(st, "jobs", "**", ".ecce.status"), recursive=True):
            code = open(f).read().strip()
            say("%s: %s" % (f, code))
            s.check(code == "302", ".ecce.status is 302")
    if sub:
        time.sleep(2)
        sub.terminate()
        sub.wait()
        seen = open(s.sublog).read()
        lines = [l for l in seen.splitlines() if l.startswith("ecce/")]
        say("broker messages seen by the subscriber: %d" % len(lines))
        for l in lines[:12]:
            say("  " + l[:150])
        s.check(any("state" in l for l in lines), "state messages arrived via the broker")
        broker(s, "stop")
    say("")
    say("FAILED: " + "; ".join(s.fail) if s.fail else "PASSED")
    return 1 if s.fail else 0


if __name__ == "__main__":
    sys.exit(main())
