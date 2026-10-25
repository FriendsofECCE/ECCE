#!/usr/bin/env python3
"""Build an MO-diagram fixture from a REAL run of a code (#132).

    makecalc.py CODE MOLECULE NOISE OUTDIR [--opt] [--seed N] [--keep]

CODE is g16, orca, nwchem or ecceqm; MOLECULE one of h2o, nh3, ch4, co2, c2h4, c3h6, ch3oh,
c2h6 (staggered ethane), c2, he2, n2, co, hf (the last five are Teaching > Diatomics, bond along x).
ecceqm runs the built-in engine (B3LYP/6-31G*), which keeps the input
frame; --no-rotate leaves the molecule as the library has it.  NOISE
(Angstrom) perturbs every coordinate by up to that much after a random
rotation, which is what a structure drawn in the Builder and never
symmetrised looks like: off the symmetric geometry and in no standard
orientation.  0 gives the exact geometry, still rotated.

The deck is written the way ai.gauss16 / ai.orca / ai.nwchem write it for
a molecule whose point group is C1 with "Use symmetry" ticked: Gaussian
gets no Symmetry keyword (it detects the group itself), ORCA gets
UseSym, NWChem "autosym 0.01".  HF/def2-SVP throughout.

Then the real code runs, the real scripts/eccejobmonitor matches its
output and the real scripts/parsers turn the blocks into properties
(tests/e2e/pipeline.py), and they are written into OUTDIR/Props in the
XML the data server stores.  Parameters/chemsys.mvm is the PERTURBED
INPUT geometry, as the Builder stored it; the code's own geometry
reaches the diagram only through GEOMTRACE, as in a real calculation.
Parameters/BasisSet.ecce_basisset is def2-SVP from ECCE's basis library
and the DAV metadata comes from the existing fixture for the same code.
"""
import argparse
import math
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.append(os.path.join(ROOT, "tests", "e2e"))
sys.path.append(os.path.join(ROOT, "tests", "parsers"))
import pipeline                              # noqa: E402
from eccejobmonitor_sim import read_desc     # noqa: E402

PARSERS = os.path.join(ROOT, "scripts", "parsers")

_A = 1.087 / math.sqrt(3.0)
MOLECULES = {
    "h2o": [("O", 0.0, 0.0, 0.1173), ("H", 0.0, 0.7572, -0.4692),
            ("H", 0.0, -0.7572, -0.4692)],
    "nh3": [("N", 0.0, 0.0, 0.1162)] +
           [("H", 0.9377 * math.cos(t), 0.9377 * math.sin(t), -0.2711)
            for t in (0.0, 2 * math.pi / 3, 4 * math.pi / 3)],
    "ch4": [("C", 0.0, 0.0, 0.0), ("H", _A, _A, _A), ("H", -_A, -_A, _A),
            ("H", -_A, _A, -_A), ("H", _A, -_A, -_A)],
    "co2": [("C", 0.0, 0.0, 0.0), ("O", 0.0, 0.0, 1.16),
            ("O", 0.0, 0.0, -1.16)],
    #  Ethene, D2h, C=C along x, molecule in the xy plane.
    "c2h4": [("C", 0.6695, 0.0, 0.0), ("C", -0.6695, 0.0, 0.0),
             ("H", 1.2321, 0.9289, 0.0), ("H", 1.2321, -0.9289, 0.0),
             ("H", -1.2321, 0.9289, 0.0), ("H", -1.2321, -0.9289, 0.0)],
    #  Methanol, Cs (C-O-H in the xz plane), staggered, experimental
    #  geometry: a carbon whose neighbours are not all alike and an atom
    #  (the hydroxyl H) bonded to none of the methyl's atoms.
    "ch3oh": [("C", 0.0, 0.0, 0.0), ("O", 0.0, 0.0, 1.4246),
              ("H", 0.8962, 0.0, 1.7245),
              ("H", -1.0451, 0.0, -0.3232),
              ("H", 0.5086, 0.8809, -0.4012),
              ("H", 0.5086, -0.8809, -0.4012)],
}

