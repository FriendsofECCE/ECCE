#!/usr/bin/env python3
"""An xtb run made outside ECCE is imported through xtb.expt (Organizer >
File > Import).  xtb prints no coordinates and keeps orbitals, charges and
modes in files beside the output, so the importer reads them from the
output's directory (as the import's symbolic link resolves to it) and
appends them in the form gensub does, replacing the link.  Checked here: the
structure and settings, the original left alone, a stale file with another
atom count ignored, and that the real parse specification then delivers every
property a run launched from ECCE gets.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from eccejobmonitor_sim import read_desc, replay  # noqa: E402

REPO = os.path.dirname(os.path.dirname(HERE))
EXPT = os.path.join(REPO, "scripts", "parsers", "xtb.expt")
DESC = os.path.join(REPO, "scripts", "parsers", "xtb.desc")
FIX = os.path.join(HERE, "fixtures", "xtb")
EXT = os.path.join(FIX, "external")

failures = []


def check(ok, msg):
    print(("ok   " if ok else "FAIL ") + msg)
    if not ok:
        failures.append(msg)


def scratch():
    return tempfile.mkdtemp(prefix="xtbimp-", dir=os.environ.get("TMPDIR"))


def do_import(srcdir, out, work):
    """What JobParser::importCalculation does: link the file into a fresh
    directory and run the importer there."""
    link = os.path.join(work, out)
    os.symlink(os.path.join(srcdir, out), link)
    env = dict(os.environ, ECCE_HOME=REPO)
    r = subprocess.run(["perl", EXPT, out], cwd=work, env=env,
                       capture_output=True, text=True)
    return r, link


def types_delivered(path):
    desc = read_desc(DESC)
    res = replay(desc, path)
    got = set()
    for e in desc.live_entries():
        if res.delivered_for(e):
            got.update(t for t in e.type.replace("[", "").split("]") if t)
    return got


def run(name, out, theory, runtype, natoms, need, absent=()):
    tmp = scratch()
    try:
        src = os.path.join(tmp, "src")
        shutil.copytree(os.path.join(EXT, name), src)
        before = open(os.path.join(src, out), encoding="utf-8", errors="replace").read()
        work = os.path.join(tmp, "job")
        os.mkdir(work)
        r, link = do_import(src, out, work)
        check(r.returncode == 0, "%s: importer exit 0 (%s)" %
              (name, r.stdout.strip() or r.stderr.strip()))
        if r.returncode:
            return
        stem = out.rsplit(".", 1)[0]
        param = open(os.path.join(work, stem + ".param"), encoding="utf-8", errors="replace").read()
        frag = open(os.path.join(work, stem + ".frag"), encoding="utf-8", errors="replace").read()
        check("Theory:\t%s" % theory in param, "%s: theory %s" % (name, theory))
        check("RunType:\t%s" % runtype in param, "%s: run type %s" %
              (name, runtype))
        check("num_atoms:\t%d" % natoms in frag, "%s: %d atoms" %
              (name, natoms))
        check(not os.path.islink(link), "%s: link replaced by a copy" % name)
        check(open(os.path.join(src, out), encoding="utf-8", errors="replace").read() == before,
              "%s: the user's output file untouched" % name)
        got = types_delivered(link)
        for t in need:
            check(t in got, "%s: parse delivers %s" % (name, t))
        for t in absent:
            check(t not in got, "%s: parse delivers no %s" % (name, t))
    finally:
        shutil.rmtree(tmp, True)


run("h2o_ohess", "h2o_ohess.out", "GFN2-xTB", "GeoVib", 3,
    ["TE", "HLGAP", "DIPOLE", "GEOMTRACE", "TEVEC", "MO", "ORBENG", "MULLIKEN",
     "VIBFREQ", "VIBIR", "VIB", "EZEROPT", "EGIBBS"])
run("h2o_sp", "h2o_sp.out", "GFN2-xTB", "Energy", 3,
    ["TE", "GEOMTRACE", "MULLIKEN"], absent=["MO", "VIBFREQ"])

# A leftover molden.input from another molecule must not be attached.
tmp = scratch()
try:
    src = os.path.join(tmp, "src")
    shutil.copytree(os.path.join(EXT, "h2o_sp"), src)
    shutil.copy(os.path.join(FIX, "ch4_gfn2.molden"),
                os.path.join(src, "molden.input"))
    work = os.path.join(tmp, "job")
    os.mkdir(work)
    r, link = do_import(src, "h2o_sp.out", work)
    check(r.returncode == 0 and "ECCE-XTB-MOLDEN" not in open(link, encoding="utf-8", errors="replace").read(),
          "stale molden.input of another molecule ignored")
    # An output without any geometry beside it is refused with the reason.
    os.remove(os.path.join(src, "h2o.xyz"))
    work2 = os.path.join(tmp, "job2")
    os.mkdir(work2)
    r, link = do_import(src, "h2o_sp.out", work2)
    check(r.returncode == 1 and "No molecular geometry" in r.stdout,
          "no geometry beside the output: refused with a reason")
finally:
    shutil.rmtree(tmp, True)

# An output from an ECCE run (frames already appended) is read as it is.
tmp = scratch()
try:
    shutil.copy(os.path.join(FIX, "ch4_opt.out"), tmp)
    r = subprocess.run(["perl", EXPT, "ch4_opt.out"], cwd=tmp,
                       capture_output=True, text=True)
    frag = open(os.path.join(tmp, "ch4_opt.frag"), encoding="utf-8", errors="replace").read()
    check(r.returncode == 0 and "num_atoms:\t5" in frag,
          "ECCE-run output imports from its appended frames")
finally:
    shutil.rmtree(tmp, True)

print("%d failure(s)" % len(failures))
sys.exit(1 if failures else 0)
