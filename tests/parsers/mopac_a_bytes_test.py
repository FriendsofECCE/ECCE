#!/usr/bin/env python3
"""Compare MOPAC part A with byte records captured from the original Perl."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from eccejobmonitor_sim import read_desc, replay

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
SCRIPTS = REPO / "scripts/parsers"
NAMES = ("charges", "egradvec", "energy", "espcharge", "etime", "ezeropt",
         "forceframe", "geomtrace", "ip", "orbocc", "pntgrp", "solvmodel")
ARGS = [".", "GeoVib", "SE", "PM7", "0"]
GOLDEN = HERE / "expected/mopac-a-bytes.json"


def fixture_cases():
    desc = read_desc(str(SCRIPTS / "mopac.desc"))
    paths = sorted((HERE / "fixtures/mopac").glob("*.out"))
    paths += [REPO / "data/client/examples/getting-started/mopac-ammonia/ammonia.out"]
    for path in paths:
        result = replay(desc, str(path))
        assert result.runaway is None, str(path)
        for entry in desc.live_entries():
            if entry.script not in ["mopac." + n for n in NAMES]:
                continue
            for i, block in enumerate(result.delivered_for(entry)):
                # The simulator decodes with replacement. Restore this fixture's
                # invalid-byte decoy so the child sees the original pipe bytes.
                data = block.text.encode("utf-8").replace(b"\xef\xbf\xbd", b"\xff")
                args = list(ARGS)
                if "uhf" in path.stem:
                    args[-2:] = ["UPM7", "1"]
                name = "real/{}/{}/{}".format(path.stem, entry.script, i)
                yield name, entry.script, data, args, {}


def crafted_cases():
    # Each token is mutated independently: row anchors, whitespace, symbols,
    # coordinates and values all exercise separate sites in the Perl patterns.
    rows = [
        ("charges-atom", "charges", [b"1", b"O", b"-0.5000", b"6.5000"]),
        ("charges-sum", "charges", [b"SUM", b"-0.000", b"0.125", b"-2.141"]),
        ("gradient", "egradvec", [b"1", b"1", b"O", b"CARTESIAN", b"X", b"0.000000", b"97.413669"]),
        ("final", "energy", [b"FINAL", b"HEAT", b"OF", b"FORMATION", b"=", b"-57.79986"]),
        ("force-energy", "energy", [b"HEAT", b"OF", b"FORMATION", b"=", b"-57.791897"]),
        ("esp", "espcharge", [b"1", b"O", b"-0.5889"]),
        ("time", "etime", [b"TOTAL", b"JOB", b"TIME:", b"0.14"]),
        ("zpe", "ezeropt", [b"ZERO", b"POINT", b"ENERGY", b"10.115"]),
        ("force-frame", "forceframe", [b"1", b"O", b"0.0000", b"-0.0000", b"0.1173"]),
        ("geometry", "geomtrace", [b"1", b"O", b"0.0000", b"-0.0000", b"0.1173"]),
        ("ionization", "ip", [b"IONIZATION", b"POTENTIAL", b"=", b"12.115488"]),
        ("populations", "orbocc", [b"1", b"O", b"2.00000", b"2.00000", b"2.00000", b"0.00000"]),
        ("point-group", "pntgrp", [b"MOLECULAR", b"POINT", b"GROUP", b":", b"C2v"]),
        ("dielectric", "solvmodel", [b"PM7", b"EPS", b"=", b"78.4"]),
    ]
    mutations = [("nbsp", b"\xc2\xa0"), ("digit", b"\xd9\xa3"),
                 ("word", b"\xc3\x85"), ("overflow", b"*****"),
                 ("prefix", b"3x"), ("empty", b""), ("stray", b"\xff")]
    for site, script, tokens in rows:
        clean = b" ".join(tokens) + b"\n"
        initial = {"mopacForceGeom": b"stale frame\n"} if script == "forceframe" else {}
        yield site + "/clean", "mopac." + script, clean, ARGS, initial
        for label, value in mutations:
            yield site + "/leading-" + label, "mopac." + script, value + clean, ARGS, initial
            for i in range(len(tokens)):
                changed = tokens[:i] + [value] + tokens[i + 1:]
                yield "{}/field-{}/{}".format(site, i, label), "mopac." + script, b" ".join(changed) + b"\n", ARGS, initial
            for i in range(1, len(tokens)):
                data = b" ".join(tokens[:i]) + value + b" ".join(tokens[i:]) + b"\n"
                yield "{}/space-{}/{}".format(site, i, label), "mopac." + script, data, ARGS, initial
        for ending in (b"", b"\r\n", b"\r"):
            yield site + "/ending-" + ending.hex(), "mopac." + script, clean[:-1] + ending, ARGS, initial
    for label, value in mutations + [("zero", b"0"), ("one", b"1"), ("negative", b"-1"), ("fraction", b"0.5")]:
        for theory in ("PM7", "UPM7", "uPM7", "ÅPM7"):
            args = [".", "Energy", "SE", theory, value.decode("utf-8", "replace")]
            yield "open-shells/{}/{}".format(label, theory), "mopac.orbocc", b"1 C 2.0000 1.0000 0.0000 0.0000\n", args, {}
    for label, value in mutations:
        args = [".", "Energy", "SE", value.decode("utf-8", "replace"), "0"]
        yield "theory/" + label, "mopac.orbocc", b"1 C 2.0000\n", args, {}
    extras = {
        "egradvec": [b"1 2 O CARTESIAN Z 1.000 -0.000\n", b"1 1 O CARTESIAN Y 1.000 -0.000\n", b"1 11 O CARTESIAN X 1.000 1.234567890123456789\n"],
        "orbocc": [b"1 C 1.04478 1.18610 1.18610 1.18609\n2 H 0.84924\n", b"1 C -1.0000\n", b"1 C 0.2500\n", b"1 C 4.0000\n", b"1 C 2.0000\r3.0000\n"],
        "solvmodel": [b"PM7 EPS=7.84D+1\n", b"PM7 EPS=1e999\n", b"PM7 EPS=.5\n", b"PM7 EPS=78.4\nPM7 EPS=2\n", b"PM7 EPS=1.234567890123456789\n", b"PM7 EPS=1e-999\n"],
    }
    for script, inputs in extras.items():
        for i, data in enumerate(inputs):
            yield "extra/{}/{}".format(script, i), "mopac." + script, data, ARGS, {}
    for script in NAMES:
        yield "empty/" + script, "mopac." + script, b"", ARGS, {}


def cases():
    return list(fixture_cases()) + list(crafted_cases())


def run(case, scripts=SCRIPTS, interpreter=None):
    name, script, data, args, initial = case
    with tempfile.TemporaryDirectory(prefix="ecce-mopac-a-") as work:
        for path, contents in initial.items():
            Path(work, path).write_bytes(contents)
        cmd = [str(scripts / script)] + list(args)
        if interpreter:
            cmd.insert(0, interpreter)
        proc = subprocess.run(cmd, input=data, cwd=work, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
        files = {str(p.relative_to(work)): p.read_bytes().decode("latin1")
                 for p in sorted(Path(work).rglob("*")) if p.is_file()}
        return dict(stdout=proc.stdout.decode("latin1"), stderr=proc.stderr.decode("latin1"),
                    rc=proc.returncode, files=files)


def main():
    with open(str(GOLDEN), encoding="utf-8", errors="replace") as stream:
        golden = json.load(stream)
    inputs = cases()
    assert len(inputs) == len(golden)
    counts = {}
    for case in inputs:
        name, script = case[:2]
        actual = run(case, interpreter=sys.executable if sys.platform == "win32" else None)
        assert actual == golden[name], "{}: {}\nactual: {!r}\nPerl: {!r}".format(script, name, actual, golden[name])
        counts[script] = counts.get(script, 0) + 1
    # The invalid-byte/NBSP fixture must retain every clean sibling value.
    for name in golden:
        if name.startswith("real/h2o_cosmo_bytes/"):
            clean = name.replace("real/h2o_cosmo_bytes/", "real/h2o_cosmo/")
            assert golden[name] == golden[clean], name
    for script, count in sorted(counts.items()):
        print("PASS {}: {} Perl-anchored stdout/stderr/rc/file comparisons".format(script, count))
    return 0


if __name__ == "__main__":
    sys.exit(main())
