#!/usr/bin/env python3
"""Harmless and stopping grompp warnings (scripts/parsers/GromppWarn.pm).

Real gmx on two small systems made from the water fixture:
  * the structure's atom names differ from the topology's: a harmless
    warning, so the launch check says nothing about it and the job script
    runs grompp with -maxwarn 1;
  * a Na+ ion with PME: a net-charge warning that is not harmless, so the
    launch check reports it after ECCE-LAUNCH-WARNING (exit 0: the Launcher
    asks whether to launch anyway) and the job script fails with grompp's
    own message.
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
    big = out.replace("0.000001", "1.000000")
    code1 = code.replace("GromppWarn::parse($o)", "GromppWarn::parse($o, 1)")
    outs = [run(["perl", "-e", c], input=big.encode())[1].strip() for c in (code, code1)]
    check(outs == ["1,1,0,0", "1,1,1,0"],
          "table: a net charge of 1 stops, unless the Prepare marked it intended (%s)" % outs)
    # ECCE's wording of common stopping warnings
    more = """WARNING 1 [file a.mdp]:
  For proper integration of the V-rescale thermostat, tau-t (0.01) should be at
  least 20 times larger than nsttcouple*dt (0.04)

WARNING 2 [file a.mdp]:
  You are using constraints on all bonds, whereas the forcefield has been
  parametrized only with constraints involving hydrogen atoms.

WARNING 3 [file a.mdp]:
  You are using a plain Coulomb cut-off, which might produce artifacts.

WARNING 4 [file a.mdp]:
  Something nobody has classified.

NOTE 1 [file topol.top, line 10]:
  System has non-zero total charge: -1.000000

WARNING 5 [file topol.top, line 10]:
  You are using Ewald electrostatics in a system with net charge. This can
  lead to severe artifacts.
"""
    code2 = ("use lib '%s'; use GromppWarn; local $/; my $o = <STDIN>;"
             "print join('|', map { GromppWarn::explain($_) // 'NONE' } GromppWarn::parse($o)), qq{\\n};"
             % PARSERS)
    rc, ex = run(["perl", "-e", code2], input=more.encode())
    parts = ex.strip().split("|")
    check(len(parts) == 5 and "tau-t (0.01 ps)" in parts[0] and "0.8 ps or more" in parts[0]
          and "thermostat" in parts[0] and "h-bonds" in parts[1] and "PME" in parts[2]
          and parts[3] == "NONE" and parts[4].startswith("Net charge "),
          "explain: tau, constraints, plain cut-off and net charge are worded, an unknown one is not (%s)" % ex.strip()[:300])
    check("Net charge \u22121 e." in ex,
          "explain: a negative net charge uses a real minus sign")
    # .mdp keys in grompp's text are named as the editor names them
    msg = ("nstenergy should be a multiple of nstcalcenergy, "
           "changing nstenergy to 200")
    code3 = ("use lib '%s'; use GromppWarn; print GromppWarn::translate($ARGV[0]);"
             % PARSERS)
    rc, tr = run(["perl", "-e", code3, msg])
    check("Files tab, write energies interval (nstenergy) should be" in tr
          and "to 200" in tr and "ECCE writes nstcalcenergy itself" in tr
          and "report it" in tr,
          "translate: nstenergy gets the editor's name with the key kept; nstcalcenergy is flagged as ECCE's (%s)" % tr[:200])
    rc, tr = run(["perl", "-e", code3, "tau-p is too small; nsttcouple*dt"])
    check("Dynamics tab, pressure relaxation time (tau-p)" in tr and "ECCE writes" not in tr
          and "nsttcouple*dt" in tr,
          "translate: a field key is translated once, plain words are left alone (%s)" % tr[:200])
    prog = subprocess.run(["perl", "-I" + PARSERS, "-MGromppWarn", "-e",
                           "print GromppWarn::awkProgram()"],
                          stdout=subprocess.PIPE).stdout
    with tempfile.NamedTemporaryFile(suffix=".awk") as h:
        h.write(prog)
        h.flush()
        rc, awk_out = run(["awk", "-f", h.name], input=out.encode())
    check(awk_out.splitlines()[:2] == ["harmless 3", "stopping 1"],
          "awk form of the table counts the same (%s)" % awk_out.splitlines()[:2])
    with tempfile.NamedTemporaryFile(suffix=".awk") as h:
        h.write(prog)
        h.flush()
        res = [run(["awk"] + v + ["-f", h.name], input=big.encode())[1].splitlines()[:2]
               for v in ([], ["-v", "marked=1"])]
    check(res == [["harmless 2", "stopping 2"], ["harmless 3", "stopping 1"]],
          "awk form: the marker lets a net charge of 1 through (%s)" % res)


def system(tmp, name, charged, marked=False):
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
    if marked:
        top += "; ECCE-NETCHARGE-INTENDED\n"
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
        check(rc == 0 and "ECCE-LAUNCH-WARNING" not in out and "non-matching" not in out
              and "harmless" not in out,
              "launch check: the harmless atom-name warning is not shown and does not stop the launch")
        status, text = job(home, d, "names")
        check(status == "0", "job script: runs grompp with -maxwarn 1 and the job completes (status %s)" % status)

        d = system(tmp, "charge", True)
        rc, out = run(launch, cwd=d)
        head, _, orig = out.partition("\nECCE-ORIGINAL-MESSAGE\n")
        check(rc == 0 and head.startswith("ECCE-LAUNCH-WARNING\nThe job will stop at grompp with this error:\n\n")
              and "non-matching" not in out,
              "launch check: the net-charge warning is reported without blocking, the atom-name one is not")
        check("Net charge \u2212" not in head and "Net charge 1 e. With PME" in head
              and "Neutralise the system with counter-ions (Prepare: Neutralise)" in head
              and " we " not in head and " you " not in head.lower()
              and "Ewald electrostatics in a system with net charge" in orig
              and "Ewald" not in head,
              "launch check: the net charge is worded by ECCE with the number, grompp's text follows the marker")
        status, text = job(home, d, "charge")
        check(status != "0" and "net charge" in text,
              "job script: grompp stops at the net-charge warning with its own message")

        # Prepare with Neutralise unticked: the marker makes the net charge a decision
        d = system(tmp, "intended", True, marked=True)
        rc, out = run(launch, cwd=d)
        check(rc == 0 and "ECCE-LAUNCH-WARNING" not in out and "net charge" not in out,
              "launch check: a net charge the Prepare left on purpose is not reported")
        status, text = job(home, d, "intended")
        check(status == "0", "job script: grompp lets the marked net charge through (status %s)" % status)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    if rt.failures:
        print("FAILED: %d" % len(rt.failures))
        return 1
    print("grompp warning classification: ok")
    return 0


if __name__ == "__main__":
    sys.exit(main())
