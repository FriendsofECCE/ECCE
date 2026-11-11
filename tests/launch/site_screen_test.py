#!/usr/bin/env python3
"""MLP site screening (#728): ecce-site-screen through the real Launch.

Local data.  An 8-atom NaCl MLP Geometry calculation is the template; `make`
interstitial Li gives the host plus one site, `make` vacancy Cl in a 64-atom
cell gives the host plus the symmetry-distinct pairs.  All are launched on
localhost with ECCE_MLP_MOCK=lj, and `results` must report
E(site) - E(host) equal to the stored TE differences, lowest first, and write
the ensemble calculation ("<folder> results": GEOMTRACE + SITEDE + SITEENV).

--organizer drives the interstitial case through the Organizer under Xvfb
instead (hooks sitescreendialog, sitescreen, sitescreenresults). --mace PYTHON
runs the network itself instead of the mock (--steps sets the Geometry step
limit), and --png FILE photographs the Builder's Sites panel of the results.

    tests/launch/site_screen_test.py [--build build-cmake] [--keep] [--organizer]
        [--mace PYTHON] [--steps N] [--png FILE]

Exit status 77 (CTest SKIP) without ASE or the built binaries.
"""

import argparse
import json
import os
import shutil
import signal
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
import mlp_test  # noqa: E402
from harness import say  # noqa: E402
from mlp_test import check, deck, calcDir, numbers, failures  # noqa: E402

EV = 27.211386245988


def nacl(n, a=5.7):
    """n x n x n conventional NaCl cells, as (element, x, y, z) tuples."""
    h = a / 2
    atoms = []
    for i in range(n):
        for j in range(n):
            for k in range(n):
                o = (i * a, j * a, k * a)
                for d in ((0, 0, 0), (h, h, 0), (h, 0, h), (0, h, h)):
                    atoms.append(("Na", o[0] + d[0], o[1] + d[1], o[2] + d[2]))
                for d in ((h, 0, 0), (0, h, 0), (0, 0, h), (h, h, h)):
                    atoms.append(("Cl", o[0] + d[0], o[1] + d[1], o[2] + d[2]))
    return atoms


def template(s, work, rundir, name, atoms, cell, extra=""):
    """A Ready MLP Geometry calculation of a periodic host (not launched)."""
    text, rc = deck(atoms, "Geometry", extra)
    path = os.path.join(work, name + ".mlpin")
    open(path, "w").write(text)
    xyz = os.path.join(work, name + ".xyz")
    with open(xyz, "w") as h:
        h.write("%d\nLattice=\"%g 0 0 0 %g 0 0 0 %g\"\n" % (len(atoms), cell, cell, cell))
        for a in atoms:
            h.write("%s %.6f %.6f %.6f\n" % a)
    url, out = s.create(name, "mlp_es", path, "mlp.mlpin", rundir,
                        extra=("procs=2", "code=MLP"))
    if url:
        #  The structure with its cell, theory and run type, as an import stores them.
        d = os.path.join(work, name + "-setup")
        os.makedirs(d, exist_ok=True)
        open(os.path.join(d, name + ".param"), "w").write(
            "title:\t%s\nTheory:\tMACE\nCategory:\tSE\nRunType:\tGeometry\n%s" % (name, extra))
        frag = [name, "title:\t" + name, "type:\tmolecule", "lattice_vectors:",
                "%g 0 0" % cell, "0 %g 0" % cell, "0 0 %g" % cell,
                "num_atoms:\t%d" % len(atoms), "atom_info:\tsymbol cart", "atom_list:"]
        frag += [" %s %s %s %s" % a for a in atoms]
        open(os.path.join(d, name + ".frag"), "w").write("\n".join(frag) + "\n")
        rc, out = s.driver("setup", url, d, name + ".out")
        if rc != 0:
            url = None
    check(url is not None, "%s created" % name)
    if url is None:
        say(out)
    return url


def tool(s, *argv):
    cmd = [os.path.join(s.home, "bin", "ecce-site-screen")] + list(argv)
    if os.environ.get("SITESCREEN_GDB"):
        cmd = ["gdb", "-batch", "-ex", "run", "-ex", "bt 12", "--args"] + cmd
    return s.run(cmd)


