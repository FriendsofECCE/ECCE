#!/usr/bin/env python3
"""An imported MOPAC run's <job>.mgf, beside <job>.out, is sent for the
File= parse (mopac.mgf) under its own name, from the import directory, which
need not be the monitor's working directory (an installed example is not);
and that parse yields SLATERBASIS, which the MO composition needs.  Runs the
real scripts/eccejobmonitor and scripts/parsers/mopac.mgf on the bundled
example.  The full path (import into the data server) is checked by
tests/apps/examples_test.py with the example's Props.
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
EX = os.path.join(ROOT, "data", "client", "examples", "getting-started", "mopac-ammonia") + "/"

with tempfile.TemporaryDirectory() as work:
    os.symlink(EX + "ammonia.out", os.path.join(work, "ammonia.out"))
    res = os.path.join(work, "x.res")
    subprocess.run(
        ["perl", os.path.join(ROOT, "scripts", "eccejobmonitor"),
         "-commType", "stdio", "-monitoringMode", "post",
         "-jobOutputFile", "ammonia.out", "-parseDescriptorFile",
         os.path.join(ROOT, "scripts", "parsers", "mopac.desc"),
         "-parseTypes", "all", "-outputFile", res, "-calcName", "ammonia",
         "-logFile", os.path.join(work, "m.log"), "-filesToTransfer", "",
         "-importDir", EX], cwd=work, stdin=subprocess.DEVNULL,
        stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
    sent = b"ammonia.mgf" in open(res, "rb").read()
    ok = not os.path.exists(os.path.join(work, "mopac.mgf"))
    out = subprocess.run(
        ["perl", os.path.join(ROOT, "scripts", "parsers", "mopac.mgf")],
        stdin=open(EX + "ammonia.mgf"), stdout=subprocess.PIPE, text=True,
        check=True).stdout
    slater = "key: SLATERBASIS" in out

for what, good in (("nothing written into the working directory", ok),
                   ("ammonia.mgf sent for the File= parse", sent),
                   ("SLATERBASIS parsed", slater)):
    print(("PASS " if good else "FAIL ") + what)
sys.exit(0 if ok and sent and slater else 1)
