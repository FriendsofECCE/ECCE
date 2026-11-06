#!/usr/bin/env python3
"""An ORCA IRC has orbitals at the TS and its two end points (#751).

    tests/parsers/orca_irc_orbitals_test.py

fixtures/orca/hcn_irc_mos.out is a real ORCA 6.1.1 run (HF/3-21G HCN IRC,
Print[P_MOs]) with the two end-point blocks gensub appends (ORCA run again
with MOREAD NoIter from <job>_IRC_B.gbw / _IRC_F.gbw).  Replayed through
orca.desc and orca.ptmo/orca.mo: MO/ORBENG/ORBOCC come from the TS block
only, PTORB/PTMO hold three tables (TS, backward end, forward end), and a
run that is not an IRC emits no tables.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import eccejobmonitor_sim as sim            # noqa: E402

FIXTURE = os.path.join(HERE, "fixtures", "orca", "hcn_irc_mos.out")
PARSERS = os.path.join(REPO, "scripts", "parsers")
NMO, NBF = 20, 20


def floats(s):
    return [float(x) for x in s.split()]


def records(runtype):
    desc = sim.read_desc(os.path.join(PARSERS, "orca.desc"))
    result = sim.replay(desc, FIXTURE)
    out = {}
    for blk in result.blocks:
        if not blk.delivered:
            continue
        args = [".", runtype] + list(sim.DEFAULT_PARSE_ARGS[2:])
        text, err, rc = sim.run_parser(PARSERS, blk.entry, blk, parse_args=args)
        if rc != 0:
            sys.exit("%s failed: %s" % (blk.entry.script, err))
        for rec in sim.parse_parser_output(text):
            out.setdefault(rec["key"], []).append(rec)
    return out


def main():
    bad = []
    irc = records("IRC")
    for key in ("MO", "ORBENG", "ORBOCC"):
        if len(irc.get(key, [])) != 1:
            bad.append("IRC: %d %s records, want 1" % (len(irc.get(key, [])), key))
    # Frequency=all gives one record, hence one table, per orbital block.
    tables = {}
    for key in ("PTMO", "PTORB"):
        tables[key] = irc.get(key, [])
        if len(tables[key]) != 3:
            bad.append("IRC: %d %s records, want 3" % (len(tables[key]), key))
    if not bad:
        for t in range(3):
            if tables["PTORB"][t]["flat"]["size"].split() != ["1", str(NMO), "2"]:
                bad.append("PTORB %d size %s" % (t, tables["PTORB"][t]["flat"]["size"]))
            if tables["PTMO"][t]["flat"]["size"].split() != ["1", str(NMO), str(NBF)]:
                bad.append("PTMO %d size %s" % (t, tables["PTMO"][t]["flat"]["size"]))
        eng = floats(tables["PTORB"][0]["flat"]["values"])
        want = floats(irc["ORBENG"][0]["flat"]["values"])
        occ = floats(irc["ORBOCC"][0]["flat"]["values"])
        if eng[0::2] != want or eng[1::2] != occ:
            bad.append("PTORB 0 is not the TS ORBENG/ORBOCC")
        if floats(tables["PTMO"][0]["flat"]["values"]) != floats(irc["MO"][0]["flat"]["values"]):
            bad.append("PTMO 0 is not the TS MO")
        e = [floats(tables["PTORB"][t]["flat"]["values"])[0::2] for t in range(3)]
        if e[0] == e[1] or e[0] == e[2] or e[1] == e[2]:
            bad.append("two of the three orbital sets are identical")
    plain = records("Energy")
    if plain.get("PTMO") or plain.get("PTORB"):
        bad.append("a non-IRC run emitted PTMO/PTORB")
    if len(plain.get("MO", [])) != 1:
        bad.append("non-IRC run lost MO")
    print("IRC: PTORB/PTMO tables %s/%s, non-IRC: %s"
          % (len(irc.get("PTORB", [])), len(irc.get("PTMO", [])),
             len(plain.get("PTMO", []))))
    for b in bad:
        print("FAIL " + b)
    print("PASS" if not bad else "FAILED")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
