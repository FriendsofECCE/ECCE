#!/usr/bin/env python3
"""The Builder's normal-mode arrows sit in the frame of the drawn atoms (#312).

    tests/apps/vib_frame_builder_test.py             own Xvfb and services

ORCA with UseSym reorients the molecule and prints its modes in that frame.
The calculation here is made from real ORCA output the way a job leaves it:
chemsys.mvm is the input geometry, GEOMTRACE and VIB are what the real
parser scripts extract (tests/parsers/vib_frame_test.py checks those two
alone).  The Builder draws the last geometry step; "vibframe" reads back the
cosine between the arrow of the hydrogen that moves most in a high-frequency
mode and its X-H bond, which is 1 only if atoms and arrows share a frame.
The structure in the input frame gives an arbitrary value, the control.
"""
import argparse
import importlib.util
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "tests", "parsers"))

import apps                                                   # noqa: E402
import fixture                                                # noqa: E402
import vib_frame_test as parsed                               # noqa: E402

BASE = os.path.join(HERE, "fixtures", "calc-water-vib")
ORCA = os.path.join(ROOT, "tests", "parsers", "fixtures", "orca")
MARKERS = ("ended by SIG", "Segmentation fault", "terminate called")
MIN_COS = 0.95

#  (name, output, runtype, trace kept); the highest-frequency mode is the last
#  one.  Without GEOMTRACE the Builder shows the input frame: the control,
#  which must NOT line up.
CASES = (
    ("freq", "ethanol_sym_freq", "Vibration", True),
    ("optfreq", "ethanol_sym_optfreq", "GeoVib", True),
    ("control", "ethanol_sym_freq", "Vibration", False),
)


def loadPlots():
    spec = importlib.util.spec_from_file_location(
        "plots_makecalc", os.path.join(ROOT, "tests", "plots", "makecalc.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def inputAtoms(deck):
    """[(symbol, x, y, z)] from the "* xyz" block of an ORCA deck."""
    atoms, inside = [], False
    for line in open(deck):
        if line.startswith("* xyz"):
            inside = True
        elif inside and line.strip() == "*":
            break
        elif inside:
            s, x, y, z = line.split()
            atoms.append((s, float(x), float(y), float(z)))
    return atoms


def vibXml(rec, natoms):
    size = rec["size"].split()
    nmodes = int(size[0])
    values = rec["values"].split()
    head = ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vectable columnLabel="Atom" columnLabels="X Y Z" columns="3" '
            'name="VIB" rowLabel="Normal Mode" rowLabels="%s" rows="%d" '
            'units="NA" vectorLabel="Coordinate" vectorLabels="%s" '
            'vectors="%d">\n\n  <step number="0"/>\n'
            % (" ".join("%d-X" % (i + 1) for i in range(natoms)), natoms,
               " ".join("%d-A" % (i + 1) for i in range(nmodes)), nmodes))
    body = "".join('\n  <matrix number="%d">%s</matrix>\n'
                   % (m + 1, "\n".join(values[m * natoms * 3:
                                              (m + 1) * natoms * 3]))
                   for m in range(nmodes))
    return head + body + "\n</vectable>\n"


def vecXml(name, rec, label):
    vals = rec["values"].split()
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="Normal Mode" rowLabels="%s" '
            'rows="%d" units="%s">%s</vector>\n'
            % (name, " ".join("%d-A" % (i + 1) for i in range(len(vals))),
               len(vals), label, "   ".join(vals)))