#  Cyclopropane, D3h: the smallest ring, for a bond whose cut does not
#  split the molecule.
_R = 0.8700
for _k in range(3):
    _t = math.pi / 2 + 2 * math.pi * _k / 3
    _c = (_R * math.cos(_t), _R * math.sin(_t))
    MOLECULES.setdefault("c3h6", []).append(("C", _c[0], _c[1], 0.0))
for _k in range(3):
    _t = math.pi / 2 + 2 * math.pi * _k / 3
    _c = (_R * math.cos(_t), _R * math.sin(_t))
    _o = (math.cos(_t), math.sin(_t))
    _r, _z = 1.083 * math.cos(math.radians(57.5)), 1.083 * math.sin(math.radians(57.5))
    for _sign in (1, -1):
        MOLECULES["c3h6"].append(
            ("H", _c[0] + _r * _o[0], _c[1] + _r * _o[1], _sign * _z))


#  Ethane, staggered, D3d, C-C along z (r(CC) 1.536, r(CH) 1.094, HCC 111.2).
_ZC, _ZH, _RH = 0.768, 0.768 + 1.094 * math.cos(math.radians(68.8)), \
    1.094 * math.sin(math.radians(68.8))
MOLECULES["c2h6"] = [("C", 0.0, 0.0, _ZC), ("C", 0.0, 0.0, -_ZC)]
for _k in range(3):
    _t = 2 * math.pi * _k / 3
    MOLECULES["c2h6"].append(("H", _RH * math.cos(_t), _RH * math.sin(_t), _ZH))
for _k in range(3):
    _t = math.pi / 3 + 2 * math.pi * _k / 3
    MOLECULES["c2h6"].append(("H", _RH * math.cos(_t), _RH * math.sin(_t), -_ZH))

#  ecceqm method keyword ("hf" gives RHF for a closed shell).
METHOD = "b3lyp"
BASIS = "6-31G*"
MULTIPLICITY = 1

#  Single atoms (radial distributions,).
for _el in ("H", "C", "Ne", "Ar"):
    MOLECULES[_el.lower()] = [(_el, 0.0, 0.0, 0.0)]
MOLECULES["h2"] = [("H", 0.0, 0.0, -0.37), ("H", 0.0, 0.0, 0.37)]


def libraryDiatomics():
    """Teaching > Diatomics, read from the structure library itself."""
    folder = os.path.join(ROOT, "data", "client", "StructureLibrary",
                          "Teaching", "Diatomics")
    for name in os.listdir(folder):
        if not name.endswith(".mvm"):
            continue
        atoms, on = [], False
        for line in open(os.path.join(folder, name)):
            if line.startswith("atom_list"):
                on = True
            elif on and len(line.split()) == 4:
                el, *xyz = line.split()
                atoms.append((el, *[float(v) for v in xyz]))
            elif on:
                break
        MOLECULES[name[:-4].lower()] = atoms


libraryDiatomics()

#  Existing fixtures whose DAV metadata (code, owner, state) is reused.
TEMPLATE = {"g16": "gaussian-co", "orca": "orca-co", "nwchem": "nwchem-water",
            "ecceqm": "orca-co"}


def geometry(name, noise, seed, rotate=True):
    rnd = random.Random(seed)
    #  A random rotation (from a random unit quaternion).
    q = [rnd.gauss(0, 1) for _ in range(4)]
    n = math.sqrt(sum(x * x for x in q))
    w, x, y, z = (c / n for c in q)
    if not rotate:
        w, x, y, z = 1.0, 0.0, 0.0, 0.0
    R = [[1 - 2*(y*y + z*z), 2*(x*y - z*w), 2*(x*z + y*w)],
         [2*(x*y + z*w), 1 - 2*(x*x + z*z), 2*(y*z - x*w)],
         [2*(x*z - y*w), 2*(y*z + x*w), 1 - 2*(x*x + y*y)]]
    out = []
    for el, *p in MOLECULES[name]:
        r = [sum(R[i][k] * p[k] for k in range(3)) for i in range(3)]
        r = [c + rnd.uniform(-noise, noise) for c in r]
        out.append((el, r[0], r[1], r[2]))
    return out


def xyzLines(atoms):
    return "".join(" %-3s %12.6f %12.6f %12.6f\n" % a for a in atoms)


