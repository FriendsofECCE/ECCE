#!/usr/bin/env python3
"""The Geometry Trace panel under stress, headless (#217).

    tests/apps/geomtrace_stress.py                 own Xvfb and services
    tests/apps/geomtrace_stress.py --valgrind      the builder under memcheck

Run by tests/apps/run_tests.py as well.  Three calculations are built from
real output files (Gaussian, ORCA and NWChem water optimisations in
tests/parsers/fixtures, put through the real parsers) and opened in one
Builder.  A scene script (ECCE_VIEWER_SCENE, the Builder's "gt..." commands)
then steps through every trace index and past both ends, plays and stops,
delivers GEOMTRACE messages the way a running job does -- the next step, a
step that skips ahead, a step whose size disagrees with its header, a step
the data server cannot deliver -- while the animation runs, floats and docks
the panel, switches and closes calculations while playing, unfocuses every
panel as quit does, and removes the panels while their timer is pending.

Passing means the Builder ran the whole script, exited by itself, did not
die on a signal, and still held the stored number of steps after each
message that should not have changed it.  With --valgrind, memcheck must
also report no invalid reads or writes.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(ROOT, "tests", "parsers")
sys.path.insert(0, HERE)
sys.path.append(PARSERS)

import apps                                                   # noqa: E402
import fixture                                                # noqa: E402

BASE = os.path.join(HERE, "fixtures", "calc-water-vib")

#  Water optimisations: the same three atoms as the base calculation.
CASES = (("gt-g16", "g16-h2o-optfreq"),
         ("gt-orca", "orca-h2o-opt"),
         ("gt-nwchem", "nwchem-h2o-opt"))

MARKERS = ("ended by SIG", "Segmentation fault", "double free or corruption",
           "terminate called", "Bad Geomtrace", "Throw Log",
           "Unhandled standard exception", "Unhandled unknown exception")


def traces(caseName):
    """Every GEOMTRACE and TEVEC record the parsers produce, in order."""
    #  tests/apps has a cases.py of its own, so load the parsers' by path.
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "parser_cases", os.path.join(PARSERS, "cases.py"))
    CASEDEFS = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(CASEDEFS)
    from eccejobmonitor_sim import (read_desc, replay, run_parser,
                                    parse_parser_output)
    case = [c for c in CASEDEFS.CASES if c["name"] == caseName][0]
    desc = read_desc(os.path.join(ROOT, "scripts", "parsers", case["desc"]))
    result = replay(desc, os.path.join(PARSERS, "fixtures", case["fixture"]))
    found = {"GEOMTRACE": [], "TEVEC": []}
    for entry in desc.live_entries():
        for block in result.delivered_for(entry):
            out, _err, _rc = run_parser(
                os.path.join(ROOT, "scripts", "parsers"), entry, block,
                case.get("parse_args"))
            for rec in parse_parser_output(out):
                if rec["key"] in found:
                    found[rec["key"]].append(rec["flat"])
    return found


def geomtrace(steps):
    first = steps[0]
    _v, rows, columns = first["size"].split()
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<tsvectable columnLabel="Atom" columnLabels="%s" columns="%s" '
           'name="GEOMTRACE" rowLabel="Geometry Step" rowLabels="%s" '
           'rows="%s" units="%s" vectorLabel="Coordinate" vectors="1">\n'
           % (first.get("columnlabels", "X Y Z"), columns,
              first.get("rowlabels", ""), rows, first.get("units", "Angstrom"))]
    for i, step in enumerate(steps):
        out.append('  <step number="%d">%s</step>\n'
                   % (i + 1, " ".join(step["values"].split())))
    out.append("</tsvectable>\n")
    return "".join(out)


def tevec(steps):
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
           '<tsvector name="TEVEC" rowLabel="Geometry Step" units="%s">\n'
           % steps[0].get("units", "Hartree")]
    for i, step in enumerate(steps):
        out.append('  <step number="%d">%s</step>\n'
                   % (i + 1, step["values"].split()[0]))
    out.append("</tsvector>\n")
    return "".join(out)


def makeCalc(caseName, out):
    """A copy of the base calculation carrying caseName's whole trace.
    Returns the number of steps."""
    found = traces(caseName)
    if len(found["GEOMTRACE"]) < 2:
        raise RuntimeError("%s: fewer than two GEOMTRACE steps" % caseName)
    if os.path.exists(out):
        shutil.rmtree(out)
    shutil.copytree(BASE, out)
    props = os.path.join(out, "Props")
    with open(os.path.join(props, "GEOMTRACE"), "w") as handle:
        handle.write(geomtrace(found["GEOMTRACE"]))
    if found["TEVEC"]:
        with open(os.path.join(props, "TEVEC"), "w") as handle:
            handle.write(tevec(found["TEVEC"]))
        shutil.copy(os.path.join(props, ".DAV", "VIBFREQ"),
                    os.path.join(props, ".DAV", "TEVEC"))
    return len(found["GEOMTRACE"])


def script(calcs):
    """The scene script: calcs is [(url, steps)], the first one opened."""
    (a, na), (b, nb), (c, nc) = calcs
    return "\n".join([
        "gtfocus", "gtsweep", "gtexpect %d" % na,
        #  clicking a point of the plot shows that step's geometry
        "gtpick 0", "gtpick mid", "gtpick last",
        "gtplay 300 1", "gtsweep", "gtstop",
        #  messages from a running job, each while the animation runs
        "gtplay 100 1", "gtupdate grow", "gtexpect %d" % (na + 1),
        "gtplay 100 1", "gtupdate gap", "gtexpect %d" % na,
        "gtplay 100 1", "gtupdate badsize", "gtexpect %d" % na,
        "gtplay 100 1", "gtupdate junk", "gtexpect %d" % na,
        "gtsweep", "gtstop",
        #  Tools > Dock Floating Panels, then away and back while playing
        "gtplay 100 1", "gtfloat",
        "gtcontext %s" % b, "gtfocus", "gtplay 200 1", "gtexpect %d" % nb,
        "gtcontext %s" % a, "gtfocus", "gtplay 200 1",
        "gtunfocusall", "gtexpect %d" % na,
        #  the panels go while their timer is pending
        "gtfocus", "gtplay 100 1", "gtremovepanels", "hold 1",
        "gtcontext %s" % c, "gtfocus", "gtplay 200 1", "gtexpect %d" % nc,
        "gtclose", "hold 1",
        "gtunfocusall",
        "",
    ])


def expectCount(text):
    return len(re.findall(r"^(?:gtexpect|gtpick) ", text, re.M))


def check(display, results, verbose=False, timeout=240):
    """Run the stress script once; record a failure in results."""
    results.checks += 1
    what = "geometry trace stress"
    state = fixture.stateHome()
    work = os.path.join(state, "geomtrace-stress")
    if os.path.exists(work):
        shutil.rmtree(work)
    os.makedirs(work)
    try:
        calcs = []
        for name, case in CASES:
            steps = makeCalc(case, os.path.join(work, name))
            url, error = fixture.install(name, source_dir=os.path.join(work,
                                                                       name))
            if error:
                results.fail(what, "could not install %s: %s" % (name, error))
                return
            calcs.append((url, steps))
        text = script(calcs)
        path = os.path.join(work, "scene")
        with open(path, "w") as handle:
            handle.write(text)

        #  As the fixture account: another user cannot list its Props, and
        #  a calculation without properties has no panels.
        args = ("-context", calcs[0][0])
        if (os.path.realpath(state)
                != os.path.realpath(os.path.expanduser("~"))):
            auth = fixture.authFile(os.path.join(state, ".ECCE", "auth.pipe"),
                                    user=fixture.USER)
            args = ("-pipe", auth) + args
        result = apps.run(display, "builder", args=args, windowTimeout=120,
                          settle=timeout,
                          env={"ECCE_VIEWER_SCENE": path,
                               "ECCE_VIEWER_SCENE_OUT": work,
                               "ECCE_REALUSER": fixture.USER})
        log = result.log or ""
        tail = "\n".join("      | " + l for l in log.splitlines()[-15:])
        found = [m for m in MARKERS if m in log]
        failed = os.path.join(work, "FAILED")
        done = log.count("GTSTRESS:")
        if result.crashed:
            results.fail(what, "builder CRASHED (%s)\n%s"
                         % (result.signalName or result.returncode, tail))
        elif os.path.exists(failed):
            with open(failed) as handle:
                results.fail(what, "the script stopped: %s\n%s"
                             % (handle.read().strip(), tail))
        elif not result.exitedAfterWindow:
            results.fail(what, "builder did not finish the script within "
                         "%ds\n%s" % (timeout, tail))
        elif found:
            results.fail(what, "its output contains %s\n%s"
                         % (", ".join(repr(m) for m in found), tail))
        elif done != expectCount(text):
            results.fail(what, "%d of %d trace checks ran\n%s"
                         % (done, expectCount(text), tail))
        elif "ERROR SUMMARY" in log and not re.search(
                r"ERROR SUMMARY: 0 errors", log):
            results.fail(what, "memcheck reported errors:\n%s"
                         % "\n".join("      | " + l for l in log.splitlines()
                                     if "Invalid" in l or "SUMMARY" in l))
        elif verbose:
            results.notes.append("%-14s %d checks over %s steps"
                                 % ("geomtrace", done,
                                    "/".join(str(n) for _u, n in calcs)))
        if os.environ.get("GTSTRESS_LOG"):
            with open(os.environ["GTSTRESS_LOG"], "w") as h:
                h.write(log)
    finally:
        for name, _case in CASES:
            fixture.remove(name)


class _Results(object):
    def __init__(self):
        self.failures, self.notes, self.checks = [], [], 0

    def fail(self, where, message):
        self.failures.append("%s: %s" % (where, message))


def _wrapBuilder(home, prefix):
    """Run $ECCE_HOME/bin/builder under `prefix` (e.g. valgrind)."""
    binLink = os.path.join(home, "bin")
    real = os.path.realpath(binLink)
    os.remove(binLink)
    os.makedirs(binLink)
    for entry in os.listdir(real):
        if entry != "builder":
            os.symlink(os.path.join(real, entry), os.path.join(binLink, entry))
    wrapper = os.path.join(binLink, "builder")
    with open(wrapper, "w") as handle:
        handle.write('#!/bin/sh\nexec %s "%s" "$@"\n'
                     % (prefix, os.path.join(real, "builder")))
    os.chmod(wrapper, 0o755)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--valgrind", action="store_true")
    parser.add_argument("--gdb", action="store_true",
                        help="print a backtrace if the builder crashes")
    parser.add_argument("--timeout", type=int, default=None)
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args()

    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"
    if args.valgrind:
        _wrapBuilder(settings["ECCE_HOME"],
                     "valgrind --error-limit=no --num-callers=30")
    if args.gdb:
        _wrapBuilder(settings["ECCE_HOME"],
                     "gdb -q -batch -ex run -ex bt -ex quit --args")
    timeout = args.timeout or (1800 if args.valgrind else 240)

    results = _Results()
    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        try:
            if not all(apps.serviceState(display).values()):
                print("FAIL  services did not come up: %s" % log)
                return 1
            fixture.ensureRealUserAccount()
            restore = fixture.settleUpgradeNotices()
            try:
                check(display, results, verbose=args.verbose, timeout=timeout)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for note in results.notes:
        print("  " + note)
    for failure in results.failures:
        print("FAIL  " + failure)
    if not results.failures:
        print("PASS  geometry trace stress")
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
