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
    # urea (Open Babel --gen3d).  Not an ion: CREST 3.0.2's QCG grows the
    # cluster of a charged solute as the neutral species.
    "solv": ("solvation-shell", [
        ("N", 1.06505, 0.07772, -0.01705), ("C", 2.40919, 0.03409, -0.07041),
        ("O", 3.01912, -0.63460, -0.88828), ("N", 3.02157, 0.79273, 0.85746),
        ("H", 0.55583, -0.47502, -0.69309), ("H", 0.55565, 0.62681, 0.65453),
        ("H", 2.52500, 1.34651, 1.53477), ("H", 4.03243, 0.79549, 0.86084)], 0),
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
# Na+(H2O)6, all six waters on the ion (the start of the former example)
NA6 = [("Na", 0.0, 0.0, 0.0),
       ("O", 2.4, 0.0, 0.0), ("H", 1.812271, 0.643683, -0.402302), ("H", 1.812271, -0.643683, 0.402302),
       ("O", -2.4, 0.0, 0.0), ("H", -1.812271, -0.643683, 0.402302), ("H", -1.812271, 0.643683, -0.402302),
       ("O", 0.0, 2.4, 0.0), ("H", -0.710732, 1.812271, 0.266524), ("H", 0.710732, 1.812271, -0.266524),
       ("O", 0.0, -2.4, 0.0), ("H", 0.710732, -1.812271, -0.266524), ("H", -0.710732, -1.812271, 0.266524),
       ("O", 0.0, 0.0, 2.4), ("H", 0.65089, -0.390534, 1.812271), ("H", -0.65089, 0.390534, 1.812271),
       ("O", 0.0, 0.0, -2.4), ("H", -0.65089, 0.390534, -1.812271), ("H", 0.65089, -0.390534, -1.812271)]
WORKFLOWS["na"] = ("xtb-crest", NA6, 1)
RECIPE_FLAGS = {"na": " solvent=water"}
TITLES = {"na": "na_water6", "solv": "urea_water", "prot": "aminopyridine"}

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
        for wf in args.only or ("solv", "prot"):
            make_seed(args.work, wf)
            proj = "file://" + project_dir(args.work, wf)
            url = wf_url(args.work, wf)
            commands += ["new %s workflow" % proj,
                         "workflowcopy %s %s/seed seed" % (url, proj),
                         "workflowrecipe %s %s orca %s/seed%s" % (url, WORKFLOWS[wf][0], url,
                                                                     RECIPE_FLAGS.get(wf, "")),
                         "workflowinfo %s" % url]
        e.organizer(commands, "create")


def cmd_organizer(args):
    with Ecce(args.work) as e:
        e.organizer([c.replace("@solv", wf_url(args.work, "solv"))
                      .replace("@prot", wf_url(args.work, "prot"))
                     .replace("@na", wf_url(args.work, "na"))
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
# Neither Debian's xtb 6.7.1 nor the 6.7.1 release docks (`xtb dock` dies in
# its final optimisation, #758); the upstream bleeding-edge build does.
UPSTREAM_XTB = "/home/andy/opt/xtb-bleed/stage"


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
                    "-f", target], env=env, check=True, cwd=rundir)
    return open(target).read()


def write_job(jobs, name, code, deck_text, scratch, before="", hours=4):
    """jobs/name/: the deck ECCE wrote and job.sh, gensub's script for Slurm."""
    folder = os.path.join(jobs, name)
    os.makedirs(folder, exist_ok=True)
    deck = DECKS[code][0]
    if code == "orca" and "%pal" not in deck_text:
        # what orca.launchpp does to the stored deck before gensub reads it
        deck_text = "%%pal nprocs %d end\n%s" % (PROCS, deck_text)
    with open(os.path.join(folder, deck), "w") as f:
        f.write(deck_text)
    script = gensub_script(code, folder, scratch)
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


# ECCE keeps a carried structure to six significant digits; the decks print
# that value (ai.xtb/ai.crest "%14.9f", ai.orca the stored text, "%#.6g").
SUBSTITUTE = r"""#  The geometry is the final structure of %(src)s, written as ECCE
#  writes it once that step is imported (same atoms, same order).
if [ ! -s %(src)s ]; then echo "no %(src)s"; exit 1; fi
awk -v frame=%(frame)s -v style=%(style)s '
  function r(v) { return sprintf("%%.6g", v) + 0 }
  function f(v) { return style == "orca" ? sprintf("%%#.6g", r(v)) : sprintf("%%14.9f", r(v)) }
  FNR==NR {if (left==0 && NF==1) {n=$1; left=n+1; nf++; k=0; next}
           if (left>0) {left--; if (left<n && (frame=="last" || nf==frame)) {k++; s[k]=$1; x[k]=$2; y[k]=$3; z[k]=$4}}; next}
  body==0 && style=="orca" && /^\* xyz/ {body=1; print; next}
  body==0 && style!="orca" && NF==1 && $1==n {print; getline; print; body=1; next}
  body==1 && k2<n {k2++; sym=toupper(substr(s[k2],1,1)) tolower(substr(s[k2],2))
    if (style=="orca") printf " %%s     %%s     %%s     %%s\n", sym, f(x[k2]), f(y[k2]), f(z[k2])
    else printf "%%-2s  %%s  %%s  %%s\n", sym, f(x[k2]), f(y[k2]), f(z[k2]); next}
  {print}' %(src)s %(deck)s.ecce > %(deck)s
"""


