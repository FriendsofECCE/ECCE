#!/usr/bin/env python3
"""#298: a dummy atom X (NH3 + X on the C3 axis, with fixed X-N-H angles) must
reach every code's input in that code's own syntax, with no basis set on X and
the constraint atom numbers still counting X.  Generates decks with the real
scripts/parsers/ai.<code>; --run also runs the installed code (tiny job)."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")

# atom 1 = N, 2-4 = H, 5 = X on the axis above N.
FRAG = ("# frag\nnum_atoms: 5\ntitle: nh3x\natom_info: symbol\natom_list:\n"
        "N 0.0 0.0 0.1\nH 0.0 0.94 -0.3\nH 0.81 -0.47 -0.3\nH -0.81 -0.47 -0.3\n"
        "X 0.0 0.0 1.1\n")
# Fixed X-N-H angles; groups are 0-based, as the Builder writes them.
CON = ('<GeomConstraintModel>\n<Constraint type="angle">\n<Value>113.0</Value>\n'
       '<Group>4 0 1</Group>\n</Constraint>\n<Constraint type="angle">\n'
       '<Value>113.0</Value>\n<Group>4 0 2</Group>\n</Constraint>\n</GeomConstraintModel>\n')
# Basis files as ECCE hands them over: no entry for X.
BASIS = {
    "nwchem": 'basis "ao basis" cartesian print\nN library "3-21G"\n'
              'H library "3-21G"\nEND\n',
    "gauss16": "useRouteCard 6-31G*\n",
    "orca": "useRouteCard 3-21G\n",
}
PARAMS = {
    "orca": ["Category: SCF", "Theory: RHF", "RunType: Geometry",
             "Charge: 0", "ChemSys.Multiplicity: 1"],
    "nwchem": ["Category: SCF", "Theory: RHF", "RunType: Geometry",
               "Charge: 0", "ChemSys.Multiplicity: 1"],
    "gauss16": ["Category: SCF", "Theory: RHF", "RunType: Geometry",
                "Charge: 0", "ChemSys.Multiplicity: 1"],
}
TPL = {"nwchem": "nwch.tpl", "gauss16": "g16.tpl", "orca": "orca.tpl"}


def deck(code):
    work = tempfile.mkdtemp(prefix="ecce-dummy-")
    try:
        shutil.copy(os.path.join(PARSERS, TPL[code]), os.path.join(work, "t.tpl"))
        for ext, txt in (("frag", FRAG), ("basis", BASIS.get(code, "")),
                         ("con", CON), ("param", "\n".join(PARAMS[code]) + "\n")):
            with open(os.path.join(work, "w." + ext), "w") as f:
                f.write(txt)
        env = dict(os.environ, ECCE_HOME=ROOT)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai." + code), "-p", "-f",
                            "-b", "-c", "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                           capture_output=True, text=True)
        if p.returncode:
            raise RuntimeError(p.stderr)
        with open(os.path.join(work, "t.tpl")) as f:
            return f.read()
    finally:
        shutil.rmtree(work, ignore_errors=True)


def check(name, ok):
    print("%-4s %s" % ("ok" if ok else "FAIL", name))
    return 0 if ok else 1


def first(line):
    t = line.split()
    return t[0] if t else ""


def nwchem(d):
    lines = d.splitlines()
    return (check("nwchem: X in geometry", any(first(l) == "X" for l in lines))
            + check("nwchem: constraint counts X as atom 5",
                    "angle 5 1 2 113.0 constant" in d)
            + check("nwchem: no basis for X",
                    not any(first(l).upper() == "X" and "library" in l for l in lines)))


def gauss16(d):
    lines = d.splitlines()
    return (check("gauss16: X in geometry", any(first(l) == "X" for l in lines))
            + check("gauss16: constraint counts X as atom 5",
                    any(l.split()[:4] == ["A", "5", "1", "2"] for l in lines)))


def orca(d):
    lines = d.splitlines()
    return (check("orca: X written as DA", any(first(l) == "DA" for l in lines))
            + check("orca: no X left", not any(first(l) == "X" for l in lines))
            + check("orca: constraints use 0-based indices incl. DA",
                    "{ A 4 0 1 113.0 C }" in d and "{ A 4 0 2 113.0 C }" in d))


CODES = {"nwchem": nwchem, "gauss16": gauss16, "orca": orca}


def main():
    fails = 0
    for code, fn in CODES.items():
        d = deck(code)
        if "--show" in sys.argv:
            print(d)
        fails += fn(d)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
