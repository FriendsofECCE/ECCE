#!/usr/bin/env python3
"""Run General Chemistry example calculations through ECCE (ecce-series,
Calculation Editor, Launcher on localhost) and collect their decks and outputs.

    ECCE_SRC=<checkout> ECCE_TEST_HOME=<install> ECCE_TEST_WRAPPERS=<install bin> \\
        python3 tools/examples/redo_genchem_teaching.py <out dir> [NAME ...]

Each job in JOBS is made with ecce-series from the stored N2 example (ECCE-QM,
B3LYP/6-31G*, GeoVib) and the structure given, then changed in the Calculation
Editor (theory, run type, basis set, charges, unrestricted orbitals), saved and
launched on localhost, at most PARALLEL at a time.  <out>/<name>/ receives the
generated .qmin and the .qmout; copy them into
data/client/examples/genchem/<topic>/ecceqm-<name>/ and run preparse.py.
"""
import glob, os, re, shutil, subprocess, sys, time
REPO = os.environ.get("ECCE_SRC", os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))
import apps, isolate, xdisplay  # noqa: E402
import ecceqm_builder_test as eb  # noqa: E402

GC = os.path.join(REPO, "data", "client", "examples", "genchem")
DIA = "molecular-orbitals-of-diatomics"
POL = "polar-bonds-and-dipole-moments"
VS = "vsepr"
DIP = dict(theory=None, runtype="Geometry", basis="aug-cc-pVDZ")
# name: (structure: example folder or atom list, charge, mult, settings)
JOBS = {
    "B2": ([("B", 0, 0, 0), ("B", 0, 0, 1.59)], 0, 3, dict(unrestricted=True)),
    "C2": ([("C", 0, 0, 0), ("C", 0, 0, 1.243)], 0, 1, {}),
    "H2_cation": ([("H", 0, 0, 0), ("H", 0, 0, 1.06)], 1, 2,
                  dict(theory="HF", basis="cc-pVTZ", unrestricted=True)),
    "He2_cation": ([("He", 0, 0, 0), ("He", 0, 0, 1.08)], 1, 2,
                   dict(theory="HF", basis="cc-pVTZ", unrestricted=True)),
    "HF_dipole": (POL + "/ecceqm-HF_dipole", 0, 1, DIP),
    "HCl": (POL + "/ecceqm-HCl", 0, 1, DIP),
    "HBr": (POL + "/ecceqm-HBr", 0, 1, DIP),
    "CO2": (POL + "/ecceqm-CO2", 0, 1, DIP),
    "SO2": (POL + "/ecceqm-SO2", 0, 1, DIP),
    "CH3F": (POL + "/ecceqm-CH3F", 0, 1, DIP),
    "LiH_dipole": (DIA + "/ecceqm-LiH", 0, 1, DIP),
    "LiF_dipole": ([("Li", 0, 0, 0), ("F", 0, 0, 1.564)], 0, 1, DIP),
    "NaCl_dipole": ("intermolecular-interactions/ecceqm-nacl_ion_pair", 0, 1, DIP),
    "H2O_dipole": (VS + "/ecceqm-H2O", 0, 1, DIP),
    "NH3_dipole": (VS + "/ecceqm-NH3", 0, 1, DIP),
    "BF3_dipole": (VS + "/ecceqm-BF3", 0, 1, DIP),
    "CH4_dipole": (VS + "/ecceqm-CH4", 0, 1, DIP),
    "SF6_dipole": (VS + "/ecceqm-SF6", 0, 1, DIP),
}
PARALLEL = int(os.environ.get("PARALLEL", "3"))
TERMINAL = ("Finished", "Complete", "Completed", "Failed", "Killed", "Unsuccessful", "Loaded")


def meta_set(path, key, value):
    text = open(path, errors="surrogateescape").read()
    text = re.sub(r"(ecce:%s\t\t)[^\n]*" % key, lambda m: m.group(1) + value, text)
    open(path, "w", errors="surrogateescape").write(text)


def meta_get(path, key):
    m = re.search(r"ecce:%s\t\t([^\n]*)" % key, open(path, errors="surrogateescape").read())
    return m.group(1) if m else ""


def atoms_of(example):
    """Final structure of a stored example (its chemsys.mvm)."""
    mvm = glob.glob(os.path.join(GC, example, "stored", "*", "Parameters", "chemsys.mvm"))[0]
    lines = open(mvm).read().splitlines()
    n = int(next(l for l in lines if l.startswith("num_atoms:")).split()[1])
    k = next(i for i, l in enumerate(lines) if l.startswith("atom_list:"))
    return [(f[0], float(f[1]), float(f[2]), float(f[3])) for f in (l.split() for l in lines[k + 1:k + 1 + n])]


def app(display, out, name, var, lines, url, settle=60):
    script = os.path.join(out, "%s-%s.script" % (os.path.basename(url.rstrip("/")), name))
    open(script, "w").write("\n".join(lines) + "\n")
    r = apps.run(display, name, args=["-context", url], windowTimeout=90, settle=settle,
                 env={var: script, "ECCE_FEEDBACK_STDERR": "1"})
    open(script[:-7] + ".log", "w").write(r.log or "")
    return r.log or ""


