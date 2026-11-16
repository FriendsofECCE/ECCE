#!/usr/bin/env python3
"""Bounded FileEDSI reads and the exact SSH-side scripts, without a login.

Run through srun. strace verifies bytes actually read, including a request
smaller than a stream buffer; output length alone cannot catch a whole-file read.
"""
import base64
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BUILD = Path(os.environ.get("ECCE_TEST_BUILD", ROOT / "build-cmake"))
LIBS = ["eccedsi", "eccexml", "eccetdat", "eccedav", "eccefaces",
        "eccecipc", "ecceutil", "eccecomm", "eccercmd"]


def main():
    if not shutil.which("strace"):
        print("SKIP strace unavailable")
        return 77
    with tempfile.TemporaryDirectory(prefix="ecce-range-") as tmp:
        tmp = Path(tmp)
        driver = tmp / "rangeTest"
        deps = subprocess.check_output(["pkg-config", "--libs", "hdf5", "libssh"], text=True).split()
        subprocess.run(["g++", "-std=c++17", "-I" + str(ROOT / "include"),
                        str(ROOT / "tests/filedsi/rangeTest.C"), "-L" + str(BUILD),
                        *["-l" + lib for lib in LIBS] * 3,
                        "-lxerces-c", "-lmosquitto", "-lssl", "-lcrypto", *deps,
                        "-o", str(driver)], check=True)
        path = tmp / "a file's output.gbw"
        data = (bytes(range(256)) * 40960)  # 10 MiB, NULs and CR/LF included.
        path.write_bytes(data)
        env = dict(os.environ, HOME=str(tmp), ECCE_REALUSERHOME=str(tmp),
                   ECCE_REALUSER="tester", ECCE_HOME=str(ROOT))
        env.pop("ECCE_LOCAL_DATA", None)

        def call(*args):
            return subprocess.check_output([str(driver), *map(str, args)], env=env)

        trace_count = 0

        def traced(command, script=None):
            nonlocal trace_count
            trace_count += 1
            trace = tmp / ("trace" + str(trace_count))
            out = subprocess.check_output(["strace", "-ff", "-yy", "-e", "trace=read,readv,pread64",
                                           "-o", str(trace), *command], input=script, env=env)
            reads = re.findall(r"(?:read|readv|pread64)\(\d+<" + re.escape(str(path)) +
                               r">.*?= (\d+)", "\n".join(p.read_text() for p in tmp.glob(trace.name + ".*")))
            return out, sum(map(int, reads))

        size, read = traced([str(driver), "size", str(path)])
        assert int(size) == len(data) and read == 0, (size, read)
        print("PASS local stat reads zero content bytes")
        for at, n in ((len(data) - 65553, 65553), (12345, 17),
                      (len(data) - 4, 20), (len(data) + 1, 20), (0, 0)):
            out, read = traced([str(driver), "range", str(path), str(at), str(n)])
            want = data[at:at+n]
            assert out == want and read == len(want), (at, n, len(out), read)
        print("PASS local 10 MiB ranges: actual bytes read equal range, EOF and empty")
        failed = subprocess.run([str(driver), "range", str(path), "-1", "10"], env=env)
        assert failed.returncode == 1
        print("PASS negative local offset refused")
        marker = b"\nECCE_RANGE_OK"
        for at, n in ((len(data) - 257, 257), (len(data) - 4, 20),
                      (len(data) + 1, 20), (0, 0)):
            script = call("script", path, at, n)
            out, read = traced(["sh"], script)
            decoded = base64.b64decode(out)
            want = data[at:at+n]
            assert decoded == want + marker and read == len(want), (at, n, read)
        print("PASS SSH range script: bounded seek, binary/CRLF, quoted path, EOF, empty")
        out, read = traced(["sh"], call("size-script", path))
        assert int(out) == len(data) and read == 0
        print("PASS SSH stat script reads zero content bytes")
        missing = call("script", tmp / "missing", 0, 10)
        assert subprocess.check_output(["sh"], input=missing) == b""
        print("PASS SSH missing file has no success marker")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
