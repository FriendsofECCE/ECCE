#!/usr/bin/env python3
"""Run generated water-dimer counterpoise inputs and verify electron counts."""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("generator", help="counterpoise_test executable")
    parser.add_argument("--orca", default=os.environ.get("ECCE_TEST_ORCA"))
    args = parser.parse_args()
    candidates = [args.orca] if args.orca else []
    orca = next((os.path.abspath(p) for p in candidates if os.access(p, os.X_OK)), None)
    if not orca:
        print("SKIP: chemical ORCA unavailable; set ECCE_TEST_ORCA or --orca")
        return 77
    generator = os.path.abspath(args.generator)
    # Keep run files beside the test executable, inside the build worktree.
    work = Path(tempfile.mkdtemp(prefix="counterpoise-orca-", dir=Path(generator).parent))
    failures = 0
    try:
        inputs = work / "inputs"
        subprocess.run([generator, "--write-inputs", str(inputs)], cwd=work, check=True)
        for filename, electrons in (("complex.inp", 20), ("fragment-1.inp", 10),
                                    ("fragment-2.inp", 10)):
            output = inputs / (Path(filename).stem + ".out")
            with output.open("w") as handle:
                result = subprocess.run([orca, filename], cwd=inputs, stdout=handle,
                                        stderr=subprocess.STDOUT, timeout=600)
            text = output.read_text(errors="replace")
            counts = re.findall(r"Number of Electrons\s+(?:NEL\s+)?\.*\s*[:=]?\s*(\d+)", text,
                                flags=re.IGNORECASE)
            ok = (result.returncode == 0 and "ORCA TERMINATED NORMALLY" in text and
                  counts and int(counts[-1]) == electrons)
            print("%s: %s terminates normally with %d electrons (reported %s)" %
                  ("PASS" if ok else "FAIL", filename, electrons, counts))
            if not ok:
                failures += 1
                print(text[-6000:])
    finally:
        shutil.rmtree(work)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
