#!/usr/bin/env python3
"""The Builder's open/import file dialog lists the right files per filter.

    tests/look/filedialog.py [--libdir DIR]

Reads the masks from the *Calculation.C inputMasks() (not from
CalculationFactory::openMask()), builds the "All Supported Types" plus
per-type filter string the way OpenCalculationDialog does, lists a scratch
directory under each filter with tests/look/filedialog.C on a private Xvfb,
and checks the names.  Also fails if a top-row widget overlaps another.
"""
import argparse
import fnmatch
import glob
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
LIBS = ["eccewxgui", "eccewxplotctrl", "eccewxthings", "eccewxgui",
        "eccecomm", "eccercmd", "eccedsi", "eccedav", "eccecipc",
        "eccefaces", "eccexml", "eccetdat", "ecceutil"]
FILES = ["benzene.xyz", "water.xyz", "WATER2.XYZ", "glycine.pdb", "glycine.ent",
         "1ABC.PDB", "benzene.car", "nacl_periodic.car", "phenol.mvm",
         "nwchem_water_density.cube", "run.trj", "water.out", "water.log",
         "notes.txt"]


def out(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def masks():
    """label -> glob list, from each Calculation class's inputMasks()."""
    res = {}
    for path in sorted(glob.glob(os.path.join(
            ROOT, "src/apps/builder/*Calculation.C"))):
        src = open(path).read()
        m = re.search(r'::inputMasks\(\)\s*\{\s*return\s*"([^"]*)";', src)
        if m and m.group(1):
            res[os.path.basename(path)[:-2]] = m.group(1).split(";")
    return res


def got_final(lines):
    names, on = [], False
    for ln in lines:
        if ln.startswith("FINAL "):
            on = True
        elif not ln.startswith("  "):
            on = False
        elif on and ln.strip() not in ("..", "sub"):
            names.append(ln.strip())
    return names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--libdir", default=os.path.join(ROOT, "build-cmake"))
    ap.add_argument("--display", default=":184")
    o = ap.parse_args()

    per = masks()
    assert len(per) >= 5, per
    allpats = sorted({p for v in per.values() for p in v})
    wild = "All Supported Types|" + ";".join(allpats)
    labels = ["All Supported Types"]
    expect = [allpats]
    for label, pats in per.items():
        wild += "|%s|%s" % (label, ";".join(pats))
        labels.append(label)
        expect.append(pats)

    work = tempfile.mkdtemp(prefix="filedialog")
    data = os.path.join(work, "data")
    os.makedirs(data)
    for f in FILES:
        open(os.path.join(data, f), "w").close()
    binary = os.path.join(work, "filedialog")
    cxx = out(["wx-config", "--cxxflags"]).stdout.split()
    wxl = out(["wx-config", "--libs", "core,base,adv,html"]).stdout.split()
    srcs = [os.path.join(HERE, "filedialog.C"),
            os.path.join(ROOT, "src/wxgui/ewxClasses/ewxGenericFileDialog.C"),
            os.path.join(ROOT, "src/wxgui/ewxClasses/ewxFileCtrl.C")]
    b = out(["nice", "-n", "19", "g++", "-std=c++17", "-w", "-o", binary]
            + srcs + ["-I" + os.path.join(ROOT, "include")] + cxx
            + ["-L" + o.libdir] + ["-l" + lib for lib in LIBS] + wxl
            + ["-lxerces-c", "-lssh", "-lmosquitto"])
    if b.returncode:
        print(b.stderr[-3000:])
        return 1

    home = os.path.join(work, "home")
    os.makedirs(home)
    open(os.path.join(home, "h.xyz"), "w").close()
    os.makedirs(os.path.join(data, "sub"))
    open(os.path.join(data, "sub", "b.xyz"), "w").close()
    # The settings an old bug left behind: the list's "*" placeholder saved
    # as part of the directory.
    prefs = os.path.join(home, ".ECCE")
    os.makedirs(prefs)
    ini = os.path.join(prefs, "FileDialog.ini")
    typed = {os.path.join(data, "water.xyz"): os.path.join(data, "water.xyz"),
             "water.xyz": os.path.join(data, "water.xyz"),
             "./water.xyz": os.path.join(data, "water.xyz"),
             "sub/b.xyz": os.path.join(data, "sub", "b.xyz"),
             "~/h.xyz": os.path.join(home, "h.xyz")}
    env = dict(os.environ, DISPLAY=o.display, HOME=home, FD_RESTORE="1",
               ECCE_REALUSERHOME=home, ECCE_HOME=ROOT, ECCE_NO_MESSAGING="1")
    xvfb = subprocess.Popen(["Xvfb", o.display, "-screen", "0", "1400x900x24"],
                            stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    failures = 0
    try:
        import time
        time.sleep(2)
        for size in ("default", "300x300"):
            with open(ini, "w") as f:  # each run starts from the stale file
                f.write("DIR=*/%s\nFILENAME=benzene.xyz\nFILTER=0\n[DIRS]\n"
                        "0=*\n[MOUNTS]\n0=/\n" % data)
            r = out([binary, data, wild, size, "50"] + list(typed), env=env,
                    timeout=60)
            lines = r.stdout.splitlines()
            if os.environ.get("FD_DEBUG"): print(r.stderr[-400:])
            got, cur, rows, dlg, ntyped = {}, None, [], None, 0
            restored = False
            for ln in lines:
                if ln.startswith("FILTER "):
                    cur = int(ln.split()[1])
                    got[cur] = []
                elif ln.startswith("FINAL "):
                    cur = "final"
                    got[cur] = []
                    final_label = ln[6:]
                elif ln.startswith("  "):
                    if ln.strip() not in ("..", "sub"):
                        got[cur].append(ln.strip())
                elif ln.startswith("ROW "):
                    m = re.search(r"x=(\d+) y=(\d+) w=(\d+) h=(\d+)", ln)
                    rows.append(tuple(int(v) for v in m.groups()))
                elif ln.startswith("RESTORED "):
                    if os.path.normpath(ln[9:]) != data:
                        print("FAIL: restored directory %r, want %r"
                              % (ln[9:], data))
                        failures += 1
                    restored = True
                elif ln.startswith("TYPED "):
                    name, _, path = ln[6:].partition(" => ")
                    if os.path.normpath(path) != typed[name]:
                        print("FAIL: typed %r gave %r, want %r"
                              % (name, path, typed[name]))
                        failures += 1
                    ntyped += 1
                elif ln.startswith("DIALOG "):
                    dlg = ln
            if got.pop("final", None) is None or final_label != labels[0]:
                print("FAIL: %s: displayed choice %r, want %r"
                      % (size, final_label, labels[0]))
                failures += 1
            elif sorted(got_final(lines)) != sorted(f for f in FILES if any(
                    fnmatch.fnmatchcase(f, p) for p in expect[0])):
                print("FAIL: %s: list does not match displayed %r"
                      % (size, final_label))
                failures += 1
            saved = open(ini).read()
            if not restored or "*" in saved:
                print("FAIL: %s: settings not restored or '*' saved again:\n%s"
                      % (size, saved))
                failures += 1
            if ntyped != len(typed):
                print("FAIL: %s: %d typed names answered, want %d"
                      % (size, ntyped, len(typed)))
                failures += 1
            if len(got) != len(labels):
                print("FAIL: %s: %d filters listed, want %d\n%s" % (
                    size, len(got), len(labels), r.stdout + r.stderr[-500:]))
                failures += 1
                continue
            for i, label in enumerate(labels):
                want = sorted(f for f in FILES if any(
                    fnmatch.fnmatchcase(f, p) for p in expect[i]))
                if sorted(got[i]) != want:
                    print("FAIL: %s filter %r: got %s want %s"
                          % (size, label, sorted(got[i]), want))
                    failures += 1
            for i, a in enumerate(rows):
                for c in rows[i + 1:]:
                    if (a[0] < c[0] + c[2] and c[0] < a[0] + a[2]
                            and a[1] < c[1] + c[3] and c[1] < a[1] + a[3]):
                        print("FAIL: %s: overlapping widgets %s %s"
                              % (size, a, c))
                        failures += 1
            print("%s: %d filters checked; %s" % (size, len(labels), dlg))
    finally:
        xvfb.terminate()
        xvfb.wait()
        shutil.rmtree(work, True)
    print("PASS" if not failures else "%d failure(s)" % failures)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
