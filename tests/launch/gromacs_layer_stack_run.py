#!/usr/bin/env python3
"""A Layer Builder stack equilibrated with GROMACS and MINFF, all through ECCE (#1140).

Not part of ctest (it runs for many minutes).  Local data, isolated HOME:

  * a GROMACS MD study with a Prepare, an Optimize, an Equilibrate and a
    Dynamics task (launchjob gromacsprepstudy ... bare equilibrate dynamics);
  * the Builder, on the Prepare, imports the layer, folds it, stacks it with
    the Layer Builder (ions and water per gap) and saves (scene script);
  * gmxprepare: Mineral, "Mineral from: the Builder", Build topology, Save;
  * Prepare, Optimize (EM), Equilibrate (NVT) and Dynamics (NPT, C-rescale,
    semi-isotropic) run in turn through the real Launch on this machine;
  * the box before and after, and the layer spacing, are printed.

    tests/launch/gromacs_layer_stack_run.py --layer LAYER.gro [--build DIR]
        [--layers 3] [--spacing 15.5] [--ions "Na 5 1;Ca 3 2;Mg 2 2"] [--water 330]
        [--seed 7] [--nvt-ps 100] [--npt-ps 300] [--procs 6] [--temp 300]
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402
from gromacs_prepare_test import urls_of, outputs, FIXTURES  # noqa: E402


def box(gro):
    v = [float(x) for x in open(gro).read().rstrip("\n").split("\n")[-1].split()]
    return v + [0.0] * (9 - len(v))


def mineral_z_layers(gro, nlayers):
    """Mean z (nm) of each layer's mineral atoms, sorted: the spacing."""
    L = open(gro).read().splitlines()
    z = sorted(float(l[36:44]) for l in L[2:-1] if l[5:10].strip() == "MIN")
    if not z:
        return []
    c = box(gro)[2]
    # split at the widest gaps of the sorted z (periodic: wrap last to first)
    gaps = sorted(((z[i + 1] - z[i], i) for i in range(len(z) - 1)), reverse=True)
    cuts = sorted(i for _g, i in gaps[:nlayers - 1])
    groups, s = [], 0
    for cut in cuts + [len(z) - 1]:
        groups.append(z[s:cut + 1])
        s = cut + 1
    return [sum(g) / len(g) for g in groups], c


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    bld = os.environ.get("BLD")
    ap.add_argument("--build", default=os.path.join(bld, "bi") if bld else
                    os.path.join(REPO, "build-cmake"))
    ap.add_argument("--layer", required=True)
    ap.add_argument("--layers", type=int, default=3)
    ap.add_argument("--spacing", default="15.5")
    ap.add_argument("--ions", default="Na 5 1;Ca 3 2;Mg 2 2")
    ap.add_argument("--water", default="330")
    ap.add_argument("--seed", default="7")
    ap.add_argument("--nvt-ps", type=float, default=100)
    ap.add_argument("--npt-ps", type=float, default=300)
    ap.add_argument("--procs", default="6")
    ap.add_argument("--temp", default="300")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    gmx = shutil.which("gmx")
    if not gmx:
        harness.skip("gmx is not installed")
    harness.prerequisites(build, ())
    sys.path.insert(0, os.path.join(REPO, "scripts"))
    import minff_topology as mt
    py = mt.find_atomipy_python()
    import xdisplay
    import gromacs_test

    s = harness.Session(build, "layer1140", {"GROMACS": gmx}, local=True, keep=True)
    s.registerMachine("localhost", "Shell", {"GROMACS": gmx})
    rundir = os.path.join(s.state, "run")
    t0 = time.time()
    if not s.services(True):
        say("FAIL: services")
        return 1
    rc, out = s.driver("gromacsprepstudy", s.userUrl(), "claystack",
                       os.path.join(FIXTURES, "1l2y.pdb"), "bare", "equilibrate", "dynamics")
    urls = urls_of(out)
    if rc != 0 or len(urls) != 5:
        say("FAIL: study: %s" % out)
        return 1
    _study, prep, opt, eq, dyn = urls
    for u in (prep, opt, eq, dyn):
        s.driver("mdsetup", u, "localhost", rundir, s.user(), "procs=" + args.procs)
    for exe in ("builder", "gmxprepare", "mdoptimize", "mddynamics"):
        harness.link(os.path.join(build, exe), os.path.join(s.home, "bin", exe))
    gw = os.path.join(REPO, "packaging", "gateway")
    for name in os.listdir(gw):
        if name.startswith("ecce-"):
            harness.link(os.path.join(gw, name), os.path.join(s.home, "bin", name))
    wrappers = os.path.join(s.state, "wrappers")
    shutil.rmtree(wrappers, ignore_errors=True)
    shutil.copytree(os.path.join(build, "wrappers"), wrappers)
    for entry in os.listdir(wrappers):
        os.chmod(os.path.join(wrappers, entry), 0o755)

    stack_gro = os.path.join(s.state, "stack.gro")
    scene = ["hold 3", "importfile " + os.path.abspath(args.layer), "hold 3",
             "pbcpress fold", "hold 3", "pbclayer layers %d" % args.layers,
             "pbclayer spacing " + args.spacing]
    for k, ion in enumerate(args.ions.split(";")):
        el, n, q = ion.split()
        scene += ["pbclayer ion%d %s" % (k + 1, el), "pbclayer ion%dn %s" % (k + 1, n),
                  "pbclayer ion%dq %s" % (k + 1, q)]
    scene += ["pbclayer water " + args.water, "pbclayer seed " + args.seed,
              "pbcpress layers", "hold 8", "savegro " + stack_gro, "savecalc", "hold 5"]
    sc = os.path.join(s.state, "stack.scene")
    open(sc, "w").write("\n".join(scene) + "\n")
    with xdisplay.Display() as display:
        env = s.env({"DISPLAY": display.name, "ECCE_VIEWER_SCENE": sc,
                     "ECCE_VIEWER_SCENE_OUT": s.state, "ECCE_FEEDBACK_STDERR": "1",
                     "PATH": wrappers + os.pathsep + s.env()["PATH"]})
        r = subprocess.run([os.path.join(wrappers, "ecce-builder"), "-context", prep],
                           env=env, cwd=os.path.join(s.home, "bin"),
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=900)
        open(os.path.join(s.state, "builder.log"), "w").write(r.stdout.decode("utf-8", "replace"))
        if not os.path.exists(stack_gro):
            say("FAIL: the Builder wrote no stack: see %s/builder.log" % s.state)
            return 1
        n0 = int(open(stack_gro).read().splitlines()[1])
        b0 = box(stack_gro)
        say("stack: %d atoms, box %s nm (%.0f s)" % (n0, b0, time.time() - t0))

        cmddir = os.path.join(s.state, "mded")
        os.makedirs(cmddir, exist_ok=True)
        genv = s.env({"DISPLAY": display.name, "ECCE_TEST_MDED": cmddir,
                      "PATH": wrappers + os.pathsep + s.env()["PATH"],
                      **({"ECCE_ATOMIPY_PYTHON": py} if py else {})})
        ed = gromacs_test.Editor(s, display, genv, wrappers, cmddir, "gmxprepare", "GMXPrepare")
        if not ed.up():
            say("FAIL: gmxprepare")
            return 1
        say("open: " + ed.cmd("open " + prep))
        say("source: " + ed.cmd("set source minff"))
        say("mstruct: " + ed.cmd("set mstruct builder"))
        say("build: " + ed.cmd("set mbuild go", timeout=1800))
        say("fields: " + ed.cmd("fields"))
        say("save: " + ed.cmd("save"))
        say("recipe: " + ed.cmd("recipe"))
        say("verify: " + ed.cmd("verify")[:1500])
        ed.cmd("quit")
        say("Prepare built (%.0f s)" % (time.time() - t0))

        def launch(url, what, timeout):
            t = time.time()
            rc, out = s.launch(url)
            state = s.waitState(url, timeout)
            say("%s: launch rc=%d, %s, %.0f s" % (what, rc, state, time.time() - t))
            return state in ("completed", "loaded")

        if not launch(prep, "Prepare", 1800):
            return 1
        steps = {opt: None, eq: int(args.nvt_ps / 0.001), dyn: int(args.npt_ps / 0.001)}
        for url, exe, tool, what in ((opt, "mdoptimize", "MDOptimize", "Optimize (EM)"),
                                     (eq, "mddynamics", "MDDynamics", "Equilibrate (NVT)"),
                                     (dyn, "mddynamics", "MDDynamics", "Dynamics (NPT)")):
            e = gromacs_test.Editor(s, display, genv, wrappers, cmddir, exe, tool)
            if not e.up():
                say("FAIL: " + exe)
                return 1
            say("%s open: %s" % (what, e.cmd("open " + url)))
            if steps[url]:
                say(" steps: " + e.cmd("set-steps %d" % steps[url]))
                say(" ensemble: " + e.cmd("ensemble-set " + ("npt" if url == dyn else "nvt")))
                say(" temp: " + e.cmd("temperature-set " + args.temp))
                if url == dyn:
                    say(" barostat: " + e.cmd("run-set barostat 0"))
                    say(" pcoupltype: " + e.cmd("run-set pcoupltype 1"))
            else:
                say(" steps: " + e.cmd("set-steps 5000"))
            say(" save: " + e.cmd("save"))
            say(" run: " + e.cmd("run"))
            e.cmd("quit")
            mdp = open(os.path.join(url.replace("file://", "").rstrip("/"), "Inputs",
                                    "gromacs.mdp")).read()
            say(" mdp: " + "; ".join(l.strip() for l in mdp.splitlines()
                                       if re.match(r"\s*(integrator|nsteps|dt|tcoupl|pcoupl|"
                                                   r"pcoupltype|ref-t|ref_t|ref-p|tau-p|"
                                                   r"compressibility|periodic-molecules|"
                                                   r"constraints)\b", l)))
            if not launch(url, what, 3600):
                return 1
    say("total wall time %.0f s" % (time.time() - t0))

    # every structure the tasks wrote, oldest first: box and layer spacing
    gros = sorted((os.path.join(dp, f) for dp, _d, fs in os.walk(rundir) for f in fs
                   if f.endswith(".gro")), key=os.path.getmtime)
    for g in gros:
        try:
            zs, c = mineral_z_layers(g, args.layers)
        except Exception:
            continue
        b = box(g)
        sp = [round(10 * (zs[i + 1] - zs[i]), 2) for i in range(len(zs) - 1)] + \
             [round(10 * (zs[0] + b[2] - zs[-1]), 2)] if zs else []
        say("%s: box %.4f %.4f %.4f (tilt %s) nm; layer spacings %s A"
            % (os.path.relpath(g, rundir), b[0], b[1], b[2],
               " ".join("%.3f" % x for x in b[3:]), sp))
    say("state dir: " + s.state)
    return 0


if __name__ == "__main__":
    sys.exit(main())
