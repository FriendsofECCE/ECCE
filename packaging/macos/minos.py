#!/usr/bin/env python3
"""minos.py APP [OUT.txt] [--floor X.Y] [--arch ARCH]: architecture and
minimum macOS of every Mach-O in an .app, then the highest minimum and the
files that set it.  With --floor / --arch the exit status is 1 when any file
needs a newer macOS than the floor or is not exactly the given architecture."""
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
    args = sys.argv[1:]
    opts = {}
    for o in ("--floor", "--arch"):
        if o in args:
            i = args.index(o)
            opts[o] = args[i + 1]
            del args[i:i + 2]
    app = args[0]
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
    bad = []
    if "--floor" in opts:
        fl = key(opts["--floor"])
        bad += ["%s: minos %s above floor %s" % (r[0], r[2], opts["--floor"])
                for r in rows if not r[2] or key(r[2]) > fl]
    if "--arch" in opts:
        bad += ["%s: architecture %s, wanted %s" % (r[0], r[1], opts["--arch"])
                for r in rows if r[1] != opts["--arch"]]
    if bad:
        text += "\nFAILED: %d violations\n" % len(bad) + "\n".join(bad[:60]) + "\n"
    if len(args) > 1:
        open(args[1], "w").write(text)
    print(text)
    if bad:
        sys.exit(1)


main()
