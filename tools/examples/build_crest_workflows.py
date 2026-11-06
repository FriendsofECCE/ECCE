#!/usr/bin/env python3
"""Build the CREST workflow examples (#841) with ECCE's own recipes.

    tools/examples/build_crest_workflows.py create  --work DIR --jobs DIR
    tools/examples/build_crest_workflows.py organizer --work DIR CMD...
    tools/examples/build_crest_workflows.py save    --work DIR --jobs DIR STEP...
    tools/examples/build_crest_workflows.py import  --work DIR RESULTS STEP...

The workflows live in an isolated local-data ECCE under --work (its own HOME,
state and Xvfb display), so the decks are the ones the Calculation Editor
writes.  "save" opens the editor on each step (WORKFLOW/STEP, e.g.
solv/crest), saves, and writes the deck plus a Slurm script into
--jobs/<workflow>-<step>/; the compute runs elsewhere.  "import" puts each
step's output from RESULTS/<workflow>-<step>/ into the step as the job store
does (Outputs/, Re-Parse Outputs, state completed).

ECCE_TEST_HOME and ECCE_TEST_WRAPPERS name the installed tree, as for the
app tests.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
APPS = os.path.join(REPO, "tests", "apps")
FIXTURE = os.path.join(APPS, "fixtures", "calc-water-opt")
PROJECT_META = os.path.join(REPO, "tools", "screenshots", "data",
                            "project.ecce-meta")
DISPLAY = 253

# workflow: (recipe, start structure, charge)
WORKFLOWS = {
    "solv": ("solvation-shell", [("Na", 0.0, 0.0, 0.0)], 1),
    # 4-aminopyridine (Open Babel --gen3d)
    "prot": ("protonation-sites", [
        ("N", 2.39079, 0.01049, -0.18870), ("C", 1.00274, 0.00644, -0.00275),
        ("C", 0.28437, 1.19886, -0.03022), ("C", -1.10025, 1.14563, 0.02422),
        ("N", -1.80721, -0.00254, 0.06799), ("C", -1.09262, -1.14614, 0.02924),
        ("C", 0.29228, -1.19069, -0.02507), ("H", 2.84976, 0.85525, 0.13867),
        ("H", 2.85510, -0.83001, 0.14196), ("H", 0.78504, 2.15779, -0.09370),
        ("H", -1.69403, 2.05503, 0.02096), ("H", -1.68044, -2.05946, 0.03005),
        ("H", 0.79918, -2.14657, -0.08446)], 0),
}
TITLES = {"solv": "na_water", "prot": "aminopyridine"}

# Deck and output names by code, as in the .edml files.
DECKS = {"xtb": ("xtb.xtbin", "xtb.xtbout"), "crest": ("crest.crestin", "crest.crestout"),
         "orca": ("orca.orcain", "orca.orcaout")}


def isolate_env(work):
    """Own HOME, cache, state and data under work; nothing of the real ones."""
    home = os.path.join(work, "home")
    for d in (home, os.path.join(home, ".cache"), os.path.join(work, "state"),
              os.path.join(work, "data", "users", "local")):
        os.makedirs(d, exist_ok=True)
    os.environ["HOME"] = home
    os.environ["XDG_CACHE_HOME"] = os.path.join(home, ".cache")
    os.environ["ECCE_LOCAL_DATA"] = os.path.join(work, "data")
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    sys.path.insert(0, APPS)
    import isolate
    settings = isolate.apply(os.environ["ECCE_TEST_HOME"], os.path.join(work, "state"))
    print(isolate.describe(settings))


class Ecce(object):
    """The Xvfb display and the gateway of the isolated ECCE."""

    def __init__(self, work):
        isolate_env(work)
        import apps
        import xdisplay
        self.apps = apps
        self.work = work
        self.display = xdisplay.Display(number=DISPLAY)

    def __enter__(self):
        self.display.__enter__()
        start = os.path.join(self.apps.INSTALL, "bin", "ecce-gateway-start")
        subprocess.run([start], env=self.display.env(), timeout=180)
        return self

    def __exit__(self, *exc):
        stop = os.path.join(self.apps.INSTALL, "bin", "ecce-gateway-stop")
        subprocess.run([stop], env=self.display.env(), timeout=120)
        self.display.__exit__(*exc)

    def organizer(self, commands, tag="org", settle=40):
        script = os.path.join(self.work, "commands-" + tag)
        with open(script, "w") as f:
            f.write("\n".join(commands) + "\n")
        result = self.apps.run(self.display, "organizer", windowTimeout=60,
                               settle=settle,
                               env={"ECCE_TEST_ORGANIZER": script,
                                    "ECCE_TEST_ORGANIZER_NOLAUNCH": "1"})
        log = result.log or ""
        with open(os.path.join(self.work, "log-" + tag), "w") as f:
            f.write(log)
        if result.crashed:
            raise SystemExit("Organizer crashed:\n" + log[-3000:])
        answers = re.findall(r"ECCE_TEST_ORGANIZER: (.*)$", log, re.M)
        for a in answers:
            print("  " + a[:400])
        return log

    def save(self, url, tag):
        commands = os.path.join(self.work, "calced-" + tag)
        with open(commands, "w") as f:
            f.write("state\nsave\nstate\n")
        result = self.apps.run(self.display, "calced", args=("-context", url),
                               windowTimeout=60, settle=25,
                               env={"ECCE_TEST_CALCED": commands,
                                    "ECCE_TMPDIR": self.work})
        log = result.log or ""
        with open(os.path.join(self.work, "log-calced-" + tag), "w") as f:
            f.write(log)
        for a in re.findall(r"ECCE_TEST_CALCED: (.*)$", log, re.M):
            print("  " + a[:300])
        return log


def project_dir(work, wf):
    return os.path.join(work, "data", "users", "local", wf)


def wf_url(work, wf):
    return "file://" + project_dir(work, wf) + "/Workflow"


def make_seed(work, wf):
    """A stored calculation holding only the start structure and charge."""
    recipe, atoms, charge = WORKFLOWS[wf]
    project = project_dir(work, wf)
    os.makedirs(project, exist_ok=True)
    shutil.copy(PROJECT_META, os.path.join(project, ".ecce-meta"))
    seed = os.path.join(project, "seed")
    shutil.rmtree(seed, ignore_errors=True)
    shutil.copytree(FIXTURE, seed, symlinks=True)
    for d in ("Props", "Inputs", "Outputs"):
        shutil.rmtree(os.path.join(seed, d), ignore_errors=True)
    chem = ["title: " + TITLES[wf], "type: molecule", "num_atoms: %d" % len(atoms),
            "atom_info: symbol cart", "atom_list: "]
    chem += ["%s %.5f %.5f %.5f" % a for a in atoms]
    chem += ["attr_list:"] + [" 0.000000   1   0   0   0"] * len(atoms)
    chem += ["num_bonds: 0", "bond_list: "]
    with open(os.path.join(seed, "Parameters", "chemsys.mvm"), "w") as f:
        f.write("\n".join(chem) + "\n")
    meta = os.path.join(seed, ".ecce-meta")
    lines = [l for l in open(meta, errors="surrogateescape").read().split("\n")
             if l and "ecce:hasProperties" not in l]
    with open(meta, "w", errors="surrogateescape") as f:
        f.write("\n".join(lines) + "\n")
    # The charge is a property of the structure document.
    meta = os.path.join(seed, "Parameters", ".ecce-meta")
    lines = [l for l in open(meta, errors="surrogateescape").read().split("\n")
             if l and not l.startswith("chemsys.mvm\t")]
    lines.append("chemsys.mvm\thttp://www.emsl.pnl.gov/ecce:charge\t\t%d" % charge)
    with open(meta, "w", errors="surrogateescape") as f:
        f.write("\n".join(lines) + "\n")


def cmd_create(args):
    with Ecce(args.work) as e:
        commands = []
        for wf in WORKFLOWS:
            make_seed(args.work, wf)
            proj = "file://" + project_dir(args.work, wf)
            url = wf_url(args.work, wf)
            commands += ["new %s workflow" % proj,
                         "workflowcopy %s %s/seed seed" % (url, proj),
                         "workflowrecipe %s %s orca %s/seed" % (url, WORKFLOWS[wf][0], url),
                         "workflowinfo %s" % url]
        e.organizer(commands, "create")


def cmd_organizer(args):
    with Ecce(args.work) as e:
        e.organizer([c.replace("@solv", wf_url(args.work, "solv"))
                      .replace("@prot", wf_url(args.work, "prot"))
                     for c in args.commands], "manual", settle=args.settle)


def step_dir(work, spec):
    wf, step = spec.split("/")
    return os.path.join(project_dir(work, wf), "Workflow", step)


def code_of(folder):
    for code, (deck, _out) in DECKS.items():
        if os.path.exists(os.path.join(folder, "Inputs", deck)):
            return code
    return None


def cmd_save(args):
    with Ecce(args.work) as e:
        for spec in args.steps:
            wf, step = spec.split("/")
            e.save(wf_url(args.work, wf) + "/" + step, wf + "-" + step)
    for spec in args.steps:
        folder = step_dir(args.work, spec)
        code = code_of(folder)
        if not code:
            print("%s: no deck written" % spec)
            continue
        print("%s: %s" % (spec, os.path.join(folder, "Inputs", DECKS[code][0])))


#  The compute host: same home layout as here, Slurm, 6 physical cores.
HOST_CODES = {"xtb": ("xTB", "/usr/bin/xtb"), "crest": ("CREST", "/usr/local/bin/crest"),
              "orca": ("ORCA", "/home/andy/orca_6_1_1/orca")}
PROCS = 6
# QCG docks with `xtb dock`, which crashes in Debian's xtb 6.7.1 (#758); the
# solvation-shell CREST step runs with the upstream static build.
UPSTREAM_XTB = "/home/andy/opt/xtb-dist"


def gensub_script(code, rundir, scratch):
    """The job script ECCE's gensub writes for a Shell machine."""
    install = os.environ["ECCE_TEST_HOME"]
    cfg = os.path.join(scratch, "gensub-cfg")
    os.makedirs(cfg, exist_ok=True)
    with open(os.path.join(cfg, "MyMachines"), "w") as f:
        f.write("radium\tradium\tGeneric\tradium\tUnspecified\t1:%d\tssh\t:xTB:CREST:ORCA\t"
                "MN:RD:SD:Q:TL:MM:SS:AA\n" % PROCS)
    with open(os.path.join(cfg, "CONFIG.radium"), "w") as f:
        for label, path in HOST_CODES.values():
            f.write("%s: %s\n" % (label, path))
        f.write("perlPath: /usr/bin\n")
    deck, out = DECKS[code]
    target = os.path.join(scratch, "submit-" + code)
    env = dict(os.environ, ECCE_HOME=install, ECCE_REALUSERHOME=scratch,
               ECCE_SITECONFIG_DIRS=cfg + ":" + os.path.join(install, "siteconfig"))
    subprocess.run(["perl", os.path.join(install, "scripts", "gensub"),
                    "-c", HOST_CODES[code][0], "-Q", "Shell", "-H", "radium",
                    "-r", rundir, "-i", deck, "-o", out, "-n", str(PROCS),
                    "-f", target], env=env, check=True)
    return open(target).read()


