#!/usr/bin/env python3
"""Organizer > Run Mgmt > Reconnect Selected (#601): several jobs whose
monitoring stopped are reconnected in one go.

Two MOPAC jobs (a stand-in that waits for a `release` file) run on localhost
under a local Shell machine.  Their job stores are stopped with SIGTERM, as at
logout, which parks both as "waiting for login"; the jobs then finish.  A
third calculation was never launched.  The Organizer hooks are used without
their dialogs:

  reconnectselectedlist URL...        what the dialog lists, with the reasons
  reconnectselectedshot PNG URL...    the dialog, saved
  reconnectselected URL...            the batch

Both parked jobs must end completed with their properties; the calculation
that was never launched must be listed and skipped with its reason.

    tests/launch/reconnect_selected_test.py [--build DIR] [--keep] [--shot PNG]

Exit status 77 (CTest SKIP) without Xvfb, mopac or the built binaries.
"""

import argparse
import glob
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

DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def procs(needle):
    found = []
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        try:
            with open("/proc/%s/cmdline" % entry, "rb") as handle:
                cmd = handle.read().replace(b"\0", b" ").decode("utf-8", "replace")
        except OSError:
            continue
        if all(n in cmd for n in needle):
            found.append(int(entry))
    return found


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--shot", default="")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    import shutil as sh
    if not sh.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    harness.prerequisites(build, ("mopac",))
    if not os.access(os.path.join(build, "organizer"), os.X_OK):
        harness.skip("organizer is not built in %s" % build)
    import xdisplay

    s = harness.Session(build, "reconnsel", {}, local=True, keep=args.keep)
    wrapper = os.path.join(s.state, "held-mopac")
    with open(wrapper, "w") as h:
        h.write("#!/bin/sh\n"
                "n=0\n"
                "while [ ! -e release ] && [ $n -lt 600 ]; do sleep 1; n=$((n+1)); done\n"
                "exec %s \"$@\"\n" % shutil.which("mopac"))
    os.chmod(wrapper, 0o755)
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
    s.registerMachine("localhost", "Shell", {"MOPAC": wrapper})
    jobs = os.path.join(s.state, "jobs")
    os.makedirs(jobs, exist_ok=True)
    shutil.rmtree(os.path.join(s.state, ".ECCE", "waiting"), ignore_errors=True)

    #  No "0" in the names: see waiting_test.py.
    stamp = int(str(int(time.time()) % 100000).replace("0", "9"))
    display = org = None
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        names = ["recon-a-%d" % stamp, "recon-b-%d" % stamp]
        urls, rundirs = [], []
        for name in names:
            url, out = s.create(name, "mopac_es", DECK, "mopac.mop", jobs)
            if not check(url is not None, "%s created" % name):
                say(out)
                return 1
            rc, out = s.launch(url)
            if not check(rc == 0, "%s launched" % name):
                say(out[-600:])
                return 1
            ran = [l.split(":", 1)[1].strip() for l in out.splitlines()
                   if l.startswith("run directory:")]
            urls.append(url)
            rundirs.append(ran[-1] if ran else "")
        check(all(rundirs), "run directories: %s" % rundirs)
        never, out = s.create("recon-never-%d" % stamp, "mopac_es", DECK,
                              "mopac.mop", jobs)
        check(never is not None, "the calculation that is never launched exists")

        #  Wait until each monitor runs, then stop the stores as logout does.
        for name in names:
            seen = False
            for _ in range(120):
                logs = glob.glob(os.path.join(s.state, "tmp", "*", "jobs",
                                              name + "__*", "eccejob*.log*"))
                if any("job monitor running on" in open(l, errors="replace").read()
                       for l in logs):
                    seen = True
                    break
                time.sleep(1)
            check(seen, "%s is monitored" % name)
        for name, url in zip(names, urls):
            for pid in procs(["eccejobstore", name]) + procs(["eccejobmaster", url]):
                try:
                    os.kill(pid, signal.SIGTERM)
                except OSError:
                    pass
        for _ in range(60):
            if not any(procs(["eccejobstore", n]) for n in names):
                break
            time.sleep(0.5)
        for u in urls:
            st = s.waitState(u, seconds=60, want=("waiting",))
            check(st == "waiting", "parked as waiting (%s)" % st)
        for r in rundirs:
            open(os.path.join(r, "release"), "w").close()
        for r in rundirs:
            ok = False
            for _ in range(120):
                if os.path.exists(os.path.join(r, ".ecce.status")):
                    ok = True
                    break
                time.sleep(1)
            check(ok, "the job in %s finished while away" % os.path.basename(r))

        display = xdisplay.Display().__enter__()
        genv = s.env({"DISPLAY": display.name,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        #  The Organizer's own session-start catch-up would reconnect both
        #  before the hooks run; mark this session as already caught up.
        import sessionkey
        sid = genv.get("ECCE_SESSION_ID", "")
        check(sid != "", "the session id is known (%s)" % sid)
        with open(os.path.join(s.state, ".ECCE", "waiting", "session"), "w") as h:
            h.write(sessionkey.key(sid, genv) + "\n")
        cmds = os.path.join(s.state, "organizer-commands")
        open(cmds, "w").close()
        olog = os.path.join(s.state, "organizer.log")
        org = subprocess.Popen(
            [os.path.join(wrappers, "ecce-organizer")],
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

        every = urls + [never]
        ans = send("reconnectselectedlist %s" % " ".join(every))
        say("  list: %s" % ans)
        for n in names:
            check(("%s|localhost|Waiting for Login|ok" % n) in ans,
                  "%s is listed as reconnectable" % n)
        check("recon-never-%d|localhost|Ready|not launched" % stamp in ans,
              "the unlaunched calculation is listed with its reason")
        check(ans.index("recon-never") > ans.index(names[-1]),
              "calculations that cannot be reconnected come last")

        shot = args.shot or os.path.join(s.state, "reconnect.png")
        ans = send("reconnectselectedshot %s %s" % (shot, " ".join(every)))
        check(ans == "not reconnected" and os.path.getsize(shot) > 1000,
              "dialog saved (%s)" % ans)
        check(all(s.state_of(u) == "waiting" for u in urls),
              "the dialog alone reconnects nothing")

        ans = send("reconnectselected %s" % " ".join(every))
        say("  answer: %s" % ans)
        check(ans.startswith("ok "), "batch answered ok")
        for n in names:
            check(("%s: reconnected" % n) in ans, "%s reported reconnected" % n)
        check("recon-never-%d: skipped: not launched" % stamp in ans,
              "the unlaunched calculation is skipped")
        for u in urls:
            st = s.waitState(u, seconds=180)
            check(st == "completed", "completed after the reconnect (%s)" % st)
            check("TE" in s.props(u), "TE stored")
        check(s.state_of(never) == "ready", "the skipped calculation is unchanged")
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
