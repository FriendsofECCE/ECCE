#!/usr/bin/env python3
"""Write the ORCA decks of the H2 potential-curve examples (#858).

    tools/examples/make_h2_scans.py OUTDIR [--orca PATH]

Each deck is made by the real ai.orca from what the Calculation Editor stores
(.param, .frag, .basis useRouteCard, the Builder's bond-scan constraint .con),
the same files CalcEd hands it, with output printing cut down so the stored
examples stay small (see SLIM).  OUTDIR/<name>/ gets orca.orcain and a Slurm
script run.sh (one task; ORCA at --orca) that runs it in that folder.  Copy
OUTDIR/<name>/orca.orcaout back into the example folder afterwards.
"""
import argparse
import os
import shutil
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")

# name: theory, first H-H distance, steps, step (Angstrom), broken-symmetry guess
SCANS = {
    "h2_ccsd_curve": ("CCSD", 5.0, 92, -0.05, False),
    "h2_rhf_scan": ("RHF", 4.0, 37, -0.1, False),
    "h2_uhf_scan": ("UHF", 4.0, 37, -0.1, True),
    "h2_ccsd_scan": ("CCSD", 4.0, 37, -0.1, False),
    "h2_uhf_scan_out": ("UHF", 0.5, 35, 0.1, False),
}
# ORCA prints MOs, populations and settings at every scan point; none of it is
# needed for a scan, and it made up most of the 26 MB of the first outputs.
# MiniPrint keeps everything the scan parsers read (energies, geometries,
# <S**2>, gradients, the scan table); the atomic charges go, zero for H2.
SLIM = (("Opt\n", "Opt MiniPrint\n"), ("Print[P_MOs] 1", "Print[P_MOs] 0"))
CATEGORY = {"RHF": "SCF", "UHF": "SCF", "CCSD": "CC"}
SLURM = """#!/bin/bash
#SBATCH -n 1
#SBATCH -J {name}
#SBATCH -t 02:00:00
cd {work} || exit 1
export PATH={orcadir}:$PATH LD_LIBRARY_PATH={orcadir}/lib:${{LD_LIBRARY_PATH:-}}
{orca} orca.orcain > orca.orcaout 2>&1
"""


def deck(work, theory, r0, steps, step, bs):
    os.makedirs(work, exist_ok=True)
    open(os.path.join(work, "h.frag"), "w").write(
        "h2\ntitle:\th2\ntype:\tmolecule\nnum_atoms:\t2\natom_info:\tsymbol cart\n"
        "atom_list:\n H 0.0 0.0 0.0\n H 0.0 0.0 %.4f\n" % r0)
    open(os.path.join(work, "h.basis"), "w").write("useRouteCard cc-pVTZ\n")
    open(os.path.join(work, "h.con"), "w").write(
        '<GeomConstraintModel>\n<Constraint type="bond">\n  <Group>0 1</Group>\n'
        '  <Value>%.4f</Value>\n  <Scan steps="%d" step="%g"/>\n</Constraint>\n'
        '</GeomConstraintModel>\n' % (r0, steps, step))
    param = ("Category:\t%s\nTheory:\t%s\nRunType:\tGeometry\nCharge:\t0\n"
             "ChemSys.Multiplicity:\t1\n" % (CATEGORY[theory], theory))
    if bs:
        param += "ES.Theory.SCF.BrokenSymmetryGuess:\t1\n"
    open(os.path.join(work, "h.param"), "w").write(param)
    out = os.path.join(work, "orca.orcain")
    shutil.copy(os.path.join(PARSERS, "orca.tpl"), out)
    subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-p", "-f", "-b", "-c",
                    "-n", "h", "-t", "orca.orcain"], cwd=work, check=True,
                   env=dict(os.environ, ECCE_HOME=REPO), capture_output=True)
    text = open(out).read()
    for old, new in SLIM:
        assert old in text, old
        text = text.replace(old, new, 1)
    open(out, "w").write(text)
    for f in ("h.frag", "h.basis", "h.con", "h.param"):
        os.unlink(os.path.join(work, f))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("outdir")
    ap.add_argument("--orca", default="/home/andy/orca_6_1_1/orca")
    a = ap.parse_args()
    for name, (theory, r0, steps, step, bs) in SCANS.items():
        work = os.path.join(a.outdir, name)
        deck(work, theory, r0, steps, step, bs)
        open(os.path.join(work, "run.sh"), "w").write(SLURM.format(
            name=name, work=os.path.abspath(work), orca=a.orca, orcadir=os.path.dirname(a.orca)))
        os.chmod(os.path.join(work, "run.sh"), 0o755)
        print("%s: %s %.2f -> %.2f A, %d points" % (name, theory, r0, r0 + steps * step,
                                                   steps + 1))


if __name__ == "__main__":
    main()
