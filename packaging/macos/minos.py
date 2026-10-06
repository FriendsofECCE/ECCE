#!/usr/bin/env python3
"""minos.py APP [OUT.txt]: architecture and minimum macOS of every Mach-O in
an .app, then the highest minimum and the files that set it.  Measures only;
the deployment target is decided elsewhere."""
import os
import re
import subprocess
import sys


def macho(path):
    try:
        with open(path, "rb") as f:
            return f.read(4) in (b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe",
                                 b"\xfe\xed\xfa\xcf")
    except OSError:
        return False


def info(path):
    """(archs, minos) from otool -l; minos is the largest over slices."""
    arch = subprocess.run(["lipo", "-archs", path], capture_output=True,
                          text=True).stdout.strip() or "?"
    txt = subprocess.run(["otool", "-l", path], capture_output=True,
                         text=True).stdout
    vers = re.findall(r"LC_BUILD_VERSION.*?minos (\S+)", txt, re.S)
    vers += re.findall(r"LC_VERSION_MIN_MACOSX.*?version (\S+)", txt, re.S)
    vers = [v for v in vers if re.match(r"^\d+(\.\d+)*$", v)]
    key = lambda v: tuple(int(x) for x in v.split("."))
    return arch, (max(vers, key=key) if vers else None)


def main():
    app = sys.argv[1]
    rows = []
    for d, _, files in os.walk(app):
        for n in files:
            p = os.path.join(d, n)
            if not os.path.islink(p) and macho(p):
                rows.append((os.path.relpath(p, app),) + info(p))
    key = lambda v: tuple(int(x) for x in v.split("."))
    rows.sort(key=lambda r: (key(r[2]) if r[2] else (0,), r[0]), reverse=True)
    lines = ["%-8s %-10s %s" % ("minos", "arch", "file")]
    lines += ["%-8s %-10s %s" % (r[2] or "none", r[1], r[0]) for r in rows]
    vs = [r[2] for r in rows if r[2]]
    if vs:
        top = max(vs, key=key)
        culprits = [r[0] for r in rows if r[2] == top]
        lines += ["", "Mach-O files: %d" % len(rows),
                  "architectures: %s" % ", ".join(sorted({r[1] for r in rows})),
                  "maximum minos: %s (set by %d files)" % (top, len(culprits))]
        lines += ["  " + c for c in culprits[:40]]
        dist = {}
        for v in vs:
            dist[v] = dist.get(v, 0) + 1
        lines += ["minos distribution: " + ", ".join(
            "%s x%d" % (v, dist[v]) for v in sorted(dist, key=key, reverse=True))]
    else:
        lines += ["", "no minos found"]
    text = "\n".join(lines) + "\n"
    if len(sys.argv) > 2:
        open(sys.argv[2], "w").write(text)
    print(text)


main()
