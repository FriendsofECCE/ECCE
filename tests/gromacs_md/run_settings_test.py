#!/usr/bin/env python3
"""The GROMACS run settings of the MD task editor, through the real scripts.

Defines and position restraints, groups made from residue names (with the
index file), the barostat and coupling type, and free .mdp lines: each is
set the way the model XML carries it, run through md.gmxtask, and checked
against the .mdp text and then against grompp itself (the oracle for what
is accepted).  The index file is checked against an independent count of
the residue names in the structure.

    tests/gromacs_md/run_settings_test.py [-v]

Exit status 77 (CTest SKIP) without gmx.
"""

import argparse
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as rt  # noqa: E402

PARSERS = rt.PARSERS
check = rt.check


def esc(text):
    return (text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
            .replace("\n", "&#10;"))


def tag(name, value):
    return "    <%s>%s</%s>\n" % (name, esc(str(value)), name)


def mdp_of(task, interaction="", dynamics="", optimize=""):
    xml = rt.model(task, interaction=interaction, dynamics=dynamics,
                   optimize=optimize)
    r = rt.run([os.path.join(PARSERS, "md.gmxtask")], input=xml.encode())
    return r


def keys(mdp_text):
    """key -> [values] with - and _ alike and case ignored, comments dropped."""
    out = {}
    for line in mdp_text.splitlines():
        line = line.split(";")[0]
        m = re.match(r"^\s*(\S+)\s*=\s*(.*?)\s*$", line)
        if m:
            out.setdefault(m.group(1).lower().replace("_", "-"), []).append(m.group(2))
    return out


def npt(**kw):
    return rt.dyn(500, True, npt=True)


EQ = "ecceMdEquilibrate"


def text_tests():
    print("== defines and position restraints")
    ok, mdp = mdp_of(EQ, dynamics=npt())
    k = keys(mdp)
    check("define" not in k and "refcoord-scaling" not in k
          and "comm-grps" not in k and k["tc-grps"] == ["System"]
          and k["pcoupltype"] == ["isotropic"] and "nstpcouple" not in k,
          "no new setting, no new line")
    rc, mdp = mdp_of(EQ, dynamics=npt(),
                     interaction=tag("GmxDefines", "-DA  -DFLEXIBLE") +
                     tag("GmxPosres", 1) + tag("GmxPosresName", "POSRES_noH"))
    k = keys(mdp)
    check(k.get("define") == ["-DA -DFLEXIBLE -DPOSRES_noH"],
          "defines, then -D<restraint name> (%s)" % k.get("define"))
    check(k.get("refcoord-scaling") == ["com"],
          "restraints with pressure coupling write refcoord-scaling = com")
    rc, mdp = mdp_of(EQ, dynamics=npt(),
                     interaction=tag("GmxDefines", "-DPOSRES_noH -DB") +
                     tag("GmxPosres", 1) + tag("GmxPosresName", "POSRES_noH"))
    check(keys(mdp).get("define") == ["-DPOSRES_noH -DB"],
          "a restraint name already in the defines is not added twice")
    rc, mdp = mdp_of(EQ, dynamics=rt.dyn(500, False),
                     interaction=tag("GmxPosres", 1))
    k = keys(mdp)
    check(k.get("define") == ["-DPOSRES"] and "refcoord-scaling" not in k,
          "restraints with no barostat: -DPOSRES and no refcoord-scaling")
    rc, mdp = mdp_of("ecceMdOptimize", interaction=tag("GmxPosres", 1),
                     optimize="    <UseSD>1</UseSD>\n")
    check(keys(mdp).get("define") == ["-DPOSRES"],
          "an Optimize task takes the restraints too")

    print("== barostat, coupling type")
    for typ, rp, cp in ((0, "1.01325", "4.53e-05"),
                        (1, "1.01325 1.01325", "4.53e-05 4.53e-05"),
                        (2, "1.01325 1.01325 1.01325 0 0 0",
                         "4.53e-05 4.53e-05 4.53e-05 0 0 0")):
        rc, mdp = mdp_of(EQ, dynamics=npt(),
                         interaction=tag("GmxPcoupltype", typ) +
                         tag("GmxBarostat", 2) + tag("GmxNstpcouple", 20))
        k = keys(mdp)
        check(k["ref-p"] == [rp] and k["compressibility"] == [cp]
              and k["pcoupl"] == ["Parrinello-Rahman"]
              and k["nstpcouple"] == ["20"],
              "pcoupltype %d expands ref-p and compressibility (%s | %s)"
              % (typ, k["ref-p"], k["compressibility"]))
    rc, mdp = mdp_of(EQ, dynamics=rt.dyn(500, True),
                     interaction=tag("GmxBarostat", 1))
    check("pcoupl" not in keys(mdp), "no barostat setting without pressure coupling")

    print("== groups")
    groups = "MMT = MMT\nWater_and_ions = SOL Cu"
    rc, mdp = mdp_of(EQ, dynamics=npt(),
                     interaction=tag("GmxGroups", groups) + tag("GmxComGroups", 1)
                     + tag("GmxTcGroups", 1))
    k = keys(mdp)
    check(k["comm-grps"] == ["MMT Water_and_ions"]
          and k["tc-grps"] == ["MMT Water_and_ions"]
          and k["tau-t"] == ["0.1 0.1"] and k["ref-t"] == ["300 300"],
          "tc-grps with two groups repeats tau-t and ref-t (%s %s)"
          % (k["tau-t"], k["ref-t"]))
    check("; ecce-group MMT = MMT" in mdp
          and "; ecce-group Water_and_ions = SOL Cu" in mdp,
          "the group definitions are in the .mdp for the index writer")
    rc, mdp = mdp_of(EQ, dynamics=npt(),
                     interaction=tag("GmxGroups", groups) + tag("GmxTcGroups", 1))
    k = keys(mdp)
    check(k["tc-grps"] == ["MMT Water_and_ions"] and "comm-grps" not in k,
          "the two group choices are independent")
    r = rt.run([os.path.join(PARSERS, "md.gmxtask")],
               input=rt.model(EQ, dynamics=npt(),
                              interaction=tag("GmxTcGroups", 1)).encode())
    check(r[0] != 0 and "none is defined" in r[1],
          "defined groups chosen with none defined is an error, not System")

    print("== extra lines")
    extra = ("DispCorr = EnerPres\nFourierSpacing = 0.16 ; grid\n"
             "tau_p = 5\nVerlet_Buffer-Tolerance = 0.005\n; a comment\n"
             "NSTCALCENERGY = 100\ntau_p = 6")
    rc, mdp = mdp_of(EQ, dynamics=npt(), interaction=tag("GmxExtraMdp", extra))
    k = keys(mdp)
    check(k["tau-p"] == ["6"], "a user key replaces ours; the last of two wins (%s)"
          % k["tau-p"])
    check(all(len(v) == 1 for v in k.values()),
          "never two lines for one key (%s)" % [a for a, v in k.items() if len(v) > 1])
    check(k["dispcorr"] == ["EnerPres"] and k["fourierspacing"] == ["0.16"]
          and k["verlet-buffer-tolerance"] == ["0.005"]
          and k["nstcalcenergy"] == ["100"],
          "keys we never write are added")
    check("; a comment" in mdp, "comment lines pass through")
    rc, mdp = mdp_of(EQ, dynamics=npt(), interaction=tag("GmxExtraMdp", "TCOUPL = berendsen"))
    check(keys(mdp)["tcoupl"] == ["berendsen"], "keys are compared without regard to case")


