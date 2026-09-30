"""Input decks made the way the Calculation Editor makes them.

CalcEd writes <name>.frag, .basis and .param and runs the real
scripts/parsers/ai.nwchem over a copy of nwch.tpl (CalcEd::input_controller).
The settings a student does not touch come from the theory and runtype
dialogs' own defaults, which CalcEd collects by running each dialog once with
NO_GUIValues; do the same here (tests/dialogs/harness.py) instead of writing
the keys by hand, so a change to a dialog default changes the deck.
"""

import importlib.util
import os
import shutil
import subprocess
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")

_spec = importlib.util.spec_from_file_location(
    "dialogs_harness", os.path.join(REPO, "tests", "dialogs", "harness.py"))
dialogs = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(dialogs)

Z = {"H": 1, "He": 2, "C": 6, "N": 7, "O": 8, "F": 9, "P": 15, "S": 16,
     "Cl": 17, "Se": 34}


def electrons(atoms, charge):
    return sum(Z[a[0]] for a in atoms) - charge


class Defaults(object):
    """Dialog defaults per (category, theory, runtype), run once each."""

    def __init__(self):
        self.display = dialogs.Display()
        self.cache = {}

    def __enter__(self):
        self.display.__enter__()
        return self

    def __exit__(self, *exc):
        return self.display.__exit__(*exc)

    def get(self, category, theory, runtype, nelec, mult, natoms):
        key = (category, theory, runtype, mult)
        if key not in self.cache:
            nocc = (nelec + mult - 1) // 2
            values = {}
            for script in ("nedtheory.py", "nedruntype.py"):
                inv = dialogs.runDialog(
                    self.display, script, category=category, theory=theory,
                    runType=runtype, numElectrons=nelec,
                    spinMultiplicity=mult, numFrozenOrbs=0,
                    numOccupiedOrbs=nocc, numVirtualOrbs=20,
                    numNormalModes=max(1, 3 * natoms - 6))
                #  name|value|unit|enabled|export|type; the last line for a
                #  key wins, and GUIValues::dumpKeyVals() writes a key only
                #  when it is enabled and exported.
                for line in inv["emitted"]:
                    f = line.split("|")
                    if len(f) >= 6:
                        values[f[0]] = (f[1], f[3] == "1", f[4] == "1")
            self.cache[key] = {k: v[0] for k, v in values.items()
                               if v[1] and v[2]}
        return dict(self.cache[key])


def param(case, defaults):
    nelec = electrons(case.atoms, case.charge)
    lines = [
        "title: %s" % case.name, "annotation: %s" % case.name,
        "parseFile: ecce.out",
        "Category: %s" % case.category, "Theory: %s" % case.theory,
        "RunType: %s" % case.runtype,
        "Charge: %d" % case.charge, "Symmetry: C1",
        "NumElectrons: %d" % nelec, "NumFrozenOrbs: 0",
        "NumOccupiedOrbs: %d" % ((nelec + case.mult - 1) // 2),
        "NumVirtualOrbs: 20",
        "NumNormalModes: %d" % max(1, 3 * len(case.atoms) - 6),
        "ChemSys.Multiplicity: %d" % case.mult,
        "BasisSet.Coordinates: Cartesian",
        #  CalcEd::storeUseSymmetry(): written for every calculation, default on.
        "ES.Theory.UseSymmetry: 1",
    ]
    vals = defaults.get(case.category, case.theory, case.runtype, nelec,
                        case.mult, len(case.atoms))
    for key in sorted(vals):
        lines.append("%s: %s" % (key, vals[key]))
    return "\n".join(lines) + "\n"


def frag(case):
    lines = ["title: %s" % case.name, "type: molecule",
             "num_atoms: %d" % len(case.atoms), "atom_info: symbol cart",
             "atom_list:"]
    for sym, x, y, z in case.atoms:
        lines.append("%s %.6f %.6f %.6f" % (sym, x, y, z))
    return "\n".join(lines) + "\n"


def basis(case):
    """What TGBSConfig::dump("NWChem") writes: one library line per element."""
    lines = ['basis "ao basis" cartesian print']
    for el in sorted({a[0] for a in case.atoms}):
        lines.append('  %s library "%s"' % (el, case.basis[el]))
    lines.append("END")
    return "\n".join(lines) + "\n"


def generate(case, defaults):
    """Return (deck text, param text).  Raises RuntimeError if ai.nwchem fails."""
    work = tempfile.mkdtemp(prefix="ecce-teaching-gen-")
    try:
        name = "calc"
        for ext, text in (("frag", frag(case)), ("basis", basis(case)),
                          ("param", param(case, defaults))):
            with open(os.path.join(work, "%s.%s" % (name, ext)), "w") as h:
                h.write(text)
        #  ai.nwchem overwrites its -t file; never the repository's own.
        tpl = os.path.join(work, "nwch.nw.orig")
        shutil.copy(os.path.join(PARSERS, "nwch.tpl"), tpl)
        env = dict(os.environ, ECCE_HOME=REPO)
        proc = subprocess.run(
            ["perl", os.path.join(PARSERS, "ai.nwchem"), "-n", name, "-p", "-f",
             "-b", "-t", tpl], cwd=work, capture_output=True, text=True, env=env)
        if proc.returncode != 0:
            raise RuntimeError("ai.nwchem exited %d\n%s%s" % (
                proc.returncode, proc.stdout, proc.stderr))
        with open(tpl) as h:
            deck = h.read()
        with open(os.path.join(work, "calc.param")) as h:
            prm = h.read()
        return deck, prm, proc.stdout + proc.stderr
    finally:
        shutil.rmtree(work, ignore_errors=True)
