#!/usr/bin/env python3
"""qm_point_orbitals (#1114): `point_orbitals no` leaves the per-point orbital
blocks out of a scan and an IRC, keeps the tables, geometries and the final
orbitals, and ecceqm.parse handles the lean output without PTORB/PTMO.

    point_orbitals_test.py ECCE_QM_EXE
"""
import json, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PARSE = os.path.join(os.path.dirname(os.path.dirname(HERE)), "scripts", "parsers", "ecceqm.parse")
fails = 0


def check(ok, what):
    global fails
    print("  %s %s" % ("ok  " if ok else "FAIL", what))
    fails += not ok


SCAN = ("runtype optimize\nmethod hf\nbasis STO-3G\n%sgeometry\nO 0 0 0.12\nH 0 0.76 -0.48\nH 0 -0.76 -0.48\nend\n"
        "scan\nbond 1 2 0.95 1.05 2\nend\n")


def irc_deck(extra):
    xyz = json.load(open(os.path.join(HERE, "irc_oracle.json")))["sto3g"]["ts_xyz"]
    return ("runtype irc\nmethod hf\nbasis STO-3G\nirc_step 0.1\nirc_max_points 2\n" + extra + "geometry\n" +
            "".join("%s %.8f %.8f %.8f\n" % tuple(a) for a in xyz) + "end\n")


def run(exe, deck):
    with tempfile.TemporaryDirectory() as work:
        path = os.path.join(work, "in.qm")
        open(path, "w").write(deck)
        p = subprocess.run([exe, path], capture_output=True, text=True)
    if p.returncode:
        sys.exit("ecce-qm failed: " + p.stderr[-500:])
    return p.stdout


def parse_ok(out, names):
    """Every block the parser is given must parse without a message on stderr."""
    bad = 0
    for name in names:
        for blk in re.findall(r"^(begin %s\b.*?^end %s)\n" % (name, name), out, re.S | re.M):
            p = subprocess.run(["perl", PARSE], input=blk, capture_output=True, text=True)
            bad += p.returncode != 0 or bool(p.stderr.strip()) or "PTMO" in p.stdout or "PTORB" in p.stdout
    return bad == 0


def main():
    exe = sys.argv[1]
    for label, mk, table, geom in (("scan", lambda x: SCAN % x, "scan_table", "scan_geometry"),
                                   ("irc", irc_deck, "irc_table", "irc_geometry")):
        on = run(exe, mk(""))
        off = run(exe, mk("point_orbitals no\n"))
        check("begin point_orbitals" in on, label + ": default still writes point_orbitals")
        check("begin point_orbitals" not in off, label + ": no point_orbitals blocks with point_orbitals no")
        check("begin point_mo_coefficients" not in off, label + ": no point_mo_coefficients blocks")
        check(("begin " + table) in off and ("begin " + geom) in off, label + ": table and geometries remain")
        check("begin mo_coefficients" in off or "begin orbitals" in off or "begin mo" in off,
              label + ": final orbitals remain")
        check(len(off) < len(on), label + ": output is smaller (%d < %d)" % (len(off), len(on)))
        check(parse_ok(off, [table, geom]), label + ": parser gives no PTORB/PTMO and no error")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
