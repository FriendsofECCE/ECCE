#!/usr/bin/env python3
"""Regenerate the checked-in NWChem H2/O2/CO decks and diff against them.

tests/e2e's own cases run a stored .nw fixture through the real code and
the real monitor/parser pipeline -- see cases.py's own docstring for why
a stored deck, not a captured run, is the right anchor for THAT half.  But
a stored deck says nothing about whether ECCE's own generator (ai.nwchem)
still produces it: the deck could go stale relative to the generator
without anything here noticing.

This is the other half, for exactly three cases (H2, O2 triplet, CO) that
matter for the "build a diatomic, look at its MOs" class exercise: the
input files a student's default clicks would hand ai.nwchem --
<name>.frag/.param/.basis, captured once under fixtures/nwchem/gen-inputs/
-- are re-run through the real, installed ai.nwchem, and the result must
be byte-identical to fixtures/nwchem/<name>.nw.  A generator regression
(a changed default, a broken template resolver, a translation table edit)
fails HERE, on the generator alone, rather than showing up as a confusing
downstream property mismatch in run_tests.py.

Usage:
    tests/e2e/check_nwchem_generation.py            check all three
    tests/e2e/check_nwchem_generation.py --update   regenerate the fixtures
"""

import argparse
import difflib
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
FIXTURES = os.path.join(HERE, "fixtures", "nwchem")
GEN_INPUTS = os.path.join(FIXTURES, "gen-inputs")

#  Category/Theory follow NWChem.edml's own listed order (SCF is the first
#  category, RHF its first theory, Energy the first runtype) -- what a new
#  NWChem calculation defaults to.  O2 is the one exception: RHF is closed-
#  shell only and NWChem refuses multiplicity 3 under it, so the sane
#  choice for a triplet -- the one nedtheory.py actually offers under SCF
#  for an open shell -- is UHF, which is what CalcEd would need selected.
#  "co-opt" is a Geometry runtype (the others are all Energy) -- it
#  regenerates the trailing "task scf optimize / scf / task scf
#  gradient" that reprints the converged-geometry orbitals, so a
#  generator regression there fails here rather than only downstream in
#  run_tests.py's nwchem-co-opt-mos case.
CASES = ("h2", "o2", "co", "co-opt")


def regenerate(name, workdir):
    """Run the real ai.nwchem over this case's stored .frag/.param/.basis.

    Mirrors what CalcEd::generateInput() does (ESInputController.C):
    ai.nwchem -n <name> -p -f -b -t <template copy>.  The template is
    copied first -- ai.nwchem's cleanup() overwrites -t's target in
    place, and pointing it at the repo's own scripts/parsers/nwch.tpl
    would clobber that file (a documented gotcha in CLAUDE.md).
    """
    src = os.path.join(GEN_INPUTS, name)
    for ext in ("frag", "param", "basis"):
        shutil.copy(os.path.join(src, "%s.%s" % (name, ext)),
                    os.path.join(workdir, "%s.%s" % (name, ext)))
    tpl = os.path.join(workdir, "%s.nw.orig" % name)
    shutil.copy(os.path.join(PARSERS, "nwch.tpl"), tpl)

    env = dict(os.environ)
    env["ECCE_HOME"] = ROOT
    proc = subprocess.run(
        ["perl", os.path.join(PARSERS, "ai.nwchem"),
         "-n", name, "-p", "-f", "-b", "-t", tpl],
        cwd=workdir, capture_output=True, text=True, env=env)
    if proc.returncode != 0:
        raise RuntimeError("ai.nwchem exited %d\n%s\n%s"
                           % (proc.returncode, proc.stdout, proc.stderr))
    with open(tpl) as handle:
        return handle.read()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--update", action="store_true")
    args = ap.parse_args()

    failed = False
    for name in CASES:
        workdir = tempfile.mkdtemp(prefix="ecce-nwchem-gen-")
        try:
            generated = regenerate(name, workdir)
        except Exception as exc:
            print("%-4s FAIL  ai.nwchem did not run: %s" % (name, exc))
            failed = True
            continue
        finally:
            shutil.rmtree(workdir, ignore_errors=True)

        goldenPath = os.path.join(FIXTURES, "%s.nw" % name)
        if args.update:
            with open(goldenPath, "w") as handle:
                handle.write(generated)
            print("%-4s updated" % name)
            continue

        golden = open(goldenPath).read()
        if generated == golden:
            print("%-4s pass" % name)
        else:
            failed = True
            print("%-4s FAIL  ai.nwchem no longer reproduces %s.nw:"
                  % (name, name))
            diff = difflib.unified_diff(
                golden.splitlines(True), generated.splitlines(True),
                fromfile="fixtures/nwchem/%s.nw (checked in)" % name,
                tofile="ai.nwchem (just now)")
            sys.stdout.writelines(diff)

    print("FAILED" if failed else "PASSED")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
