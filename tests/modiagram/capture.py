#!/usr/bin/env python3
"""
Capture (or check) the MO-diagram golden dumps in tests/modiagram/expected/.

These are the byte-identity oracle for a later refactor that moves the MO
diagram engine (MoDiagramPanel::buildOnce() and friends) out of wx and into
a plain library -- see the ECCE_OPEN_PANEL/ECCE_MODIAGRAM_DUMP/
ECCE_EXIT_AFTER_DUMP hooks added to Builder.C and MoDiagramPanel.C for #171.
The refactor is trusted only as far as it reproduces these dumps verbatim
against the same fixtures.

Usage:
    ECCE_TEST_HOME=/path/to/an/install python3 capture.py [--update] [name ...]

With no names, every fixture below is run.  Without --update, a captured
dump is compared against tests/modiagram/expected/<name>.dump and any
difference is reported; with --update, that file is (over)written.

Reuses tests/apps' isolate/xdisplay/apps/fixture helpers exactly as
run_tests.py does -- this is one more caller of the same "bring up an
isolated gateway + data server, launch one app headlessly" machinery, not
a separate suite with its own service-startup code (see tests/apps/README).
"""
import argparse
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
TESTS_APPS = os.path.join(os.path.dirname(HERE), "apps")
sys.path.insert(0, TESTS_APPS)

import apps          # noqa: E402
import fixture        # noqa: E402
import isolate         # noqa: E402
import xdisplay          # noqa: E402

FIXTURES_DIR = os.path.join(HERE, "fixtures")
EXPECTED_DIR = os.path.join(HERE, "expected")

# name -> source directory.  calc-water-vib is tests/apps' own fixture
# (C2v water, VIB+MO+GEOMTRACE); the rest are tests/modiagram's own,
# trimmed to just what MoDiagramPanel::build() reads -- see
# tests/modiagram/README.
FIXTURES = {
    "calc-water-vib": os.path.join(os.path.dirname(HERE), "apps",
                                   "fixtures", "calc-water-vib"),
    "orca-co": os.path.join(FIXTURES_DIR, "orca-co"),
    "gaussian-water": os.path.join(FIXTURES_DIR, "gaussian-water"),
    "gaussian-co": os.path.join(FIXTURES_DIR, "gaussian-co"),
    "orca-crco6": os.path.join(FIXTURES_DIR, "orca-crco6"),
    "nwchem-water": os.path.join(FIXTURES_DIR, "nwchem-water"),
}


