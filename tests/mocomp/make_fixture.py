#!/usr/bin/env python3
"""
Build a calculation folder for ecce-mocomp from an ORCA output file.

    make_fixture.py ORCA.out OUTDIR [--meta DIR]
    make_fixture.py --fort7 FORT.7 OUTDIR [--meta DIR]       (Gaussian 16)

Props/ (MO, MOBETA, ORBENG, ORBOCC, ... and MOAOORDER) come from the real
scripts/parsers/orca.mo, fed the blocks scripts/eccejobmonitor would feed it
(tests/parsers/eccejobmonitor_sim.py), or from gaussian-16.mo fed the punch
file, written in the XML the data store keeps.  Parameters/ (chemsys.mvm, BasisSet.ecce_basisset) are copied from
--meta, a folder importmeta made by importing the same output; see README.
"""
import os
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
SCRIPTS = os.path.join(REPO, "scripts", "parsers")
sys.path.insert(0, os.path.join(REPO, "tests", "parsers"))
from eccejobmonitor_sim import read_desc, replay, run_parser   # noqa: E402

KEEP = ("MO", "MOBETA", "ORBENG", "ORBENGBETA", "ORBOCC", "ORBOCCBETA",
        "MOAOORDER")


def records(text):
    cur = None
    sec = None
    for line in text.splitlines():
        if line.startswith("key: "):
            cur = {"key": line[5:].strip(), "size": "", "values": [],
                   "rowlabels": [], "columnlabels": [], "units": ""}
            sec = None
        elif cur is None:
            continue
        elif line.strip() == "END":
            yield cur
            cur = None
        elif line.rstrip(":") in ("size", "values", "units", "rowlabels",
                                  "columnlabels") and line.endswith(":"):
            sec = line.rstrip(":")
        elif sec == "size":
            cur["size"] = line.strip()
        elif sec == "units":
            cur["units"] = line.strip()
        elif sec in ("values", "rowlabels", "columnlabels"):
            cur[sec].extend(line.split())


def xml(rec):
    head = '<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
    vals = rec["values"]
    k = rec["key"]
    if k == "MOAOORDER":
        return head + '<string name="MOAOORDER" units="NA">%s</string>' % vals[0]
    wrapped = "\n".join("   ".join(vals[i:i+4]) for i in range(0, len(vals), 4))
    dims = rec["size"].split()
    if len(dims) == 2:
        return (head + '<table columnLabel="Basis Function" columnLabels="%s" '
                'columns="%s" name="%s" rowLabel="Basis Function" '
                'rowLabels="%s" rows="%s" units="%s">%s</table>\n'
                % (" ".join(rec["columnlabels"]), dims[1], k,
                   " ".join(rec["rowlabels"]), dims[0], rec["units"] or "NA",
                   wrapped))
    return (head + '<vector name="%s" rowLabel="Basis Function" rows="%s" '
            'units="%s">%s\n</vector>\n' % (k, dims[0], rec["units"] or "NA",
                                             wrapped))


def main(argv):
    meta = None
    args = argv[1:]
    if "--meta" in args:
        i = args.index("--meta"); meta = args[i+1]; del args[i:i+2]
    fort7 = None
    if "--fort7" in args:
        i = args.index("--fort7"); fort7 = args[i+1]; del args[i:i+2]
        args.insert(0, None)
    out_file, outdir = args
    props = os.path.join(outdir, "Props")
    os.makedirs(props, exist_ok=True)
    seen = set()
    if fort7:
        import subprocess
        env = dict(os.environ, ECCE_HOME=REPO)
        text = subprocess.run(
            ["perl", os.path.join(SCRIPTS, "gaussian-16.mo"), ".",
             "Single Point Energy", "Hartree Fock", "RHF", "0"],
            stdin=open(fort7), capture_output=True, text=True, env=env).stdout
        for rec in records(text):
            if rec["key"] in KEEP:
                open(os.path.join(props, rec["key"]), "w").write(xml(rec))
                seen.add(rec["key"])
        out_file = None
    result = None
    if out_file:
        desc = read_desc(os.path.join(SCRIPTS, "orca.desc"))
        result = replay(desc, out_file)
    for block in (result.blocks if result else []):
        if not block.delivered or block.entry.script != "orca.mo":
            continue
        text, err, rc = run_parser(SCRIPTS, block.entry, block)
        for rec in records(text):
            if rec["key"] in KEEP:
                open(os.path.join(props, rec["key"]), "w").write(xml(rec))
                seen.add(rec["key"])
    if "MO" not in seen:
        sys.exit("no MO block found in " + out_file)
    if meta:
        shutil.copytree(os.path.join(meta, "Parameters"),
                        os.path.join(outdir, "Parameters"),
                        dirs_exist_ok=True)
    print("wrote", sorted(seen))


if __name__ == "__main__":
    main(sys.argv)
