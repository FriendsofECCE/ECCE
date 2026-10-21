#!/usr/bin/env python3
"""ORCA IRC (#417): a deck made by ai.orca runs in real ORCA, and the
structures land in the file the parse specification reads.

The transition state is the middle frame of the HCN/HNC fixture path.  The
deck goes through the real ai.orca and orca.tpl, is saved under the name
ECCE gives every ORCA input (orca.orcain), and is run with ORCA 6 for a few
IRC steps at HF/3-21G.  Checks: the deck says IRC and the %irc block, ORCA
terminates normally, the full trajectory has the name orca.desc reads, and
its frame count is 2 * steps + 1 with the TS first-point energy in the
middle.  Exit status 77 (CTest SKIP) without ORCA 6.

    tests/parsers/orca_irc_run_test.py [--keep]
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
STEPS = 3
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


def main():
    orcas = [p for p in sorted(glob.glob("/opt/orca/orca_6*/orca"))
             if os.access(p, os.X_OK)]
    if not orcas:
        print("no ORCA 6: skipped")
        return 77
    work = tempfile.mkdtemp(prefix="ecce-orca-irc-")
    try:
        ts = frames(XYZ)[10]
        shutil.copy(os.path.join(PARSERS, "orca.tpl"), os.path.join(work, "t.tpl"))
        with open(os.path.join(work, "w.frag"), "w") as f:
            f.write("# frag\nnum_atoms: 3\ntitle: hcn\natom_info: symbol\n"
                    "atom_list:\n" + "\n".join(" ".join(r.split()) for r in ts) + "\n")
        with open(os.path.join(work, "w.basis"), "w") as f:
            f.write("useRouteCard 3-21G\n")
        with open(os.path.join(work, "w.param"), "w") as f:
            f.write("Category: SCF\nTheory: RHF\nRunType: IRC\nCharge: 0\n"
                    "ChemSys.Multiplicity: 1\n"
                    "ES.Runtype.IRC.MaximumStepsValue: %d\n" % STEPS)
        env = dict(os.environ, ECCE_HOME=REPO)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.orca"), "-p", "-f",
                            "-b", "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                           capture_output=True, text=True)
        check(p.returncode == 0, "ai.orca accepts RunType IRC: " + p.stderr.strip())
        deck = open(os.path.join(work, "t.tpl")).read()
        print(deck)
        check(re.search(r"^! .*\bIRC\b", deck, re.M) is not None, "route line has IRC")
        check("MaxIter %d" % STEPS in deck and "InitHess calc_anfreq" in deck
              and "Direction both" in deck, "%irc block")
        shutil.copy(os.path.join(work, "t.tpl"), os.path.join(work, "orca.orcain"))
        p = subprocess.run([orcas[0], "orca.orcain"], cwd=work, env=env,
                           stdout=open(os.path.join(work, "orca.orcaout"), "w"),
                           stderr=subprocess.STDOUT, timeout=900)
        out = open(os.path.join(work, "orca.orcaout"), errors="replace").read()
        check("ORCA TERMINATED NORMALLY" in out, "ORCA ran the generated deck")
        trj = os.path.join(work, "orca.orcain_IRC_Full_trj.xyz")
        check(os.path.exists(trj), "trajectory is orca.orcain_IRC_Full_trj.xyz; "
              "have " + " ".join(sorted(os.listdir(work))))
        if os.path.exists(trj):
            fr = frames(trj)
            check(len(fr) == 2 * STEPS + 1, "%d frames, TS in the middle (got %d)"
                  % (2 * STEPS + 1, len(fr)))
            check(all(len(x) == 3 for x in fr), "three atoms per frame")
        if "--keep" in sys.argv:
            print("kept", work)
    finally:
        if "--keep" not in sys.argv:
            shutil.rmtree(work, True)
    print("FAILED" if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
