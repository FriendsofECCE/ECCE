#!/usr/bin/env python3
"""
Energy Calculator H and G at a chosen temperature (#1044), headless.

N2 + 3 H2 -> 2 NH3 from the stored ECCE-QM General Chemistry examples.  The
ECCE_TEST_ORGANIZER hook "compareenergies <A> <B> <treatment> <T>" prints the
table the Energy Calculator shows with its T / K field set to T, and
"comparedialog <A> <B> - <T>" sets the field in the dialog itself.  Checked:
- without T the H row is the codes' own sum, with the S row
  (dH - dG)/T beside it;
- at 500 K and 723 K the H and G rows equal 2 H(NH3) - H(N2) - 3 H(H2) and the
  same for G, each from `ecce-thermo --T <T> --treatment rrho --json` per
  species, and dS = (dH - dG)/T;
- the dialog's T / K field relabels H and G;
- ecce-thermo on the stored N2: Cv = 5/2 R with a tiny vibrational part,
  Cp = Cv + R, U_trans = 3/2 RT and U_rot = RT.
Same installed tree and isolation as run_tests.py.  Exit 77 (skip) without an
install or Xvfb.
"""
import glob
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import apps       # noqa: E402
import run_tests  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(HERE))
PROJECT_META = os.path.join(ROOT, "tools", "screenshots", "data", "project.ecce-meta")
EXAMPLES = os.path.join(ROOT, "data", "client", "examples", "genchem")
KJ = 2625.4996394799
R = 8.314462618
failures = []


def check(ok, what, detail=""):
    print("  %s: %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        if detail:
            print("      " + str(detail)[:2000])
    return ok


def answers(log, command):
    return re.findall(r"ECCE_TEST_ORGANIZER: %s .*?: (ok.*)" % re.escape(command), log)


def numbers(text, prefix):
    for line in text.split(" | "):
        if line.startswith(prefix):
            return [float(x) for x in re.findall(r"-?\d+\.\d+", line)], line
    return [], ""


def thermo(env, urls, T):
    out = subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-thermo")] + urls +
                         ["--json", "--treatment", "rrho", "--T", "%.2f" % T],
                         env=env, capture_output=True, text=True).stdout
    return [c["treatments"]["rrho"] for c in json.loads(out)]


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-thermot-data-")
    work = tempfile.mkdtemp(prefix="ecce-thermot-")
    home = tempfile.mkdtemp(prefix="ecce-thermot-home-")
    try:
        project = os.path.join(data, "users", "local", "haber")
        os.makedirs(project)
        shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
        urls = {}
        for name in ("N2", "H2", "NH3"):
            src = glob.glob(os.path.join(EXAMPLES, "*", "ecceqm-" + name, "stored", name))[0]
            shutil.copytree(src, os.path.join(project, name), symlinks=True)
            urls[name] = "file://" + os.path.join(project, name)
        A = "%s,3*%s" % (urls["N2"], urls["H2"])
        B = "2*%s" % urls["NH3"]
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as f:
            f.write("\n".join([
                "compareenergies %s %s stored" % (A, B),
                "compareenergies %s %s rrho 500" % (A, B),
                "compareenergies %s %s rrho 723" % (A, B),
                "comparedialog %s %s - 723" % (A, B)]) + "\n")
        result = apps.run(display, "organizer", windowTimeout=60, settle=30,
                          env={"ECCE_TEST_ORGANIZER": cmds,
                               "ECCE_ORGANIZER_OPEN": urls["N2"],
                               "ECCE_LOCAL_DATA": data})
        log = result.log or ""
        if result.crashed:
            check(False, "the Organizer ran", "CRASHED (%s)" % result.signalName)
            return
        got = answers(log, "compareenergies")
        dlg = answers(log, "comparedialog")
        if not check(len(got) == 3 and len(dlg) == 1, "every command answered",
                     run_tests._tail(log, 30)):
            return
        for line in got + dlg:
            print("      " + line[:600])
        env = dict(os.environ, ECCE_HOME=apps.INSTALL, ECCE_REALUSERHOME=home)
        order = [urls["N2"], urls["H2"], urls["NH3"]]
        coeff = [-1, -3, 2]
        stored, line = numbers(got[0], "H (298 K)")
        s298, sline = numbers(got[0], "S (298 K)")
        g298, _ = numbers(got[0], "G (298 K)")
        check(len(stored) >= 3 and len(s298) == 1 and len(g298) >= 3 and
              abs(s298[0] - (stored[2] - g298[2]) * 1000 / 298.15) < 0.2,
              "at the frequency temperature: the S row is (dH - dG)/T", got[0])
        for T, text in ((500.0, got[1]), (723.0, got[2])):
            ref = thermo(env, order, T)
            dH = sum(c * r["H"] for c, r in zip(coeff, ref)) * KJ
            dG = sum(c * r["G"] for c, r in zip(coeff, ref)) * KJ
            h, hl = numbers(text, "H (%d K)" % T)
            g, gl = numbers(text, "G (%d K)" % T)
            s, sl = numbers(text, "S (%d K)" % T)
            check(len(h) >= 3 and abs(h[2] - dH) < 0.06,
                  "dH at %d K equals ecce-thermo per species" % T, "%s vs %.3f" % (hl, dH))
            check(len(g) >= 3 and abs(g[2] - dG) < 0.06,
                  "dG at %d K equals ecce-thermo per species" % T, "%s vs %.3f" % (gl, dG))
            check(len(s) == 1 and abs(s[0] - (dH - dG) * 1000 / T) < 0.2,
                  "dS at %d K = (dH - dG)/T" % T, sl)
        h500, _ = numbers(got[1], "H (500 K)")
        h723, _ = numbers(got[2], "H (723 K)")
        check(h500 and h723 and abs(h500[2] - h723[2]) > 0.5,
              "dH changes with T (Kirchhoff)", "%s %s" % (h500, h723))
        check("H (723 K)" in dlg[0] and "G (723 K)" in dlg[0] and "S (723 K)" in dlg[0],
              "the dialog's T / K field relabels H and G and adds S", dlg[0])

        n2 = json.loads(subprocess.run(
            [os.path.join(apps.INSTALL, "bin", "ecce-thermo"), urls["N2"], "--json",
             "--treatment", "rrho"], env=env, capture_output=True, text=True).stdout)[0]
        w = n2["treatments"]["rrho"]
        T = n2["T"]
        check(abs(w["Cv"] - 2.5 * R) < 0.01 * R and 0 < w["Cv_vib"] < 0.01 * R,
              "N2 at 298 K: Cv = 5/2 R, tiny vibrational part", w)
        check(abs(w["Cp"] - w["Cv"] - R) < 1e-4, "N2: Cp = Cv + R", w)
        check(abs(w["U_trans"] - 1.5 * R * T / 1000) < 1e-3 and abs(w["U_rot"] - R * T / 1000) < 1e-3,
              "N2: U_trans = 3/2 RT, U_rot = RT", w)
    finally:
        for d in (data, work, home):
            shutil.rmtree(d, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("thermot", "Energy Calculator temperature wrong: %s" % ", ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
