#!/usr/bin/env python3
"""
Write oracle.json: Multiwfn's fragment-orbital compositions (CDA, main
function 16) for Cr(CO)6 = Cr + (CO)6.  Developer tool; run_tests.py reads the
JSON and needs neither ORCA nor Multiwfn.

    gen_oracle.py ORCADIR [oracle.json] [--multiwfn PATH]

ORCADIR holds parent.gbw, cr.gbw and co6.gbw from the inputs in inputs/
(ORCA 6.1.1); the same runs make fixtures/ (see README.md).  Each .gbw goes
through `orca_2mkl NAME -molden`.  Multiwfn lists, for a complex orbital, every
fragment orbital contributing at least compthresCDA percent (set to 0), once
with Mulliken (iCDAcomp=1) and once with SCPA (iCDAcomp=2).  The complex's
atoms must be ordered fragment 1 then fragment 2, which they are.
"""
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

WINDOW = range(30, 91)        # parent MOs (1-based): valence, HOMO 54, virtuals


def find(explicit):
    for c in (explicit, os.environ.get("MULTIWFN"),
              shutil.which("Multiwfn_noGUI")):
        if c and os.path.isfile(c):
            return c
    sys.exit("no Multiwfn: set MULTIWFN or --multiwfn")


def molden(orcadir, name, work):
    orca2mkl = glob.glob("/opt/orca/orca_6*/orca_2mkl")[0]
    shutil.copy(os.path.join(orcadir, name + ".gbw"), work)
    subprocess.run([orca2mkl, name, "-molden"], cwd=work, check=True,
                   capture_output=True)
    return os.path.join(work, name + ".molden.input")


def run(multiwfn, work, files, method):
    ini = os.path.join(os.path.dirname(multiwfn), "settings.ini")
    text = open(ini).read()
    text = re.sub(r"compthresCDA=\s*[\d.]+", "compthresCDA= 0.0", text, count=1)
    text = re.sub(r"iCDAcomp=\s*\d", "iCDAcomp= %d" % method, text, count=1)
    open(os.path.join(work, "settings.ini"), "w").write(text)
    shutil.copy(multiwfn, os.path.join(work, "mw"))
    os.chmod(os.path.join(work, "mw"), 0o755)
    keys = "16\n2\n%s\n%s\n2\n" % (files[1], files[2])
    keys += "".join("%d\n" % i for i in WINDOW) + "0\n-1\nq\n"
    env = dict(os.environ, Multiwfnpath=work, OMP_STACKSIZE="200M")
    p = subprocess.run(["bash", "-c", "ulimit -s unlimited; exec %s/mw %s"
                        % (work, files[0])], input=keys, capture_output=True,
                       text=True, env=env, timeout=1800)
    return p.stdout


def parse(out):
    """{mo: {"occ": x, "fo": {"frag:idx": percent}}}"""
    res = {}
    blocks = re.split(r"Occupation number of orbital\s+(\d+) of the complex:\s*",
                      out)
    for b in range(1, len(blocks), 2):
        mo = int(blocks[b])
        body = blocks[b + 1]
        occ = float(body.split()[0])
        fo = {}
        for m in re.finditer(r"Orbital\s+(\d+) of fragment\s*(\d+), Occ:\s*"
                             r"([\d.]+)\s+Contribution:\s*(-?[\d.]+) %", body):
            v = float(m.group(4))
            if abs(v) >= 0.05:      # Multiwfn prints two decimals
                fo["%s:%s" % (m.group(2), m.group(1))] = v
        res[mo] = {"occ": occ, "fo": fo}
    return res


def main(argv):
    args = argv[1:]
    mw = None
    if "--multiwfn" in args:
        i = args.index("--multiwfn")
        mw = args[i + 1]
        del args[i:i + 2]
    orcadir = args[0]
    dest = args[1] if len(args) > 1 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "oracle.json")
    multiwfn = find(mw)
    work = tempfile.mkdtemp(prefix="fragoracle-")
    try:
        files = [molden(orcadir, n, work) for n in ("parent", "cr", "co6")]
        result = {"program": "Multiwfn CDA (main function 16)",
                  "fragments": ["Cr", "(CO)6"],
                  "unit": "percent of the complex orbital"}
        for method, key in ((1, "mulliken"), (2, "scpa")):
            result[key] = {str(k): v for k, v in
                           parse(run(multiwfn, work, files, method)).items()}
            if len(result[key]) != len(WINDOW):
                sys.exit("Multiwfn gave %d orbitals, expected %d"
                         % (len(result[key]), len(WINDOW)))
        with open(dest, "w") as f:
            json.dump(result, f, sort_keys=True, separators=(",", ":"))
            f.write("\n")
        print("wrote", dest)
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main(sys.argv)