def makeCalc(out, name, runtype, work, trace):
    """A calculation directory from ORCA output `name`; returns its path."""
    found = parsed.records(os.path.join("orca", name + ".out"),
                           "orca.desc", runtype)
    atoms = inputAtoms(os.path.join(ORCA, name + ".inp"))
    calc = os.path.join(work, "orca-" + out)
    shutil.copytree(BASE, calc, symlinks=True)
    props = os.path.join(calc, "Props")
    template = os.path.join(props, ".DAV", "ESCF")
    keep = os.path.join(work, "dav-template")
    shutil.copy(template, keep)
    for entry in os.listdir(props):
        path = os.path.join(props, entry)
        if os.path.isfile(path):
            os.remove(path)
    shutil.rmtree(os.path.join(props, ".DAV"))
    os.makedirs(os.path.join(props, ".DAV"))
    plots = loadPlots()
    steps = [{"values": r["values"], "size": r["size"],
              "rowlabels": r["rowlabels"], "units": "Angstrom"}
             for r in found["GEOMTRACE"]]
    for key, text in (
            ("GEOMTRACE", plots.geomtrace(steps) if trace else None),
            ("VIB", vibXml(found["VIB"][-1], len(atoms))),
            ("VIBFREQ", vecXml("VIBFREQ", found["VIBFREQ"][-1], "cm-1"))):
        if text is None:
            continue
        with open(os.path.join(props, key), "w") as handle:
            handle.write(text)
        shutil.copy(keep, os.path.join(props, ".DAV", key))
    with open(os.path.join(calc, "Parameters", "chemsys.mvm"), "w") as h:
        h.write("title: \ntype: molecule\nnum_atoms: %d\n"
                "atom_info: symbol cart\natom_list: \n%s"
                % (len(atoms), "".join("%s %.6f %.6f %.6f\n" % a
                                       for a in atoms)))
    return calc, len(found["VIBFREQ"][-1]["values"].split())


def runCase(display, results, case, args, timeout, work):
    out, name, runtype, trace = case
    what = "vibration frame, ORCA UseSym " + out
    calc, nmodes = makeCalc(out, name, runtype, work, trace)
    url, error = fixture.install("orca-" + out, source_dir=calc)
    if error:
        results.fail(what, "could not install: " + error)
        return
    try:
        script = os.path.join(work, out + ".scene")
        with open(script, "w") as handle:
            handle.write("\n".join(["vibfocus", "vibmode %d" % (nmodes - 1),
                                    "vibvector", "vibframe", ""]))
        result = apps.run(display, "builder",
                          args=args + ("-context", url), windowTimeout=120,
                          settle=timeout,
                          env={"ECCE_VIEWER_SCENE": script,
                               "ECCE_VIEWER_SCENE_OUT": work,
                               "ECCE_REALUSER": fixture.USER})
    finally:
        fixture.remove("orca-" + out)
    log = result.log or ""
    tail = "\n".join("      | " + l for l in log.splitlines()[-25:])
    got = re.findall(r"VIBFRAME: atom=(-?\d+) cos=(-?[\d.]+)", log)
    if result.crashed:
        results.fail(what, "builder CRASHED\n" + tail)
    elif [m for m in MARKERS if m in log]:
        results.fail(what, "builder output has a crash marker\n" + tail)
    elif not got:
        results.fail(what, "no VIBFRAME line\n" + tail)
    elif not trace and float(got[-1][1]) >= MIN_COS:
        results.fail(what, "the input frame lines up too (cos %s): the "
                     "check cannot tell the frames apart" % got[-1][1])
    elif not trace:
        print("ok    %s: input frame cos %s, as it must" % (what, got[-1][1]))
    elif float(got[-1][1]) < MIN_COS:
        results.fail(what, "arrow of H%s makes cos %s with its bond: the "
                     "arrows are not in the frame of the drawn atoms"
                     % (got[-1][0], got[-1][1]))
    else:
        print("ok    %s: cos %s" % (what, got[-1][1]))


def check(display, results, timeout=120):
    work = os.path.join(fixture.stateHome(), "vib-frame")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    try:
        args = ()
        state = fixture.stateHome()
        if (os.path.realpath(state)
                != os.path.realpath(os.path.expanduser("~"))):
            auth = fixture.authFile(os.path.join(state, ".ECCE", "auth.pipe"),
                                    user=fixture.USER)
            args = ("-pipe", auth)
        for case in CASES:
            results.checks += 1
            runCase(display, results, case, args, timeout, work)
    finally:
        shutil.rmtree(work, ignore_errors=True)


class _Results(object):
    def __init__(self):
        self.failures, self.notes, self.checks = [], [], 0

    def fail(self, where, message):
        self.failures.append("%s: %s" % (where, message))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--timeout", type=int, default=120)
    args = parser.parse_args()

    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"

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
                check(display, results, timeout=args.timeout)
            finally:
                restore()
        finally:
            apps.stopServices(display)
    for failure in results.failures:
        print("FAIL  " + failure)
    if not results.failures:
        print("PASS  vibration frame (%d cases)" % results.checks)
    return 1 if results.failures else 0


if __name__ == "__main__":
    sys.exit(main())