def chained(jobs, name, code, spec, work, scratch, src, frame, hours):
    """A job whose ECCE deck gets the geometry of an earlier job at run time."""
    deck = DECKS[code][0]
    folder = write_job(jobs, name, code, inputs_deck(work, spec), scratch, hours=hours,
                       before=SUBSTITUTE % {"src": src, "frame": frame, "deck": deck,
                                            "style": "orca" if code == "orca" else "xyz"})
    os.rename(os.path.join(folder, deck), os.path.join(folder, deck + ".ecce"))
    return folder


def cmd_jobs_b(args):
    """Round 2: urea preopt and QCG; protonation CREST with the wider window,
    then xTB and ORCA of the lowest protomer."""
    jobs = os.path.abspath(args.jobs)
    scratch = os.path.join(args.work, "gensub")
    os.makedirs(scratch, exist_ok=True)
    j = lambda n: os.path.join(jobs, n)
    write_job(jobs, "solv-preopt", "xtb", inputs_deck(args.work, "solv/preopt"),
              scratch, hours=1)
    folder = chained(jobs, "solv-crest", "crest", "solv/crest", args.work, scratch,
                     j("solv-preopt/xtbopt.log"), "last", 10)
    up = ("export ECCE_XTB=%s/bin/xtb\nexport XTBPATH=%s/share/xtb\n"
          % (j("xtb-dist"), j("xtb-dist")))
    text = open(os.path.join(folder, "job.sh")).read()
    text = text.replace("#  The geometry is", up + "#  The geometry is", 1)
    open(os.path.join(folder, "job.sh"), "w").write(text)
    write_job(jobs, "prot-crest2", "crest", open(args.prot_crest).read(), scratch, hours=2)
    # protonated.xyz lists the protomers lowest first: frame 1 is ECCE's c1.
    chained(jobs, "prot-xtb", "xtb", "prot/xtb", args.work, scratch,
            j("prot-crest2/protonated.xyz"), "1", 1)
    chained(jobs, "prot-dft", "orca", "prot/dft", args.work, scratch,
            j("prot-xtb/xtbopt.log"), "last", 12)
    dist = j("xtb-dist")
    shutil.rmtree(dist, ignore_errors=True)
    os.makedirs(os.path.join(dist, "bin"))
    shutil.copy2(os.path.join(UPSTREAM_XTB, "bin", "xtb"), os.path.join(dist, "bin"))
    shutil.copytree(os.path.join(UPSTREAM_XTB, "share", "xtb"), os.path.join(dist, "share", "xtb"))


CHARGE_CHECK = """#!/bin/sh
#SBATCH -J 841-na-check
#SBATCH -N 1
#SBATCH -n 1
#SBATCH -c 1
#SBATCH -t 0:20:00
#SBATCH -o %(dir)s/slurm.out
# Does CREST's conformer search treat Na+(H2O)6 as a cation in ALPB water?
# Its best energy should equal an xtb single point at +1 with ALPB, not one
# at 0 or without solvent (CREST 3.0.2's QCG drops both).
cd %(dir)s || exit 1
cp %(crest)s/crest_best.xyz best.xyz || exit 1
for c in "1 --alpb water" "1" "0 --alpb water" "0"; do
  /usr/bin/xtb best.xyz --gfn 2 --sp --chrg $c > sp.out 2>&1
  printf '%%s\t%%s\n' "chrg $c" "$(awk '/TOTAL ENERGY/ {print $4}' sp.out)"
done > energies.txt
echo "crest_best\t$(sed -n 2p best.xyz)" >> energies.txt
cat energies.txt
"""


def cmd_jobs_c(args):
    """Round 3: urea cluster xTB then ORCA; ring-C and amine-N protomers xTB
    then ORCA; Na+(H2O)6 xTB, CREST conformers and the charge check."""
    jobs = os.path.abspath(args.jobs)
    scratch = os.path.join(args.work, "gensub")
    os.makedirs(scratch, exist_ok=True)
    j = lambda n: os.path.join(jobs, n)
    write_job(jobs, "solv-xtb", "xtb", inputs_deck(args.work, "solv/xtb"), scratch, hours=2)
    chained(jobs, "solv-dft", "orca", "solv/dft", args.work, scratch,
            j("solv-xtb/xtbopt.log"), "last", 12)
    for c in ("c2", "c4"):
        write_job(jobs, "prot-xtb-" + c, "xtb", inputs_deck(args.work, "prot/xtb-" + c),
                  scratch, hours=1)
        chained(jobs, "prot-dft-" + c, "orca", "prot/dft-" + c, args.work, scratch,
                j("prot-xtb-%s/xtbopt.log" % c), "last", 12)
    write_job(jobs, "na-xtb", "xtb", inputs_deck(args.work, "na/xtb"), scratch, hours=1)
    chained(jobs, "na-crest", "crest", "na/crest", args.work, scratch,
            j("na-xtb/xtbopt.log"), "last", 10)
    os.makedirs(j("na-check"), exist_ok=True)
    with open(j("na-check/job.sh"), "w") as f:
        f.write(CHARGE_CHECK % {"dir": j("na-check"), "crest": j("na-crest")})
    os.chmod(j("na-check/job.sh"), 0o755)


