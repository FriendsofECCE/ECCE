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
    "ecceqm": ["Category: DFT", "Theory: RDFT", "RunType: Geometry",
               "Charge: 0", "ChemSys.Multiplicity: 1"],
    "mopac": ["Category: SE", "Theory: RPM7", "RunType: Geometry",
              "Charge: 0", "ChemSys.Multiplicity: 1"],
    "orca": ["Category: SCF", "Theory: RHF", "RunType: Geometry",
             "Charge: 0", "ChemSys.Multiplicity: 1"],
    "nwchem": ["Category: SCF", "Theory: RHF", "RunType: Geometry",
               "Charge: 0", "ChemSys.Multiplicity: 1"],
    "gauss16": ["Category: SCF", "Theory: RHF", "RunType: Geometry",
                "Charge: 0", "ChemSys.Multiplicity: 1"],
}
TPL = {"ecceqm": "ecceqm.tpl", "nwchem": "nwch.tpl", "gauss16": "g16.tpl", "orca": "orca.tpl",
       "mopac": "mopac.tpl"}


def deck(code, frag=None, con=None, runtype=None):
    work = tempfile.mkdtemp(prefix="ecce-dummy-")
    try:
        shutil.copy(os.path.join(PARSERS, TPL[code]), os.path.join(work, "t.tpl"))
        for ext, txt in (("frag", frag or FRAG), ("basis", BASIS.get(code, "")),
                         ("con", con or CON), ("param", "\n".join(l if not (runtype and l.startswith("RunType:")) else "RunType: " + runtype
                                  for l in PARAMS[code]) + "\n")):
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


def mopac(d):
    lines = d.splitlines()
    return (check("mopac: X written as XX", any(first(l) == "XX" for l in lines))
            + check("mopac: no bare X", not any(first(l) == "X" for l in lines)))


def special():
    """Cases that need their own geometry or expect a failure."""
    f = 0
    # X first: ECCE atoms are X=1 N=2 H=3,4,5.  Gaussian numbers Cartesian
    # centres without the dummy, so the H-N-H angle 3-2-4 must become 2-1-3.
    xfirst = ("# frag\nnum_atoms: 5\ntitle: t\natom_info: symbol\natom_list:\n"
              "X 0 0 1.1\nN 0 0 0.1\nH 0 0.94 -0.3\nH 0.81 -0.47 -0.3\n"
              "H -0.81 -0.47 -0.3\n")
    con = ('<GeomConstraintModel>\n<Constraint type="angle">\n<Value>107.0</Value>\n'
           '<Group>2 1 3</Group>\n</Constraint>\n</GeomConstraintModel>\n')
    d = deck("gauss16", xfirst, con)
    f += check("gauss16: constraint numbers skip a leading dummy",
               any(l.split()[:4] == ["A", "2", "1", "3"] for l in d.splitlines()))
    d = deck("orca", xfirst, con)
    f += check("orca: constraint indices count a leading dummy",
               "{ A 2 1 3 107.0 C }" in d)
    # ECCE-QM counts the dummy too, 1-based; GeoVib would report frequencies
    # at the constrained geometry and is refused.
    try:
        deck("ecceqm", runtype="GeoVib")
        f += check("ecceqm: GeoVib with constraints refused", False)
    except RuntimeError as e:
        f += check("ecceqm: GeoVib with constraints refused", "not a minimum" in str(e))
    f += check("ecceqm: constraints only for Geometry runs",
               "constraints" not in deck("ecceqm", runtype="Energy"))
    f += nwchem_mixing()
    return f


def nwchem_mixing():
    """#306: zcoord (bond/angle/torsion) needs autoz and is ignored under noautoz;
    fixed atoms need noautoz.  Real NWChem 7.2.3: angle alone, or with a spring
    restraint, holds; a fixed atom plus an angle cannot be combined."""
    hdr = "<GeomConstraintModel>\n%s</GeomConstraintModel>\n"
    fixed = '<Constraint type="fixed">\n<Group>0</Group>\n</Constraint>\n'
    angle = '<Constraint type="angle">\n<Value>113.0</Value>\n<Group>4 0 1</Group>\n</Constraint>\n'
    spring = ('<Restraint type="bond">\n<Value>1.05</Value>\n<ForceConstant>0.5</ForceConstant>\n'
              '<Group>0 1</Group>\n</Restraint>\n')
    geo = lambda d: [l for l in d.splitlines() if l.startswith("geometry")][0]
    f = 0
    d = deck("nwchem", con=hdr % angle)
    f += check("nwchem: angle alone keeps autoz", "noautoz" not in geo(d) and "zcoord" in d)
    d = deck("nwchem", con=hdr % (spring + angle))
    f += check("nwchem: spring + angle keeps autoz", "noautoz" not in geo(d) and "spring bond" in d)
    d = deck("nwchem", con=hdr % fixed)
    f += check("nwchem: fixed atom alone uses noautoz", "noautoz" in geo(d))
    d = deck("nwchem", con=hdr % (fixed + angle))
    f += check("nwchem: fixed atom + angle is not silently noautoz", "noautoz" not in geo(d) and "zcoord" in d)
    return f


def ecceqm(d):
    lines = d.splitlines()
    geo = lines[lines.index("geometry") + 1:]
    return (check("ecceqm: X written in the geometry block", any(first(l) == "X" for l in geo))
            + check("ecceqm: constraints count X as atom 5 (1-based)",
                    "constraints" in lines and "  angle 5 1 2 113.0" in lines and "  angle 5 1 3 113.0" in lines)
            + check("ecceqm: no basis shell for X", not any(l.split()[:2] == ["shell", "X"] for l in lines)))


CODES = {"ecceqm": ecceqm, "mopac": mopac, "nwchem": nwchem, "gauss16": gauss16, "orca": orca}


def main():
    fails = 0
    for code, fn in CODES.items():
        d = deck(code)
        if "--show" in sys.argv:
            print(d)
        fails += fn(d)
    fails += special()
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