def deck(code, atoms, opt):
    if code == "g16":
        route = "#P rhf/def2SVP 6D 10F %sPunch=(MO) Pop=(full)" % (
            "Opt " if opt else "")
        return ("calc.com", "calc.log",
                "%%nprocshared=2\n%s\n\nMO diagram fixture\n\n0 1\n%s\n"
                % (route, xyzLines(atoms)))
    if code == "orca":
        return ("calc.orcain", "calc.orcaout",
                "! HF def2-SVP UseSym%s\n%%output\n  Print[P_MOs] 1\n"
                "  Print[P_OrbEn] 2\nend\n\n* xyz 0 1\n%s*\n"
                % (" Opt" if opt else "", xyzLines(atoms)))
    if code == "nwchem":
        return ("calc.nw", "ecce.out",
                'Title "fixture"\n\nStart calc\n\necho\n\ncharge 0\n\n'
                "geometry autosym 0.01 units angstrom\n%send\n\n"
                "ecce_print ecce.out\n\n"
                'basis "ao basis" spherical print\n%s'
                "END\n\nscf\n  RHF\n  nopen 0\nend\n\ntask scf %s\n"
                % (xyzLines(atoms),
                   "".join('  %s library "def2-svp"\n' % e
                           for e in sorted(set(a[0] for a in atoms))),
                   "optimize" if opt else "energy"))
    if code == "ecceqm":
        return ("ecceqm.qmin", "ecceqm.qmout",
                "title MO diagram fixture\ncharge 0\nmultiplicity %d\n"
                "method %s\nspherical true\nbasis %s\n"
                "geometry\n%send\n" % (MULTIPLICITY, METHOD, BASIS,
                                         xyzLines(atoms)))
    raise SystemExit("unknown code " + code)


def runCode(code, work, deckName, outName):
    env = dict(os.environ, OMPI_MCA_btl_sm_backing_directory=work)
    if code == "g16":
        env["GAUSS_SCRDIR"] = work
        cmd = ["g16", deckName]
        stdout = subprocess.DEVNULL
    elif code == "orca":
        exe = [os.path.join(d, "orca") for d in
               sorted(os.listdir("/opt/orca")) if d.startswith("orca_6_1_1")]
        cmd = [os.path.join("/opt/orca", os.path.dirname(exe[0]), "orca"),
               deckName]
        stdout = open(os.path.join(work, outName), "w")
    elif code == "ecceqm":
        cmd = [os.environ.get("ECCE_QM", "ecce-qm"), "-o", outName, deckName]
        stdout = subprocess.DEVNULL
    else:
        cmd = ["nwchem", deckName]
        stdout = open(os.path.join(work, "calc.nwout"), "w")
    rc = subprocess.call(cmd, cwd=work, env=env, stdout=stdout,
                         stderr=subprocess.DEVNULL)
    if rc != 0:
        raise SystemExit("%s failed (exit %d) in %s" % (code, rc, work))


def flat(rec, name):
    return pipeline.sect(rec, name, "")


def wrap(tokens, per=4):
    return "\n".join("   ".join(tokens[i:i + per])
                     for i in range(0, len(tokens), per))


HEAD = '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'