def write_job(jobs, name, code, deck_text, scratch, before="", hours=4):
    """jobs/name/: the deck ECCE wrote and job.sh, gensub's script for Slurm."""
    folder = os.path.join(jobs, name)
    os.makedirs(folder, exist_ok=True)
    deck = DECKS[code][0]
    with open(os.path.join(folder, deck), "w") as f:
        f.write(deck_text)
    script = gensub_script(code, folder, scratch)
    if code == "orca":
        # what orca.launchpp does to the stored deck at launch
        text = deck_text if "%pal" in deck_text else \
            "%%pal nprocs %d end\n%s" % (PROCS, deck_text)
        with open(os.path.join(folder, deck), "w") as f:
            f.write(text)
    slurm = "#SBATCH -J 841-%s\n#SBATCH -N 1\n#SBATCH -t %d:00:00\n" % (name, hours)
    slurm += ("#SBATCH -n %d\n" if code == "orca" else "#SBATCH -n 1\n#SBATCH -c %d\n") % PROCS
    slurm += "#SBATCH -o %s/slurm.out\n" % folder
    head, rest = script.split("\n", 1)
    body = rest
    if before:
        body = rest.replace("\n#  - code/queue manager section...\n",
                            "\n" + before + "\n#  - code/queue manager section...\n", 1)
    with open(os.path.join(folder, "job.sh"), "w") as f:
        f.write(head + "\n" + slurm + body)
    os.chmod(os.path.join(folder, "job.sh"), 0o755)
    return folder


