#!/usr/bin/env python3
"""Rerun the 4-aminopyridine protonation workflow example (#841).

    tools/examples/rerun_aminopyridine.py HOST STEP [--procs 4] [--pick K ...]

STEP is one of
    xtb     GFN2-xTB optimisation of the neutral molecule
    crest   CREST --protonate from the xTB geometry
    orca    r2SCAN-3c optimisation + frequencies of the protomers numbered
            by --pick (1-based, as ECCE's conformer pick cK)
The decks are made by the real ai.xtb / ai.crest / ai.orca and the job
scripts by gensub (so the appended frames and Molden file come with the
output), then run on HOST with Slurm (`sbatch --wait`).  Results land in
the example folders under data/client/examples/workflows/aminopyridine-protonation;
run tools/examples/preparse.py workflows afterwards.

Needs ECCE_HOME = an installed tree of this checkout, and xtb, crest and ORCA 6
on HOST at the paths in CODES.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
TOPIC = os.path.join(REPO, "data", "client", "examples", "workflows",
                     "aminopyridine-protonation")
PARSERS = os.path.join(REPO, "scripts", "parsers")
REMOTE = "ap841"          # run directory under the remote home

CODES = {"xtb": ("xTB", "/usr/bin/xtb"), "crest": ("CREST", "/usr/local/bin/crest"),
         "orca": ("ORCA", "~/orca_6_1_1/orca")}
DECK = {"xtb": ("xtb.xtbin", "xtb.xtbout"), "crest": ("crest.crestin", "crest.crestout"),
        "orca": ("orca.orcain", "orca.orcaout")}

# 4-aminopyridine (Open Babel --gen3d)
START = [("N", 2.39079, 0.01049, -0.18870), ("C", 1.00274, 0.00644, -0.00275),
         ("C", 0.28437, 1.19886, -0.03022), ("C", -1.10025, 1.14563, 0.02422),
         ("N", -1.80721, -0.00254, 0.06799), ("C", -1.09262, -1.14614, 0.02924),
         ("C", 0.29228, -1.19069, -0.02507), ("H", 2.84976, 0.85525, 0.13867),
         ("H", 2.85510, -0.83001, 0.14196), ("H", 0.78504, 2.15779, -0.09370),
         ("H", -1.69403, 2.05503, 0.02096), ("H", -1.68044, -2.05946, 0.03005),
         ("H", 0.79918, -2.14657, -0.08446)]


def aiDeck(code, work, atoms, param, basis=None):
    """Run ai.<code> on a fresh folder; returns the deck text."""
    os.makedirs(work, exist_ok=True)
    frag = ["x", "title:\tx", "type:\tmolecule", "num_atoms:\t%d" % len(atoms),
            "atom_info:\tsymbol cart", "atom_list:"]
    frag += [" %s %s %s %s" % tuple(a) for a in atoms]
    open(os.path.join(work, "x.frag"), "w").write("\n".join(frag) + "\n")
    open(os.path.join(work, "x.param"), "w").write(param)
    flags = ["-p", "-f"]
    deck = os.path.join(work, "deck")
    if code == "orca":
        open(os.path.join(work, "x.basis"), "w").write(basis)
        flags.append("-b")
        shutil.copy(os.path.join(PARSERS, "orca.tpl"), deck)
    elif code == "crest":
        shutil.copy(os.path.join(PARSERS, "crest.tpl"), deck)
    else:
        open(deck, "w").write("##XtbHeader##\n##chemsys##\n")
    subprocess.run(["perl", os.path.join(PARSERS, "ai." + code)] + flags +
                   ["-n", "x", "-t", deck], cwd=work, check=True,
                   env=dict(os.environ, ECCE_HOME=os.environ["ECCE_HOME"]),
                   capture_output=True)
    return open(deck).read()


def jobScript(code, local, remoteDir, procs, cfg):
    """gensub's script for a Shell machine, with Slurm headers added."""
    install = os.environ["ECCE_HOME"]
    label, path = CODES[code]
    os.makedirs(cfg, exist_ok=True)
    with open(os.path.join(cfg, "MyMachines"), "w") as f:
        f.write("nerdegg\tnerdegg\tGeneric\tnerdegg\tUnspecified\t1:%d\tssh\t:xTB:CREST:ORCA\t"
                "MN:RD:SD:Q:TL:MM:SS:AA\n" % procs)
    with open(os.path.join(cfg, "CONFIG.nerdegg"), "w") as f:
        for lab, p in CODES.values():
            f.write("%s: %s\n" % (lab, p.replace("~", "/home/andy")))
        f.write("perlPath: /usr/bin\n")
    deck, out = DECK[code]
    target = os.path.join(local, "job.gensub")
    env = dict(os.environ, ECCE_HOME=install, ECCE_REALUSERHOME=cfg,
               ECCE_SITECONFIG_DIRS=cfg + ":" + os.path.join(install, "siteconfig"))
    subprocess.run(["perl", os.path.join(install, "scripts", "gensub"),
                    "-c", label, "-Q", "Shell", "-H", "nerdegg", "-r", remoteDir,
                    "-i", deck, "-o", out, "-n", str(procs), "-f", target],
                   env=env, check=True, cwd=local)
    head, rest = open(target).read().split("\n", 1)
    slurm = "#SBATCH -J ap841-%s\n#SBATCH -N 1\n#SBATCH -t 4:00:00\n" % code
    slurm += ("#SBATCH -n %d\n" if code == "orca" else "#SBATCH -n 1\n#SBATCH -c %d\n") % procs
    slurm += "#SBATCH -o %s/slurm.out\n" % remoteDir
    with open(os.path.join(local, "job.sh"), "w") as f:
        f.write(head + "\n" + slurm + rest)
    os.chmod(os.path.join(local, "job.sh"), 0o755)


