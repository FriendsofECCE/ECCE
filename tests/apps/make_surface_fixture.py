#!/usr/bin/env python3
"""Turn an ORCA output into a local calculation fixture (#892).

    make_surface_fixture.py ORCA_OUTPUT ORCA_INPUT BASIS OUTDIR

The real eccejobmonitor and parsers make the properties (as
tests/modiagram/makecalc.py does); the stored basis is BASIS (e.g.
def2-tzvpd) from ECCE's library, and the metadata is that of
fixtures/calc-orca-water-ccpvdz with the basis name and size replaced.
"""
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "modiagram"))
import makecalc                                 # noqa: E402
import pipeline                                 # noqa: E402
from eccejobmonitor_sim import read_desc        # noqa: E402

TEMPLATE = os.path.join(HERE, "fixtures", "calc-orca-water-ccpvdz")
KEEP = ("MO", "MOAOORDER", "ORBENG", "ORBOCC")


def basisFile(name, elements, blocks):
    """One group per element and no ECP, as the Calculation Editor stores
    a def2 set for light elements."""
    out = ["<MolecularConfig>\n<Map>\n"]
    for el in elements:
        n = blocks[el].count("contraction")
        flags = "false " * n
        out.append(
            '<Group Tag="%s" Name="%s">\n<OrderedList>\n'
            '<BasisSet Type="ECPOrbital">%s</BasisSet>\n</OrderedList>\n'
            '<OrbitalMapping>\n<OrbitalBasisSet Name="%s">\n'
            '<BasisSet Type="ECPOrbital">%s</BasisSet>\n</OrbitalBasisSet>\n'
            '</OrbitalMapping>\n<ContractionInfoMap>\n<Element tag="%s">\n'
            '<BasisSet uniqueKey="%s:ECPOrbital">\n<DeleteFlags>%s'
            '</DeleteFlags>\n<UncontractFlags>\n%s</UncontractFlags>\n'
            '</BasisSet>\n</Element>\n</ContractionInfoMap>\n'
            '<DiffuseMap>\n<Element tag="%s">\n%s</Element>\n</DiffuseMap>\n'
            '<TightMap>\n<Element tag="%s">\n%s</Element>\n</TightMap>\n'
            '</Group>\n' % (el, name, name, name, name, el, name, flags,
                             flags, el, "0 " * n, el, "0 " * n))
    out.append("</Map>\n<Data>\nBasisSet=%s\nType=ECPOrbital "
               "Category=ORBITAL Spherical=UnknownCoordSys "
               "ContType=UnknownContType\n" % name)
    out.extend("element=%s\n%s" % (el, blocks[el]) for el in elements)
    out.append("EndBasisSet\n</Data>\n</MolecularConfig>\n")
    return "".join(out)


def main():
    out, deck, basis, dest = sys.argv[1:5]
    os.environ["ECCE_HOME"] = ROOT
    atoms = []
    text = open(deck).read()
    for line in text.split("* xyz")[1].splitlines()[1:]:
        if line.strip() == "*":
            break
        f = line.split()
        atoms.append((f[0], float(f[1]), float(f[2]), float(f[3])))
    desc = os.path.join(makecalc.PARSERS, "orca.desc")
    work = tempfile.mkdtemp(prefix="surface-fixture-")
    try:
        shutil.copy(out, os.path.join(work, "calc.orcaout"))
        results = pipeline.run_monitor(os.path.join(work, "calc.orcaout"),
                                       desc, work)
        props = pipeline.run_parsers(
            pipeline.unpack(results), read_desc(desc),
            (".", "Energy", "SCF", "RHF", "0"), workdir=work,
            files=pipeline.unpack_files(results))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    if os.path.exists(dest):
        shutil.rmtree(dest)
    shutil.copytree(TEMPLATE, dest)
    for name in os.listdir(os.path.join(dest, "Props")):
        if name != ".ecce-meta":
            os.remove(os.path.join(dest, "Props", name))
    for key in KEEP:
        open(os.path.join(dest, "Props", key), "w").write(
            makecalc.propXml(key, props[key], len(atoms)))
    meta = os.path.join(dest, "Props", ".ecce-meta")
    open(meta, "w").write("".join(
        "%s\thttp://www.emsl.pnl.gov/ecce:mime-type\t\ttext/xml\n" % k
        for k in KEEP))
    elements = []
    for a in atoms:
        if a[0] not in elements:
            elements.append(a[0])
    lib = open(os.path.join(ROOT, "data", "admin", "basissets",
                            basis + ".BAS")).read()
    blocks = dict((c.split("\n", 1)[0].strip(), c.split("\n", 1)[1])
                  for c in lib.split("atom=")[1:])
    stored = basisFile(basis, elements, blocks)
    params = os.path.join(dest, "Parameters")
    open(os.path.join(params, "BasisSet.ecce_basisset"), "w").write(stored)
    open(os.path.join(params, "chemsys.mvm"), "w").write(
        makecalc.chemsys(atoms))
    nfun = int(re.search(r"Number of basis functions\s+\.+\s+(\d+)",
                         open(out).read())[1])
    pmeta = open(os.path.join(params, ".ecce-meta")).read()
    pmeta = re.sub(r"(ecce:name\t\t).*", r"\g<1>" + basis, pmeta)
    pmeta = re.sub(r"(ecce:numFunctions\t\t).*", r"\g<1>%d" % nfun, pmeta)
    formula = "".join("%s%s" % (e, "" if n == 1 else n) for e, n in
                      ((e, sum(1 for a in atoms if a[0] == e))
                       for e in sorted(elements)))
    pmeta = re.sub(r"(ecce:empiricalFormula\t\t).*", r"\g<1>" + formula, pmeta)
    pmeta = re.sub(r"(ecce:numAtoms\t\t).*", r"\g<1>%d" % len(atoms), pmeta)
    z = {"H": 1, "C": 6, "N": 7, "O": 8, "S": 16}
    pmeta = re.sub(r"(ecce:numElectrons\t\t).*",
                   r"\g<1>%d" % sum(z[a[0]] for a in atoms), pmeta)
    open(os.path.join(params, ".ecce-meta"), "w").write(pmeta)
    os.makedirs(os.path.join(dest, "Inputs"), exist_ok=True)
    shutil.copy(deck, os.path.join(dest, "Inputs", "calc.orcain"))
    print("%s: %d atoms, %d functions" % (dest, len(atoms), nfun))


if __name__ == "__main__":
    main()