def inputs_deck(work, spec):
    folder = step_dir(work, spec)
    return open(os.path.join(folder, "Inputs", DECKS[code_of(folder)][0])).read()


# The CREST deck as ECCE writes it from the source's final geometry: the
# coordinates of the source's last optimisation frame, in ai.crest's format.
FROM_XTBOPT = r"""#  The CREST step's geometry is the final structure of %(src)s, written as
#  ECCE writes it after that step has been imported (same atoms, same order).
if [ ! -s %(src)s/xtbopt.log ]; then echo "no %(src)s/xtbopt.log"; exit 1; fi
awk 'FNR==NR {if (left==0 && NF==1) {n=$1; left=n+1; k=0; next}
              if (left>0) {left--; if (left<n) {k++; s[k]=$1; x[k]=$2; y[k]=$3; z[k]=$4}}; next}
     body==0 && /^[A-Z][a-z]? / {body=1}
     body==1 && k2<n {k2++; printf "%%-2s  %%14.9f  %%14.9f  %%14.9f\n", s[k2], x[k2], y[k2], z[k2]; next}
     {print}' %(src)s/xtbopt.log %(deck)s.ecce > %(deck)s
"""


def cmd_jobs_a(args):
    """Phase A: both preopt steps, the CREST steps after them."""
    jobs = os.path.abspath(args.jobs)
    scratch = os.path.join(args.work, "gensub")
    os.makedirs(scratch, exist_ok=True)
    for wf in ("solv", "prot"):
        write_job(jobs, wf + "-preopt", "xtb", inputs_deck(args.work, wf + "/preopt"),
                  scratch, hours=1)
    # Na+ alone: the xTB step is a single point, so the geometry is the input's.
    up = ("export ECCE_XTB=%s/bin/xtb\nexport XTBPATH=%s/share/xtb\n"
          % (os.path.join(jobs, "xtb-dist"), os.path.join(jobs, "xtb-dist")))
    write_job(jobs, "solv-crest", "crest", inputs_deck(args.work, "solv/crest"),
              scratch, before=up, hours=10)
    folder = write_job(jobs, "prot-crest", "crest", inputs_deck(args.work, "prot/crest"),
                       scratch, hours=4,
                       before=FROM_XTBOPT % {"src": os.path.join(jobs, "prot-preopt"),
                                             "deck": "crest.crestin"})
    os.rename(os.path.join(folder, "crest.crestin"),
              os.path.join(folder, "crest.crestin.ecce"))
    dist = os.path.join(jobs, "xtb-dist")
    if not os.path.isdir(dist):
        os.makedirs(os.path.join(dist, "bin"))
        shutil.copy2(os.path.join(UPSTREAM_XTB, "bin", "xtb"), os.path.join(dist, "bin"))
        shutil.copytree(os.path.join(UPSTREAM_XTB, "share", "xtb"),
                        os.path.join(dist, "share", "xtb"))


