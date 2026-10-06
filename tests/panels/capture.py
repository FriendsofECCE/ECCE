#!/usr/bin/env python3
"""The Builder's panel layouts, headless: screenshots, the viewer's width
share, and the hook-driven reachability test (View > Panel layout).

    tests/panels/capture.py --builder build-cmake/builder --test
    tests/panels/capture.py --builder build-cmake/builder --screen 1920x1080 \\
        --png DIR --tag after [--mode detail ...]

Like tests/spectrum/capture.py it loads a calculation built from a real
output file (default g16-h2o-optfreq: orbitals, energies, vibrations) in
the real Builder on a private Xvfb, with the installed ECCE for everything
but the builder binary.  No synthetic input: the Builder is driven by its
own hooks (ECCE_PANEL_MODE, ECCE_OPEN_PANEL, ECCE_PANEL_METRICS,
ECCE_PANEL_TEST, ECCE_PANEL_FULLSCREEN).

--test runs every layout in one session and checks, as a menu click
would, that every property panel and tool opens and has a real size,
that the tabs, F9 and the list work, and that switching layouts keeps the
open panels; it exits non-zero on any failure.

--png writes one screenshot per layout and reports the viewer's width as a
share of the window.  --descriptor points at a PropertyPanelDescriptor.xml
to use in place of the installed one (the installed file has no groups
until the package is rebuilt).  Not part of ctest: it needs /opt/ecce,
several services and about half a minute per run.
"""
import argparse
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
sys.path.insert(0, os.path.join(ROOT, "tests", "spectrum"))

import makecalc  # noqa: E402

MODES = ["classic", "stacked", "accordion", "detail"]


def overlay(builder, where, descriptor):
    """An $ECCE_HOME like /opt/ecce's whose builder is `builder` and, if
    given, whose PropertyPanelDescriptor.xml is `descriptor`."""
    if os.path.exists(where):
        shutil.rmtree(where)
    os.makedirs(os.path.join(where, "bin"))
    for entry in os.listdir("/opt/ecce"):
        if entry not in ("bin", "data" if descriptor else ""):
            os.symlink(os.path.join("/opt/ecce", entry),
                       os.path.join(where, entry))
    for entry in os.listdir("/opt/ecce/bin"):
        if entry != "builder":
            os.symlink(os.path.join("/opt/ecce/bin", entry),
                       os.path.join(where, "bin", entry))
    os.symlink(os.path.abspath(builder), os.path.join(where, "bin", "builder"))
    if descriptor:
        client = os.path.join(where, "data", "client")
        config = os.path.join(client, "config")
        os.makedirs(config)
        for entry in os.listdir("/opt/ecce/data"):
            if entry != "client":
                os.symlink(os.path.join("/opt/ecce/data", entry),
                           os.path.join(where, "data", entry))
        for entry in os.listdir("/opt/ecce/data/client"):
            if entry != "config":
                os.symlink(os.path.join("/opt/ecce/data/client", entry),
                           os.path.join(client, entry))
        for entry in os.listdir("/opt/ecce/data/client/config"):
            if entry != "PropertyPanelDescriptor.xml":
                os.symlink(os.path.join("/opt/ecce/data/client/config", entry),
                           os.path.join(config, entry))
        os.symlink(os.path.abspath(descriptor),
                   os.path.join(config, "PropertyPanelDescriptor.xml"))