def run(host, code, name, deckText, procs, stage):
    """Send one job to HOST, wait for it, fetch the run directory to stage/name."""
    local = os.path.join(stage, name)
    shutil.rmtree(local, ignore_errors=True)
    os.makedirs(local)
    deck, _ = DECK[code]
    remoteDir = "/home/andy/%s/%s" % (REMOTE, name)
    if code == "orca":
        deckText = "%%pal nprocs %d end\n%s" % (procs, deckText)   # as orca.launchpp does
    open(os.path.join(local, deck), "w").write(deckText)
    jobScript(code, local, remoteDir, procs, os.path.join(stage, "cfg"))
    subprocess.run(["ssh", host, "rm -rf %s; mkdir -p %s" % (remoteDir, remoteDir)], check=True)
    subprocess.run(["scp", "-q", "-r", local + "/.", "%s:%s/" % (host, remoteDir)], check=True)
    rc = subprocess.run(["ssh", host, "cd %s && sbatch --wait job.sh" % remoteDir]).returncode
    subprocess.run(["scp", "-q", "-r", "%s:%s/." % (host, remoteDir), local], check=True)
    if rc != 0:
        raise SystemExit("%s: sbatch exit %d" % (name, rc))
    return local


def lastFrame(output):
    blocks = re.findall(r"ECCE-XTB-FRAME \d+ (\d+)\n(.*?)ECCE-XTB-ENDFRAME", output, re.S)
    n, body = blocks[-1]
    return [l.split()[:4] for l in body.strip("\n").split("\n")[1:]]


def crestFrames(output):
    """[(atoms, relative kcal/mol)] of the ECCE-CREST-FRAME blocks, in order."""
    frames = []
    for body in re.findall(r"ECCE-CREST-FRAME \d+ \d+\n(.*?)ECCE-CREST-ENDFRAME", output, re.S):
        lines = body.strip("\n").split("\n")
        rel = re.search(r"relative:\s*(-?[\d.]+)", lines[0])
        frames.append(([l.split()[:4] for l in lines[1:]], float(rel.group(1))))
    return frames


def store(local, code, folder, calc, deckExt):
    """Put the deck and output of a run into the example folder."""
    dst = os.path.join(TOPIC, folder)
    os.makedirs(dst, exist_ok=True)
    deck, out = DECK[code]
    text = open(os.path.join(local, deck)).read()
    text = re.sub(r"^%pal nprocs \d+ end\n", "", text)
    open(os.path.join(dst, calc + deckExt), "w").write(text)
    shutil.copy(os.path.join(local, out), os.path.join(dst, calc + ".out"))
    print("stored", dst)


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("host")
    ap.add_argument("step", choices=("xtb", "crest", "orca"))
    ap.add_argument("--procs", type=int, default=4)
    ap.add_argument("--pick", type=int, nargs="*", default=[])
    ap.add_argument("--stage", default=None, help="local working directory")
    args = ap.parse_args()
    stage = args.stage or tempfile.mkdtemp(prefix="ap841-")
    os.makedirs(stage, exist_ok=True)
    work = os.path.join(stage, "decks")
    xtbOut = os.path.join(TOPIC, "xtb-ap_preopt", "ap_preopt.out")
    crestOut = os.path.join(TOPIC, "crest-ap_protonation", "ap_protonation.out")

    if args.step == "xtb":
        deck = aiDeck("xtb", os.path.join(work, "xtb"), START,
                      "title:\tx\nTheory:\tGFN2-xTB\nCategory:\tSE\nRunType:\tGeometry\n"
                      "Charge:\t0\nES.ChemSys.Multiplicity:\t1\n"
                      "ES.Runtype.GeomOpt.Level:\tnormal\n")
        local = run(args.host, "xtb", "preopt", deck, args.procs, stage)
        store(local, "xtb", "xtb-ap_preopt", "ap_preopt", ".xtbin")
    elif args.step == "crest":
        geom = lastFrame(open(xtbOut).read())
        deck = aiDeck("crest", os.path.join(work, "crest"), geom,
                      "title:\tx\nCategory:\tSE\nTheory:\tGFN2-xTB\n"
                      "RunType:\tProtonation Sites\nCharge:\t0\n"
                      "ChemSys.Multiplicity:\t1\nES.Runtype.Sites.EnergyWindow:\t50\n")
        local = run(args.host, "crest", "protonation", deck, args.procs, stage)
        store(local, "crest", "crest-ap_protonation", "ap_protonation", ".crestin")
        for i, (atoms, rel) in enumerate(crestFrames(open(os.path.join(local, "crest.crestout")).read()), 1):
            nH = sum(1 for a in atoms if a[0] == "H")
            print("protomer %d: %.2f kcal/mol, %d atoms" % (i, rel, len(atoms)))
    else:
        frames = crestFrames(open(crestOut).read())
        for k in args.pick:
            atoms, rel = frames[k - 1]
            deck = aiDeck("orca", os.path.join(work, "orca%d" % k), atoms,
                          "Category: DFT\nTheory: RDFT\nRunType: GeoVib\nCharge: 1\n"
                          "ChemSys.Multiplicity: 1\nES.Theory.DFT.XCFunctionals: r2SCAN-3c\n",
                          basis="useRouteCard\n")
            local = run(args.host, "orca", "orca%d" % k, deck, args.procs, stage)
            name = {1: "ring_n", 4: "amino_n"}.get(k, "c%d" % k)
            store(local, "orca", "orca-ap_" + name, "ap_" + name, ".inp")


if __name__ == "__main__":
    main()
