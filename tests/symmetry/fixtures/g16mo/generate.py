#!/usr/bin/env python3
"""How the fixtures in this directory were produced -- provenance, not a
test-time step.  Run manually (needs a working `g16`, and ECCE's own
`autosym`/`symops`/parser scripts) to regenerate; not invoked by
run_tests.py, so a machine without Gaussian still runs the suite
against the checked-in fixture.

    tests/symmetry/fixtures/g16mo/generate.py <workdir>

For each molecule (ch4-td, ...):

  1. Runs a real HF/6-31G(d) single point in Gaussian 16 with
     `symmetry=loose pop=full Punch=(MO)` -- Punch=(MO) is what ECCE's
     own ai.gauss16 adds to get a fort.7 MO punch file (see
     scripts/parsers/gauss16.tpl); without it there is nothing for
     gaussian-16.mo to read live-monitoring a real job either.
  2. Runs ECCE's REAL scripts/parsers/gaussian-16.mo on that fort.7 --
     the same script eccejobmonitor invokes -- to get the orbital
     energies and MO coefficients in ECCE's own PropTable convention
     (rows = orbitals, columns = basis functions; verified against
     gaussian-16.mo's own printThem(): `size:\\n$norb $nBasisFun\\n`,
     row-major in $values, both confirmed against ComputeMoCmd's
     PropTable::rows()==numMO / ::columns()==nbas usage).
  3. Runs ECCE's REAL scripts/parsers/gaussian-16.orbocc on the
     "Orbital symmetries:" block of the log -- again the actual
     production parser, not a hand read of the text -- to get G16's
     own ORBSYM labels: the oracle this fixture is FOR.
  4. Runs ECCE's real `autosym` binary (build-cmake/autosym) on the
     geometry to get the point group and the reoriented ("probe")
     coordinates -- the same input/output contract
     tools/modiagram/frommopac.py's symmetrise() already reverse-
     engineered from SymmetryOps::find()'s SFile protocol.

The basis text is the STANDARD, published Pople 6-31G(d) primitive
exponents/contraction coefficients for C and H (also matches ECCE's own
data/admin/basissets/6-31G.BAS + 6-31GS.BAS's carbon d exponent, 0.8,
checked directly against that file) -- not hand-fit to this molecule,
so it is not something this fixture could get quietly wrong by
construction. Written as ICalcUtils::importConfig()-format
NumericalBasis text, the same real format tests/basis and
tests/slater's round-trip tests use.
"""
import os
import subprocess
import sys

G16_ROUTE = "#p HF/6-31G(d) sp symmetry=loose pop=full Punch=(MO)\n"


def runGaussian(workdir, name, title, atoms):
    gjf = os.path.join(workdir, name + ".gjf")
    with open(gjf, "w") as f:
        f.write("%%chk=%s.chk\n%%nprocshared=4\n" % name)
        f.write(G16_ROUTE)
        f.write("\n%s\n\n0 1\n" % title)
        for sym, x, y, z in atoms:
            f.write("%-4s %14.6f %14.6f %14.6f\n" % (sym, x, y, z))
        f.write("\n")
    env = dict(os.environ)
    env.update(g16root="/opt/gaussian",
               GAUSS_EXEDIR="/opt/gaussian/g16/bsd:/opt/gaussian/g16",
               GAUSS_ARCHDIR="/opt/gaussian/g16/arch",
               GAUSS_BSDDIR="/opt/gaussian/g16/bsd",
               GAUSS_SCRDIR=workdir)
    log = os.path.join(workdir, name + ".log")
    with open(gjf) as fin, open(log, "w") as fout:
        subprocess.run(["/opt/gaussian/g16/g16"], stdin=fin, stdout=fout,
                       stderr=subprocess.STDOUT, cwd=workdir, env=env,
                       check=True)
    return log, os.path.join(workdir, "fort.7")


if __name__ == "__main__":
    sys.stderr.write(__doc__)
    sys.exit(0)
