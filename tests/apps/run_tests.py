#!/usr/bin/env python3
"""
ECCE GUI application smoke suite.

Starts every installed wx-linked ECCE app on a headless X server with
software OpenGL, waits for it to open a window, keeps watching it for a few
seconds, and shuts it down -- checking that it neither dies on construction
nor crashes shortly afterwards.

That narrow question is worth automating because it is the exact shape of the
wx3.2/GTK3 bugs this port keeps hitting: #78 segfaulted `builder` during
construction, and the layout-reentrancy class crashes *after* Show() has
returned.  Nothing else in the tree notices either.

    ./run_tests.py                 every installed GUI app
    ./run_tests.py --app builder
    ./run_tests.py --list
    ./run_tests.py -v              per-app detail, including window titles
    ./run_tests.py --keep-services leave the gateway/dataserver running

Needs the app to be INSTALLED (/opt/ecce + the /usr/bin/ecce-* wrappers), an
Xvfb, and x11-utils.  Unlike the other two suites this one is not
build-independent: it tests what was packaged, which is also what catches the
"correct in the repo, broken in the .deb" class of problem.
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                                                   # noqa: E402
import cases as CASEDEFS                                      # noqa: E402
import fixture                                                # noqa: E402
import xdisplay                                               # noqa: E402

CRASH_MARKERS = (
    "Segmentation fault", "segfault", "Aborted", "terminate called",
    "*** buffer overflow", "double free or corruption",
    "Assertion failed", "assert ",
)


class Results(object):
    def __init__(self):
        self.failures = []
        self.xfails = []
        self.xpasses = []
        self.notes = []
        self.checks = 0
        self.seenXfail = set()
        self.usedHelper = set()

    def fail(self, where, message):
        self.failures.append("%s: %s" % (where, message))


#  Titles an app puts up when it is telling you it did not start.
FAILED_TITLE = re.compile(r"\b(failure|failed|error|fatal|cannot|unable)\b",
                          re.IGNORECASE)


def checkApp(display, name, results, verbose=False):
    results.checks += 1
    isHelper = name in CASEDEFS.HELPERS
    timeout = CASEDEFS.TIMEOUTS.get(name, 40)

    result = apps.run(display, name, windowTimeout=timeout)

    if isHelper:
        results.usedHelper.add(name)
        if result.crashed:
            _record(results, name,
                    "helper exited on %s rather than cleanly:\n%s"
                    % (result.signalName, _tail(result.log)))
        elif verbose:
            results.notes.append("%-14s helper, exited status %s"
                                 % (name, result.returncode))
        return

    if result.crashed:
        _record(results, name,
                "CRASHED (%s)%s\n%s"
                % (result.signalName or "status %s" % result.returncode,
                   " -- " + result.note if result.note else "",
                   _tail(result.log)))
        return

    if not result.sawWindow:
        _record(results, name,
                "opened no window within %ds and then exited with status %s."
                "\n      An app that cannot show a window is one a user "
                "cannot use.\n%s"
                % (timeout, result.returncode, _tail(result.log)))
        return

    #  A window is not proof the app started.  ECCE reports a dead
    #  service by putting up a dialog -- "ECCE Server Failure" -- and
    #  that counted as success here, which is how ONE stopped dataserver
    #  presented as eleven unrelated app failures with the actual cause
    #  sitting in the passing column.
    #  Matched on word boundaries: a substring test would be one
    #  unlucky app title away from a false failure.
    bad = [t for _, t in result.windows if FAILED_TITLE.search(t)]
    if bad:
        _record(results, name,
                "opened a window, but it announces a failure: %s.\n"
                "      A window is not proof an app started; this is the "
                "app telling you it did not.\n%s"
                % (", ".join('"%s"' % t for t in bad[:3]), _tail(result.log)))
        return

    markers = [m for m in CRASH_MARKERS if m.lower() in result.log.lower()]
    if markers:
        _record(results, name,
                "started, but its output contains %s:\n%s"
                % (", ".join(repr(m) for m in markers), _tail(result.log)))
        return

    if verbose:
        titles = ", ".join('"%s"' % t for _, t in result.windows[:3])
        results.notes.append("%-14s window in %4ss: %s"
                             % (name, result.secondsToWindow, titles))


def _record(results, name, message):
    if name in CASEDEFS.XFAIL:
        results.seenXfail.add(name)
        results.xfails.append((name, message))
    else:
        results.fail(name, message)


def _tail(log, lines=12):
    text = [l for l in (log or "").splitlines() if l.strip()]
    return "\n".join("      | " + l for l in text[-lines:]) or "      | (no output)"


def accessLog():
    home = os.environ.get("ECCE_REALUSERHOME", os.path.expanduser("~"))
    return os.path.join(home, ".ECCE", "dataserver", "logs", "access_log")


def checkCalculation(display, results, verbose=False):
    """Open a real calculation, not an empty app.

    Starting an app empty proves it constructs.  It cannot reach #78 --
    builder segfaulting on any job with vibrational data, because
    wxGrid::CreateGrid() fired a selection event during construction into a
    handler with unmet dependencies.  That path needs data.

    "It stayed alive" is not enough on its own here: if the context were
    silently ignored, the app would also stay alive and the check would pass
    while testing nothing.  So this also requires evidence in the data
    server's access log that the calculation was actually read.
    """
    results.checks += 1
    if not fixture.available():
        results.notes.append("no calculation fixture checked in; skipped")
        return

    restorePrefs = fixture.settleUpgradeNotices()
    try:
        _openCalculation(display, results, verbose)
    finally:
        restorePrefs()


def _openCalculation(display, results, verbose):
    url, error = fixture.install()
    if error:
        results.fail("calculation", "could not install the fixture: %s" % error)
        return

    log = accessLog()
    before = os.path.getsize(log) if os.path.exists(log) else 0

    result = apps.run(display, "builder", args=("-context", url),
                      windowTimeout=CASEDEFS.TIMEOUTS.get("builder", 40),
                      settle=15)

    if result.crashed:
        results.fail("calculation",
                     "builder CRASHED (%s) opening %s%s\n%s"
                     % (result.signalName or result.returncode, url,
                        " -- " + result.note if result.note else "",
                        _tail(result.log)))
        return
    if not result.sawWindow:
        results.fail("calculation", "builder opened no window for %s" % url)
        return

    served = []
    if os.path.exists(log):
        with open(log, errors="replace") as handle:
            handle.seek(before)
            for line in handle:
                if "calc-water-vib" in line and '" 2' in line:
                    served.append(line.split('"')[1].split()[1])
    if not served:
        results.fail(
            "calculation",
            "builder started but the data server served it nothing from %s.\n"
            "      The -context argument was ignored, so this check would "
            "pass without ever loading a calculation." % url)
        return

    wanted = "Parameters/chemsys.mvm"
    if not any(wanted in path for path in served):
        results.fail(
            "calculation",
            "builder read %d resources from the calculation but never %s, so "
            "the structure was not loaded:\n      %s"
            % (len(served), wanted, ", ".join(sorted(set(served))[:8])))
        return

    if verbose:
        results.notes.append(
            "%-14s opened the fixture calculation, read %d resources "
            "(%s...)" % ("calculation", len(served),
                         ", ".join(sorted(set(served))[:3])))


def checkStale(results, tested):
    for name in CASEDEFS.XFAIL:
        if name in tested and name not in results.seenXfail:
            results.xpasses.append(
                "%s no longer reproduces -- remove it from XFAIL" % name)
    for name in CASEDEFS.HELPERS:
        if name in tested and name not in results.usedHelper:
            results.xpasses.append(
                "%s is listed in HELPERS but was not run as one" % name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", action="append", help="only this app")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("--display", type=int, default=None,
                        help="use this X display number instead of picking a "
                             "free one")
    parser.add_argument("--keep-services", action="store_true",
                        help="leave the gateway and data server running "
                             "afterwards even if this suite started them")
    args = parser.parse_args()

    binaries = apps.guiBinaries()
    if args.list:
        for name in binaries:
            kind = "helper" if name in CASEDEFS.HELPERS else "app"
            print("%-16s %s" % (name, kind))
        return 0
    if not binaries:
        print("SKIP: no installed ECCE GUI apps found under %s "
              "(this suite tests an INSTALLED build, not the build tree)"
              % apps.INSTALL)
        return 0

    selected = [n for n in binaries if not args.app or n in args.app]
    if not selected:
        print("no such app; --list shows what is installed", file=sys.stderr)
        return 2

    try:
        display = xdisplay.Display(number=args.display).__enter__()
    except xdisplay.DisplayUnavailable as exc:
        print("SKIP: %s" % exc)
        return 0

    before = apps.serviceState()
    results = Results()
    try:
        gl = display.hasGL()
        print("display: %s (GL: %s)"
              % (display.name,
                 "yes" if gl else
                 ("unknown -- glxinfo not installed (mesa-utils)"
                  if gl is None else
                  "NO -- the viewer apps will not render")))
        serviceLog = []
        apps.startServices(display, serviceLog)
        for line in serviceLog:
            print("  %s" % line)

        #  Check the services actually came up before sweeping every app
        #  against them.  Without this, a dataserver that failed to start
        #  is reported as ten apps that "opened no window within 40s" --
        #  a list that reads like ten bugs and names none of them.
        after = apps.serviceState()
        down = sorted(k for k, up in after.items() if not up)
        if down:
            results.fail("services",
                         "%s did not start, so nothing below can work.\n"
                         "      Every app that needs the server will be "
                         "reported as opening no window. That is this one "
                         "fault, not a dozen separate ones.\n      %s"
                         % (" and ".join(down), " | ".join(serviceLog)))
        #  The app's name goes out BEFORE it runs, so a wedged suite
        #  names its culprit.  The partial-line form is nicer to read
        #  ("name ... done" on one line) but GitHub Actions only shows
        #  COMPLETE lines, so a hang loses it entirely and the last
        #  SUCCESSFUL app looks like the last thing attempted -- which
        #  sent this CI hang down the wrong path twice.  ECCE_APPS_TRACE
        #  switches to whole lines for that reason.
        trace = bool(os.environ.get("ECCE_APPS_TRACE"))
        for name in selected:
            if trace:
                print("  %-16s starting" % name, flush=True)
            else:
                print("  %-16s" % name, end="", flush=True)
            checkApp(display, name, results, verbose=args.verbose)
            print("done", flush=True)
        if not args.app:
            checkCalculation(display, results, verbose=args.verbose)
            checkStale(results, set(selected))
    finally:
        display.__exit__(None, None, None)
        if not args.keep_services:
            # Leave the machine as we found it: these are somebody's per-user
            # services and this suite is not entitled to leave them running.
            if not all(before.get(k) for k in ("gateway", "dataserver")):
                apps.stopServices()

    return report(results, args.verbose)


def report(results, verbose):
    if verbose and results.notes:
        print("\n--- detail " + "-" * 58)
        for note in results.notes:
            print("  " + note)
    if results.xfails:
        print("\n--- KNOWN BUGS (reproduced) " + "-" * 41)
        for name, detail in results.xfails:
            print("  * %s: %s" % (name, detail))
            print("\n".join("        " + l.strip()
                            for l in CASEDEFS.XFAIL[name].strip().splitlines()))
    if results.xpasses:
        print("\n--- STALE ENTRIES " + "-" * 51)
        for item in results.xpasses:
            print("  * " + item)
    if results.failures:
        print("\n--- FAILURES " + "-" * 56)
        for item in results.failures:
            print("  * " + item + "\n")

    ok = not results.failures and not results.xpasses
    print("\n%s  %d apps checked, %d failures, %d known bugs"
          % ("PASS" if ok else "FAIL", results.checks, len(results.failures),
             len(results.xfails)))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
