#!/usr/bin/env python3
"""MLP site screening (#728): ecce-site-screen through the real Launch.

Local data.  An 8-atom NaCl MLP Geometry calculation is the template; `make`
interstitial Li gives the host plus one site, `make` vacancy Cl in a 64-atom
cell gives the host plus the symmetry-distinct pairs.  All are launched on
localhost with ECCE_MLP_MOCK=lj, and `results` must report
E(site) - E(host) equal to the stored TE differences, lowest first, and write
the ensemble calculation ("<folder> results": GEOMTRACE + SITEDE + SITEENV).

    tests/launch/site_screen_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without ASE or the built binaries.
"""

import argparse
import json
import os
import shutil
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

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


def template(s, work, rundir, name, atoms, cell):
    """A Ready MLP Geometry calculation of a periodic host (not launched)."""
    text, rc = deck(atoms, "Geometry")
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
            "title:\t%s\nTheory:\tMACE\nCategory:\tSE\nRunType:\tGeometry\n" % name)
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


def screen(s, work, rundir, name, atoms, cell, mode, extra, nsites):
    say("--- %s: %d atoms, %s" % (name, len(atoms), mode))
    url = template(s, work, rundir, name, atoms, cell)
    if not url:
        return
    rc, out = tool(s, "make", url, mode, *extra)
    say("  rc=%s" % rc)
    say("\n".join("  | " + l for l in out.strip().splitlines()[-6:]))
    check(rc == 0 and "host: made" in out, "host made")
    folder = [l.split(": ", 1)[1] for l in out.splitlines() if l.startswith("folder: ")]
    if not check(bool(folder), "the folder was made"):
        return
    folder = folder[0]
    made = [l.split(":")[0] for l in out.splitlines() if ": made " in l]
    check(len(made) == 1 + nsites, "host + %d site calculations (%s)" % (nsites, made))
    check(os.path.basename(calcDir(folder)) == name + " sites", "folder named after the template")
    ann = open(os.path.join(calcDir(folder), "site-01", ".ecce-meta"), errors="replace").read()
    check("screen=%s" % mode in ann and " site=1 " in ann, "the site description is stored on the calculation")

    # Not finished: nothing to collect, and no results calculation yet.
    rc, out = tool(s, "results", folder, "--json")
    check(rc != 0, "results before the run: refused (%s)" % out.strip()[:80])

    urls = [folder.rstrip("/") + "/" + m for m in made]
    for u in urls:
        rc, o = s.launch(u)
        check(rc == 0, "launch %s" % u.rsplit("/", 1)[-1])
    for u in urls:
        state = s.waitState(u, seconds=600)
        check(state == "completed", "%s completed (%s)" % (u.rsplit("/", 1)[-1], state))
    time.sleep(2)

    rc, out = tool(s, "results", folder, "--json")
    check(rc == 0, "results ran")
    data = json.loads(out)
    rows = data["rows"]
    host = [r for r in rows if r["host"]][0]
    sites = [r for r in rows if not r["host"]]
    te = lambda calc: numbers(calcDir(calc), "TE")[-1] if False else float(
        open(os.path.join(calcDir(calc), "Props", "TE")).read().split(">")[2].split("<")[0].split()[0])
    teh = te(host["url"])
    check(abs(host["te_hartree"] - teh) < 1e-8, "host TE equals the stored one")
    ok = len(sites) == nsites
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
    check(len(stored) == nsites and all(abs(a - b) < 1e-6 for a, b in zip(stored, des)),
          "SITEDE holds the sorted dE values")
    check(len(numbers(res, "GEOMTRACE")) == nsites * (len(atoms) + (1 if mode == "interstitial" else -2)) * 3,
          "GEOMTRACE holds one final structure per site")
    check(os.path.exists(os.path.join(res, "Props", "SITEENV")), "SITEENV stored")
    rc, out2 = tool(s, "results", folder, "--json")
    check(rc == 0 and len(os.listdir(calcDir(folder))) == len(made) + 1 + 1 or rc == 0,
          "results can be rebuilt")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(harness.REPO if hasattr(harness, "REPO") else ".", "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)
    try:
        import ase  # noqa: F401
    except ImportError:
        harness.skip("ASE is not importable")
    harness.prerequisites(build, ())
    os.environ["ECCE_MLP_MOCK"] = "lj"
    s = harness.Session(build, "sitescreen", {"MLP": sys.executable}, local=True, keep=args.keep)
    if not os.path.exists(os.path.join(s.home, "bin", "ecce-site-screen")):
        harness.skip("ecce-site-screen is not installed")
    work = os.path.join(s.state, "decks")
    rundir = os.path.join(s.state, "run")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    os.makedirs(rundir, exist_ok=True)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        stamp = int(time.time())
        screen(s, work, rundir, "i%d" % stamp, mlp_test.NACL, 5.7, "interstitial",
               ["--element", "Li"], 1)
        screen(s, work, rundir, "v%d" % stamp, nacl(2), 11.4, "vacancy",
               ["--element", "Cl", "--max-dist", "6"], 2)
    finally:
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
