#!/usr/bin/env python3
"""Replay CP2K blocks and hostile fields against the saved Perl results.

    python3 tests/parsers/cp2k_port_test.py
    python3 tests/parsers/cp2k_port_test.py --original TREE --artifacts DIR

TREE contains scripts/parsers from before the port. --write-golden requires
that oracle; the Python port can never generate its own expectations.
"""
import argparse
import base64
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

from eccejobmonitor_sim import read_desc, replay

REPO = Path(__file__).resolve().parents[2]
SCRIPTS = REPO / "scripts/parsers"
FIXTURES = REPO / "tests/parsers/fixtures/cp2k"
GOLDEN = REPO / "tests/parsers/expected/cp2k-port.json"
NAMES = ("egradvec", "energy", "ewvec", "geomtrace", "hirshfeld", "hlgap",
         "mulliken", "time", "version", "launchpp")
ARGS = (".", "Geometry", "QS", "GPW", "0")
MUTATIONS = (("nbsp", b"\xc2\xa0"), ("digit", b"\xd9\xa3"),
             ("word", b"\xc3\x85"), ("overflow", b"*****"),
             ("prefix", b"3x"), ("empty", b""), ("stray", b"\xff"))


def inputs(work):
    desc = read_desc(str(SCRIPTS / "cp2k.desc"))
    for name in ("si_scf", "si_diag", "nio_u_opt", "opt_2023_steps",
                 "si_diag_gap", "si_diag_mo", "si_scf_stray"):
        # The simulator decodes text; Latin-1 bridges the original bytes so
        # the child receives the exact job output, including invalid UTF-8.
        source = FIXTURES / (name + ".cp2kout")
        bridge = work / "replay.txt"
        bridge.write_text(source.read_bytes().decode("latin1"), encoding="utf-8")
        result = replay(desc, str(bridge))
        for entry in desc.live_entries():
            for index, block in enumerate(result.delivered_for(entry)):
                yield ("real/{}/{}/{}".format(name, entry.script, index),
                       entry.script, block.text.encode("latin1"), ARGS)

    templates = (
        ("force", "egradvec", b" FORCES| 1 1.8E-13 -0.0 9.67888929E-02 9.67888929E-02\n", b"", b""),
        ("force-stop", "egradvec", b" FORCES| Sum\n", b"", b" FORCES| 1 1.0 2.0 3.0 4.0\n"),
        ("energy", "energy", b" ENERGY| Total FORCE_EVAL ( QS ) energy [hartree] -7.244568794042680\n", b"", b""),
        ("ot", "ewvec", b"    4 OT DIIS 0.80E-01 0.7 0.06579221 -7.0482844195 -8.11E-02\n", b"", b""),
        ("diag", "ewvec", b" 1 Broy./Diag. 0.40E+00 0.3 0.01235000 -7.1234567890 -7.12E+00\n", b"", b""),
        ("frame", "geomtrace", b" i = 1, E = -1.1147417434\n", b"", b" H 0.0000 0.0000 0.1659\n"),
        ("coords", "geomtrace", b" H2 0.0000 -0.0000 0.1659\n", b" i = 1, E = -1.1147417434\n", b""),
        ("hirsh-r", "hirshfeld", b" 1 Si 1 4.000 4.000 0.000\n", b"", b""),
        ("hirsh-u", "hirshfeld", b" 1 Ni 1 18.000 9.889 8.728 1.161 -0.617\n", b"", b""),
        ("hirsh-stop", "hirshfeld", b" Total Charge\n", b"", b" 1 Si 1 4.000 4.000 0.000\n"),
        ("mull-r", "mulliken", b" 1 Si 1 3.999998 0.000002\n", b"", b""),
        ("mull-u", "mulliken", b" 1 Ni 1 9.228817 8.272317 0.498865 0.956500\n", b"", b""),
        ("mull-stop", "mulliken", b" # Total charge\n", b"", b" 1 Si 1 3.999998 0.000002\n"),
        ("time", "time", b" CP2K 1 1.0 1.403 1.432 32.862 32.862\n", b"", b""),
        ("version", "version", b" CP2K| version string: CP2K version 2025.1\n", b"", b""),
        ("version-short", "version", b" CP2K| version string: 2025.1\n", b"", b""),
        ("gap-old", "hlgap", b" HOMO - LUMO gap [eV] : 2.3456D+00\n", b"", b""),
        ("gap-new", "hlgap", b" MO| Band gap: 0.395287 a.u. 10.756318 eV\n", b"", b""),
    )
    for site, suffix, row, before, after in templates:
        script = "cp2k." + suffix
        yield ("site/" + site + "/clean", script, before + row + after, ARGS)
        # Mutate each token and each whitespace run, not only the first
        # successful field: late columns and stop patterns matter too.
        spans = list(re.finditer(rb"\S+|\s+", row, re.ASCII))
        for index, span in enumerate(spans):
            for label, value in MUTATIONS:
                data = before + row[:span.start()] + value + row[span.end():] + after
                yield ("site/{}/{}/{}".format(site, index, label), script, data, ARGS)
            for digit_index, digit in enumerate(re.finditer(rb"\d+", span[0], re.ASCII)):
                start, end = span.start() + digit.start(), span.start() + digit.end()
                yield ("site/{}/{}/digit-run-{}".format(site, index, digit_index),
                       script, before + row[:start] + b"\xd9\xa3" + row[end:] + after, ARGS)
        for label, value in MUTATIONS:
            yield ("site/{}/led/{}".format(site, label), script,
                   before + value + row + after, ARGS)

    for suffix in NAMES[:-1]:
        yield ("empty/" + suffix, "cp2k." + suffix, b"", ARGS)
    for value in (b"0", b"-0", b"3x", b"1D+03", b"1E+", b"1e999", b"-1e999",
                  b"1e-999", b"1\xff", b"3\xd9\xa3", b"3\xc2\xa0", b"3\xc3\x85"):
        yield ("number/" + value.hex(), "cp2k.egradvec",
               b" FORCES| 1 " + value + b" 1.0 2.0 3.0\n", ARGS)
    launch_args = (("-p", "postParams"), ("-ppostParams",), ("-v",),
                   ("-vp", "postParams"), ("-p", "missing"), (), ("-p",),
                   ("--",), ("-x", "-p", "postParams"), ("-p", "-v"))
    for index, args in enumerate(launch_args):
        yield ("launch/{}".format(index), "cp2k.launchpp", b"", args)


