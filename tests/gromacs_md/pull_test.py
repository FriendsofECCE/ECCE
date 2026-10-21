#!/usr/bin/env python3
"""GROMACS centre-of-mass pulling: the .mdp block and a real short pull run.

The settings are set the way the model XML carries them and run through the
real md.gmxtask, then a two-chain system (the 1L2Y fixture twice, chain B
3 nm away) is prepared, minimised and pulled through the real
gromacs.launchpp and the job script gensub writes.  gmx is the oracle:

  * the .mdp has the pull keys grompp 2025 reads, with the values set;
  * pullx.xvg's coordinate rises by rate*time, measured on gmx's own output
    and again on the joined ecce-parse-pull.txt that gromacs.pull parses;
  * the force is non-zero, and the properties carry as many values as
    pullx.xvg has frames.

    tests/gromacs_md/pull_test.py [-v]

Exit status 77 (CTest SKIP) without gmx.
"""
import os
import re
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as rt  # noqa: E402
from run_tests import check, FIXTURES, PARSERS, run  # noqa: E402

RATE = 0.03     # nm/ps
K = 5000        # kJ/mol/nm^2
STEPS = 2000    # 2 fs: 4 ps


def esc(text):
    return (text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
            .replace("\n", "&#10;"))


def tag(name, value):
    return "    <%s>%s</%s>\n" % (name, esc(str(value)), name)


GROUPS = "Chain_A = mol:Protein_chain_A\nChain_B = mol:Protein_chain_B"


def pull_tags(**kw):
    s = tag("GmxGroups", GROUPS) + tag("GmxPull", 1)
    for k, v in kw.items():
        s += tag("Gmx" + k, v)
    return s


def mdp_of(interaction):
    xml = rt.model("ecceMdDynamics", interaction=interaction, dynamics=rt.dyn(100, False))
    return run([os.path.join(PARSERS, "md.gmxtask")], input=xml.encode())


def keys(text):
    out = {}
    for line in text.splitlines():
        m = re.match(r"^\s*(\S+)\s*=\s*(.*?)\s*$", line.split(";")[0])
        if m:
            out[m.group(1).lower().replace("_", "-")] = m.group(2)
    return out


def xvg(path):
    return [[float(v) for v in l.split()] for l in open(path)
            if l.strip() and l[0] not in "#@"]


def mdp_checks():
    rc, out = mdp_of(pull_tags(PullGroup1="Chain_A", PullGroup2="Chain_B",
                               PullRate=RATE, PullK=K))
    k = keys(out)
    check(rc == 0, "md.gmxtask accepts a pull")
    want = {"pull": "yes", "pull-ngroups": "2", "pull-ncoords": "1",
            "pull-group1-name": "Chain_A", "pull-group2-name": "Chain_B",
            "pull-coord1-type": "umbrella", "pull-coord1-geometry": "distance",
            "pull-coord1-dim": "Y Y Y", "pull-coord1-groups": "1 2",
            "pull-coord1-start": "yes", "pull-coord1-rate": "0.03",
            "pull-coord1-k": "5000"}
    for key, v in want.items():
        check(k.get(key) == v, "%s = %s (got %s)" % (key, v, k.get(key)))
    check("pull-coord1-vec" not in k, "a distance pull has no vector")

    rc, out = mdp_of(pull_tags(PullGroup1="Chain_A", PullGroup2="Chain_B",
                               PullGeom=1, PullAxis=1, PullDim="N Y N"))
    k = keys(out)
    check(k.get("pull-coord1-geometry") == "direction" and
          k.get("pull-coord1-vec") == "0 1 0" and k.get("pull-coord1-dim") == "N Y N",
          "direction along y: geometry, vector 0 1 0, dim N Y N (got %s, %s, %s)"
          % (k.get("pull-coord1-geometry"), k.get("pull-coord1-vec"), k.get("pull-coord1-dim")))

    rc, out = mdp_of(tag("GmxGroups", GROUPS))
    check("pull" not in keys(out), "no pull unless asked for")
    rc, out = mdp_of(pull_tags(PullGroup1="Chain_A"))
    check(rc != 0, "pulling with one group name is refused")
    rc, out = mdp_of(pull_tags(PullGroup1="Chain_A", PullGroup2="Chain_B", PullDim="Y N"))
    check(rc != 0, "pull dimensions that are not three of Y/N are refused")