def cmd_import(args):
    urls = []
    for spec in args.steps:
        folder = step_dir(args.work, spec)
        code = code_of(folder)
        out = os.path.join(args.results, spec.replace("/", "-"), DECKS[code][1])
        os.makedirs(os.path.join(folder, "Outputs"), exist_ok=True)
        shutil.copy(out, os.path.join(folder, "Outputs", DECKS[code][1]))
        wf, step = spec.split("/")
        urls.append(wf_url(args.work, wf) + "/" + step)
    with Ecce(args.work) as e:
        e.organizer(["reparseoutputs " + " ".join(urls)] +
                    ["setstate %s completed" % u for u in urls], "import", settle=60)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("create", "organizer", "save", "import", "jobs-a"):
        p = sub.add_parser(name)
        p.add_argument("--work", required=True)
        if name == "jobs-a":
            p.add_argument("--jobs", required=True)
        if name == "organizer":
            p.add_argument("--settle", type=int, default=40)
            p.add_argument("commands", nargs="+")
        if name == "save":
            p.add_argument("steps", nargs="+")
        if name == "import":
            p.add_argument("results")
            p.add_argument("steps", nargs="+")
    args = ap.parse_args()
    args.work = os.path.abspath(args.work)
    {"create": cmd_create, "organizer": cmd_organizer, "save": cmd_save,
     "import": cmd_import, "jobs-a": cmd_jobs_a}[args.cmd](args)


if __name__ == "__main__":
    main()
