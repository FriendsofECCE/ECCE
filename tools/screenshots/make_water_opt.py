#!/usr/bin/env python3
"""Make the calculation the first-calculation tutorial runs: water, NWChem,
RHF/6-31G*, geometry optimisation, on `localhost`, in local data mode.

    tools/screenshots/make_water_opt.py --build <build dir> --out DIR \\
        [--project-meta FILE]

Needs nwchem, perl and the launch test binaries (tests/launch/harness.py).
The calculation is made as the Calculation Editor makes it: the molecule,
basis, theory and run type go in through the same import code
(`launchjob setup`), the theory dialogs' values come from running them once
(tests/teaching/deckgen.py), then Launch runs NWChem and the job store fills
Props/.  DIR receives the calculation folder, `water-opt`, as local data
keeps it; --project-meta receives the metadata file of the project folder.
"""

import argparse
import os
import re
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
for sub in ("launch", "teaching"):
    sys.path.insert(0, os.path.join(ROOT, "tests", sub))

import harness                    # noqa: E402
import deckgen                    # noqa: E402
import geometry as G              # noqa: E402
import teaching_cases as C        # noqa: E402

NAME = "water-opt"
#  What the fixture says about where it ran; the real path is the build host's.
PUBLIC_RUNDIR = "/home/user/ecce-runs"
PUBLIC_USER = "user"


def setupParams(defaults, case):
    """The theory and run-type dialogs' own lines, as SetupParams stores them."""
    nelec = deckgen.electrons(case.atoms, case.charge)
    lines = {}
    for script in ("nedtheory.py", "nedruntype.py"):
        inv = deckgen.dialogs.runDialog(
            defaults.display, script, category=case.category,
            theory=case.theory, runType=case.runtype, numElectrons=nelec,
            spinMultiplicity=case.mult, numFrozenOrbs=0,
            numOccupiedOrbs=(nelec + case.mult - 1) // 2, numVirtualOrbs=20,
            numNormalModes=max(1, 3 * len(case.atoms) - 6))
        for line in inv["emitted"]:
            fields = line.split("|")
            if len(fields) >= 6:
                lines[fields[0]] = line
    return "\n".join(lines[k] for k in sorted(lines)) + "\n"


def nameBasis(case):
    out = ["NameBasis", 'basis "ao basis" cartesian']
    for el in sorted({a[0] for a in case.atoms}):
        out.append('  %s library "%s"' % (el, case.basis[el]))
    out += ["END", "EndNameBasis"]
    return "\n".join(out) + "\n"


def tidy(root, session, rundir):
    """Replace this machine's user name and run directory in the metadata."""
    for dirpath, _, files in os.walk(root):
        for name in files:
            path = os.path.join(dirpath, name)
            try:
                text = open(path, encoding="utf-8").read()
            except (UnicodeDecodeError, OSError):
                continue
            new = text.replace(rundir, PUBLIC_RUNDIR)
            new = re.sub(r"\b%s\b" % re.escape(session.user()), PUBLIC_USER, new)
            new = re.sub(r"(ecce:job_clienthost\t\t).*", r"\g<1>localhost", new)
            if new != text:
                with open(path, "w", encoding="utf-8") as handle:
                    handle.write(new)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--project-meta",
                    help="copy the project folder's metadata file here")
    args = ap.parse_args()

    build = os.path.abspath(args.build)
    harness.prerequisites(build, ("nwchem", "perl"))
    #  In the xy plane, so the viewer's default view looks at the bend.
    atoms = [("O", 0.0, 0.0, 0.0), ("H", 0.766044, 0.642788, 0.0),
             ("H", -0.766044, 0.642788, 0.0)]
    case = C.case(NAME, "H2O", atoms, "B", "RHF", "6-31G*")
    s = harness.Session(build, "waterfix", {"NWChem": shutil.which("nwchem")},
                        keep=False, local=True)
    work = tempfile.mkdtemp(prefix="water-opt-setup-")
    try:
        with deckgen.Defaults() as defaults:
            deck = deckgen.generate(case, defaults)[0]
            params = setupParams(defaults, case)
            with open(os.path.join(work, "calc.param"), "w") as h:
                h.write(deckgen.param(case, defaults)
                        + "ES.ChemSys.Multiplicity: %d\n" % case.mult)
        with open(os.path.join(work, "calc.frag"), "w") as h:
            h.write(deckgen.frag(case))
        with open(os.path.join(work, "calc.gbs"), "w") as h:
            h.write(nameBasis(case))
        with open(os.path.join(work, "SetupParams"), "w") as h:
            h.write(params)
        with open(os.path.join(work, "deck.nw"), "w") as h:
            h.write(deck)

        if not s.services(True):
            print("FAILED: services did not start")
            return 1
        rundir = os.path.join(s.state, "jobs")
        os.makedirs(rundir, exist_ok=True)
        url, out = s.create(NAME, "nwchem_es", os.path.join(work, "deck.nw"),
                            "nwch.nw", rundir)
        if not url:
            print("create failed: " + out)
            return 1
        rc, out = s.driver("setup", url, work, "calc.out",
                           os.path.join(work, "SetupParams"))
        print(out.strip())
        if rc != 0:
            return 1
        rc, out = s.launch(url)
        print(out.strip()[-600:])
        if rc != 0:
            return 1
        state = s.waitState(url, 600)
        print("state:", state)
        if state != "completed":
            return 1
        #  The job store fills Props/ after the state changes.
        props = os.path.join(s.localData(), "users", "local",
                             NAME + "-project", NAME, "Props")
        last, since = None, time.time()
        while time.time() - since < 3 and time.time() - since < 60:
            now = sorted(os.listdir(props)) if os.path.isdir(props) else None
            if now != last:
                last, since = now, time.time()
            time.sleep(0.5)
        print("Props:", " ".join(last or []))

        calc = os.path.dirname(props)
        dest = os.path.join(args.out, NAME)
        shutil.rmtree(dest, ignore_errors=True)
        os.makedirs(args.out, exist_ok=True)
        shutil.copytree(calc, dest, symlinks=True)
        for dirpath, _, files in os.walk(dest):
            for name in files:
                if name.endswith(".lock"):
                    os.unlink(os.path.join(dirpath, name))
        tidy(dest, s, rundir)
        if args.project_meta:
            shutil.copy(os.path.join(os.path.dirname(calc), ".ecce-meta"),
                        args.project_meta)
        print("wrote", dest)
        return 0
    finally:
        shutil.rmtree(work, ignore_errors=True)
        s.stop()


if __name__ == "__main__":
    sys.exit(main())