def propXml(key, recs, natoms):
    rec = recs[-1]
    vals = flat(rec, "values").split()
    units = flat(rec, "units") or "NA"
    if key == "GEOMTRACE":
        steps = []
        for r in recs:
            v = flat(r, "values").split()
            if len(v) != 3 * natoms:
                continue
            steps.append(v)
        body = "".join('\n  <step number="%d">%s</step>\n' % (i + 1, wrap(s))
                       for i, s in enumerate(steps))
        return (HEAD + '<tsvectable columnLabel="Atom" columnLabels="X Y Z" '
                'columns="3" name="GEOMTRACE" rowLabel="Geometry Step" '
                'rowLabels="%s" rows="%d" units="Angstrom" '
                'vectorLabel="Coordinate" vectors="1">\n%s\n</tsvectable>\n'
                % (" ".join(str(i) for i in range(natoms)), natoms, body))
    if key == "MOAOORDER":
        return HEAD + '<string name="MOAOORDER" units="NA">%s</string>' % vals[0]
    dims = flat(rec, "size").split()
    rows = dims[0] if dims else str(len(vals))
    labels = " ".join(str(i + 1) for i in range(int(rows)))
    if key.startswith("ORBSYM"):
        return (HEAD + '<vecstring name="%s" rowLabel="Basis Function" '
                'rowLabels="%s" rows="%s" units="NA">%s</vecstring>\n'
                % (key, labels, rows, wrap(vals)))
    if len(dims) == 2:
        cols = " ".join("BasFun-%d" % (i + 1) for i in range(int(dims[1])))
        return (HEAD + '<table columnLabel="Basis Function" columnLabels="%s" '
                'columns="%s" name="%s" rowLabel="Basis Function" '
                'rowLabels="%s" rows="%s" units="%s">%s</table>\n'
                % (cols, dims[1], key, labels, rows, units, wrap(vals)))
    return (HEAD + '<vector name="%s" rowLabel="Basis Function" rows="%s" '
            'units="%s">%s\n</vector>\n' % (key, rows, units, wrap(vals)))


KEEP = ("MO", "MOBETA", "ORBENG", "ORBENGBETA", "ORBOCC", "ORBOCCBETA",
        "ORBSYM", "ORBSYMBETA", "MOAOORDER", "GEOMTRACE")


def chemsys(atoms):
    return ("title: \ntype: molecule\nfragment_attributes:\n"
            "  point_group: C1\nend_fragment_attributes:\n"
            "num_atoms: %d\natom_info: symbol cart\natom_list: \n%s"
            "attr_list:\n%s" % (len(atoms),
                                "".join("%s %.6f %.6f %.6f\n" % a
                                        for a in atoms),
                                " 0.000000   1   0   0   0\n" * len(atoms)))


#  What the Calculation Editor stored for a Pople set before it
#  forced spherical functions on a spherical-only code: Cartesian ("N"),
#  while ECCE-QM always runs five d functions.  "Y" is the forced value.
COORDSYS = "N"


def basisFilePople(elements):
    """BasisSet.ecce_basisset for 6-31G* as the Calculation Editor stores it
    for ECCE-QM (tests/ecceqm/fixtures/o2-roks): the 6-31G contractions plus
    the polarisation shell from 6-31GS."""
    def blocks(name):
        out = {}
        folder = os.path.join(ROOT, "data", "admin", "basissets")
        found = sorted(f for f in os.listdir(folder)
                       if f.lower() == name.lower() + ".bas")
        text = open(os.path.join(folder, found[0])).read()
        for chunk in text.split("atom=")[1:]:
            el, body = chunk.split("\n", 1)
            out[el.strip()] = body
        return out
    if BASIS == "6-31G*":
        base, pol = blocks("6-31G"), blocks("6-31GS")
    else:   # the library file holds the whole set
        base, pol = blocks(BASIS), {}
    flags = "false " * 8
    out = ["<MolecularConfig>\n<Map>\n"]
    #  One Group per element, as the Calculation Editor writes a Pople set.
    for el in elements:
        out.append(
            '<Group Tag="%s" Name="6-31G*">\n<OrderedList>\n'
            '<BasisSet Type="pople">6-31G*</BasisSet>\n</OrderedList>\n'
            '<OrbitalMapping>\n<OrbitalBasisSet Name="6-31G*">\n'
            '<BasisSet Type="pople">6-31G*</BasisSet>\n'
            '</OrbitalBasisSet>\n</OrbitalMapping>\n<ContractionInfoMap>\n'
            '<Element tag="%s">\n<BasisSet uniqueKey="6-31G*:pople">\n'
            '<DeleteFlags>%s</DeleteFlags>\n<UncontractFlags>\n%s'
            '</UncontractFlags>\n</BasisSet>\n</Element>\n'
            '</ContractionInfoMap>\n<DiffuseMap>\n<Element tag="%s">\n%s'
            '</Element>\n</DiffuseMap>\n<TightMap>\n<Element tag="%s">\n%s'
            '</Element>\n</TightMap>\n</Group>\n'
            % (el, el, flags, flags, el, "0 " * 8, el, "0 " * 8))
    out.append("</Map>\n<Data>\nBasisSet=6-31G*\nType=pople "
               "Category=ORBITAL Spherical=UnknownCoordSys "
               "ContType=UnknownContType\n")
    for el in elements:
        out.append("element=%s\n%s%s" % (el, base[el], pol.get(el, "")))
    out.append("EndBasisSet\n</Data>\n</MolecularConfig>")
    return "".join(out).replace("6-31G*", BASIS)


