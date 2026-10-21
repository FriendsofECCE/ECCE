#!/usr/bin/env python3
"""ORCA NEB-TS (#419): a deck made by ai.orca runs in real ORCA and finds the
HCN -> HNC transition state.

The end points are the first and last frames of the HCN/HNC fixture path.  The
deck goes through the real ai.orca and orca.tpl with the product written next
to it as neb_product.xyz (what CalcEd stages), is saved as orca.orcain and run
with ORCA 6 at HF/3-21G with four images.  Checks: the route line and %neb
block, both run types' routes, a missing product refused, ORCA terminates
normally, and the frequencies of the "+ Frequencies" run hold exactly one
imaginary mode.  Exit status 77 (CTest SKIP) without ORCA 6.

    tests/parsers/orca_neb_run_test.py [--keep]
"""

import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(REPO, "scripts", "parsers")
XYZ = os.path.join(HERE, "fixtures", "orca", "hcn_irc_IRC_Full_trj.xyz")
failures = []


def check(ok, what):
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def frames(path):
    lines = open(path).read().splitlines()
    out = []
    i = 0
    while i < len(lines) and lines[i].strip():
        n = int(lines[i])
        out.append(lines[i + 2:i + 2 + n])
        i += n + 2
    return out


def deck(work, runtype, product=True):
    shutil.copy(os.path.join(PARSERS, "orca.tpl"), os.path.join(work, "t.tpl"))
    start = frames(XYZ)[0]
    with open(os.path.join(work, "w.frag"), "w") as f:
        f.write("# frag\nnum_atoms: 3\ntitle: hcn\natom_info: symbol\n"
                "atom_list:\n" + "\n".join(" ".join(r.split()) for r in start) + "\n")
    with open(os.path.join(work, "w.basis"), "w") as f:
        f.write("useRouteCard 3-21G\n")
    with open(os.path.join(work, "w.param"), "w") as f:
        f.write("Category: SCF\nTheory: RHF\nRunType: %s\nCharge: 0\n"
                "ChemSys.Multiplicity: 1\nES.Runtype.NEB.Images: 4\n"
                "ES.Theory.UseSymmetry: 1\n" % runtype)
    if product:
        with open(os.path.join(work, "neb_product.xyz"), "w") as f:
            f.write("3\nproduct\n" + "\n".join(frames(XYZ)[-1]) + "\n")
    env = dict(os.environ, ECCE_HOME=REPO)
    return subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-p", "-f",
                           "-b", "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                          capture_output=True, text=True)


def main():
    orcas = [p for p in sorted(glob.glob("/opt/orca/orca_6*/orca"))
             if os.access(p, os.X_OK)]
    if not orcas:
        print("no ORCA 6: skipped")
        return 77
    work = tempfile.mkdtemp(prefix="ecce-orca-neb-")
    try:
        p = deck(work, "Transition State (NEB-TS)", product=False)
        check(p.returncode != 0 and "product" in p.stderr,
              "no product picked is refused: " + p.stderr.strip())
        p = deck(work, "Transition State (NEB-TS)")
        check(p.returncode == 0, "ai.orca accepts NEB-TS: " + p.stderr.strip())
        text = open(os.path.join(work, "t.tpl")).read()
        check(re.search(r"^! .*\bNEB-TS\b(?! Freq)", text, re.M) is not None,
              "NEB-TS route has no Freq")
        p = deck(work, "Transition State (NEB-TS) + Frequencies")
        check(p.returncode == 0, "ai.orca accepts NEB-TS + Frequencies: " + p.stderr.strip())
        text = open(os.path.join(work, "t.tpl")).read()
        print(text)
        check(re.search(r"^! .*\bNEB-TS Freq\b", text, re.M) is not None,
              "route line has NEB-TS Freq")
        check("UseSym" not in text, "UseSym dropped for NEB-TS (ORCA aborts with it)")
        check('NEB_End_XYZFile "neb_product.xyz"' in text and "Nimages 4" in text
              and "NEB_TS_XYZFile" not in text, "%neb block without a guess")
        shutil.copy(os.path.join(work, "t.tpl"), os.path.join(work, "orca.orcain"))
        env = dict(os.environ, ECCE_HOME=REPO)
        subprocess.run([orcas[0], "orca.orcain"], cwd=work, env=env,
                       stdout=open(os.path.join(work, "orca.orcaout"), "w"),
                       stderr=subprocess.STDOUT, timeout=900)
        out = open(os.path.join(work, "orca.orcaout"), errors="replace").read()
        check("ORCA TERMINATED NORMALLY" in out, "ORCA ran the generated deck")
        check("THE NEB OPTIMIZATION HAS CONVERGED" in out, "the NEB converged")
        block = out.split("VIBRATIONAL FREQUENCIES")[-1].split("NORMAL MODES")[0]
        imag = re.findall(r"^\s*\d+:\s+(-\d+\.\d+) cm\*\*-1", block, re.M)
        check(len(imag) == 1, "exactly one imaginary frequency: %s" % imag)
        if "--keep" in sys.argv:
            print("kept", work)
    finally:
        if "--keep" not in sys.argv:
            shutil.rmtree(work, True)
    print("FAILED" if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
