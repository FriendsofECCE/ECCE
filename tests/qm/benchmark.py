#!/usr/bin/env python3
"""ecce-qm timing and result benchmark (not a ctest; run by hand).

    benchmark.py run EXE OUT.json [--threads N] [--filter REGEX] [--skip-slow]
    benchmark.py compare BEFORE.json AFTER.json

`run` times energy, gradient and optfreq (GeoVib) for a set of molecules with
HF and B3LYP / 6-31G* and records energies, gradients and frequencies.
`compare` prints a timing table and the largest result changes, flagged
against the limits: energy 1e-7 Eh (2e-6 when the grid changed), gradient
1e-6 Eh/bohr, frequency 0.5 cm-1.
"""
import json, os, re, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from molecules import MOLECULES

LIB = os.path.join(HERE, "..", "..", "data", "client", "StructureLibrary")


def from_mvm(path):
    atoms, grab = [], False
    for line in open(path):
        t = line.split()
        if line.startswith("atom_list"):
            grab = True
        elif grab and len(t) == 4:
            atoms.append((t[0], float(t[1]), float(t[2]), float(t[3])))
        elif grab:
            break
    return atoms


def molecules():
    m = {k: MOLECULES[k][2] for k in ("h2o", "nh3", "ch4", "benzene")}
    m["caffeine"] = from_mvm(os.path.join(LIB, "Natural_products_and_drugs", "Alkaloids", "Caffeine.mvm"))
    return m


# GeoVib on the 24-atom molecule is a many-hour job.
SLOW = {("caffeine", "optfreq")}


def write_input(path, name, atoms, method, runtype):
    with open(path, "w") as f:
        f.write("title %s\ncharge 0\nmultiplicity 1\nmethod %s\nbasis 6-31G*\nruntype %s\n" % (name, method, runtype))
        f.write("geometry\n")
        for a in atoms:
            f.write("%s %.8f %.8f %.8f\n" % a)
        f.write("end\n")


def parse(path):
    r, sect = {"grad": [], "freq": []}, None
    for line in open(path):
        t = line.split()
        if not t:
            continue
        if t[0] == "energy_total":
            r["energy"] = float(t[1])
        elif t[0] == "begin" and t[1] in ("gradient", "vibrations"):
            sect = t[1]
        elif t[0] == "end" and t[1] in ("gradient", "vibrations"):
            sect = None
        elif sect == "gradient":
            r["grad"] += [float(x) for x in t[2:5]]
        elif sect == "vibrations" and float(t[1]) != 0.0:
            r["freq"].append(float(t[1]))
    return r


def run(args):
    exe, out = os.path.abspath(args[0]), args[1]
    threads = args[args.index("--threads") + 1] if "--threads" in args else "3"
    pat = re.compile(args[args.index("--filter") + 1]) if "--filter" in args else None
    res = {}
    env = dict(os.environ, OMP_NUM_THREADS=threads)
    with tempfile.TemporaryDirectory(dir=os.getcwd()) as work:
        for name, atoms in molecules().items():
            for method in ("hf", "b3lyp"):
                for rt in ("energy", "gradient", "optfreq"):
                    key = "%s/%s/%s" % (name, method, rt)
                    if (pat and not pat.search(key)) or ((name, rt) in SLOW and "--skip-slow" in args):
                        continue
                    inp, o = os.path.join(work, "i.qm"), os.path.join(work, "o.out")
                    write_input(inp, name, atoms, method, rt)
                    t0 = time.time()
                    p = subprocess.run([exe, "-o", o, inp], env=env, capture_output=True, text=True)
                    dt = time.time() - t0
                    if p.returncode:
                        print("FAIL", key, p.stderr[-300:])
                        continue
                    res[key] = dict(parse(o), seconds=dt)
                    print("%-28s %8.1f s  E=%.9f" % (key, dt, res[key]["energy"]), flush=True)
    json.dump({"threads": threads, "results": res}, open(out, "w"), indent=1)


def compare(args):
    a, b = (json.load(open(p)) for p in args[:2])
    bad = 0
    print("| case | before s | after s | speed-up | dE (Eh) | max dgrad | max dfreq (cm-1) |")
    print("|---|---|---|---|---|---|---|")
    for k, x in a["results"].items():
        y = b["results"].get(k)
        if not y:
            continue
        dE = abs(x["energy"] - y["energy"])
        dg = max([abs(p - q) for p, q in zip(x["grad"], y["grad"])] or [0])
        df = max([abs(p - q) for p, q in zip(x["freq"], y["freq"])] or [0])
        flag = "" if dE < 2e-6 and dg < 1e-6 and df < 0.5 else "  <-- over limit"
        bad += bool(flag)
        print("| %s | %.1f | %.1f | %.1fx | %.1e | %.1e | %.2f |%s" % (
            k, x["seconds"], y["seconds"], x["seconds"] / y["seconds"], dE, dg, df, flag))
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    {"run": run, "compare": compare}[sys.argv[1]](sys.argv[2:])