def basisFile(elements):
    """BasisSet.ecce_basisset for def2-SVP on these elements, in the shape
    the Calculation Editor stores (orca-co's), with the contractions taken
    from ECCE's own basis library."""
    lib = open(os.path.join(ROOT, "data", "admin", "basissets",
                            "def2-svp.BAS")).read()
    blocks = {}
    for chunk in lib.split("atom=")[1:]:
        el, body = chunk.split("\n", 1)
        blocks[el.strip()] = body
    flags = "false " * 8
    tag = " ".join(elements)
    out = ['<MolecularConfig>\n<Map>\n<Group Tag="%s" Name="def2-svp">\n'
           '<OrderedList>\n<BasisSet Type="ECPOrbital">def2-svp</BasisSet>\n'
           '<ECP Type="ecp">Def2-ECP</ECP>\n</OrderedList>\n<OrbitalMapping>\n'
           '<OrbitalBasisSet Name="def2-svp">\n'
           '<BasisSet Type="ECPOrbital">def2-svp</BasisSet>\n'
           '<ECP Type="ecp">Def2-ECP</ECP>\n</OrbitalBasisSet>\n'
           '</OrbitalMapping>\n<ContractionInfoMap>\n' % tag]
    for el in elements:
        out.append('<Element tag="%s">\n<BasisSet uniqueKey="def2-svp:'
                   'ECPOrbital">\n<DeleteFlags>%s</DeleteFlags>\n'
                   '<UncontractFlags>\n%s</UncontractFlags>\n</BasisSet>\n'
                   '</Element>\n' % (el, flags, flags))
    out.append("</ContractionInfoMap>\n")
    for name in ("DiffuseMap", "TightMap"):
        out.append("<%s>\n" % name)
        out.extend('<Element tag="%s">\n%s</Element>\n' % (el, "0 " * 8)
                   for el in elements)
        out.append("</%s>\n" % name)
    out.append("</Group>\n</Map>\n<Data>\nBasisSet=Def2-ECP\nType=ecp "
               "Category=AUXILIARY Spherical=UnknownCoordSys "
               "ContType=UnknownContType\nEndBasisSet\nBasisSet=def2-svp\n"
               "Type=ECPOrbital Category=ORBITAL Spherical=UnknownCoordSys "
               "ContType=UnknownContType\n")
    for el in elements:
        out.append("element=%s\n%s" % (el, blocks[el]))
    out.append("EndBasisSet\n</Data>\n</MolecularConfig>\n")
    return "".join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("code", choices=sorted(TEMPLATE))
    ap.add_argument("molecule", choices=sorted(MOLECULES))
    ap.add_argument("noise", type=float)
    ap.add_argument("outdir")
    ap.add_argument("--opt", action="store_true")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--no-rotate", action="store_true")
    ap.add_argument("--spherical-flag", action="store_true",
                    help="record the basis as spherical (coordsys Y)")
    ap.add_argument("--method", default="b3lyp",
                    help="ecceqm only: b3lyp (default) or hf")
    ap.add_argument("--basis", default="6-31G*", help="ecceqm only")
    ap.add_argument("--multiplicity", type=int, default=1,
                    help="ecceqm only; above 1 with hf gives ROHF")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--save-output", help="copy the code's output here")
    a = ap.parse_args()
    #  The parser scripts find pertab.pl and friends through ECCE_HOME.
    os.environ["ECCE_HOME"] = ROOT

    global METHOD, BASIS, MULTIPLICITY
    METHOD = a.method
    BASIS = a.basis
    MULTIPLICITY = a.multiplicity
    atoms = geometry(a.molecule, a.noise, a.seed, not a.no_rotate)
    deckName, outName, text = deck(a.code, atoms, a.opt)
    work = tempfile.mkdtemp(prefix="modiagram-%s-" % a.code,
                            dir=os.environ.get("TMPDIR"))
    try:
        open(os.path.join(work, deckName), "w").write(text)
        runCode(a.code, work, deckName, outName)
        out = os.path.join(work, outName)
        if a.save_output:
            shutil.copy(out, a.save_output)
        descName = {"g16": "gaussian-16.desc", "orca": "orca.desc",
                    "nwchem": "nwchem.desc",
                    "ecceqm": "ecceqm.desc"}[a.code]
        descPath = os.path.join(PARSERS, descName)
        results = pipeline.run_monitor(out, descPath, work)
        blocks = pipeline.unpack(results)
        files = pipeline.unpack_files(results)
        props = pipeline.run_parsers(
            blocks, read_desc(descPath),
            (".", "Geometry" if a.opt else "Energy", "SCF", "RHF", "0"),
            workdir=work, files=files)

        template = os.path.join(HERE, "fixtures", TEMPLATE[a.code])
        if os.path.exists(a.outdir):
            shutil.rmtree(a.outdir)
        os.makedirs(os.path.join(a.outdir, "Props", ".DAV"))
        os.makedirs(os.path.join(a.outdir, "Parameters"))
        os.makedirs(os.path.join(a.outdir, "Inputs"))
        shutil.copytree(os.path.join(template, ".DAV"),
                        os.path.join(a.outdir, ".DAV"))
        shutil.copytree(os.path.join(template, "Parameters", ".DAV"),
                        os.path.join(a.outdir, "Parameters", ".DAV"))
        davProp = os.path.join(template, "Props", ".DAV", "ORBENG")
        for key in KEEP:
            if key in props:
                open(os.path.join(a.outdir, "Props", key), "w").write(
                    propXml(key, props[key], len(atoms)))
                shutil.copy(davProp,
                            os.path.join(a.outdir, "Props", ".DAV", key))
        open(os.path.join(a.outdir, "Parameters", "chemsys.mvm"), "w").write(
            chemsys(atoms))
        elements = []
        for atom in atoms:
            if atom[0] not in elements:
                elements.append(atom[0])
        open(os.path.join(a.outdir, "Parameters", "BasisSet.ecce_basisset"),
             "w").write(basisFilePople(elements) if a.code == "ecceqm"
                        else basisFile(elements))
        open(os.path.join(a.outdir, "Parameters", "SetupParams"), "w").write(
            "ES.Theory.UseSymmetry|1||1|1|toggle_input\n")
        open(os.path.join(a.outdir, "Inputs", deckName), "w").write(text)
        if a.code == "ecceqm":
            #  The template's stored metadata is ORCA's (code, basis
            #  coordinate system and size), a binary property database;
            #  capture.py patches these over WebDAV after installing.
            import json
            coordsys = "Y" if a.spherical_flag else COORDSYS
            basis = open(os.path.join(a.outdir, "Parameters",
                                      "BasisSet.ecce_basisset")).read()
            nfun = int(re.search(r"^nbf (\d+)", open(out).read(), re.M)[1])
            nprim = 0
            for el in (x[0] for x in atoms):
                body = basis.split("element=%s\n" % el)[1].split("element=")[0]
                nprim += sum(int(n) for n in re.findall(
                    r"num_primitives=(\d+)", body.split("EndBasisSet")[0]))
            json.dump({"": {"application": "ECCE-QM"},
                       "Parameters/BasisSet.ecce_basisset": {
                           "name": BASIS, "coordsys": coordsys,
                           "numFunctions": str(nfun),
                           "numPrimitives": str(nprim),
                           "ecpName": "", "gbs-type": "element"}},
                      open(os.path.join(a.outdir, ".davprops"), "w"), indent=1)
        pg = [flat(r, "values") for r in props.get("PNTGRP", [])]
        print("%s %s noise=%g: code's group %s; %s"
              % (a.code, a.molecule, a.noise, pg[-1] if pg else "-",
                 " ".join(k for k in KEEP if k in props)))
    finally:
        if a.keep:
            print("workdir kept: " + work)
        else:
            shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
