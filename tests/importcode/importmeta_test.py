#!/usr/bin/env python3
"""
What an imported calculation records about its chemical system and basis
(#235), against what the output file itself says.

    importmeta_test.py <importmeta-binary>

importmeta runs the code's importer and TaskJob::import() into a local-mode
calculation (ECCE_LOCAL_DATA, no data server) and prints the point group,
multiplicity, basis name, ECP name and spherical flag read back from it.
The expected values come from the output text with patterns of this file's
own, not from the importer.  The job store, which in the real import also
parses PNTGRP from the output afterwards, does not run here.
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
FIXTURES = os.path.join(HERE, "..", "parsers", "fixtures")

CASES = [
    ("orca/h2o_sym.out", "ORCA"),
    ("orca/hof_sym.out", "ORCA"),
    ("orca/oh_uhf.out", "ORCA"),
    ("orca/h2o_optfreq.out", "ORCA"),
    ("orca/c6h6_sym.out", "ORCA"),
    ("orca/hi_ecp.out", "ORCA"),
    ("gaussian-16/co_freq.log", "Gaussian-16"),
    ("gaussian-16/oh_uhf.log", "Gaussian-16"),
    ("gaussian-16/h2o_optfreq.log", "Gaussian-16"),
    ("nwchem/h2o_opt_stdout.out", "NWChem"),
]

# The finite group ECCE keeps for a linear molecule (LinearPointGroup.H).
LINEAR = {"C*V": "C4v", "CINFV": "C4v", "C(INF)V": "C4v",
          "D*H": "D4h", "DINFH": "D4h", "D(INF)H": "D4h"}


def last(pattern, text):
    found = re.findall(pattern, text, re.M)
    return found[-1] if found else None


def first(pattern, text):
    m = re.search(pattern, text, re.M)
    return m.group(1) if m else None


def oracle(code, text):
    """{field: expected value}; a field left out is not checked."""
    want = {}
    if code == "ORCA":
        want["pointgroup"] = (
            first(r"^Auto-detected point group\s+\.+\s+(\S+)", text) or
            last(r"^Point Group:\s*([^\s,]+)", text) or "C1")
        want["multiplicity"] = first(r"^\s*Multiplicity\s+Mult\s+\.+\s+(\d+)",
                                     text)
        want["basis"] = first(r"^Your calculation utilizes the basis:\s*(\S+)",
                              text)
        want["ecp"] = bool(re.search(r"\bECP\b", text))
        want["spherical"] = "yes"
    elif code == "Gaussian-16":
        want["pointgroup"] = last(r"^\s*Full point group\s+(\S+)", text) or "C1"
        want["multiplicity"] = first(
            r"Charge\s*=\s*-?\d+\s+Multiplicity\s*=\s*(\d+)", text)
        want["basis"] = first(r"^\s*Standard basis:\s*(\S+)", text)
        want["ecp"] = "Pseudopotential Parameters" in text
        shells = first(r"^\s*Standard basis:.*\((\dD)", text)
        want["spherical"] = "yes" if shells == "5D" else "no"
    elif code == "NWChem":
        # NWChem.expt writes no .gbs for a library basis (expt_cases.py
        # NOTES), so only the chemical system is checked.
        want["pointgroup"] = last(r"^\s*Group name\s+(\S+)", text) or "C1"
        nopen = last(r"^\s*open shells\s*=\s*(\d+)", text)
        if nopen is not None:
            want["multiplicity"] = str(int(nopen) + 1)
    group = want.get("pointgroup")
    if group is not None:
        want["pointgroup"] = LINEAR.get(group.upper(), group)
    return want


def imported(binary, path, code, env):
    proc = subprocess.run([binary, path, code], env=env, capture_output=True,
                          text=True, timeout=240)
    got = dict(re.findall(r"^(\w+): ?(.*)$", proc.stdout, re.M))
    return proc, got


def compare(want, got):
    problems = []
    for field, value in want.items():
        have = got.get(field)
        if field == "multiplicity":
            ok = have is not None and have.split()[0] == value
        elif field == "ecp":
            ok = bool(have) == value
            value = "an ECP" if value else "no ECP"
        elif field == "basis":
            # TGBSConfig::name() marks more than one element group "...".
            ok = (have or "").rstrip(".").lower() == (value or "").lower()
        else:
            ok = (have or "").lower() == (value or "").lower()
        if not ok:
            problems.append("%s: expected %s, got %r" % (field, value, have))
    return problems


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    binary = sys.argv[1]
    scratch = tempfile.mkdtemp(prefix="ecce-importmeta-")
    failures = 0
    try:
        env = dict(os.environ)
        env["ECCE_LOCAL_DATA"] = os.path.join(scratch, "data")
        env["ECCE_REALUSERHOME"] = os.path.join(scratch, "home")
        env.setdefault("ECCE_REALUSER", "importmeta")
        os.makedirs(env["ECCE_REALUSERHOME"])
        for name, code in CASES:
            path = os.path.abspath(os.path.join(FIXTURES, name))
            with open(path, errors="replace") as handle:
                want = oracle(code, handle.read())
            proc, got = imported(binary, path, code, env)
            problems = compare(want, got) if proc.returncode == 0 else [
                "importmeta exit %d: %s" % (proc.returncode,
                                            proc.stderr.strip()[-500:])]
            summary = ", ".join("%s=%s" % (k, got.get(k)) for k in
                                ("pointgroup", "multiplicity", "basis", "ecp",
                                 "spherical"))
            if problems:
                failures += 1
                print("FAIL %s (%s)\n     %s" % (name, summary,
                                                "\n     ".join(problems)))
            else:
                print("ok   %s (%s)" % (name, summary))
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    print("%d of %d imports wrong" % (failures, len(CASES)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