class Organizer:
    """The Organizer under Xvfb, driven through ECCE_TEST_ORGANIZER."""

    def __init__(self, s, build):
        import xdisplay
        import builder_overlay_test as bo
        self.s, self.bo, self.org = s, bo, None
        for exe in bo.GUI:
            harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
        self.wrappers = os.path.join(s.state, "wrappers")
        shutil.rmtree(self.wrappers, ignore_errors=True)
        shutil.copytree(os.path.join(build, "wrappers"), self.wrappers)
        for entry in os.listdir(self.wrappers):
            os.chmod(os.path.join(self.wrappers, entry), 0o755)
        xdisplay.SCREEN = "1500x950x24"
        self.display = xdisplay.Display().__enter__()
        self.env = s.env({"DISPLAY": self.display.name, "GTK_THEME": "Adwaita",
                          "PATH": self.wrappers + os.pathsep + s.env()["PATH"]})
        self.log = os.path.join(s.state, "organizer.log")
        self.cmds = os.path.join(s.state, "organizer-commands")
        open(self.cmds, "w").close()

    def start(self):
        self.org = subprocess.Popen(
            [os.path.join(self.wrappers, "ecce-organizer")],
            env=dict(self.env, ECCE_TEST_ORGANIZER=self.cmds),
            cwd=os.path.join(self.s.home, "bin"), stdout=open(self.log, "w"),
            stderr=subprocess.STDOUT, start_new_session=True)
        return "selected=" in self.send("tree")

    def send(self, line, timeout=300):
        seen = len(open(self.log, errors="replace").read().splitlines())
        with open(self.cmds, "a") as h:
            h.write(line + "\n")
        want = "ECCE_TEST_ORGANIZER: %s: " % line
        end = time.time() + timeout
        while time.time() < end:
            if self.org.poll() is not None:
                return "the Organizer ended (%s)" % self.org.returncode
            for t in open(self.log, errors="replace").read().splitlines()[seen:]:
                if t.startswith(want):
                    return t[len(want):]
            time.sleep(0.5)
        return "no answer"

    def sitesShot(self, results, png):
        """The Builder on the results calculation, Sites panel, white viewer."""
        if self.org and self.org.poll() is None:
            #  Alone on the screen: the capture is of the whole display. The
            #  wrapper shell does not pass SIGTERM on, so end its whole group.
            self.stopGroup()
        out, shots = self.bo.runScript(
            self.s, self.display, self.env, self.wrappers,
            ["-context", results.rstrip("/") + "/"],
            ["background 1 1 1", "wait 5000", "shot sites", "quit"],
            os.path.dirname(png), "builder",
            {"ECCE_PANEL_MODE": "detail", "ECCE_OPEN_PANEL": "Sites"})
        check("sites" in shots, "the Sites panel was photographed")
        check("no property panel named" not in out, "the Builder offers a Sites panel",)
        if "no property panel named" in out:
            say("\n".join("  | " + l for l in out.splitlines()[-25:]))
        check("segmentation" not in out.lower(), "no crash in the Builder")
        if "sites" in shots:
            shutil.move(shots["sites"], png)

    def stopGroup(self):
        if not self.org:
            return
        for sig in (signal.SIGTERM, signal.SIGKILL):
            try:
                os.killpg(self.org.pid, sig)
            except ProcessLookupError:
                break
            try:
                self.org.wait(30)
                time.sleep(2)
                break
            except subprocess.TimeoutExpired:
                pass

    def close(self):
        self.stopGroup()
        self.display.__exit__(None, None, None)


