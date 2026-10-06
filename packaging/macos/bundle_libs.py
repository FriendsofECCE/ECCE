#!/usr/bin/env python3
"""Copy the non-system dylibs a tree of Mach-O files needs into a
Frameworks directory and point every reference at them.

    bundle_libs.py <Frameworks dir> <root> [<root> ...]

Every Mach-O file under a root gets an @loader_path rpath to the
Frameworks directory, copied libraries an @loader_path one to their
neighbours.  Homebrew's install names are @rpath/... with several
LC_RPATHs, so a reference is resolved through the file's original
rpaths first and by file name in the Homebrew lib directories after.
"""
import os, shutil, subprocess, sys

fw, roots = os.path.abspath(sys.argv[1]), sys.argv[2:]
os.makedirs(fw, exist_ok=True)
SYSTEM = ("/usr/lib/", "/System/")
MAGIC = {b"\xcf\xfa\xed\xfe", b"\xca\xfe\xba\xbe", b"\xce\xfa\xed\xfe"}


def run(*a):
    return subprocess.run(a, check=True, capture_output=True, text=True).stdout


def is_macho(p):
    if os.path.islink(p) or not os.path.isfile(p):
        return False
    with open(p, "rb") as f:
        return f.read(4) in MAGIC


def deps(p):
    lines = run("otool", "-L", p).splitlines()[1:]
    out = [l.split(" (")[0].strip() for l in lines]
    if p.endswith(".dylib") and out:
        out = out[1:]  # a library's first entry is its own id
    return out


ORIG_RPATHS = {}


def rpaths(p):
    """The rpaths the file had before we touched it."""
    if p not in ORIG_RPATHS:
        out, lines = [], run("otool", "-l", p).splitlines()
        for i, l in enumerate(lines):
            if "cmd LC_RPATH" in l:
                out.append(lines[i + 2].split("path ", 1)[1].split(" (")[0])
        ORIG_RPATHS[p] = out
    return ORIG_RPATHS[p]


def reset_rpaths(p, new):
    for r in rpaths(p):
        run("install_name_tool", "-delete_rpath", r, p)
    run("install_name_tool", "-add_rpath", new, p)


brew = run("brew", "--prefix").strip()
search = [brew + "/lib"] + [os.path.join(brew, "opt", n, "lib")
                            for n in sorted(os.listdir(brew + "/opt"))]


def resolve(ref, user):
    if os.path.isabs(ref) and os.path.exists(ref):
        return os.path.realpath(ref)
    name = os.path.basename(ref)
    here = os.path.dirname(user)
    cands = [r.replace("@loader_path", here).replace("@executable_path", here) + "/" + name
             for r in rpaths(user)]
    cands += [d + "/" + name for d in search + sorted(SOURCE_DIRS)]
    for c in cands:
        if os.path.exists(c):
            return os.path.realpath(c)
    sys.exit("cannot resolve %s needed by %s" % (ref, user))


SOURCE_DIRS = set()  # where bundled libraries came from: gcc's libquadmath etc. sit beside libgfortran
done = {}  # real source path -> name in Frameworks


def bundle(user):
    for ref in deps(user):
        if ref.startswith(SYSTEM):
            continue
        if ref.startswith("@") and not ref.startswith("@rpath"):
            continue
        real = resolve(ref, user)
        name = os.path.basename(real)
        if real not in done:
            done[real] = name
            SOURCE_DIRS.add(os.path.dirname(real))
            dst = os.path.join(fw, name)
            shutil.copy2(real, dst)
            os.chmod(dst, 0o755)
            run("install_name_tool", "-id", "@rpath/" + name, dst)
            rpaths(dst)
            bundle(dst)
            reset_rpaths(dst, "@loader_path")
        run("install_name_tool", "-change", ref, "@rpath/" + name, user)


for root in roots:
    for d, _, files in os.walk(root):
        for f in files:
            p = os.path.join(d, f)
            if not is_macho(p):
                continue
            rpaths(p)
            bundle(p)
            reset_rpaths(p, "@loader_path/" + os.path.relpath(fw, d))
print("bundled %d libraries into %s" % (len(done), fw))
