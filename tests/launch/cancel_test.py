#!/usr/bin/env python3
"""A job that vanishes is "killed" only when the user asked for the kill.

eccejobmonitor reports 302 when a job is gone and left neither output nor a
status file.  eccejobstore used to store that as killed, so every unexplained
loss looked like a cancel.  It now does so only if RunMgmt::terminate recorded
a request, and says system_failure otherwise.  Runs on localhost/Shell with a
MOPAC stand-in that sleeps first:

  a. terminate a running job                         -> killed
  b. kill the job's processes, nobody asked          -> system_failure
     (the calculation carried a stale request, which Launch must clear)
  c. rerun after (a), then lose the job as in (b)    -> system_failure,
     so the request of the earlier run is not inherited

    tests/launch/cancel_test.py [--build build] [--keep] [--local]

--local keeps the data in a folder (ECCE_LOCAL_DATA, #216) with no data
server.

Exit status 77 (CTest SKIP) when a prerequisite is missing.
"""

import argparse
import os
import signal
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import harness  # noqa: E402
from harness import say  # noqa: E402

DECK = os.path.join(REPO, "tests", "e2e", "fixtures", "mopac", "mos", "mopac.mop")


def flag(s, url):
    rc, out = s.driver("killflag", url)
    return out.strip().splitlines()[-1] if out.strip() else ""


def monitoring(s, url):
    """Wait until eccejobmonitor reports the job alive.

    The state stays "submitted" until the code writes output, which the
    stand-in delays, so the monitor's own progress line is the signal.
    """
    import glob
    name = url.rstrip("/").rsplit("/", 1)[1]
    pattern = os.path.join(s.state, "tmp", "*", "jobs", name + "__*",
                           "eccejobstore.log")
    for _ in range(120):
        for log in glob.glob(pattern):
            try:
                with open(log, errors="replace") as handle:
                    if "job monitor running on" in handle.read():
                        return True
            except OSError:
                pass
        time.sleep(1)
    return False


def launched(s, name, rundir, label):
    url, out = s.create(name, "mopac_es", DECK, "mopac.mop", rundir)
    if not s.check(url is not None, "%s: calculation created" % label):
        say(out)
        return None
    return url


def start(s, url, label):
    rc, out = s.launch(url)
    if not s.check(rc == 0, "%s: Launch ran to the end" % label):
        say(out[-600:])
        return None
    rc, out = s.driver("jobid", url)
    jobid = out.strip().splitlines()[-1] if out.strip() else ""
    s.check(jobid.isdigit(), "%s: job id %s" % (label, jobid))
    return int(jobid) if jobid.isdigit() else None


def explained(s, url, label):
    """The run's recorded reason says the job was lost, not cancelled."""
    text = ""
    for _ in range(20):
        rc, out = s.driver("reason", url)
        text = out
        if "not cancelled by the user" in text:
            break
        time.sleep(1)
    s.check("not cancelled by the user" in text,
            "%s: the recorded reason says it was not cancelled" % label)


def lose(s, url, jobid, label):
    """Kill the job's whole process group, so no status file is written."""
    s.check(monitoring(s, url), "%s: the monitor is watching the job" % label)
    time.sleep(2)
    try:
        #  The job is not always its own group leader (built-in ssh starts
        #  each command in a new group), so look the group up, as the
        #  Shell cancel command does.
        os.killpg(os.getpgid(jobid), signal.SIGKILL)
    except OSError as err:
        s.check(False, "%s: could not kill process group %d (%s)" % (label, jobid, err))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--build", default=os.path.join(REPO, "build"))
    parser.add_argument("--keep", action="store_true")
    parser.add_argument("--local", action="store_true",
                        help="#216: data in a local folder, no data server")
    args = parser.parse_args()
    build = os.path.abspath(args.build)
    harness.prerequisites(build, ("mopac",))

    s = harness.Session(build, "cancel-local" if args.local else "cancel", {},
                        (8697, 8689), keep=args.keep, local=args.local)
    wrapper = os.path.join(s.state, "slow-mopac")
    with open(wrapper, "w") as h:
        h.write("#!/bin/sh\nsleep 300\nexec /usr/bin/mopac \"$@\"\n")
    os.chmod(wrapper, 0o755)
    with open(os.path.join(s.state, ".ECCE", "CONFIG.localhost"), "w") as h:
        h.write("MOPAC: %s\n" % wrapper)
    rundir = os.path.join(s.state, "jobs")
    os.makedirs(rundir, exist_ok=True)
    stamp = int(time.time()) % 100000

    try:
        if not s.services(True):
            s.check(False, "services started")
        else:
            say("--- a. the user terminates a running job")
            url = launched(s, "cancel-a-%d" % stamp, rundir, "a")
            jobid = start(s, url, "a") if url else None
            if jobid:
                s.check(monitoring(s, url), "a: the monitor is watching the job")
                time.sleep(2)
                rc, out = s.driver("kill", url)
                msg = out.strip().splitlines()[-1] if out.strip() else ""
                #  The Shell cancel command's second kill fails once the first worked.
                s.check(rc == 0 and ("has been issued" in msg or "process not found" in msg),
                        "a: terminate: %s" % msg)
                s.check(flag(s, url) == "set", "a: the request is recorded")
                state = s.waitState(url, 240)
                s.check(state == "killed", "a: state is killed (last: %s)" % state)

                say("--- c. rerun after the kill, then lose the job")
                rc, out = s.driver("rerun", url)
                s.check(rc == 0, "c: reset for rerun")
                s.check(flag(s, url) == "clear", "c: the reset cleared the request")
                jobid = start(s, url, "c")
                if jobid:
                    lose(s, url, jobid, "c")
                    state = s.waitState(url, 240)
                    s.check(state == "system_failure",
                            "c: state is system_failure, not killed (last: %s)" % state)
                    explained(s, url, "c")

            say("--- b. the job is lost, nobody asked, a stale request is on the calculation")
            url = launched(s, "cancel-b-%d" % stamp, rundir, "b")
            if url:
                s.driver("killflag", url, "set")
                s.check(flag(s, url) == "set", "b: stale request planted")
                jobid = start(s, url, "b")
                s.check(flag(s, url) == "clear", "b: Launch cleared the stale request")
                if jobid:
                    lose(s, url, jobid, "b")
                    state = s.waitState(url, 240)
                    s.check(state == "system_failure",
                            "b: state is system_failure, not killed (last: %s)" % state)
                    explained(s, url, "b")
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
