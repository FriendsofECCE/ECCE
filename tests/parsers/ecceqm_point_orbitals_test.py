#!/usr/bin/env python3
"""ECCE-QM stores the orbitals of every scan point (#549).

    tests/parsers/ecceqm_point_orbitals_test.py

Replays a real ecce-qm relaxed scan (fixtures/ecceqm/h2o_scan_orb.qmout:
H2O, O-H bond 0.90-1.10 A in 5 points, HF/STO-3G) through ecceqm.desc and
ecceqm.parse as the job monitor does, and stores every record through the
real PropertyInterpreter (propinterp.C).  Checks: one PTORB and one PTMO
table per SCANGEOM table, each of the size of the final ORBENG/MO; the last
point's orbitals are the final set; the first point's differ.

With an ecce-qm binary in the build tree (ECCE_TEST_BUILD, default
build-cmake) the fixture input is also run again and the fresh output must
give the same per-point energies.  Exit 77 (skip) without built libraries.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import eccejobmonitor_sim as sim            # noqa: E402
import propinterp_test as pit               # noqa: E402

FIXTURE = os.path.join(HERE, "fixtures", "ecceqm", "h2o_scan_orb")
POINTS = 5
NBF = 7


def records(output):
    """key -> list of parsed records, in output order."""
    desc = sim.read_desc(os.path.join(REPO, "scripts", "parsers", "ecceqm.desc"))
    result = sim.replay(desc, output)
    out = {}
    stream = []
    for blk in result.blocks:
        if not blk.delivered:
            continue
        text, err, rc = sim.run_parser(os.path.join(REPO, "scripts", "parsers"),
                                       blk.entry, blk)
        if rc != 0:
            sys.exit("ecceqm.parse failed on %s: %s" % (blk.entry, err))
        stream.append(text)
        for rec in sim.parse_parser_output(text):
            out.setdefault(rec["key"], []).append(rec)
    return out, "".join(stream)


def floats(s):
    return [float(x) for x in s.split()]


def check(output, label):
    recs, stream = records(output)
    bad = []
    for key in ("PTORB", "PTMO", "SCANGEOM"):
        if len(recs.get(key, [])) != POINTS:
            bad.append("%s: %d %s records, want %d" % (label, len(recs.get(key, [])), key, POINTS))
    if bad:
        return bad, None
    for r in recs["PTORB"]:
        if r["flat"]["size"].split() != ["1", str(NBF), "2"]:
            bad.append("%s: PTORB size %s" % (label, r["flat"]["size"]))
    for r in recs["PTMO"]:
        if r["flat"]["size"].split() != ["1", str(NBF), str(NBF)]:
            bad.append("%s: PTMO size %s" % (label, r["flat"]["size"]))
    eng = [floats(r["flat"]["values"])[0::2] for r in recs["PTORB"]]
    final = floats(recs["ORBENG"][-1]["flat"]["values"])
    if max(abs(a - b) for a, b in zip(eng[-1], final)) > 1e-8:
        bad.append("%s: last point's energies are not the final ORBENG" % label)
    if max(abs(a - b) for a, b in zip(eng[0], final)) < 1e-4:
        bad.append("%s: first point's energies equal the last point's" % label)
    mo = [floats(r["flat"]["values"]) for r in recs["PTMO"]]
    fmo = floats(recs["MO"][-1]["flat"]["values"])
    if max(abs(abs(a) - abs(b)) for a, b in zip(mo[-1], fmo)) > 1e-7:
        bad.append("%s: last point's coefficients are not the final MO" % label)
    print("%s: %d points, HOMO %.6f (point 1) %.6f (point %d) hartree"
          % (label, POINTS, eng[0][4], eng[-1][4], POINTS))
    return bad, stream


def store(stream, work):
    """Every record through the real PropertyInterpreter; per key the counts."""
    driver = os.path.join(work, "propinterp")
    if not pit.buildDriver(driver):
        return None
    env = dict(os.environ, ECCE_HOME=REPO, ECCE_REALUSERHOME=work, HOME=work)
    proc = subprocess.run([driver], input=stream, env=env, capture_output=True,
                          text=True)
    lines = [l for l in proc.stdout.splitlines() if l.startswith("PT")]
    return proc.returncode, lines, proc.stdout + proc.stderr[-1500:]


def main():
    bad, stream = check(FIXTURE + ".qmout", "fixture")
    qm = None
    for name in ("ecce-qm", os.path.join("src", "qm", "ecce-qm")):
        p = os.path.join(pit.BUILD, name)
        if os.access(p, os.X_OK):
            qm = p
    with tempfile.TemporaryDirectory(prefix="ptorb-") as work:
        if qm:
            out = os.path.join(work, "fresh.qmout")
            with open(out, "w") as f:
                subprocess.run([qm, FIXTURE + ".qmin"], stdout=f, check=False)
            b, _ = check(out, "fresh run")
            bad += b
        else:
            print("no ecce-qm in %s: fresh run skipped" % pit.BUILD)
        if stream is not None:
            res = store(stream, work)
            if res is None:
                print("no built libraries: storage check skipped")
                return 77 if not bad else 1
            rc, lines, raw = res
            want = {"PTORB rows %d columns 2 values %d" % (NBF, 2 * NBF): POINTS,
                    "PTMO rows %d columns %d values %d" % (NBF, NBF, NBF * NBF): POINTS}
            for line, n in want.items():
                got = lines.count(line)
                if got != n:
                    bad.append("stored: %d x '%s', want %d" % (got, line, n))
            if "UNKNOWN" in raw or "FAILED" in raw:
                bad.append("stored: interpreter reported UNKNOWN/FAILED")
            print("stored: %d per-point records through PropertyInterpreter" % len(lines))
    # IRC (fixtures/ecceqm/hcn_irc_orb.qmout, 3+1+3 points): one table per
    # IRCGEOM table, in the same (path) order.
    irc, _ = records(os.path.join(HERE, "fixtures", "ecceqm", "hcn_irc_orb.qmout"))
    counts = [len(irc.get(k, [])) for k in ("IRCGEOM", "PTORB", "PTMO")]
    if counts[0] == 0 or len(set(counts)) != 1:
        bad.append("irc: IRCGEOM/PTORB/PTMO records %s" % counts)
    print("irc: %d points, IRCGEOM/PTORB/PTMO %s" % (counts[0], counts))
    for b in bad:
        print("FAIL " + b)
    print("PASS" if not bad else "FAILED")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
