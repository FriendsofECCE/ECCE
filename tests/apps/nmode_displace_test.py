#!/usr/bin/env python3
"""Vibrations panel > Displace (#618): new calculations along a mode.

    tests/apps/nmode_displace_test.py

A Builder opens the Gaussian 16 water frequency calculation (the only
fixture with stored normal modes) with the frequencies of modes 1 and 3
negated, so there are two imaginary modes.  The panel's displace action runs
for a real mode (row 1, which also checks the row index), an imaginary mode
and all imaginary modes.  For each new calculation the test compares:

  * its structure with ecce-displace's output for the modes the test read
    from the fixture files, to 1e-5 Angstrom;
  * its annotation with the tool's, naming the mode and frequency;
  * its run type with Geometry, the name every code's optimisation uses
    (scripts/codereg/*runtype.py);
  * that it sits in the source's folder and carries none of the frequency
    step's ES.Runtype.Vibration settings, which the source has.

No ORCA fixture stores normal modes; the ORCA arithmetic is in tests/displace.
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import fixture    # noqa: E402

BASE = os.path.join(HERE, "fixtures", "calc-water-vib")
NAME = "nmode-displace-water"
CLI = os.path.join(ROOT, "scripts", "ecce_displace.py")
#  (scene arguments, CLI arguments, number of new calculations)
CASES = [("1 0.15 both", ["--mode", "2", "--amplitude", "0.15", "--sign", "both"], 2),
         ("2 0.2 +", ["--mode", "3", "--amplitude", "0.2", "--sign", "+"], 1),
         ("all 0.1 -", ["--imaginary", "--amplitude", "0.1", "--sign", "-"], 1)]
MARKERS = ("ended by SIG", "Segmentation fault", "double free or corruption",
           "terminate called", "Unhandled standard exception")
fails = []


def check(name, ok, detail=""):
    print(("PASS  " if ok else "FAIL  ") + name + ("" if ok else "  " + str(detail)))
    if not ok:
        fails.append(name)


def floats(text):
    return [float(t) for t in text.split()]


def modesFile(path, els, xyz, props):
    freqs = floats(re.search(r">([^<]*)</vector>", open(os.path.join(props, "VIBFREQ")).read()).group(1))
    vib = open(os.path.join(props, "VIB")).read()
    mats = [floats(m) for m in re.findall(r"<matrix number=\"\d+\">([^<]*)</matrix>", vib)]
    n = len(els)
    with open(path, "w") as f:
        f.write("%d\n" % n)
        for a in range(n):
            f.write("%s %s %s %s\n" % (els[a], xyz[3 * a], xyz[3 * a + 1], xyz[3 * a + 2]))
        f.write("modes %d\n" % len(freqs))
        for fr, m in zip(freqs, mats):
            f.write("%s\n" % fr)
            for a in range(n):
                f.write("%s %s %s\n" % tuple(m[3 * a:3 * a + 3]))


def run(display, args, work, src):
    script = os.path.join(work, "scene")
    with open(script, "w") as h:
        h.write("\n".join(["vibfocus"] + ["vibdisplace " + c[0] for c in CASES] + [""]))
    result = apps.run(display, "builder", args=args, windowTimeout=120, settle=120,
                      env={"ECCE_VIEWER_SCENE": script, "ECCE_VIEWER_SCENE_OUT": work,
                           "ECCE_REALUSER": fixture.USER})
    log = result.log or ""
    if result.crashed or os.path.exists(os.path.join(work, "FAILED")) or \
            not result.exitedAfterWindow or any(m in log for m in MARKERS):
        check("builder ran the displace script", False, "\n".join(log.splitlines()[-20:]))
        return None
    return log


def verify(log, work, src):
    source = re.search(r"VIBSOURCE: url=(\S*) els=(.*?) xyz=(.*)$", log, re.M)
    if not source:
        return check("VIBSOURCE line", False, log[-800:])
    url, els, xyz = source.group(1), source.group(2).split(), floats(source.group(3))
    modes = os.path.join(work, "modes")
    modesFile(modes, els, xyz, os.path.join(src, "Props"))
    name = url.rstrip("/").split("/")[-1]
    blocks = re.split(r"^VIBDISPLACE: ", log, flags=re.M)[1:]
    check("three displace commands ran", len(blocks) == 3, len(blocks))
    for (scene, cliArgs, count), block in zip(CASES, blocks):
        tag = "vibdisplace " + scene
        got = re.findall(r"VIBDISPLACED: url=(\S*) vibkeys=(\d) runtype=(\S*) note=(.*?) xyz=(.*)$",
                         block, re.M)
        check(tag + ": error empty, %d new" % count,
              block.startswith("error= n=%d" % count) and len(got) == count, block[:120])
        want = subprocess.run([sys.executable, CLI, modes, "--source", name, "--lines"] + cliArgs,
                              capture_output=True, text=True, check=True).stdout.splitlines()
        n = len(els)
        for k, (u, vibkeys, runtype, note, nxyz) in enumerate(got):
            chunk = want[k * (n + 1):(k + 1) * (n + 1)]
            exp = floats(" ".join(chunk[1:]))
            have = floats(nxyz)
            err = max(abs(a - b) for a, b in zip(exp, have)) if len(exp) == len(have) else 9
            check(tag + " #%d structure = ecce-displace" % k, err < 1e-5, err)
            check(tag + " #%d annotation" % k, note == chunk[0][5:], (note, chunk[0]))
            check(tag + " #%d run type Geometry" % k, runtype == "Geometry", runtype)
            check(tag + " #%d next to the source" % k,
                  u.rsplit("/", 1)[0] == url.rsplit("/", 1)[0] and u != url, (u, url))
            check(tag + " #%d no Vibration settings" % k, vibkeys == "0", vibkeys)


def main():
    import isolate
    import xdisplay
    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    isolate.killLeftovers(settings["ECCE_REALUSERHOME"])
    os.environ["ECCE_NO_REAP"] = "1"
    work = tempfile.mkdtemp(prefix="ecce-nmode-displace-")
    src = os.path.join(work, NAME)
    shutil.copytree(BASE, src, symlinks=True)
    vf = os.path.join(src, "Props", "VIBFREQ")
    text = open(vf).read()
    freqs = floats(re.search(r">([^<]*)</vector>", text).group(1))
    new = "  ".join(repr(-f if i != 1 else f) for i, f in enumerate(freqs))
    open(vf, "w").write(re.sub(r">[^<]*</vector>", ">" + new + "</vector>", text))
    try:
        with xdisplay.Display() as display:
            log = []
            apps.startServices(display, log)
            try:
                if not all(apps.serviceState(display).values()):
                    print("FAIL  services did not come up: %s" % log)
                    return 1
                fixture.ensureRealUserAccount()
                restore = fixture.settleUpgradeNotices()
                try:
                    url, error = fixture.install(NAME, source_dir=src)
                    if error:
                        print("FAIL  install: %s" % error)
                        return 1
                    try:
                        args = ("-context", url)
                        state = fixture.stateHome()
                        if os.path.realpath(state) != os.path.realpath(os.path.expanduser("~")):
                            auth = fixture.authFile(os.path.join(state, ".ECCE", "auth.pipe"),
                                                    user=fixture.USER)
                            args = ("-pipe", auth) + args
                        out = run(display, args, work, src)
                        if out is not None:
                            verify(out, work, src)
                    finally:
                        fixture.remove(NAME)
                finally:
                    restore()
            finally:
                apps.stopServices(display)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print("FAILED: %s" % ", ".join(fails) if fails else "PASS  nmode displace")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
