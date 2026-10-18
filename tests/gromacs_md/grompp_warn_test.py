#!/usr/bin/env python3
"""Harmless and stopping grompp warnings (scripts/parsers/GromppWarn.pm).

Real gmx on two small systems made from the water fixture:
  * the structure's atom names differ from the topology's: a harmless
    warning, so the launch check passes (and says so) and the job script
    runs grompp with -maxwarn 1;
  * a Na+ ion with PME: a net-charge warning that is not harmless, so the
    launch check stops with it marked STOPPING and the job script fails with
    grompp's own message.
The table itself is also run against canned warning text.

    tests/gromacs_md/grompp_warn_test.py [-v]

Exit status 77 (CTest SKIP) without gmx.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as rt  # noqa: E402
from run_tests import check, PARSERS, FIXTURES, run  # noqa: E402


def canned():
    """The table and its awk form agree on text grompp 2025 printed."""
    out = """WARNING 1 [file a.mdp]:
  The Berendsen thermostat does not generate the correct kinetic energy
  distribution, and should not be used.

WARNING 2 [file topol.top, line 8]:
  466 non-matching atom names
  atom names from topol.top will be used

NOTE 1 [file topol.top, line 10]:
  System has non-zero total charge: 0.000001

WARNING 3 [file topol.top, line 10]:
  You are using Ewald electrostatics in a system with net charge. This can
  lead to severe artifacts.

WARNING 4 [file a.mdp]:
  Something nobody has classified.

"""
    code = ("use lib '%s'; use GromppWarn; local $/; my $o = <STDIN>;"
            "print join(',', map { $_->{harmless} } GromppWarn::parse($o)), qq{\\n};"
            % PARSERS)
    rc, perl_out = run(["perl", "-e", code], input=out.encode())
    check(perl_out.strip() == "1,1,1,0",
          "table: Berendsen, atom names and a net charge of 1e-6 are harmless, an unknown warning is not (%s)"
          % perl_out.strip())
    prog = subprocess.run(["perl", "-I" + PARSERS, "-MGromppWarn", "-e",
                           "print GromppWarn::awkProgram()"],
                          stdout=subprocess.PIPE).stdout
    with tempfile.NamedTemporaryFile(suffix=".awk") as h:
        h.write(prog)
        h.flush()
        rc, awk_out = run(["awk", "-f", h.name], input=out.encode())
    check(awk_out.splitlines()[:2] == ["harmless 3", "stopping 1"],
          "awk form of the table counts the same (%s)" % awk_out.splitlines()[:2])


def system(tmp, name, charged):
    d = os.path.join(tmp, name)
    os.makedirs(d)
    lines = open(os.path.join(FIXTURES, "conf.gro")).read().splitlines()
    n = int(lines[1])
    atoms, box = lines[2:2 + n], lines[2 + n]
    top = open(os.path.join(FIXTURES, "topol.top")).read()
    atoms = [a.replace(" OW ", " OX ") for a in atoms]
    if charged:
        atoms.append("%5d%-5s%5s%5d%8.3f%8.3f%8.3f" % (n + 1, "NA", "NA", n + 1, 1.0, 1.0, 1.0))
        n += 1
        top = top.replace('#include "oplsaa.ff/spc.itp"',
                          '#include "oplsaa.ff/spc.itp"\n#include "oplsaa.ff/ions.itp"')
        top = top.replace("SOL   466", "SOL   466\nNA    1")
    open(os.path.join(d, "conf.gro"), "w").write(
        "test\n%d\n%s\n%s\n" % (n, "\n".join(atoms), box))
    open(os.path.join(d, "topol.top"), "w").write(top)
    open(os.path.join(d, "gromacs.mdp"), "w").write(
        "integrator = steep\nnsteps = 5\ncoulombtype = pme\n")
    open(os.path.join(d, "postParams"), "w").write(
        "inputFile: gromacs.mdp\nnumProcs: 1\nrunDir: %s\n" % d)
    return d


def job(home, d, name):
    params = os.path.join(d, "subParams")
    with open(params, "w") as h:
        h.write(" -Q Shell\n -H localtest\n -d localhost\n -c GROMACS\n"
                " -n 1\n -N 1\n -r %s\n -i gromacs.mdp\n -o gromacs.gmxlog\n"
                " -f %s/submit__%s\n -S ecceSession\n -C ecceMdOptimize\n" % (d, d, name))
    rc, out = run([os.path.join(home.home, "scripts", "gensub"), "-v", "-p", params],
                  env=home.env, cwd=d)
    check(rc == 0, "%s: gensub wrote the job script" % name)
    run(["sh", os.path.join(d, "submit__" + name)], cwd=d)
    status = open(os.path.join(d, ".ecce.status")).read().strip()
    gout = os.path.join(d, "gromacs.gmxlog")
    return status, open(gout, errors="replace").read() if os.path.exists(gout) else ""


def main():
    rt.verbose = "-v" in sys.argv
    gmx = rt.find_gmx()
    if gmx is None:
        print("SKIP: gmx is not installed")
        return 77
    canned()
    tmp = tempfile.mkdtemp(prefix="grompp-warn-")
    try:
        home = rt.Home(tmp, gmx)
        launch = [os.path.join(PARSERS, "gromacs.launchpp"), "-p", "postParams"]

        d = system(tmp, "names", False)
        rc, out = run(launch, cwd=d)
        check(rc == 0 and "accepted" in out and "non-matching atom names" in out,
              "launch check: the atom-name warning is shown and does not stop the launch")
        status, text = job(home, d, "names")
        check(status == "0", "job script: runs grompp with -maxwarn 1 and the job completes (status %s)" % status)

        d = system(tmp, "charge", True)
        rc, out = run(launch, cwd=d)
        check(rc != 0 and "STOPPING" in out and "net charge" in out and "[harmless]" in out,
              "launch check: the net-charge warning is marked STOPPING, the atom-name one harmless")
        status, text = job(home, d, "charge")
        check(status != "0" and "net charge" in text,
              "job script: grompp stops at the net-charge warning with its own message")
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    if rt.failures:
        print("FAILED: %d" % len(rt.failures))
        return 1
    print("grompp warning classification: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
