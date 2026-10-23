#!/usr/bin/env python3
"""A calculation's start and completion times are the job's, to the second (#463).

eccejobstore wrote the start date on every "running" it got, and a
restarted monitor reports running again, so a job whose monitor was
restarted near the end showed the same start and completion time.  Runs a
held MOPAC job on localhost/Shell (as tests/launch/waiting_test.py does):

  a. a normal job: the start is before the completion by at least the time
     the job was held
  b. the monitor is killed while the job runs and is started again: the
     start date is still the one stored before the kill

Both cases check that the stored dates and the Organizer's display of them
carry seconds.

    tests/launch/jobtimes_test.py [--build build] [--keep] [--only a,b]

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import calendar
import os
import re
import shutil
import signal
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import harness  # noqa: E402
from harness import say  # noqa: E402
from waiting_test import Case, cwdPids  # noqa: E402

HOLD = 5


def dates(c):
    rc, out = c.drive("dates", c.url)
    got = {}
    for line in out.splitlines():
        key, sep, value = line.partition(": ")
        if sep and key in ("start", "completion", "shown start",
                           "shown completion"):
            got[key] = value.strip()
    return got


def seconds(stored):
    """RFC 1123 GMT, as TDateTime::toString() writes it."""
    return calendar.timegm(time.strptime(stored, "%a, %d %b %Y %H:%M:%S GMT"))


def checkDates(c, held, startBefore=None):
    d = dates(c)
    c.say("dates: %r" % d)
    for key in ("start", "completion"):
        c.check(re.search(r" \d\d:\d\d:\d\d GMT$", d.get(key, "")),
                "%s date stored with seconds" % key)
        c.check(re.search(r" \d\d:\d\d:\d\d$", d.get("shown " + key, "")),
                "%s date shown with seconds" % key)
    try:
        start, end = seconds(d["start"]), seconds(d["completion"])
    except (KeyError, ValueError):
        c.check(False, "dates readable")
        return
    c.check(end - start >= held,
            "completion is at least %d s after the start (%d s)"
            % (held, end - start))
    if startBefore is not None:
        c.check(d["start"] == startBefore,
                "the start date survived the monitor restart (%s, was %s)"
                % (d["start"], startBefore))


def waitStart(c, seconds=60):
    for _ in range(seconds):
        start = dates(c).get("start", "")
        if start:
            return start
        time.sleep(1)
    return ""


def normalJob(c):
    if not c.start():
        return
    c.check(c.monitoring(1), "the monitor is watching")
    time.sleep(HOLD)
    c.release()
    c.completed()
    checkDates(c, HOLD)


def monitorRestart(c):
    env = {"ECCE_JOB_MAXQUICKTIME": "1"}
    if not c.start(env):
        return
    c.check(c.monitoring(1), "the monitor is watching")
    start = waitStart(c)
    c.check(start, "a start date is stored while the job runs")
    time.sleep(HOLD)
    pids = cwdPids(c.rundir, "eccejobmonitor")
    for pid in pids:
        os.kill(pid, signal.SIGKILL)
    c.check(pids, "the monitor was killed")
    c.check(c.monitoring(2), "a second monitor run is watching")
    #  The second run reports running before the job is released.
    time.sleep(3)
    c.release()
    c.completed()
    checkDates(c, HOLD, start)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=os.path.join(REPO, "build"))
    parser.add_argument("--keep", action="store_true")
    parser.add_argument("--only", default="a,b")
    args = parser.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ("mopac",))

    s = harness.Session(build, "jobtimes", {}, keep=args.keep)
    wrapper = os.path.join(s.state, "held-mopac")
    with open(wrapper, "w") as h:
        #  The output exists, so the monitor reports running, until the
        #  test drops a release file.
        h.write("#!/bin/sh\n"
                "%s \"$@\"\nrc=$?\n"
                "n=0\n"
                "while [ ! -e release ] && [ $n -lt 600 ]; do sleep 1; n=$((n+1)); done\n"
                "exit $rc\n" % shutil.which("mopac"))
    os.chmod(wrapper, 0o755)
    with open(os.path.join(s.state, ".ECCE", "CONFIG.localhost"), "w") as h:
        h.write("MOPAC: %s\n" % wrapper)
    os.makedirs(os.path.join(s.state, "jobs"), exist_ok=True)

    stamp = int(str(int(time.time()) % 100000).replace("0", "9"))
    cases = {
        "a": ("a. a normal job", normalJob),
        "b": ("b. the monitor is restarted while the job runs", monitorRestart),
    }
    try:
        if not s.services(True):
            s.check(False, "services started")
        else:
            for key in args.only.split(","):
                title, fn = cases[key]
                say("--- " + title)
                c = Case(s, key, s.state, stamp)
                c.name = "times-%s-%d" % (key, stamp)
                fn(c)
    finally:
        left = s.stop()
        if left:
            s.failures.append("processes left running: %r" % left)

    say("")
    if s.failures:
        say("FAILED (%d): %s" % (len(s.failures), "; ".join(s.failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
