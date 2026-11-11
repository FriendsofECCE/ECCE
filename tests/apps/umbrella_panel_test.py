#!/usr/bin/env python3
"""Umbrella sampling (#762) through the Organizer and the Pull Force panel.

A stored GROMACS study holds a completed Dynamics pull task (Na+ and Cl-,
rate 0.01, k 6000) with the files of a real pull run from
fixtures/umbrella-nacl.  Then

  1. the Organizer hook `umbrellawindows` makes the -eq and production tasks:
     names, links, each task's own window .gro, Ready, and an .mdp with
     pull-coord1-rate 0 and the step count asked for;
  2. real window runs (the fixture's topol.tpr and pullf.xvg) go into the
     first production tasks, which are set Complete;
  3. the Builder's Umbrella sampling page finds windows, runs WHAM, exports
     the CSV and draws the PMF.

    tests/apps/umbrella_panel_test.py [--build build-cmake] [--png FILE]

Exit status 77 (skip) without gmx, Xvfb, binaries or the fixture.
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "launch"))

import builder_overlay_test as bo  # noqa: E402
import harness  # noqa: E402
from harness import say  # noqa: E402

check = bo.check
FIX = os.path.join(HERE, "fixtures", "umbrella-nacl")
UMB = os.path.join(REPO, "scripts", "ecce_umbrella.py")
EQ_STEPS, PROD_STEPS = 500, 2000


def vector(name, values, units):
    return ('<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>\n'
            '<vector name="%s" rowLabel="Pull Frame" rowLabels="%s" rows="%d" '
            'units="%s">%s</vector>\n'
            % (name, " ".join("%d-A" % (i + 1) for i in range(len(values))),
               len(values), units, "   ".join("%.6f" % v for v in values)))


def xvg(path):
    rows = [l.split() for l in open(path) if l.strip() and l[0] not in "#@"]
    return [[float(v) for v in col] for col in zip(*rows)]


def where(u):
    return u[len("file://"):].rstrip("/")


class Organizer:
    """The Organizer's test hook: lines appended to a file, answers in a log."""

    def __init__(self, s, display, wrappers, env):
        self.cmds = os.path.join(s.state, "organizer.commands")
        self.log = os.path.join(s.state, "organizer.log")
        open(self.cmds, "w").close()
        benv = dict(env, ECCE_TEST_ORGANIZER=self.cmds)
        self.proc = subprocess.Popen([os.path.join(wrappers, "ecce-organizer")],
                                     env=benv, cwd=os.path.join(s.home, "bin"),
                                     stdout=open(self.log, "w"), stderr=subprocess.STDOUT,
                                     start_new_session=True)

    def ask(self, line, timeout=240):
        with open(self.cmds, "a") as h:
            h.write(line + "\n")
        key = "ECCE_TEST_ORGANIZER: " + line + ": "
        end = time.time() + timeout
        while time.time() < end and self.proc.poll() is None:
            for l in open(self.log, errors="replace").read().splitlines():
                if l.startswith("ECCE_TEST_ORGANIZER: " + line.split()[0] + ": ") \
                        and key[:60] in l or l.startswith(key):
                    return l.split(": ", 2)[2]
            time.sleep(0.5)
        return "no answer"

    def stop(self):
        bo.stop(self.proc)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--png")
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--fixture", default=FIX, help="folder with pull/ and wNN/ (the full run's too)")
    ap.add_argument("--jacobian", action="store_true")
    ap.add_argument("--windows", type=int, default=0, help="at most this many windows (0: all)")
    ap.add_argument("--clip", action="store_true",
                    help="clip audit of both pages, docked, on a 1366x768 screen")
    ap.add_argument("--panel-mode", default="detail", help="detail (docked) or classic (own window)")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    gmx = shutil.which("gmx")
    if not gmx:
        harness.skip("gmx is not installed")
    for tool in ("Xvfb", "import"):
        if not shutil.which(tool, path=os.environ.get("PATH", "") + os.pathsep + "/usr/sbin"):
            harness.skip("%s is not installed" % tool)
    if not os.path.isdir(os.path.join(args.fixture, "pull")):
        harness.skip("no fixture in %s" % args.fixture)
    harness.prerequisites(build, ())
    for exe in bo.GUI:
        if not os.access(os.path.join(build, exe), os.X_OK):
            harness.skip("%s is not built in %s" % (exe, build))
    import xdisplay

    s = harness.Session(build, "umbpanel", {"GROMACS": gmx}, local=True, keep=args.keep)
    png = os.path.join(s.state, "png")
    os.makedirs(png, exist_ok=True)
    for exe in bo.GUI:
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    harness.link(UMB, os.path.join(s.home, "bin", "ecce-umbrella"))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)
    display = org = None
    try:
        s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
        if not check(s.services(True), "services came up"):
            return 1
        rc, out = s.driver("gromacsstudy", s.userUrl(), "umb")
        urls = [l for l in out.splitlines() if l.startswith("file://")]
        if not check(rc == 0 and len(urls) == 4, "a GROMACS study was created"):
            return 1
        study, pull = urls[0], urls[3].rstrip("/") + "/"
        check(s.driver("mdsetup", pull, "localhost", os.path.join(s.state, "run-pull"),
                       s.user(), "queue=normal", "procs=1")[0] == 0, "launch settings stored")
        rc, out = s.driver("mdpull", pull, "0.01", "6000")
        check(rc == 0, "the pull model is stored (rate 0.01, k 6000) %s" % out.strip()[-100:])
        task = where(pull)
        mdp = open(os.path.join(task, "Inputs", "gromacs.mdp")).read()
        check("pull-coord1-rate" in mdp and "= 0.01" in mdp.split("pull-coord1-rate")[1].split("\n")[0],
              "the pull task's .mdp has rate 0.01")

        out_dir = os.path.join(task, "Outputs")
        os.makedirs(out_dir, exist_ok=True)
        for f in os.listdir(os.path.join(args.fixture, "pull")):
            shutil.copy(os.path.join(args.fixture, "pull", f), out_dir)
        px, pf = xvg(os.path.join(out_dir, "pullx.xvg")), xvg(os.path.join(out_dir, "pullf.xvg"))
        props = os.path.join(task, "Props")
        os.makedirs(props, exist_ok=True)
        for key, vals, units in (("PULLTIME", pf[0], "ps"), ("PULLDIST", px[1], "nm"),
                                 ("PULLFORCE", pf[1], "kJ/mol/nm")):
            with open(os.path.join(props, key), "w") as f:
                f.write(vector(key, vals, units))

        display = xdisplay.Display(
            screen="1366x768x24" if args.clip else "1500x1000x24").__enter__()
        env = s.env({"DISPLAY": display.name, "GTK_THEME": "Adwaita",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        org = Organizer(s, display, wrappers, env)
        time.sleep(12)
        check(org.ask("setstate %s completed" % pull) == "ok", "the pull task is Complete")

        # --- 1: the windows ------------------------------------------------
        t0 = time.time()
        ans = org.ask("umbrellawindows %s 0.04 0.24 1.2 %d %d" % (pull, EQ_STEPS, PROD_STEPS), 600)
        say("umbrellawindows: %s (%.0f s)" % (ans[:160], time.time() - t0))
        names = [n.strip() for n in ans.split("|")]
        m = re.match(r"ok (\d+)", names[0])
        n = int(m.group(1)) if m else 0
        names = names[1:]
        check(n >= 10 and len(names) == 2 * n, "%d windows, %d tasks" % (n, len(names)))
        base = os.path.basename(task)
        ok = n > 0 and all(
            names[2 * i] == "%s-w%02d-eq" % (base, i + 1) and names[2 * i + 1] == "%s-w%02d" % (base, i + 1)
            for i in range(n))
        check(ok, "names run %s-w01-eq, %s-w01, ..." % (base, base))
        sdir = os.path.dirname(task)
        lb = os.path.join(sdir, "linkbase.xml")
        text = open(lb).read() if os.path.exists(lb) else ""
        label = dict((m.group(2), m.group(1)) for m in re.finditer(
            r'xlink:label="(\w+)"\s+xlink:href="([^"]+)"', text.replace("\n", " ")))
        arcs = set(re.findall(r'input_provider"\s+xlink:from="(\w+)"\s+xlink:to="(\w+)"',
                              text.replace("\n", " ")))
        check(bool(arcs), "the study's linkbase holds %d input_provider links" % len(arcs))
        for i in range(min(n, 3)):
            eq, pr = names[2 * i], names[2 * i + 1]
            d = os.path.join(sdir, eq)
            gro = glob.glob(os.path.join(d, "Inputs", "window_%02d.gro" % (i + 1)))
            check(len(gro) == 1, "%s holds its window .gro" % eq)
            for t, steps, tag in ((eq, EQ_STEPS, "eq"), (pr, PROD_STEPS, "prod")):
                f = os.path.join(sdir, t, "Inputs", "gromacs.mdp")
                text = open(f).read() if os.path.exists(f) else ""
                rate = re.search(r"pull-coord1-rate\s*=\s*(\S+)", text)
                nst = re.search(r"nsteps\s*=\s*(\d+)", text)
                check(rate is not None and float(rate.group(1)) == 0 and nst and int(nst.group(1)) == steps,
                      "%s: .mdp has pull-coord1-rate 0 and nsteps %d" % (t, steps))
                check(s.driver("state", "file://" + os.path.join(sdir, t))[1].strip() == "ready",
                      "%s is Ready" % t)
            # each -eq follows the pull task, each prod its -eq
            for a, b in ((base, eq), (eq, pr)):
                check((label.get(a), label.get(b)) in arcs, "linked %s -> %s" % (a, b))
        allLinked = all((label.get(base), label.get(names[2 * i])) in arcs and
                        (label.get(names[2 * i]), label.get(names[2 * i + 1])) in arcs
                        for i in range(n))
        check(allLinked, "all %d windows are linked pull -> eq -> prod" % n)

        # --- 2: window runs ------------------------------------------------
        wins = sorted(glob.glob(os.path.join(args.fixture, "w[0-9][0-9]")))
        if args.windows:
            wins = wins[:args.windows]
        for i, w in enumerate(wins):
            prod = os.path.join(sdir, names[2 * i + 1], "Outputs")
            os.makedirs(prod, exist_ok=True)
            for f in os.listdir(w):
                shutil.copy(os.path.join(w, f), prod)
            org.ask("setstate file://%s completed" % os.path.join(sdir, names[2 * i + 1]))
        check(len(wins) >= 4, "%d window runs stored" % len(wins))

        # --- 3: the panel --------------------------------------------------
        csv = os.path.join(s.state, "pmf.csv")
        shot = os.path.join(png, "umb-pmf.png")
        text, shots = bo.runScript(
            s, display, env, wrappers, ["-context", pull],
            ["wait 6000", "umbrellapage", "umbrellafind 0.04 0.24 1.2",
             "umbrellawham" + (" jacobian" if args.jacobian else ""), "umbrellaexport %s" % csv,
             "umbrellashot %s 900 600" % shot, "umbrellastate", "wait 1500", "shot page",
             "umbrellaanalysis gyrate", "wait 1500", "shot analysis", "quit"],
            png, "umbrella", {"ECCE_PANEL_MODE": args.panel_mode, "ECCE_OPEN_PANEL": "Pull Force",
                              "ECCE_OPEN_PANEL_SIZE": "1000x1000"})
        for l in text.splitlines():
            if l.startswith("BUILDER: umbrella"):
                say(l)
        find = re.search(r"BUILDER: umbrellafind ok windows=(\d+)", text)
        check(find and int(find.group(1)) > 0, "umbrellafind listed windows (%s)" % (find.group(1) if find else "none"))
        wh = re.search(r"BUILDER: umbrellawham ok windows=\d+ pmf=(\d+) hist=(\d+)", text)
        check(wh and int(wh.group(1)) == 1 and int(wh.group(2)) >= len(wins) - 1,
              "WHAM drew the PMF and %s histograms" % (wh.group(2) if wh else "no"))
        check("umbrellaexport ok" in text and "umbrellashot ok" in text, "export and shot reported ok")
        # the pull page's link opens MD Analysis on the radius of gyration when
        # the run has a trajectory; the fixture's may not, and then it says so
        an = re.search(r"BUILDER: umbrellaanalysis (\w+)", text)
        check(an is not None, "the Radius of gyration link answered (%s)" % (an.group(1) if an else "nothing"))
        if args.clip:
            # docked at the size the layout gives on a 1366x768 screen
            audit = os.path.join(s.state, "clip.txt")
            open(audit, "w").close()
            cdir = os.path.join(args.png if args.png and os.path.isdir(args.png) else png, "clip")
            os.makedirs(cdir, exist_ok=True)
            text, _ = bo.runScript(
                s, display, env, wrappers, ["-context", pull],
                ["wait 6000", "umbrellawham", "umbrellaclip", "wait 500", "shot docked",
                 "umbrellapage", "wait 1000", "shot dockedumb", "quit"],
                png, "clip", {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Pull Force",
                              "ECCE_CLIP_AUDIT": audit})
            found = [l for l in open(audit).read().splitlines() if l.strip()]
            for l in found:
                say("  clip: " + l)
            for k in ("docked", "dockedumb"):
                if k in _:
                    shutil.copy(_[k], os.path.join(cdir, k + ".png"))
            check("umbrellaclip done" in text, "the clip audit ran on both pages")
            bad = [l for l in found if l.startswith("FINDING\tPullForce")]
            check(not bad, "nothing clipped on the Pull Force pages at 1366x768: %s" % bad)
        if os.path.exists(csv):
            lines = [l for l in open(csv).read().splitlines() if l and l[0] != "#"]
            head = lines[0].lower()
            check("distance" in head and "pmf" in head and "hist" in head and len(lines) > 20,
                  "the CSV has distance, pmf and histogram columns (%d rows): %s" % (len(lines) - 1, lines[0][:70]))
        else:
            check(False, "the CSV exists")
        check(os.path.exists(shot) and os.path.getsize(shot) > 2000, "the PMF picture was saved")
        if args.png:
            for k in ("page",):
                if k in shots:
                    shutil.copy(shots[k], args.png)
            shutil.copy(shot, os.path.splitext(args.png)[0] + "-plot.png")
        say("screenshots: %s" % shots)
    finally:
        if org:
            org.stop()
        if display:
            display.__exit__(None, None, None)
        s.stop()
    failures = bo.failures
    say("FAILED: " + "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
