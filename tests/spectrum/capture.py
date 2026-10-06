#!/usr/bin/env python3
"""The spectrum panel in the real Builder, against the codes' own numbers (#214).

    tests/spectrum/capture.py --builder build-cmake/builder [--png DIR] [case ...]

For each case this builds a calculation from the real parser scripts over a
real output file (makecalc.py), loads it in the real Builder on a private
Xvfb with the Vibrational Frequencies panel open, and reads back what the
panel's canvas holds (ECCE_SPECTRUM_DUMP).  That is compared with the
frequencies and intensities read straight out of the output file by
oracle.py, which shares no code with the parsers.  The hook also clicks a
peak through the canvas's hit test and checks the table row and the mode
that follow.

Not part of ctest: it needs the installed ECCE for everything but the
builder binary (an overlay tree links to /opt/ecce), several services and
about half a minute per case.  With --png the whole window is also saved.
"""
import argparse
import os
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "tests", "apps"))
sys.path.insert(0, HERE)

import oracle       # noqa: E402
import makecalc     # noqa: E402

#  case -> mode (1-based) whose peak is clicked.
CLICK = {"g16-h2o-optfreq": 2, "g16-co-freq": 1,
         "orca-h2o-freq-raman": 8, "nwchem-h2o-freq": 8}


def overlay(builder, where):
    """An $ECCE_HOME like /opt/ecce's whose builder is `builder`."""
    if os.path.exists(where):
        shutil.rmtree(where)
    os.makedirs(os.path.join(where, "bin"))
    for entry in os.listdir("/opt/ecce"):
        if entry != "bin":
            os.symlink(os.path.join("/opt/ecce", entry),
                       os.path.join(where, entry))
    for entry in os.listdir("/opt/ecce/bin"):
        if entry != "builder":
            os.symlink(os.path.join("/opt/ecce/bin", entry),
                       os.path.join(where, "bin", entry))
    os.symlink(os.path.abspath(builder), os.path.join(where, "bin", "builder"))


def readDump(path):
    sticks, click, axes = [], None, {}
    for line in open(path):
        f = line.split()
        if not f:
            continue
        if f[0] == "stick":
            sticks.append(dict(kind=f[1], mode=int(f[2]), wn=float(f[3]),
                               inten=float(f[4]), irrep=f[5], x=int(f[6]),
                               y=int(f[7])))
        elif f[0] == "click":
            click = line.strip()
        elif f[0] == "axis":
            axes[f[1]] = " ".join(f[2:])
    return sticks, click, axes


def near(a, b):
    return abs(a - b) <= 1e-6 * max(1.0, abs(b))


def compare(case, sticks, click, axes):
    problems = []
    reader = [r for n, r, rel in oracle.CASES if n == case][0]
    rel = [rel for n, r, rel in oracle.CASES if n == case][0]
    want = reader(os.path.join(ROOT, "tests", "parsers", "fixtures", rel))
    for kind in ("ir", "raman"):
        values = want[kind]
        drawn = dict((s["mode"] - 1, s) for s in sticks if s["kind"] == kind)
        if not values or not any(values):
            if drawn:
                problems.append("%s drawn but the output file has none" % kind)
            continue
        expect = [i for i, f in enumerate(want["freq"]) if abs(f) >= 10]
        if sorted(drawn) != expect:
            problems.append("%s modes %s, expected %s"
                            % (kind, sorted(drawn), expect))
        for i in expect:
            s = drawn.get(i)
            if s is None:
                continue
            if not (near(s["wn"], want["freq"][i]) and
                    near(s["inten"], values[i])):
                problems.append("%s mode %d: panel %s / %s, output file "
                                "%s / %s" % (kind, i + 1, s["wn"], s["inten"],
                                             want["freq"][i], values[i]))
            if want["irrep"] and s["irrep"] != want["irrep"][i]:
                problems.append("%s mode %d irrep %s, output file %s"
                                % (kind, i + 1, s["irrep"], want["irrep"][i]))
        xs = [s["x"] for s in sorted(drawn.values(), key=lambda s: s["wn"])]
        if xs != sorted(xs, reverse=True):
            problems.append("%s: high wavenumber is not on the left: %s"
                            % (kind, xs))
    m = CLICK[case]
    if click != ("click mode %d: panel mode %d, table row %d, canvas "
                 "selection %d" % (m, m, m, m)):
        problems.append("click: %s" % click)
    return problems