def main():
    out = os.path.abspath(sys.argv[1])
    names = sys.argv[2:] or list(JOBS)
    os.makedirs(out, exist_ok=True)
    data = os.path.join(out, "data")
    isolate.apply(apps.INSTALL)
    os.environ.update(ECCE_NO_REAP="1", GTK_THEME="Adwaita", ECCE_LOCAL_DATA=data)
    os.environ.setdefault("OMP_NUM_THREADS", "2")
    shutil.rmtree(data, ignore_errors=True)
    root = os.path.join(data, "users", "local", "qm")
    os.makedirs(root)
    shutil.copy(eb.PROJECT_META, os.path.join(root, ".ecce-meta"))
    tmpl = os.path.join(root, "template")
    src = glob.glob(os.path.join(GC, DIA, "ecceqm-N2", "stored", "*"))
    shutil.copytree([s for s in src if os.path.isdir(s)][0], tmpl, symlinks=True)
    for sub in ("Props", "Outputs"):
        for f in glob.glob(os.path.join(tmpl, sub, "*")):
            (shutil.rmtree if os.path.isdir(f) else os.remove)(f)
    os.makedirs(os.path.join(tmpl, "Inputs"), exist_ok=True)
    meta_set(os.path.join(tmpl, ".ecce-meta"), "state", "Ready")
    meta_set(os.path.join(tmpl, ".ecce-meta"), "launch_machine", "localhost")
    xyzdir = os.path.join(out, "xyz")
    os.makedirs(xyzdir, exist_ok=True)
    xdisplay.SCREEN = "1500x950x24"
    with xdisplay.Display() as display:
        subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-start")], env=display.env(), timeout=180)
        try:
            for n in names:
                struct, charge, mult, opt = JOBS[n]
                at = atoms_of(struct) if isinstance(struct, str) else struct
                xyz = os.path.join(xyzdir, n + ".xyz")
                open(xyz, "w").write("%d\n%s\n" % (len(at), n) +
                                     "".join("%s %.6f %.6f %.6f\n" % a for a in at))
                r = subprocess.run([os.path.join(apps.WRAPPERS, "ecce-series"), "--charge", str(charge),
                                    "--mult", str(mult), tmpl, root, xyz],
                                   env=display.env(), capture_output=True, text=True, timeout=300)
                print("series", n, r.returncode, r.stdout.strip(), r.stderr[-400:], flush=True)
                url = "file://" + os.path.join(root, n) + "/"
                steps = ["wait 8000"]
                if opt.get("theory"):
                    steps += ["theory " + opt["theory"], "wait 4000"]
                # The theory GUI values exist once the Details dialog has run.
                steps += ["details theory", "wait 5000"]
                steps += ["runtype " + opt.get("runtype", "GeoVib"), "wait 3000"]
                if opt.get("basis"):
                    steps += ["basis " + opt["basis"], "wait 3000"]
                for k in ("Loewdin", "CHELPG", "MK"):
                    steps.append("guiw ES.Theory.ECCEQM.%s 1" % k)
                if opt.get("unrestricted"):
                    steps.append("guiw ES.Theory.ECCEQM.Unrestricted 1")
                steps += ["wait 2000", "close-details", "wait 2000", "button save", "wait 6000", "info", "quit"]
                log = app(display, out, "calced", "ECCE_CALCED_SCRIPT", steps, url)
                print(n, "\n".join(l for l in log.splitlines() if "FAIL" in l)[-600:], flush=True)
                qmin = glob.glob(os.path.join(root, n, "Inputs", "*.qmin"))
                if not qmin:
                    print(n, "NO INPUT", flush=True)
                    continue
                keys = [l for l in open(qmin[0]).read().splitlines()
                        if re.match(r"(runtype|method|charge|multiplicity|charges|localise|basis )", l.strip())]
                print(n, "input:", keys, flush=True)
            # Launch, at most PARALLEL running at once.
            pending = [n for n in names if glob.glob(os.path.join(root, n, "Inputs", "*.qmin"))]
            def state(n):
                # A finished output counts even if its job monitor has gone.
                s = meta_get(os.path.join(root, n, ".ecce-meta"), "state")
                if s not in TERMINAL and any("end_of_output" in open(f, errors="replace").read()[-200:]
                                             for f in glob.glob(os.path.join(data, "run-" + n, "**", "*.qmout"),
                                                                recursive=True)):
                    return "Finished(" + s + ")"
                return s
            launched = []
            end = time.time() + 20 * 3600
            while time.time() < end:
                running = [n for n in launched if not state(n).startswith(TERMINAL)]
                while pending and len(running) < PARALLEL:
                    n = pending.pop(0)
                    run = os.path.join(data, "run-" + n)
                    os.makedirs(run, exist_ok=True)
                    log = app(display, out, "launcher", "ECCE_LAUNCHER_SCRIPT",
                              ["wait 3000", "machine localhost", "rundir " + run, "wait 1500"] +
                              # The "Large ECCE-QM calculation" question is a modal
                              # dialog in the stand-alone Launcher; answer Yes.
                              (["exec 'sh -c \"(xdotool search --sync --name Large windowfocus --sync"
                                " key Return) >/dev/null 2>&1 &\"'"] if os.environ.get("LAUNCH_YES") else []) +
                              ["launch",
                               "wait 8000", "quit"], "file://" + os.path.join(root, n) + "/", settle=30)
                    print(time.strftime("%H:%M:%S"), "launched", n, state(n), flush=True)
                    launched.append(n)
                    running.append(n)
                if not pending and not running:
                    break
                print(time.strftime("%H:%M:%S"), {n: state(n) for n in launched}, flush=True)
                time.sleep(60)
            for n in launched:
                d = os.path.join(out, n)
                os.makedirs(d, exist_ok=True)
                for f in glob.glob(os.path.join(root, n, "Inputs", "*.qmin")) + \
                        (glob.glob(os.path.join(root, n, "Outputs", "*.qmout")) or
                         glob.glob(os.path.join(data, "run-" + n, "**", "*.qmout"), recursive=True)):
                    shutil.copy(f, d)
            print("states:", {n: state(n) for n in launched}, flush=True)
        finally:
            subprocess.run([os.path.join(apps.INSTALL, "bin", "ecce-gateway-stop")], env=display.env(), timeout=120)


if __name__ == "__main__":
    main()