def run(case, builder, descriptor, modes, png, tag, test, timeout, open_panel):
    base = os.path.dirname(os.path.abspath(builder))
    state = os.path.join(base, "panels-state")
    os.environ["ECCE_TEST_STATE"] = state
    home = os.path.join(base, "panels-home")
    overlay(builder, home, descriptor)
    os.environ["ECCE_TEST_HOME"] = home
    import apps          # reads ECCE_TEST_HOME at import
    import fixture
    import isolate
    import xdisplay
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"

    calc = os.path.join(state, "calc-" + case)
    makecalc.make(case, calc)
    results = []

    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        if not all(apps.serviceState(display).values()):
            apps.stopServices(display)
            return ["services did not come up: %s" % log], results
        queues = os.path.join(state, ".ECCE", "Queues")
        os.makedirs(os.path.dirname(queues), exist_ok=True)
        open(queues, "a").close()
        fixture.ensureRealUserAccount()
        restorePrefs = fixture.settleUpgradeNotices()
        url, error = fixture.install(case, source_dir=calc)
        if error:
            apps.stopServices(display)
            restorePrefs()
            return ["fixture install failed: %s" % error], results

        problems = []
        for mode in (["test"] if test else modes):
            #  Each run starts from a fresh Builder preferences file, so a
            #  saved panel layout from the previous mode cannot leak in.
            for ini in ("wxbuilder.ini",):
                for root, _dirs, files in os.walk(state):
                    if ini in files:
                        os.remove(os.path.join(root, ini))
            env = display.env()
            env["ECCE_PANEL_FULLSCREEN"] = "1"
            if open_panel:
                env["ECCE_OPEN_PANEL"] = open_panel
            report = os.path.join(state, "panel-%s.txt" % mode)
            metrics = os.path.join(state, "panel-%s.metrics" % mode)
            for f in (report, metrics):
                if os.path.exists(f):
                    os.remove(f)
            if test:
                env["ECCE_PANEL_TEST"] = report
                env["ECCE_EXIT_AFTER_DUMP"] = "1"
            else:
                env["ECCE_PANEL_MODE"] = mode
                env["ECCE_PANEL_METRICS"] = metrics
            auth = os.path.join(state, "auth.pipe")
            fixture.authFile(auth, port=int(settings["ECCE_DATASERVER_PORT"]))
            proc = subprocess.Popen(
                [os.path.join(apps.WRAPPERS, "ecce-builder"), "-pipe", auth,
                 "-context", url], env=env, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT, start_new_session=True)
            time.sleep(8)
            try:
                print("exe:", os.readlink("/proc/%d/exe" % proc.pid))
            except OSError as e:
                print("exe?", e)
            target = report if test else metrics
            deadline = time.time() + timeout
            while time.time() < deadline:
                if os.path.exists(target) and os.path.getsize(target) > 0:
                    break
                if proc.poll() is not None:
                    break
                time.sleep(0.5)
            if png and not os.path.exists(target):
                subprocess.run(["import", "-display", display.name, "-window",
                                "root", os.path.join(png, "timeout-%s.png" % mode)],
                               env=env, check=False)
            if not test and os.path.exists(target):
                time.sleep(3)
                out = os.path.join(png, "%s-%s.png" % (tag, mode))
                subprocess.run(["import", "-display", display.name,
                                "-window", "root", out], env=env, check=False)
                results.append((mode, out, open(target).read().strip()))
            if test:
                try:
                    proc.wait(timeout=60)
                except subprocess.TimeoutExpired:
                    pass
            for sig in (15, 9):
                if proc.poll() is not None:
                    break
                try:
                    os.killpg(os.getpgid(proc.pid), sig)
                    proc.wait(timeout=10)
                except Exception:
                    pass
            text = (proc.stdout.read() or b"").decode("utf-8", "replace")
            if os.environ.get("PANELS_VERBOSE"):
                print("builder exit", proc.returncode)
                print(text[-2500:])
            if not os.path.exists(target):
                problems.append("%s: nothing written; builder said:\n%s"
                                % (mode, text[-2500:]))
            elif test:
                body = open(target).read()
                print(body)
                if "RESULT ok" not in body:
                    problems.append("panel test failed (see FAIL lines)")
        apps.stopServices(display)
        restorePrefs()
    return problems, results


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawTextHelpFormatter)
    ap.add_argument("--builder", default="build-cmake/builder")
    ap.add_argument("--case", default="g16-h2o-optfreq")
    ap.add_argument("--descriptor",
                    default=os.path.join(ROOT, "data", "client", "config",
                                         "PropertyPanelDescriptor.xml"))
    ap.add_argument("--no-descriptor", action="store_true",
                    help="use the installed descriptor (for a before run)")
    ap.add_argument("--mode", action="append", choices=MODES)
    ap.add_argument("--png", metavar="DIR")
    ap.add_argument("--tag", default="live")
    ap.add_argument("--test", action="store_true")
    ap.add_argument("--screen", default="1280x1024")
    ap.add_argument("--open-panel", default="Vibrational Frequencies")
    ap.add_argument("--timeout", type=float, default=150)
    args = ap.parse_args()

    import xdisplay
    xdisplay.SCREEN = args.screen + "x24"
    builder = os.path.abspath(args.builder)
    if not os.access(builder, os.X_OK):
        print("no builder at %s" % builder)
        return 2
    if args.png:
        os.makedirs(args.png, exist_ok=True)
    elif not args.test:
        print("give --png DIR or --test")
        return 2
    problems, results = run(
        args.case, builder, None if args.no_descriptor else args.descriptor,
        args.mode or MODES, args.png, "%s-%s" % (args.tag, args.screen),
        args.test, args.timeout, args.open_panel)
    for mode, out, metrics in results:
        print("%s %s: %s -> %s" % (args.screen, mode, metrics, out))
    for p in problems:
        print("FAIL  " + p)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