def run(case, builder, png, timeout):
    import apps
    import fixture
    import isolate
    import xdisplay

    for var in ("ECCE_DATASERVER_PORT", "ECCE_BROKER_PORT"):
        os.environ.pop(var, None)
    os.environ["ECCE_REALUSER"] = fixture.USER
    settings = isolate.apply(apps.INSTALL)
    state = settings["ECCE_REALUSERHOME"]
    isolate.killLeftovers(state)
    os.environ["ECCE_NO_REAP"] = "1"

    calc = os.path.join(state, "calc-" + case)
    makecalc.make(case, calc)
    out = os.path.join(state, "spectrum-%s.dump" % case)

    with xdisplay.Display() as display:
        log = []
        apps.startServices(display, log)
        if not all(apps.serviceState(display).values()):
            apps.stopServices(display)
            return ["services did not come up: %s" % log]
        queues = os.path.join(state, ".ECCE", "Queues")
        os.makedirs(os.path.dirname(queues), exist_ok=True)
        open(queues, "a").close()
        fixture.ensureRealUserAccount()
        restorePrefs = fixture.settleUpgradeNotices()
        url, error = fixture.install(case, source_dir=calc)
        if error:
            apps.stopServices(display)
            restorePrefs()
            return ["fixture install failed: %s" % error]

        env = display.env()
        env["ECCE_OPEN_PANEL"] = "Vibrational Frequencies"
        env["ECCE_SPECTRUM_DUMP"] = out
        env["ECCE_SPECTRUM_CLICK"] = str(CLICK[case])
        env["ECCE_EXIT_AFTER_DUMP"] = "1"
        if png:
            env["ECCE_EXIT_AFTER_DUMP_DELAY_MS"] = "8000"
        auth = os.path.join(state, "auth.pipe")
        fixture.authFile(auth, port=int(settings["ECCE_DATASERVER_PORT"]))
        proc = subprocess.Popen(
            [os.path.join(apps.WRAPPERS, "ecce-builder"), "-pipe", auth,
             "-context", url], env=env, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT, start_new_session=True)

        deadline = time.time() + timeout
        shot = False
        while time.time() < deadline:
            if os.path.exists(out) and os.path.getsize(out) > 0:
                #  The binary that ran must be the one under test.
                if png and not shot:
                    time.sleep(3)
                    subprocess.run(["import", "-display", display.name,
                                    "-window", "root", png], env=env,
                                   check=False)
                    shot = True
                if not png:
                    break
            if proc.poll() is not None:
                break
            time.sleep(0.5)
        exe = ""
        try:
            exe = os.readlink("/proc/%d/exe" % proc.pid)
        except OSError:
            pass
        try:
            proc.wait(timeout=20)
        except subprocess.TimeoutExpired:
            for sig in (15, 9):
                try:
                    os.killpg(os.getpgid(proc.pid), sig)
                    proc.wait(timeout=10)
                    break
                except Exception:
                    pass
        tail = (proc.stdout.read() or b"").decode("utf-8", "replace")[-3000:]
        apps.stopServices(display)
        restorePrefs()

    if not os.path.exists(out):
        return ["no dump written; builder said:\n" + tail]
    sticks, click, axes = readDump(out)
    problems = compare(case, sticks, click, axes)
    if png and os.path.exists(out + ".png"):
        shutil.copy(out + ".png", png[:-4] + "-canvas.png")
    print("  binary: %s" % os.path.realpath(builder))
    print("  axes: %s" % axes)
    print("  %s" % click)
    return problems


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("cases", nargs="*")
    ap.add_argument("--builder", default="build-cmake/builder")
    ap.add_argument("--png", metavar="DIR")
    ap.add_argument("--timeout", type=float, default=120)
    args = ap.parse_args()

    builder = os.path.abspath(args.builder)
    if not os.access(builder, os.X_OK):
        print("no builder at %s" % builder)
        return 2
    state = os.path.join(os.path.dirname(builder), "spectrum-state")
    os.environ["ECCE_TEST_STATE"] = state
    home = os.path.join(os.path.dirname(builder), "spectrum-home")
    overlay(builder, home)
    os.environ["ECCE_TEST_HOME"] = home
    if args.png:
        os.makedirs(args.png, exist_ok=True)

    failed = 0
    for case in args.cases or sorted(CLICK):
        print(case)
        png = os.path.join(args.png, "live-%s.png" % case) if args.png else None
        problems = run(case, builder, png, args.timeout)
        for p in problems:
            print("  FAIL  " + p)
        failed += bool(problems)
        if not problems:
            print("  matches the output file")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
