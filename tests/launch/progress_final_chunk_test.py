#!/usr/bin/env python3
"""The live monitor's last progress snapshot must include the output's last
lines (#722).

A CP2K CELL_OPT ends with "GEOMETRY OPTIMIZATION COMPLETED", the last block of
the file, which cp2k.desc turns into opt converged=yes.  Runs the real
scripts/eccejobmonitor live with cp2k.desc on a Shell job that writes the
output in pieces, the banner (once cut in the middle of a line) in the final
piece, and checks the last jmPROG snapshot.

    tests/launch/progress_final_chunk_test.py
"""

import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
MONITOR = os.path.join(REPO, "scripts", "eccejobmonitor")
DESC = os.path.join(REPO, "scripts", "parsers", "cp2k.desc")
FIXTURE = os.path.join(REPO, "tests", "parsers", "fixtures", "cp2k",
                       "opt_2023_steps.cp2kout")

CONF = """host localhost
calcName test
ecceVersion test
monitoringMode live
jobQ Shell
commType stdio
jobOutputFile job.out
jobOutputFile2 job.out
parseTypes ALL
mdTask no
mdPrepareTask no
mdBatchOutput no
mdPMFOutput no
logMode yes
parseDescriptorFile cp2k.desc
timePauseJobExist 1
"""

LAST_STEP = """ --------  Informations at step =     4 ------------
  Optimization Method        =                LBFGS
  Total Energy               =     -4863.8101000000
  Max. gradient              =         0.0001000000
  Conv. limit for gradients  =         0.0004500000
  Conv. in gradients         =                  YES
  RMS gradient               =         0.0001000000
  Conv. limit for RMS grad.  =         0.0003000000
  Conv. in RMS gradients     =                  YES
  Conv. for PRESSURE         =                  YES
 ---------------------------------------------------
"""

BANNER = """
 *******************************************************************************
 ***                    GEOMETRY OPTIMIZATION COMPLETED                      ***
 *******************************************************************************
"""


def run(tail_cut):
    work = tempfile.mkdtemp(prefix="ecce-finalchunk-")
    try:
        shutil.copy(DESC, os.path.join(work, "cp2k.desc"))
        with open(os.path.join(work, "eccejobmonitor.conf"), "w") as f:
            f.write(CONF)
        body = open(FIXTURE).read() + LAST_STEP
        pieces = [body[:len(body) // 2], body[len(body) // 2:]]
        cut = BANNER.index("GEOMETRY") + 5 if tail_cut else len(BANNER)
        pieces += [BANNER[:cut], BANNER[cut:]]

        def writer():
            with open(os.path.join(work, "job.out"), "w") as f:
                for p in pieces:
                    if not p:
                        continue
                    f.write(p)
                    f.flush()
                    time.sleep(1.5)
            with open(os.path.join(work, ".ecce.status"), "w") as f:
                f.write("0\n")

        job = subprocess.Popen(["sh", "-c", "sleep 12"], cwd=work)
        threading.Thread(target=job.wait, daemon=True).start()
        threading.Thread(target=writer, daemon=True).start()
        mon = subprocess.run(
            ["perl", MONITOR, "-configFile", "eccejobmonitor.conf",
             "-jobId", str(job.pid), "-bookmark", "0", "-progress", "1"],
            cwd=work, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, timeout=90)
        return mon.returncode, mon.stdout.replace(b"\x00", b" ").decode(
            errors="replace")
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    if not shutil.which("perl") or not shutil.which("ps"):
        print("SKIP: needs perl and ps")
        return 77
    failures = []
    for tail_cut in (False, True):
        rc, text = run(tail_cut)
        snaps = re.findall(r"jmPROG\s+(.*?)(?=\x01|\Z)", text, re.S)
        last = snaps[-1] if snaps else ""
        opt = [l for l in re.split(r"[\n\x02]", last) if l.startswith("opt ")]
        ok = bool(opt) and "converged=yes" in opt[0]
        print("%s last snapshot has converged=yes (banner %s): %s" %
              ("ok   " if ok else "FAIL ", "cut mid-line" if tail_cut
               else "whole", opt[:1]))
        if not ok:
            failures.append(tail_cut)
            print(text[-1500:])
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