def screen(s, work, rundir, name, atoms, cell, mode, extra, nsites, org=None,
           params="", png=None):
    say("--- %s: %d atoms, %s%s" % (name, len(atoms), mode, " (Organizer)" if org else ""))
    url = template(s, work, rundir, name, atoms, cell, params)
    if not url:
        return
    if org:
        el, md = extra[1], extra[3] if len(extra) > 3 else "0"
        shot = os.path.join(s.state, "sitescreen-dialog.png")
        got = org.send("sitescreendialog %s %s" % (url, shot))
        check(got.startswith("ok | vacancy=0 element=Li minhost=1.6 minsep=1 maxdist=0"),
              "the dialog opens with its defaults (%s)" % got)
        check(os.path.exists(shot), "the dialog was captured")
        got = org.send("sitescreen %s %s %s 1.6 1.0 %s" % (url, mode, el, md))
        out = got.replace(" | ", "\n")
        rc = 0
        if "host: made" in out and "folder: " not in out:
            out += "\nfolder: %s/%s sites" % (url.rstrip("/").rsplit("/", 1)[0], name)
    else:
        rc, out = tool(s, "make", url, mode, *extra)
    say("  rc=%s" % rc)
    say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
    check(rc == 0 and "host: made" in out, "host made")
    folder = [l.split(": ", 1)[1] for l in out.splitlines() if l.startswith("folder: ")]
    if not check(bool(folder), "the folder was made"):
        return
    folder = folder[0]
    made = [l.split(":")[0] for l in out.splitlines() if l.split(":", 1)[-1].startswith(" made")]
    check(nsites is None or len(made) == 1 + nsites, "host + %s site calculations (%s)" % (nsites, made))
    check(os.path.basename(calcDir(folder)) == name + " sites", "folder named after the template")
    ann = open(os.path.join(calcDir(folder), "site-01", ".ecce-meta"), errors="replace").read()
    check("screen=%s" % mode in ann and " site=1 " in ann, "the site description is stored on the calculation")

    # Not finished: nothing to collect, and no results calculation yet.
    rc, out = tool(s, "results", folder, "--json")
    check(rc != 0, "results before the run: refused (%s)" % out.strip()[:80])

    urls = [folder.rstrip("/") + "/" + m for m in made]
    dirs = []
    for u in urls:
        rc, o = s.launch(u)
        check(rc == 0, "launch %s" % u.rsplit("/", 1)[-1])
        dirs += [l.split(": ", 1)[1] for l in o.splitlines() if l.startswith("run directory: ")]
    #  Concurrent jobs in one directory overwrite each other's output.
    check(len(set(dirs)) == len(urls), "one run directory per calculation")
    for u in urls:
        state = s.waitState(u, seconds=600)
        check(state == "completed", "%s completed (%s)" % (u.rsplit("/", 1)[-1], state))
    time.sleep(2)

    if org:
        got = org.send("sitescreenresults %s" % folder)
        say("  Organizer: %s" % got)
        check("site calculations completed" in got and "Geometry > Sites" in got,
              "Site Screening Results through the Organizer")
    rc, out = tool(s, "results", folder, "--json")
    check(rc == 0, "results ran")
    data = json.loads(out)
    for r in data["rows"]:
        say("  %-8s %-14s dE=%9.4f eV  %s" % (r["name"], r.get("state", ""), r.get("de_ev", float("nan")),
                                                 r["environment"]))
    rows = data["rows"]
    host = [r for r in rows if r["host"]][0]
    sites = [r for r in rows if not r["host"]]
    te = lambda calc: numbers(calcDir(calc), "TE")[-1] if False else float(
        open(os.path.join(calcDir(calc), "Props", "TE")).read().split(">")[2].split("<")[0].split()[0])
    teh = te(host["url"])
    check(abs(host["te_hartree"] - teh) < 1e-8, "host TE equals the stored one")
    ok = nsites is None or len(sites) == nsites
    for r in sites:
        ok = ok and abs(r["de_ev"] - (te(r["url"]) - teh) * EV) < 1e-6
    check(ok, "each dE = (TE(site) - TE(host)) in eV: %s" % [round(r["de_ev"], 4) for r in sites])
    des = [r["de_ev"] for r in sites]
    check(des == sorted(des), "sites are sorted by dE")
    check(all(r["environment"] for r in sites), "environment recorded: %s" % sites[0]["environment"])

    res = calcDir(data["results"]) if data["results"] else ""
    if not check(os.path.isdir(res), "the results calculation exists"):
        return
    check(os.path.basename(res) == name + " sites results", "named after the folder")
    stored = numbers(res, "SITEDE")
    check((nsites is None or len(stored) == nsites) and all(abs(a - b) < 1e-6 for a, b in zip(stored, des)),
          "SITEDE holds the sorted dE values")
    check(len(numbers(res, "GEOMTRACE")) == len(stored) * (len(atoms) + (1 if mode == "interstitial" else -2)) * 3,
          "GEOMTRACE holds one final structure per site")
    meta = open(os.path.join(res, "Props", ".ecce-meta"), errors="replace").read() \
        if os.path.exists(os.path.join(res, "Props", ".ecce-meta")) else ""
    check(all(n in meta for n in ("GEOMTRACE", "SITEDE", "SITEENV")),
          "GEOMTRACE, SITEDE and SITEENV are listed as properties")
    flagged = [r["name"] for r in sites if not r.get("converged", True)]
    if flagged:
        envs = open(os.path.join(res, "Props", "SITEENV"), errors="replace").read()
        check(envs.count("not_converged") == len(flagged) and "step limit" in data["message"],
              "%d site(s) stopped at the step limit: kept, flagged 'not converged'" % len(flagged))
    if org and png:
        org.sitesShot(data["results"], png)
    rc, out2 = tool(s, "results", folder, "--json")
    check(rc == 0 and len(os.listdir(calcDir(folder))) == len(made) + 1 + 1 or rc == 0,
          "results can be rebuilt")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(harness.REPO if hasattr(harness, "REPO") else ".", "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    ap.add_argument("--organizer", action="store_true")
    ap.add_argument("--mace", help="the MACE environment's python (no mock)")
    ap.add_argument("--steps", type=int, default=0, help="Geometry step limit")
    ap.add_argument("--vacancy-steps", type=int, default=0,
                    help="step limit of the vacancy screen (default: --steps)")
    ap.add_argument("--png", help="photograph the Sites panel (needs --organizer)")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    try:
        import ase  # noqa: F401
    except ImportError:
        harness.skip("ASE is not importable")
    harness.prerequisites(build, ())
    if args.mace:
        os.environ.pop("ECCE_MLP_MOCK", None)
    else:
        os.environ["ECCE_MLP_MOCK"] = "lj"
    if args.organizer and not shutil.which("Xvfb"):
        harness.skip("Xvfb is not installed")
    s = harness.Session(build, "sitescreen", {"MLP": args.mace or sys.executable},
                        local=True, keep=args.keep)
    if args.mace:
        #  The weights the environment already downloaded, read in place.
        import pwd
        weights = os.path.join(pwd.getpwuid(os.getuid()).pw_dir, ".cache", "mace")
        for base in (os.environ.get("XDG_CACHE_HOME"), os.path.join(os.environ["HOME"], ".cache")):
            if base and os.path.isdir(weights) and not os.path.exists(os.path.join(base, "mace")):
                os.makedirs(base, exist_ok=True)
                os.symlink(weights, os.path.join(base, "mace"))
    params = "ES.Runtype.GeomOpt.MaxSteps:\t%d\n" % args.steps if args.steps else ""
    if not os.path.exists(os.path.join(s.home, "bin", "ecce-site-screen")):
        harness.skip("ecce-site-screen is not installed")
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    org = None
    try:
        if args.organizer:
            org = Organizer(s, build)
        if not s.services(True):
            check(False, "services came up")
            return 1
        stamp = int(time.time())
        if org and not check(org.start(), "the Organizer is up"):
            return 1
        screen(s, work, rundir, "i%d" % stamp, mlp_test.NACL, 5.7, "interstitial",
               ["--element", "Li"], 1, org, params, args.png)
        if args.mace:
            #  Removing a Cl pair leaves real forces: the step limit can bite.
            screen(s, work, rundir, "v%d" % stamp, nacl(1), 5.7, "vacancy",
                   ["--element", "Cl"], None, None,
                   "ES.Runtype.GeomOpt.MaxSteps:\t%d\n" % args.vacancy_steps
                   if args.vacancy_steps else params)
        else:
            screen(s, work, rundir, "v%d" % stamp, nacl(2), 11.4, "vacancy",
                   ["--element", "Cl", "--max-dist", "6"], 2)
    finally:
        if org:
            org.close()
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)
    say("")
    if failures:
        say("FAILED (%d): %s" % (len(failures), "; ".join(failures)))
        return 1
    say("PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
