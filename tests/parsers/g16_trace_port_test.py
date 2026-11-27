#!/usr/bin/env python3
"""Check the bilingual periodic table and optionally replay the Perl oracle.

    python3 tests/parsers/g16_trace_port_test.py
    python3 tests/parsers/g16_trace_port_test.py --original TREE --artifacts DIR

TREE contains the original scripts/parsers from git. Artifacts retain stdout
and every side file after every invocation, including files later consumed.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from eccejobmonitor_sim import read_desc, replay

REPO = Path(__file__).resolve().parents[2]
SCRIPTS = REPO / "scripts/parsers"
CALLERS = ("gaussian-16.geomtrace", "gaussian-16.geomtrace-input",
           "gaussian-16.geomtrace-flush")
INPUTS = [
    ("sn2-reactant", "data/client/examples/workflows/sn2-cl-ch3br/g16-reactant/sn2_reactant.out", "Geometry"),
    ("sn2-product", "data/client/examples/workflows/sn2-cl-ch3br/g16-product/sn2_product.out", "Geometry"),
    ("sn2-irc", "data/client/examples/workflows/sn2-cl-ch3br/g16-irc/sn2_irc.out", "IRC"),
    ("stray-byte", "tests/parsers/fixtures/gaussian-16/h2o_optfreq_stray_byte.log", "GeoVib"),
    ("nosymm", "tests/parsers/fixtures/gaussian-16/h2co_nosymm_opt.log", "Geometry"),
    ("dummy-zmat", "tests/parsers/fixtures/gaussian-16/nh3_dummy_zmat_opt.log", "Geometry"),
]


def pertab_check():
    spec = importlib.util.spec_from_file_location("pertab", str(SCRIPTS / "pertab.py"))
    pt = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(pt)
    program = r'''
require $ARGV[0]; loadPT();
use JSON::PP;
print encode_json([\@atomicSymbol, \@atomicName, \%symbol_to_name,
                   \%name_to_symbol, \%symbol_to_number, \%number_to_symbol]);
'''
    proc = subprocess.run(["perl", "-e", program, str(SCRIPTS / "pertab.pl")],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True)
    expected = json.loads(proc.stdout.decode("utf-8"))
    actual = [pt.atomicSymbol, pt.atomicName, pt.symbol_to_name,
              pt.name_to_symbol, pt.symbol_to_number,
              {str(k): v for k, v in pt.number_to_symbol.items()}]
    assert actual == expected, "Python and Perl periodic tables differ"
    print("PASS pertab: both arrays and all four maps identical (%d elements + ghost)"
          % (len(pt.atomicSymbol) - 1))


def snapshot(work):
    # Length framing retains names, contents and absence without ambiguity.
    data = bytearray()
    for path in sorted(work.iterdir()):
        if path.is_file():
            name, body = path.name.encode("utf-8"), path.read_bytes()
            data.extend(("%d %d\n" % (len(name), len(body))).encode("ascii"))
            data.extend(name + body)
    return bytes(data)


def oracle(original, artifacts):
    artifacts.mkdir(parents=True, exist_ok=True)
    desc = read_desc(str(SCRIPTS / "gaussian-16.desc"))
    totals = {name: 0 for name in CALLERS}
    for name, source, runtype in INPUTS:
        with tempfile.TemporaryDirectory(prefix="ecce-g16-port-") as scratch:
            scratch = Path(scratch)
            # Preserve raw bytes through the simulator's permissive text reader.
            fixture = scratch / "input.log"
            fixture.write_text((REPO / source).read_bytes().decode("latin1"), encoding="utf-8")
            result = replay(desc, str(fixture))
            blocks = sorted((b for b in result.blocks if b.delivered),
                            key=lambda b: b.begin_line)
            outputs = {}
            for language, tree in (("perl", original), ("python", REPO)):
                work = scratch / language
                work.mkdir()
                env = dict(os.environ, ECCE_HOME=str(tree))
                streams = {script: bytearray() for script in CALLERS}
                states = {script: bytearray() for script in CALLERS}
                counts = {script: 0 for script in CALLERS}
                for block in blocks:
                    script = block.entry.script
                    path = tree / "scripts/parsers" / script
                    # Unported peers still run in both pipelines, in file order.
                    interpreter = "python3" if "python" in path.read_text(errors="replace").splitlines()[0] else "perl"
                    proc = subprocess.run([interpreter, str(path), ".", runtype, "SCF", "RHF", "0"],
                                          input=block.text.encode("latin1"), cwd=str(work), env=env,
                                          stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
                    assert proc.returncode == 0, (name, language, script, proc.stderr)
                    if script in CALLERS:
                        streams[script].extend(proc.stdout)
                        state = snapshot(work)
                        states[script].extend(("%d %d\n" % (block.begin_line, len(state))).encode("ascii") + state)
                        counts[script] += 1
                    else:
                        # This includes IRC consumers reading Python's side files.
                        streams.setdefault("peers", bytearray()).extend(proc.stdout)
                        state = snapshot(work)
                        states.setdefault("peers", bytearray()).extend(
                            ("%d %d\n" % (block.begin_line, len(state))).encode("ascii") + state)
                outputs[language] = streams, states
                target = artifacts / name / language
                target.mkdir(parents=True, exist_ok=True)
                for script in streams:
                    (target / (script + ".stdout")).write_bytes(streams[script])
                    (target / (script + ".sidefiles")).write_bytes(states[script])
                final = target / "final"
                final.mkdir(exist_ok=True)
                for file in work.iterdir():
                    if file.is_file():
                        (final / file.name).write_bytes(file.read_bytes())
            assert outputs["perl"] == outputs["python"], name
            for script in CALLERS:
                totals[script] += counts[script]
            print("PASS %s: stdout and every side-file snapshot identical; calls %s" % (name, counts))
    assert all(totals.values()), totals
    print("PASS all three callers exercised: %s" % totals)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path)
    parser.add_argument("--artifacts", type=Path)
    args = parser.parse_args()
    pertab_check()
    if args.original:
        if not args.artifacts:
            parser.error("--original requires --artifacts")
        oracle(args.original.resolve(), args.artifacts.resolve())


if __name__ == "__main__":
    main()