def main():
    rt.verbose = "-v" in sys.argv
    gmx = rt.find_gmx()
    if gmx is None:
        print("SKIP: gmx is not installed")
        return 77
    mdp_checks()

    base = os.path.expanduser("~/.cache")
    os.makedirs(base, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix="gmx-pull-", dir=base)
    try:
        home = rt.Home(tmp, gmx)
        pdb = os.path.join(tmp, "two.pdb")
        # 2 nm apart; the box must stay over twice the distance the pull reaches
        # (the distance is a minimum-image one), so DIST leaves room
        atoms = [l for l in open(os.path.join(FIXTURES, "1l2y.pdb")) if l.startswith("ATOM")]
        with open(pdb, "w") as h:
            for chain, dx in (("A", 0.0), ("B", 20.0)):
                for l in atoms:
                    h.write(l[:21] + chain + l[22:30] + "%8.3f" % (float(l[30:38]) + dx) + l[38:])
                h.write("TER\n")
            h.write("END\n")
        prep = rt.prepare(gmx, home, tmp, "two", "SOURCE=protein\nFF=amber99sb-ildn\n"
                          "WATER=tip3p\nBOX=cubic\nDIST=0.9\nMERGE=0\nCONC=0\n", pdb)
        if prep is None:
            return 1
        extras = dict((f, os.path.join(prep, f)) for f in os.listdir(prep)
                      if f.endswith(".itp"))
        top = os.path.join(prep, "topol.top")
        spec = dict(rt.STAGES[0][2])
        em = rt.stage(gmx, home, tmp, "em", "ecceMdOptimize", spec,
                      os.path.join(prep, "ion.gro"), nprocs=8, topology=top, extras=extras)
        if em is None:
            return 1
        inter = (pull_tags(PullGroup1="Chain_A", PullGroup2="Chain_B", PullRate=RATE,
                           PullK=K) + tag("GmxPosres", 1) + tag("GmxPosresName", "POSRES_A")
                 + tag("GmxExtraMdp", "gen-seed = 42"))
        res = rt.stage(gmx, home, tmp, "pull", "ecceMdDynamics",
                       dict(interaction=inter, dynamics=rt.dyn(STEPS, False)),
                       em["gro"], nprocs=8, topology=top, extras=extras)
        if res is None:
            return 1
        d = res["rundir"]
        px, pf = xvg(os.path.join(d, "pullx.xvg")), xvg(os.path.join(d, "pullf.xvg"))
        check(len(px) > 10 and len(px) == len(pf), "pullx.xvg and pullf.xvg have %d frames" % len(px))
        t = [r[0] for r in px]
        x = [r[1] for r in px]
        f = [r[1] for r in pf]
        x_all = x
        # the second half: the first is the spring and the solvent settling
        t, x = t[len(t) // 2:], x[len(x) // 2:]
        mt, mx = sum(t) / len(t), sum(x) / len(x)
        slope = (sum((a - mt) * (b - mx) for a, b in zip(t, x))
                 / sum((a - mt) ** 2 for a in t))
        print("  distance %.4f -> %.4f nm over the last %.2f ps: slope %.4f nm/ps; force %.1f .. %.1f"
              % (x[0], x[-1], t[-1] - t[0], slope, min(f), max(f)))
        check(abs(slope - RATE) < 0.35 * RATE,
              "the COM distance rises at about the rate (%.4f vs %.3f nm/ps)" % (slope, RATE))
        check(max(abs(v) for v in f) > 1.0, "the pull force is not zero")

        # the joined file and the parser
        joined = os.path.join(d, "ecce-parse-pull.txt")
        check(os.path.exists(joined), "gensub joined the pull files")
        rc, out = run([os.path.join(PARSERS, "gromacs.pull")],
                      input=open(joined, "rb").read())
        vals = {}
        for blk in out.split("END\n"):
            m = re.search(r"key: (\w+)\nsize:\n(\d+)\nvalues:\n(.*?)\nunits:\n(\S+)", blk, re.S)
            if m:
                vals[m.group(1)] = (int(m.group(2)), [float(v) for v in m.group(3).split()],
                                    m.group(4))
        check(sorted(vals) == ["PULLDIST", "PULLFORCE", "PULLTIME"], "gromacs.pull prints three keys")
        check(all(v[0] == len(px) == len(v[1]) for v in vals.values()),
              "each has one value per frame (%d)" % len(px))
        check(vals["PULLDIST"][1] == x_all and vals["PULLFORCE"][1] == f,
              "PULLDIST and PULLFORCE equal the columns of pullx.xvg and pullf.xvg")
        check(vals["PULLFORCE"][2] == "kJ/mol/nm" and vals["PULLTIME"][2] == "ps",
              "units are kJ/mol/nm and ps")
    finally:
        if os.environ.get("KEEP"):
            print("kept", tmp)
        else:
            shutil.rmtree(tmp, ignore_errors=True)
    print("FAILED" if rt.failures else "OK")
    return 1 if rt.failures else 0


if __name__ == "__main__":
    sys.exit(main())
