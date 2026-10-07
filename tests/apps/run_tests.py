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
import atexit
import os
import re
import shutil
import signal
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps                                                   # noqa: E402
import cases as CASEDEFS                                      # noqa: E402
import geomtrace_stress                                       # noqa: E402
import pbc_edit_test                                          # noqa: E402
import pbc_ux_test                                            # noqa: E402
import fixture                                                # noqa: E402
import isolate                                                # noqa: E402
import xdisplay                                               # noqa: E402

#  How long the whole run may take before it is abandoned.  A suite that
#  can hang has no upper bound on what one bad app costs: this one held a
#  CI job for two days, and the only thing that finally stopped it was
#  GitHub's own six-hour ceiling -- by which point the run had told nobody
#  anything (#127).  Every individual step is bounded now, but "every step
#  I thought of is bounded" is not the same claim as "the run terminates",
#  and only the second one is worth relying on.
#
#  A healthy full run is about six minutes.
DEFAULT_BUDGET = int(os.environ.get("ECCE_APPS_BUDGET", "1200"))


class BudgetExpired(Exception):
    pass


class Teardown(object):
    """Stop this run's services exactly once, however the run ends.

    Reused from three places: the normal `finally:` block, and the
    SIGTERM/SIGINT/SIGHUP handlers and atexit hook installed in main() --
    those exist because a killed run (a ctest timeout, ^C, a CI job
    cancellation) used to skip `finally:` entirely and leave the gateway,
    broker and data server running for days (see isolate.killLeftovers's
    docstring for what that costs the NEXT run). `display`/`before` are
    filled in as they become known, so a signal that arrives before the
    display even exists still runs a (no-op) teardown rather than crashing
    inside the handler.
    """

    def __init__(self):
        self.display = None
        self.before = {}
        self.keepServices = False
        self.state = None
        self.done = False

    def run(self):
        if self.done:
            return
        self.done = True
        signal.alarm(0)                       # cancel the run budget alarm
        if self.display is None:
            return
        try:
            if (not self.keepServices
                    and not all(self.before.get(k)
                               for k in ("gateway", "dataserver"))):
                apps.stopServices(self.display)
        except Exception:
            pass                              # teardown must never itself
        try:                                   # be what crashes the run
            if os.environ.get("ECCE_APPS_TRACE"):
                print("Xvfb stderr: %s" % (self.display.serverLog(300)
                                           or "empty"), flush=True)
            self.display.__exit__(None, None, None)
        except Exception:
            pass
        #  Last, the same state-directory sweep a run does on startup.
        #  The services scripts stop only the services: the app under test
        #  when a signal lands (seen: organizer) is a child of this process
        #  and outlived both the display and the run.  Isolated runs only --
        #  state stays None under --use-real-state, so this can never reach
        #  the developer's own ~/.ECCE session.
        if self.state is not None and not self.keepServices:
            try:
                isolate.killLeftovers(self.state)
            except Exception:
                pass

    def onSignal(self, signum, frame):
        self.run()
        sys.exit(128 + signum)