SETUP_KEYS = ("runtype", "theory", "useExpCoeff", "basisSetDocument",
              "setupParametersDocument")


def copy_setup(src, dst):
    """The level of a recipe step on a step made by hand (code button), as the
    user sets it in the Calculation Editor: setup, basis set, theory, run type."""
    def meta(path):
        return [l for l in open(path, errors="surrogateescape").read().split("\n") if l]
    for f in ("SetupParams", "BasisSet.ecce_basisset"):
        if os.path.exists(os.path.join(src, "Parameters", f)):
            shutil.copy(os.path.join(src, "Parameters", f), os.path.join(dst, "Parameters", f))
    pm = os.path.join(dst, "Parameters", ".ecce-meta")
    lines = [l for l in meta(pm) if not l.startswith("BasisSet.ecce_basisset\t")]
    lines += [l for l in meta(os.path.join(src, "Parameters", ".ecce-meta"))
              if l.startswith("BasisSet.ecce_basisset\t")]
    open(pm, "w", errors="surrogateescape").write("\n".join(lines) + "\n")
    keys = ["ecce:" + k + "\t" for k in SETUP_KEYS] + [":hasConfigurations\t"]
    picked = [l for l in meta(os.path.join(src, ".ecce-meta")) if any(k in l for k in keys)]
    sm = os.path.join(dst, ".ecce-meta")
    lines = [l for l in meta(sm) if not any(k in l for k in keys)] + picked
    open(sm, "w", errors="surrogateescape").write("\n".join(lines) + "\n")


def cmd_copysetup(args):
    for pair in args.pairs:
        src, dst = pair.split("=")
        copy_setup(step_dir(args.work, src), step_dir(args.work, dst))


def cmd_import(args):
    urls = []
    for spec in args.steps:
        folder = step_dir(args.work, spec)
        code = code_of(folder)
        job = os.path.join(args.results, args.job_names.get(spec, spec.replace("/", "-")))
        os.makedirs(os.path.join(folder, "Outputs"), exist_ok=True)
        shutil.copy(os.path.join(job, DECKS[code][1]),
                    os.path.join(folder, "Outputs", DECKS[code][1]))
        # the deck that ran, as the launch leaves it in Inputs/
        shutil.copy(os.path.join(job, DECKS[code][0]),
                    os.path.join(folder, "Inputs", DECKS[code][0]))
        wf, step = spec.split("/")
        urls.append(wf_url(args.work, wf) + "/" + step)
    with Ecce(args.work) as e:
        e.organizer(["reparseoutputs " + " ".join(urls)] +
                    ["setstate %s completed" % u for u in urls], "import", settle=60)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("create", "organizer", "save", "import", "jobs-a", "jobs-b", "jobs-c", "copysetup"):
        p = sub.add_parser(name)
        p.add_argument("--work", required=True)
        if name in ("jobs-a", "jobs-b", "jobs-c"):
            p.add_argument("--jobs", required=True)
        if name == "jobs-b":
            p.add_argument("--prot-crest", required=True,
                           help="the protonation CREST deck ECCE wrote after the preopt import")
        if name == "create":
            p.add_argument("--only", nargs="*")
        if name == "organizer":
            p.add_argument("--settle", type=int, default=40)
            p.add_argument("commands", nargs="+")
        if name == "copysetup":
            p.add_argument("pairs", nargs="+", help="WF/SRC=WF/DST")
        if name == "save":
            p.add_argument("steps", nargs="+")
        if name == "import":
            p.add_argument("--job", action="append", default=[],
                           help="STEP=JOBDIR when the job directory has another name")
            p.add_argument("results")
            p.add_argument("steps", nargs="+")
    args = ap.parse_args()
    args.work = os.path.abspath(args.work)
    args.job_names = dict(j.split("=", 1) for j in getattr(args, "job", []))
    {"create": cmd_create, "organizer": cmd_organizer, "save": cmd_save,
     "import": cmd_import, "jobs-a": cmd_jobs_a, "jobs-b": cmd_jobs_b, "copysetup": cmd_copysetup, "jobs-c": cmd_jobs_c}[args.cmd](args)


if __name__ == "__main__":
    main()