def captureOne(name, source, outfile, timeout):
    """Run builder headlessly against `source`, dump to `outfile`.

    Returns an error string, or None on success.  Brings its own isolated
    gateway/data server/X display up and down -- callers that capture
    several fixtures in one process still get a fresh service pair per
    fixture, which costs ~10s each but keeps one fixture's failure from
    wedging the rest.
    """
    #  Each call picks its own free ports -- isolate.apply() treats an
    #  already-set ECCE_DATASERVER_PORT/ECCE_BROKER_PORT as an explicit
    #  request and insists on reusing exactly that port, which races the
    #  previous call's own teardown when this runs several fixtures (or
    #  several repeats of one) in the same process.
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"

    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        after = apps.serviceState(display)
        if not all(after.values()):
            apps.stopServices(display)
            return "services did not come up: %s (%s)" % (after, log)

        #  RunMgmt/processmachine dies without ~/.ECCE/Queues -- normally
        #  created the first time a user does Tools > Register Machines.
        queuesPath = os.path.join(state, ".ECCE", "Queues")
        os.makedirs(os.path.dirname(queuesPath), exist_ok=True)
        if not os.path.exists(queuesPath):
            open(queuesPath, "w").close()

        fixture.ensureRealUserAccount()
        restorePrefs = fixture.settleUpgradeNotices()
        url, error = fixture.install(name, source_dir=source)
        if error:
            apps.stopServices(display)
            restorePrefs()
            return "fixture install failed: %s" % error

        if os.path.exists(outfile):
            os.remove(outfile)
        os.makedirs(os.path.dirname(outfile), exist_ok=True)

        env = display.env()
        env["ECCE_OPEN_PANEL"] = "MO Diagram"
        env["ECCE_MODIAGRAM_DUMP"] = outfile
        env["ECCE_EXIT_AFTER_DUMP"] = "1"

        #  -pipe avoids the "ECCE Authentication" modal, which otherwise
        #  blocks the event loop before the calc ever loads.
        authPath = os.path.join(state, "auth.pipe")
        fixture.authFile(authPath, port=int(settings["ECCE_DATASERVER_PORT"]))

        command = [os.path.join(apps.WRAPPERS, "ecce-builder"),
                  "-pipe", authPath, "-context", url]
        proc = subprocess.Popen(command, env=env, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,
                                start_new_session=True)
        deadline = time.time() + timeout
        while time.time() < deadline:
            if os.path.exists(outfile) and os.path.getsize(outfile) > 0:
                break
            if proc.poll() is not None:
                break
            time.sleep(0.5)

        try:
            proc.wait(timeout=15)
        except subprocess.TimeoutExpired:
            try:
                os.killpg(os.getpgid(proc.pid), 15)
                proc.wait(timeout=10)
            except Exception:
                try:
                    os.killpg(os.getpgid(proc.pid), 9)
                except Exception:
                    pass

        out = b""
        try:
            out = proc.stdout.read() or b""
        except Exception:
            pass

        apps.stopServices(display)
        restorePrefs()

        if not os.path.exists(outfile):
            tail = out.decode("utf-8", "replace")[-4000:]
            return "no dump written; builder's last output:\n%s" % tail
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("names", nargs="*", help="fixture names to run "
                        "(default: all)")
    parser.add_argument("--update", action="store_true",
                        help="write tests/modiagram/expected/<name>.dump "
                             "instead of comparing against it")
    parser.add_argument("--repeat", type=int, default=1,
                        help="capture each fixture N times and require "
                             "byte-identical dumps (determinism check)")
    parser.add_argument("--timeout", type=float, default=90)
    args = parser.parse_args()

    names = args.names or list(FIXTURES)
    unknown = [n for n in names if n not in FIXTURES]
    if unknown:
        print("unknown fixture(s): %s" % ", ".join(unknown))
        return 2

    failures = []
    for name in names:
        source = FIXTURES[name]
        if not os.path.isdir(source):
            failures.append("%s: no such fixture directory %s"
                            % (name, source))
            continue

        dumps = []
        ok = True
        for run in range(max(1, args.repeat)):
            scratch = os.path.join("/tmp", "modiagram-capture-%s-%d.dump"
                                   % (name, run))
            error = captureOne(name, source, scratch, args.timeout)
            if error:
                failures.append("%s: %s" % (name, error))
                ok = False
                break
            dumps.append(open(scratch, "rb").read())
            os.remove(scratch)
        if not ok:
            continue

        for i in range(1, len(dumps)):
            if dumps[i] != dumps[0]:
                failures.append("%s: dump differs between run 1 and run %d "
                                "-- not deterministic" % (name, i + 1))
                ok = False
                break
        if not ok:
            continue

        expected = os.path.join(EXPECTED_DIR, "%s.dump" % name)
        if args.update:
            os.makedirs(EXPECTED_DIR, exist_ok=True)
            open(expected, "wb").write(dumps[0])
            print("%s: wrote %s (%d bytes)"
                 % (name, expected, len(dumps[0])))
        elif not os.path.exists(expected):
            failures.append("%s: no golden at %s (run with --update first)"
                            % (name, expected))
        elif open(expected, "rb").read() != dumps[0]:
            failures.append("%s: dump differs from %s" % (name, expected))
        else:
            print("%s: matches golden (%d bytes)" % (name, len(dumps[0])))

    if failures:
        print("\nFAILURES:")
        for f in failures:
            print(" ", f)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
