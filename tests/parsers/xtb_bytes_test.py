#!/usr/bin/env python3
"""Replay untrusted-byte decoys against the clean sibling's Perl goldens."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from eccejobmonitor_sim import read_desc, replay

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
SCRIPTS = REPO / "scripts/parsers"


def records(fixture, scripts=SCRIPTS, interpreter=None):
    desc = read_desc(str(SCRIPTS / "xtb.desc"))
    result = replay(desc, str(HERE / "fixtures/xtb" / fixture))
    assert result.runaway is None, fixture
    found = []
    with tempfile.TemporaryDirectory(prefix="ecce-xtb-bytes-") as work:
        for entry in desc.live_entries():
            for block in result.delivered_for(entry):
                # The replayer decodes bytes; these fixtures use only FF as
                # invalid UTF-8. Restore it to exercise the parser's decoder.
                data = block.text.encode("utf-8").replace(b"\xef\xbf\xbd", b"\xff")
                cmd = [str(scripts / entry.script), ".", "GeoVib", "SE", "GFN2-xTB", "0"]
                cmd.insert(0, interpreter or sys.executable)
                proc = subprocess.run(cmd, input=data, stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE, cwd=work)
                found.append(dict(script=entry.script, stdout=proc.stdout.decode("utf-8"),
                                  stderr=proc.stderr.decode("utf-8"), rc=proc.returncode))
        assert not list(Path(work).iterdir()), fixture + ": unexpected written files"
    return found


def main():
    with open(str(HERE / "expected/xtb-bytes.json"), encoding="utf-8") as stream:
        golden = json.load(stream)
    for stem, expected in sorted(golden.items()):
        for suffix in (".out", "_bytes.out"):
            fixture = stem + suffix
            actual = records(fixture)
            assert actual == expected, fixture + ": differs from clean Perl output"
            print("PASS {}: {} blocks, stdout/stderr/rc and written files".format(
                fixture, len(actual)))
    # The descriptor replayer normalizes newlines; exercise the raw pipe too.
    with tempfile.TemporaryDirectory(prefix="ecce-xtb-bytes-") as work:
        proc = subprocess.run([sys.executable, str(SCRIPTS / "xtb.charges")],
                              input=b" -1.250\r -2.500\n", cwd=work,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        assert (proc.stdout, proc.stderr, proc.returncode) == (b"", b"", 0)
        assert not list(Path(work).iterdir())
        print("PASS bare CR: original Perl rejects the complete charge record")
        data = b"x T B\n program call : xtb molecule.xyz\n coordinate file : \xff.xyz\n"
        Path(work, "job.out").write_bytes(data)
        proc = subprocess.run([sys.executable, str(SCRIPTS / "xtb.expt"), "job.out"],
                              cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        expected = (b"No molecular geometry found for job.out: xtb prints none; put "
                    b"xtbopt.log (or the coordinate file \xff.xyz) in the same directory\n")
        assert (proc.stdout, proc.stderr, proc.returncode) == (expected, b"", 1)
        assert Path(work, "job.out").read_bytes() == data
        assert sorted(p.name for p in Path(work).iterdir()) == ["job.out"]
        print("PASS missing geometry: original Perl diagnostic preserves stray byte")
    return 0


if __name__ == "__main__":
    sys.exit(main())