def snapshot(work):
    return {str(path.relative_to(work)): base64.b64encode(path.read_bytes()).decode("ascii")
            for path in sorted(work.rglob("*")) if path.is_file()}


def invoke(tree, script, data, args, work, perl=False):
    path = tree / "scripts/parsers" / script
    command = ["perl", str(path)] if perl else [str(path)]
    if os.name == "nt" and not perl:
        command.insert(0, "python3")
    env = dict(os.environ, ECCE_HOME=str(tree))
    proc = subprocess.run(command + list(args), input=data, cwd=str(work), env=env,
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=30)
    return dict(stdout=base64.b64encode(proc.stdout).decode("ascii"),
                stderr=base64.b64encode(proc.stderr).decode("ascii"),
                rc=proc.returncode, files=snapshot(work))


def run(tree, perl, root):
    found = {}
    for name, script, data, args in inputs(root):
        with tempfile.TemporaryDirectory(dir=str(root)) as scratch:
            work = Path(scratch)
            if script == "cp2k.launchpp":
                (work / "postParams").write_bytes(b"inputFile: cp2k.cp2kin\nnumProcs: 1\nstray: \xff\n")
                (work / "cp2k.cp2kin").write_bytes((FIXTURES / "si_scf.cp2kin").read_bytes())
            found[name] = dict(script=script, result=invoke(tree, script, data, args, work, perl))
    return found


def artifacts(target, results):
    target.mkdir(parents=True, exist_ok=True)
    for suffix in NAMES:
        script = "cp2k." + suffix
        selected = [(name, row["result"]) for name, row in results.items() if row["script"] == script]
        for channel in ("stdout", "stderr", "rc", "files"):
            body = bytearray()
            for name, result in selected:
                value = result[channel]
                data = (base64.b64decode(value) if channel in ("stdout", "stderr")
                        else json.dumps(value, sort_keys=True).encode("ascii"))
                body.extend(("{} {}\n".format(name, len(data))).encode("ascii") + data)
            (target / (script + "." + channel)).write_bytes(body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path)
    parser.add_argument("--artifacts", type=Path)
    parser.add_argument("--write-golden", action="store_true")
    args = parser.parse_args()
    if args.write_golden and not args.original:
        parser.error("--write-golden requires --original")
    with tempfile.TemporaryDirectory(prefix="ecce-cp2k-port-") as scratch:
        work = Path(scratch)
        actual = run(REPO, False, work)
        if args.original:
            expected = run(args.original.resolve(), True, work)
        else:
            expected = json.loads(GOLDEN.read_text(encoding="utf-8"))
    if args.artifacts:
        artifacts(args.artifacts / "python", actual)
        artifacts(args.artifacts / "perl", expected)
    assert actual and actual.keys() == expected.keys(), "case inventory differs"
    differences = [name for name in actual if actual[name] != expected[name]]
    for name in differences[:20]:
        print("FAIL", name, "\nPerl:", expected[name], "\nPython:", actual[name])
    if differences:
        print("FAIL: {} of {} invocations differ".format(len(differences), len(actual)))
        return 1
    # The decoy must leave every clean property byte-for-byte unchanged.
    for name, row in actual.items():
        if name.startswith("real/si_scf_stray/"):
            assert row == actual[name.replace("si_scf_stray", "si_scf")], name
    if args.write_golden:
        GOLDEN.write_text(json.dumps(expected, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    for suffix in NAMES:
        script = "cp2k." + suffix
        rows = [row for row in actual.values() if row["script"] == script]
        assert rows and any(row["result"]["stdout"] for row in rows), script
        print("PASS {}: {} invocations; stdout, stderr, rc and files identical".format(script, len(rows)))
    print("PASS: {} invocations; Perl-anchored golden; clean/decoy identical".format(len(actual)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