def ndx_groups(path):
    out, cur = {}, None
    for line in open(path):
        m = re.match(r"^\[\s*(\S+)\s*\]", line)
        if m:
            cur = m.group(1)
            out[cur] = []
        elif cur:
            out[cur] += [int(x) for x in line.split()]
    return out


def residues(gro):
    lines = open(gro).read().splitlines()
    n = int(lines[1])
    return [l[5:10].strip() for l in lines[2:2 + n]]


def residue_names(binary):
    print("== residue names for the group button")
    tmp = tempfile.mkdtemp(prefix="gromacs-res-", dir=os.environ.get("XDG_CACHE_HOME")
                           or os.path.expanduser("~/.cache"))
    try:
        lines = open(os.path.join(rt.FIXTURES, "conf.gro")).read().splitlines()
        for i in range(2, 2 + 30):
            lines[i] = lines[i][:5] + "ION  " + lines[i][10:]
        for i in range(2 + 30, 2 + 45):
            lines[i] = lines[i][:5] + "a b  " + lines[i][10:]
        gro = os.path.join(tmp, "x.gro")
        open(gro, "w").write("\n".join(lines) + "\n")
        rc, out = rt.run([binary, gro, "--residues"])
        want = []
        for r in residues(gro):
            if r not in want:
                want.append(r)
        check(rc == 0 and out.split("\n")[:-1] == want,
              "the structure reader lists each residue name once, in order (%s vs %s)"
              % (out.split("\n")[:-1], want))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def chains(gmx, home, tmp):
    """Groups per protein chain, from the topology's [ molecules ] (the .gro
    has no chain column), against what pdb2gmx itself wrote."""
    print("== chains")
    d = os.path.join(tmp, "chains")
    os.makedirs(d)
    atoms = [l for l in open(os.path.join(rt.FIXTURES, "1l2y.pdb"))]
    first = []
    for l in atoms:
        if l.startswith("ENDMDL"):
            break
        if l.startswith("ATOM"):
            first.append(l)
    a = [l[:21] + "A" + l[22:] for l in first]
    b = [l[:21] + "B" + l[22:30] + "%8.3f" % (float(l[30:38]) + 30) + l[38:] for l in first]
    open(os.path.join(d, "two.pdb"), "w").write("".join(a) + "TER\n" + "".join(b) + "TER\nEND\n")
    rc, out = rt.run([gmx, "pdb2gmx", "-f", "two.pdb", "-ff", "amber99sb-ildn", "-water",
                      "tip3p", "-ignh", "-o", "c.gro", "-p", "topol.top", "-i", "p.itp"], cwd=d)
    if not check(rc == 0, "pdb2gmx made a two-chain protein"):
        print(out)
        return
    rt.run([gmx, "editconf", "-f", "c.gro", "-o", "box.gro", "-box", "6", "6", "6"], cwd=d)
    top = open(os.path.join(d, "topol.top")).read()
    mols = re.findall(r"^(Protein_chain_\w)\s+1", top, re.M)
    check(mols == ["Protein_chain_A", "Protein_chain_B"], "the topology lists the chains: %s" % mols)
    # the restraint switch pdb2gmx writes is one name for every chain
    itps = [f for f in os.listdir(d) if f.startswith("topol_Protein_chain")]
    check(all("#ifdef POSRES" in open(os.path.join(d, f)).read() for f in itps),
          "each chain's include is restrained by the same POSRES define")
    extras = dict((f, os.path.join(d, f)) for f in os.listdir(d)
                  if f.endswith(".itp"))
    spec = dict(dynamics=rt.dyn(200, False),
                interaction=tag("GmxGroups", "chainA = mol:Protein_chain_A\nchainB = mol:Protein_chain_B")
                + tag("GmxComGroups", 1) + tag("GmxTcGroups", 1) + tag("GmxPosres", 1)
                + tag("GmxPosresName", "POSRES") + tag("GmxExtraMdp", "coulombtype = Reaction-Field\nepsilon-rf = 0"))
    r = rt.stage(gmx, home, tmp, "chains", EQ, spec, os.path.join(d, "box.gro"),
                 topology=os.path.join(d, "topol.top"), extras=extras)
    if r is None:
        return
    g = ndx_groups(os.path.join(r["rundir"], "ecce_groups.ndx"))
    n = len(residues(os.path.join(d, "box.gro")))
    # chain A is the first half: both chains are copies of one model
    check(g["chainA"] == list(range(1, n // 2 + 1)) and g["chainB"] == list(range(n // 2 + 1, n + 1)),
          "chain A is atoms 1-%d and chain B the rest (%d, %d atoms)"
          % (n // 2, len(g["chainA"]), len(g["chainB"])))


def gmx_tests(gmx, home, tmp):
    print("== groups, index file, grompp")
    # two residue names in the water box: the first 100 waters are "WA1"
    src = os.path.join(rt.FIXTURES, "conf.gro")
    lines = open(src).read().splitlines()
    for i in range(2, 2 + 300):
        lines[i] = lines[i][:5] + "WA1  " + lines[i][10:]
    gro = os.path.join(tmp, "two.gro")
    open(gro, "w").write("\n".join(lines) + "\n")
    res = residues(gro)
    spec = dict(dynamics=npt(),
                interaction=tag("GmxGroups", "First = WA1\nRest = SOL") +
                tag("GmxComGroups", 1) + tag("GmxTcGroups", 1) +
                tag("GmxExtraMdp", "tau-p = 3"))
    r = rt.stage(gmx, home, tmp, "groups", EQ, spec, gro)
    if r is None:
        return
    ndx = os.path.join(r["rundir"], "ecce_groups.ndx")
    check(os.path.exists(ndx), "launch wrote the index file next to the inputs")
    if os.path.exists(ndx):
        g = ndx_groups(ndx)
        check(len(g["System"]) == len(res)
              and len(g["First"]) == res.count("WA1")
              and len(g["Rest"]) == res.count("SOL")
              and g["First"] == [i + 1 for i, x in enumerate(res) if x == "WA1"],
              "index groups hold the atoms of those residue names (First %d, "
              "Rest %d of %d)" % (len(g["First"]), len(g["Rest"]), len(res)))
    check(r["mdp"].get("tc-grps") == "First Rest",
          "the job ran grompp and mdrun with the two thermostat groups")

    # a user's own .ndx wins, and ECCE writes none
    user = os.path.join(tmp, "mine.ndx")
    with open(user, "w") as h:
        h.write("[ System ]\n" + " ".join(str(i + 1) for i in range(len(res))) + "\n"
                "[ First ]\n" + " ".join(str(i + 1) for i in range(300)) + "\n"
                "[ Rest ]\n" + " ".join(str(i + 1) for i in range(300, len(res))) + "\n")
    r = rt.stage(gmx, home, tmp, "userndx", EQ, spec, gro,
                 extras={"mine.ndx": user})
    check(r is not None and not os.path.exists(os.path.join(r["rundir"], "ecce_groups.ndx")),
          "an attached .ndx is used and none is written")

    # a residue name that is not in the structure stops the launch
    bad = dict(spec, interaction=tag("GmxGroups", "X = NOPE") + tag("GmxTcGroups", 1))
    rd = os.path.join(tmp, "run-badres")
    os.makedirs(rd)
    xml = rt.model(EQ, **bad)
    rc, out = rt.run([os.path.join(PARSERS, "md.gmxtask")], input=xml.encode())
    open(os.path.join(rd, "gromacs.mdp"), "w").write(out)
    shutil.copy(gro, os.path.join(rd, "conf.gro"))
    shutil.copy(os.path.join(rt.FIXTURES, "topol.top"), os.path.join(rd, "topol.top"))
    open(os.path.join(rd, "postParams"), "w").write("inputFile: gromacs.mdp\n")
    rc, out = rt.run([os.path.join(PARSERS, "gromacs.launchpp"), "-p", "postParams"],
                     cwd=rd)
    check(rc != 0 and "NOPE" in out and "WA1" in out,
          "a residue name not in the structure stops the launch and lists the ones that are")

    chains(gmx, home, tmp)

    # Berendsen is a grompp warning; the launch check and the job let it pass
    ber = dict(dynamics=npt(), interaction=tag("GmxBarostat", 1) + tag("GmxPcoupltype", 2)
               + tag("GmxNstpcouple", 10))
    r = rt.stage(gmx, home, tmp, "berendsen", EQ, ber, src)
    check(r is not None and r["mdp"].get("pcoupltype") == "anisotropic",
          "a Berendsen anisotropic NPT task launches and runs")

    print("== which barostat / type pairs grompp takes")
    expect = {("C-rescale", 2): "error", ("Berendsen", 0): "warn",
              ("Berendsen", 1): "warn", ("Berendsen", 2): "warn"}
    base = dict(dynamics=npt())
    for b in (0, 1, 2):
        for t in (0, 1, 2):
            names = ["C-rescale", "Berendsen", "Parrinello-Rahman"]
            rc, mdp = mdp_of(EQ, dynamics=npt(),
                             interaction=tag("GmxBarostat", b) + tag("GmxPcoupltype", t))
            d = os.path.join(tmp, "pair")
            os.makedirs(d, exist_ok=True)
            open(os.path.join(d, "p.mdp"), "w").write(mdp)
            for f in ("topol.top",):
                shutil.copy(os.path.join(rt.FIXTURES, f), d)
            shutil.copy(src, os.path.join(d, "conf.gro"))
            rc, out = rt.run([gmx, "grompp", "-f", "p.mdp", "-c", "conf.gro",
                              "-p", "topol.top", "-o", "p.tpr", "-maxwarn", "0"],
                             cwd=d)
            if re.search(r"does not support pressure coupling", out):
                got = "error"
            elif "barostat does not generate" in out:
                got = "warn"
            else:
                got = "ok"
            want = expect.get((names[b], t), "ok")
            check(got == want, "%s with %s: grompp says %s (the editor says %s)"
                  % (names[b], ["isotropic", "semiisotropic", "anisotropic"][t],
                     got, want))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-v", action="store_true")
    ap.add_argument("--groconv", help="the gromacs_groconv binary")
    args = ap.parse_args()
    rt.verbose = args.v

    text_tests()
    if args.groconv:
        residue_names(args.groconv)

    gmx = rt.find_gmx()
    if gmx is None:
        print("SKIP the grompp half: gmx is not installed")
    else:
        base = os.environ.get("XDG_CACHE_HOME") or os.path.expanduser("~/.cache")
        os.makedirs(base, exist_ok=True)
        tmp = tempfile.mkdtemp(prefix="gromacs-run-", dir=base)
        try:
            gmx_tests(gmx, rt.Home(tmp, gmx), tmp)
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    print("%s" % ("FAILED: %d" % len(rt.failures) if rt.failures else "PASSED"))
    return 1 if rt.failures else 0


if __name__ == "__main__":
    sys.exit(main())