def startBudget(seconds):
    """Abandon the run if it outlives its budget, wherever it is stuck.

    SIGALRM rather than a polling check between apps, deliberately: a
    between-apps check only fires if the suite gets back between apps,
    which is precisely what a hang does not do.  The alarm raises into
    whatever is running, main()'s `finally` still stops the services, and
    a second, shorter alarm is armed on the way out so that a cleanup
    which hangs in its turn cannot inherit the hang.
    """
    def expired(signum, frame):
        signal.signal(signal.SIGALRM, _lastResort)
        signal.alarm(180)
        raise BudgetExpired(
            "the run exceeded its %ds budget and was abandoned.  Something "
            "is hung: the app named last in the log above is where to look."
            % seconds)

    def _lastResort(signum, frame):
        sys.stdout.flush()
        print("\nFAIL  the run did not even shut down within its budget; "
              "aborting hard.", flush=True)
        os._exit(2)

    signal.signal(signal.SIGALRM, expired)
    signal.alarm(seconds)


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

    if result.exitedAfterWindow:
        _record(results, name,
                "did not stay up: it %s.\n"
                "      Opening a window and then quitting is not starting."
                "\n%s" % (result.note, _tail(result.log)))
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

    #  Without credentials builder stops at the modal "ECCE Authentication"
    #  dialog and never reads the calculation.  A private state directory
    #  has no saved login, so the run supplies one, as a user would type it.
    #  Never under --use-real-state: that account's password is not ours.
    args = ("-context", url)
    if (os.path.realpath(fixture.stateHome())
            != os.path.realpath(os.path.expanduser("~"))):
        authPath = fixture.authFile(
            os.path.join(fixture.stateHome(), ".ECCE", "auth.pipe"),
            user=fixture.realUser())
        args = ("-pipe", authPath) + args
    result = apps.run(display, "builder", args=args,
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
        titles = ", ".join('"%s"' % t for _, t in result.windows[:3])
        results.fail(
            "calculation",
            "builder started but the data server served it nothing from %s.\n"
            "      Its windows: %s.  An \"ECCE Authentication\" window means "
            "the -pipe credentials were refused." % (url, titles or "none"))
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


#  Structure files the Builder opens from the command line, as `ecce-builder
#  file.pdb` and the macOS start test do.  Such a structure is not a
#  calculation: its getProperty() throws, and code that assumed a calculation
#  took the Builder down with it.  No .xyz: it opens a modal units prompt
#  first, which nothing here can answer.
STRUCTURE_FILES = ("glycine.pdb", "benzene.car", "benzene.xyz")

#  An uncaught exception is reported this way before the abort.
THROW_MARKERS = ("Throw Log", "Unhandled standard exception",
                 "Unhandled unknown exception")

#  Every panel layout: each builds its own panes for the structure.
PANEL_MODES = ("classic", "stacked", "accordion", "detail")


def checkStructureFiles(display, results, verbose=False):
    """Open structure files, require their atoms on screen and no throw.

    Staying up is not enough: with the argument ignored the Builder would
    stay up too.  The ECCE_VIEWER_SCENE hook waits for the structure's
    atoms, writes a snapshot and closes the Builder; no snapshot, or a
    FAILED file, means the structure never loaded.
    """
    data = os.path.join(os.path.dirname(HERE), "fragreaders", "data")
    base = tempfile.mkdtemp(prefix="structure-files-",
                            dir=fixture.stateHome())
    try:
        _openStructureFiles(display, results, verbose, data, base)
    finally:
        shutil.rmtree(base, ignore_errors=True)


def _openStructureFiles(display, results, verbose, data, base):
    for fileName in STRUCTURE_FILES:
        path = os.path.join(data, fileName)
        modes = PANEL_MODES if fileName.endswith(".pdb") else ("detail",)
        for mode in modes:
            results.checks += 1
            what = "builder %s (%s layout)" % (fileName, mode)
            out = os.path.join(base, "%s-%s" % (fileName, mode))
            os.makedirs(out)
            script = os.path.join(out, "scene")
            with open(script, "w") as handle:
                handle.write("viewall\nsnap loaded\n")
            result = apps.run(display, "builder", args=(path,),
                              windowTimeout=CASEDEFS.TIMEOUTS.get("builder",
                                                                  40),
                              settle=60,
                              env={"ECCE_PANEL_MODE": mode,
                                   "ECCE_VIEWER_SCENE": script,
                                   "ECCE_VIEWER_SCENE_OUT": out,
                                   #  answers the XYZ units prompt
                                   "ECCE_TEST_XYZ_UNITS": "angstrom"})
            markers = [m for m in CRASH_MARKERS + THROW_MARKERS
                       if m.lower() in result.log.lower()]
            failed = os.path.join(out, "FAILED")
            if result.crashed:
                results.fail(what, "CRASHED (%s)\n%s"
                             % (result.signalName or result.returncode,
                                _tail(result.log)))
            elif not result.sawWindow:
                results.fail(what, "opened no window\n%s"
                             % _tail(result.log))
            elif markers:
                results.fail(what, "its output contains %s:\n%s"
                             % (", ".join(repr(m) for m in markers),
                                _tail(result.log)))
            elif os.path.exists(failed):
                with open(failed) as handle:
                    results.fail(what, "the structure did not load: %s"
                                 % handle.read().strip())
            elif not os.path.exists(os.path.join(out, "loaded.ppm")):
                results.fail(what, "no snapshot of the structure; %s\n%s"
                             % (result.note or "it was still up after 60s",
                                _tail(result.log)))
            elif verbose:
                results.notes.append("%-14s %s loaded in %ss"
                                     % ("structure", what,
                                        result.secondsToWindow))


def checkStale(results, tested):
    for name in CASEDEFS.XFAIL:
        if name in tested and name not in results.seenXfail:
            results.xpasses.append(
                "%s no longer reproduces -- remove it from XFAIL" % name)
    for name in CASEDEFS.HELPERS:
        if name in tested and name not in results.usedHelper:
            results.xpasses.append(
                "%s is listed in HELPERS but was not run as one" % name)


# ctest's SKIP_RETURN_CODE: a skip must not read as a pass.
SKIP = 77


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
    parser.add_argument("--use-real-state", action="store_true",
                        help="run against $HOME/.ECCE and the real ports "
                             "instead of a private instance -- this shares "
                             "the data server, the broker and the "
                             "preferences with any ECCE session you have "
                             "running, so do not")
    parser.add_argument("--any-version", action="store_true",
                        help="test the installed ECCE even when its version "
                             "differs from this tree's")
    parser.add_argument("--budget", type=int, default=DEFAULT_BUDGET,
                        help="seconds the whole run may take before it is "
                             "abandoned (default %(default)s)")
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
        return SKIP

    #  An install from another version reports that version's failures as
    #  this tree's, which is worse than not running.
    installed, tree = apps.installedVersion(), apps.treeVersion()
    if installed != tree and not args.any_version:
        print("SKIP: installed ECCE under %s is %s, this tree is %s; install "
              "a package built from this tree, or pass --any-version"
              % (apps.INSTALL, installed or "unknown", tree))
        return SKIP

    selected = [n for n in binaries if not args.app or n in args.app]
    if not selected:
        print("no such app; --list shows what is installed", file=sys.stderr)
        return 2

    #  Before anything is started: point this run at an ECCE instance of
    #  its own.  See isolate.py -- the state directory, both ports,
    #  siteconfig/DataServers and ECCE_HELP all have to move together, and
    #  until they did, a run overlapped with the developer's own session
    #  and produced failures that looked like application bugs.
    xvfbPidfile = None
    state = None
    if not args.use_real_state:
        if args.keep_services:
            os.environ["ECCE_TEST_KEEP_STATE"] = "1"
        try:
            settings = isolate.apply(apps.INSTALL)
        except isolate.IsolationError as exc:
            print("FAIL  could not isolate this run: %s" % exc,
                  file=sys.stderr)
            return 2
        print(isolate.describe(settings))

        #  A PREVIOUS run of this suite that got killed (ctest timeout,
        #  ^C, a cancelled CI job) never reaches its own teardown, so its
        #  brokers/dataserver/Xvfb are still alive under this exact state
        #  directory and collide with the ones this run is about to
        #  start.  Sweep them first, and ONLY them -- see killLeftovers's
        #  docstring for why matching is by state-directory path and
        #  never by process name.
        state = settings["ECCE_REALUSERHOME"]
        note = isolate.killLeftovers(state)
        if note:
            print("  %s" % note)
        xvfbPidfile = os.path.join(state, "xvfb.pid")
        stale = xdisplay.killStaleXvfb(xvfbPidfile)
        if stale:
            print("  stopped a stale Xvfb (pid %d) left by a previous run"
                  % stale)
    else:
        print("NOT isolated: running against %s and the real ports"
              % os.path.expanduser("~"))

    startBudget(args.budget)

    #  From here on, however this run ends -- normal completion, a bad app
    #  hanging past the budget, or this process being killed outright --
    #  teardown.run() must be what stops the services and the display.
    #  Installed before the display even exists so a signal that arrives
    #  during Xvfb startup itself still gets a (harmless, no-op) handler
    #  instead of the default kill-with-no-cleanup behaviour.
    teardown = Teardown()
    teardown.keepServices = args.keep_services
    teardown.state = state
    atexit.register(teardown.run)
    for sig in (signal.SIGTERM, signal.SIGINT, signal.SIGHUP):
        signal.signal(sig, teardown.onSignal)

    try:
        display = xdisplay.Display(number=args.display,
                                   pidfile=xvfbPidfile).__enter__()
    except xdisplay.DisplayUnavailable as exc:
        print("SKIP: %s" % exc)
        return SKIP
    teardown.display = display

    before = apps.serviceState(display)
    teardown.before = before
    results = Results()
    try:
        gl = display.hasGL()
        print("display: %s (GL: %s)"
              % (display.name,
                 "yes" if gl else
                 ("unknown -- glxinfo not installed (mesa-utils)"
                  if gl is None else
                  "NO -- the viewer apps will not render")))
        #  Stop the per-app reaper tearing the gateway down between
        #  apps.  Its rule -- only the last app out turns the lights
        #  off (#102) -- is right for a user's session and wrong here,
        #  where every app IS the last one out: the broker and the
        #  dispatcher were being stopped after the first app exited,
        #  and every app after it met a dead gateway and was reported
        #  as opening no window.
        #
        #  This suite already stops the services itself in the finally
        #  block below, so taking that job over is not a loss.
        os.environ["ECCE_NO_REAP"] = "1"

        serviceLog = []
        apps.startServices(display, serviceLog)
        for line in serviceLog:
            print("  %s" % line)

        #  Check the services actually came up before sweeping every app
        #  against them.  Without this, a dataserver that failed to start
        #  is reported as ten apps that "opened no window within 40s" --
        #  a list that reads like ten bugs and names none of them.
        after = apps.serviceState(display)
        down = sorted(k for k, up in after.items() if not up)
        if down:
            results.fail("services",
                         "%s did not start, so nothing below can work.\n"
                         "      Every app that needs the server will be "
                         "reported as opening no window. That is this one "
                         "fault, not a dozen separate ones.\n      %s"
                         % (" and ".join(down), " | ".join(serviceLog)))

        #  Give the data server the one account that makes it a SET-UP
        #  server rather than a virgin one.  EDSIServerCentral::
        #  checkServerSetup() checks for the users collection and throws
        #  "A failure was detected in the ECCE server setup" when it
        #  cannot read it -- which on a data server nobody has ever added
        #  an account to is every app, every time, and BuilderApp quits
        #  outright on it.  A person does this once, by hand, from
        #  GETTING_STARTED; a run that seeds its own state directory has
        #  to do it too or it is testing an install that was never
        #  finished.
        note = fixture.ensureRealUserAccount()
        if note:
            print("  %s" % note)

        #  The app's name goes out BEFORE it runs, so a wedged suite
        #  names its culprit.  The partial-line form is nicer to read
        #  ("name ... done" on one line) but GitHub Actions only shows
        #  COMPLETE lines, so a hang loses it entirely and the last
        #  SUCCESSFUL app looks like the last thing attempted -- which
        #  sent this CI hang down the wrong path twice.  ECCE_APPS_TRACE
        #  switches to whole lines for that reason.
        trace = bool(os.environ.get("ECCE_APPS_TRACE"))
        swept = []
        stalled = False
        for name in selected:
            if trace:
                print("  %-16s starting" % name, flush=True)
            else:
                print("  %-16s" % name, end="", flush=True)
            checkApp(display, name, results, verbose=args.verbose)
            swept.append(name)
            print("done", flush=True)

            #  Stop the moment the DISPLAY itself stops answering.
            #
            #  This is the other half of #127.  Once the X server goes
            #  quiet, every remaining app "opens no window within 40s"
            #  and is killed -- thirteen identical failures, none of them
            #  about the app it names, and the one interesting fact (that
            #  it started right after a particular app) buried.  A run
            #  that reports "the display stopped answering after <app>"
            #  and stops has said the only true thing there is to say,
            #  and said it in one line.
            if not display.responsive():
                stalled = True
                results.fail(
                    "display",
                    "the X server stopped answering after %s, so the %d "
                    "app(s) after it were not tested.\n"
                    "      Everything from here would have failed "
                    "identically ('opened no window within 40s') no matter "
                    "what it is, which is one fault and not %d.\n"
                    "      %s (%s).  Still connected: %s"
                    % (name, len(selected) - len(swept),
                       len(selected) - len(swept) + 1,
                       display.serverState(), display.probeNote,
                       display.clients() or "(xlsclients says nothing)")
                    + ("\n      Xvfb stderr: %s" % display.serverLog(500)
                       if display.serverLog() else ""))
                break

        if not args.app and not stalled:
            checkCalculation(display, results, verbose=args.verbose)
            checkStructureFiles(display, results, verbose=args.verbose)
            restorePrefs = fixture.settleUpgradeNotices()
            try:
                geomtrace_stress.check(display, results,
                                       verbose=args.verbose)
                pbc_edit_test.check(display, results, verbose=args.verbose)
                pbc_ux_test.check(display, results)
            finally:
                restorePrefs()
            checkStale(results, set(swept))
    except BudgetExpired as exc:
        results.fail("run", str(exc))
    finally:
        # Leave the machine as we found it: these are somebody's per-user
        # services and this suite is not entitled to leave them running.
        #
        # Stopped BEFORE the display goes away, not after: the dispatcher
        # is per display and ecce-gateway-stop finds its pidfile by
        # $DISPLAY, so it has to be told which one -- and the reaper
        # decides whether any app is still alive on that display by
        # reading the processes' own environment.  (See Teardown -- this
        # is the same logic the SIGTERM/SIGINT/SIGHUP handlers and the
        # atexit hook run, so a killed run does this too, not just a
        # normal exit.)
        teardown.run()

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
