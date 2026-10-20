#!/usr/bin/env python3
"""
First-start registration of this computer looks for every supported code on
the PATH (#377), with stub programs, an isolated ECCE_HOME and user home.

    localcodes_test.py --build <build dir>

Exit status 77 (CTest SKIP) without build/localcodes.
"""
import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
failures = []


def check(ok, what):
    print("%s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)


def stub(path):
    with open(path, "w") as h:
        h.write("#!/bin/sh\nexit 0\n")
    os.chmod(path, 0o755)


def run(exe, groups):
    """groups: one list of stub names per PATH directory, in PATH order."""
    tmp = tempfile.mkdtemp(prefix="localcodes_")
    home = os.path.join(tmp, "ecce")
    user = os.path.join(tmp, "user")
    os.makedirs(os.path.join(user, ".ECCE"))
    os.makedirs(home)
    for d in ("scripts", "siteconfig", "data"):
        if os.path.isdir(os.path.join(REPO, d)):
            os.symlink(os.path.join(REPO, d), os.path.join(home, d))
    bins = []
    for i, names in enumerate(groups):
        d = os.path.join(tmp, "bin%d" % i)
        os.makedirs(d)
        bins.append(d)
        for n in names:
            stub(os.path.join(d, n))
    # A relative PATH entry must be ignored, whatever the working directory holds.
    path = ":".join(bins + ["rel"])
    env = dict(os.environ, ECCE_HOME=home, HOME=user, ECCE_REALUSERHOME=user,
               ECCE_REALUSER="tester", PATH=path + ":/usr/bin:/bin")
    for k in ("ECCE_REMOTE_SERVER", "ECCE_SERVER_LOGIN"):
        env.pop(k, None)
    r = subprocess.run([exe], env=env, cwd=tmp, stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT, text=True)
    mm = os.path.join(user, ".ECCE", "MyMachines")
    text = open(mm).read() if os.path.exists(mm) else ""
    # The code list goes to MyMachines, each code's path to CONFIG.<machine>.
    for c in glob.glob(os.path.join(user, ".ECCE", "CONFIG.*")):
        text += open(c).read()
    return r.stdout, text, tmp


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--build", required=True)
    exe = os.path.join(os.path.abspath(ap.parse_args().build), "localcodes")
    if not os.path.exists(exe):
        return 77

    out, mm, tmp = run(exe, [["gmx"]])
    print(out)
    check(out.startswith("OK"), "registered")
    check("GROMACS (%s/bin0/gmx)" % tmp in out, "message names GROMACS and its path")
    check("Organizer" in out and "Launcher Job menu" not in out,
          "message points at the Organizer for Register Machines")
    check("GROMACS" in mm and tmp + "/bin0/gmx" in mm, "MyMachines and CONFIG hold the code and path")
    shutil.rmtree(tmp, ignore_errors=True)

    out, mm, tmp = run(exe, [["orca"], ["orca", "orca_2mkl"]])
    print(out)
    check("ORCA (%s/bin1/orca)" % tmp in out,
          "ORCA is the one with orca_2mkl, from the later PATH entry")
    check("bin0/orca" not in out, "an orca without orca_2mkl is skipped")
    shutil.rmtree(tmp, ignore_errors=True)

    out, mm, tmp = run(exe, [["orca"]])
    print(out)
    check("ORCA" not in out and "ORCA" not in mm, "orca alone is not registered")
    check("No computational codes were found" in out
          or "nwchem" in out.lower(), "none-found sentence (or a system NWChem)")
    shutil.rmtree(tmp, ignore_errors=True)

    if failures:
        print("FAILED:", failures)
        return 1
    return 0


sys.exit(main())
