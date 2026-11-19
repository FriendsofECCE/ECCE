#!/usr/bin/env python3
"""GROMACS Prepare: per-chain position restraints and off-centre box placement.

Real gmx is the oracle.  A two-chain PDB (the 1L2Y fixture twice, chain B
shifted 3 nm) goes through the real md.gmxprep, gromacs.launchpp and the job
script gensub writes, with MERGE=0:

  * every chain has its own topol_*.itp and posre_*.itp, and restrains under
    POSRES (all chains) and under POSRES_<chain id> (that chain alone):
    grompp -pp counts the [ position_restraints ] blocks and their atoms;
  * the index file gromacs.mkndx writes has one group per chain (mol:...);
  * CENTER="x y z" ends in the solute's geometric centre at x y z in the
    prepared structure, and no CENTER leaves it at the middle of the box;
  * md.gmxprep refuses a CENTER that is not empty or three numbers;
  * with --solvate-cli (the build tree's solvate_cli): a structure already
    solvated as Build > Solvate does it (#1118) keeps its waters and cell,
    and the job says gmx solvate was skipped.

    tests/gromacs_md/prepare_options_test.py [-v]

Exit status 77 (CTest SKIP) without gmx.
"""
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as rt  # noqa: E402
from run_tests import check, FIXTURES, PARSERS, run  # noqa: E402


def two_chain_pdb(path):
    atoms = [l for l in open(os.path.join(FIXTURES, "1l2y.pdb")) if l.startswith("ATOM")]
    out = []
    for chain, dx in (("A", 0.0), ("B", 30.0)):
        for l in atoms:
            out.append(l[:21] + chain + l[22:30] + "%8.3f" % (float(l[30:38]) + dx) + l[38:])
        out.append("TER\n")
    out.append("END\n")
    with open(path, "w") as h:
        h.writelines(out)


def gro_atoms(path):
    lines = open(path).read().splitlines()
    n = int(lines[1])
    return lines[2:2 + n], [float(x) for x in lines[2 + n].split()]


def cog(lines):
    xs = [[float(l[20:28]), float(l[28:36]), float(l[36:44])] for l in lines]
    return [sum(c) / len(xs) for c in zip(*xs)]


def posre_blocks(gmx, rundir, define):
    """Position-restraint blocks and restrained atoms grompp sees for -D<define>."""
    mdp = os.path.join(rundir, "pr.mdp")
    open(mdp, "w").write("integrator = steep\nnsteps = 0\ncutoff-scheme = Verlet\n"
                         "define = %s\n" % define)
    rc, out = run([gmx, "grompp", "-f", "pr.mdp", "-c", "ion.gro", "-r", "ion.gro", "-p", "topol.top",
                   "-pp", "pr.top", "-o", "pr.tpr", "-maxwarn", "9"], cwd=rundir)
    if rc != 0:
        print(out)
        return None
    blocks, atoms, on = 0, 0, False
    for l in open(os.path.join(rundir, "pr.top")):
        s = l.strip()
        if s.startswith("["):
            on = s.replace(" ", "") == "[position_restraints]"
            blocks += on
        elif on and s and not s.startswith((";", "#")):
            atoms += 1
    return blocks, atoms


def presolvated_pdb(cli, path, tmp):
    """1L2Y solvated by solvate_cli, written as Fragment::dumpPDB writes a
    structure solvated in the Builder: CRYST1, HOH residues OW HW1 HW2.
    Returns the number of waters and the cell (Angstrom)."""
    atoms = [l for l in open(os.path.join(FIXTURES, "1l2y.pdb")) if l.startswith("ATOM")]
    xyz = os.path.join(tmp, "solute.xyz")
    with open(xyz, "w") as h:
        h.write("%d\n\n" % len(atoms))
        for l in atoms:
            el = l[76:78].strip() or l[12:16].strip()[0]
            h.write("%s %s %s %s\n" % (el, l[30:38], l[38:46], l[46:54]))
    rc, out = run([cli, os.path.join(rt.REPO, "data", "client", "solvents", "water216.xyz"),
                   xyz, "8"],
                  env=dict(os.environ, ECCE_HOME=rt.REPO, ECCE_REALUSER="solvate",
                           ECCE_REALUSERHOME=tmp))
    if not check(rc == 0, "solvate_cli solvates 1L2Y"):
        print(out)
        return 0, None
    lines = out.splitlines()
    cell = [float(x) for x in lines[1].split()[1:4]]
    body = lines[2:]
    pdb = ["CRYST1%9.3f%9.3f%9.3f%7.2f%7.2f%7.2f P 1           1\n" % tuple(cell + [90, 90, 90])]
    for l, b in zip(atoms, body):
        x, y, z = (float(v) for v in b.split()[1:4])
        pdb.append(l[:30] + "%8.3f%8.3f%8.3f" % (x, y, z) + l[54:])
    pdb.append("TER\n")
    waters = body[len(atoms):]
    names = (" OW ", " HW1", " HW2")
    for i, b in enumerate(waters):
        x, y, z = (float(v) for v in b.split()[1:4])
        el = b.split()[0]
        pdb.append("HETATM%5d %4s HOH  %4d    %8.3f%8.3f%8.3f  1.00  0.00          %2s\n"
                   % ((len(atoms) + i + 1) % 100000, names[i % 3], (i // 3 + 2) % 10000,
                      x, y, z, el))
    pdb.append("END\n")
    with open(path, "w") as h:
        h.writelines(pdb)
    return len(waters) // 3, cell


def presolvated_test(gmx, home, tmp, cli):
    pdb = os.path.join(tmp, "presolvated.pdb")
    nwat, cell = presolvated_pdb(cli, pdb, tmp)
    if not cell:
        return
    rcp = "SOURCE=protein\nFF=amber99sb-ildn\nWATER=tip3p\nBOX=dodecahedron\nDIST=1.0\nCONC=0\n"
    d = rt.prepare(gmx, home, tmp, "presolvated", rcp, pdb)
    if d is None:
        return
    out = open(os.path.join(d, "gromacs.gmxlog"), errors="replace").read()
    check("gmx solvate skipped" in out, "the output says gmx solvate was skipped")
    check(" solvate -cp " not in out and "Adding" not in out.split("step 3/4")[-1].split("step 4/4")[0],
          "gmx solvate did not run")
    lines, box = gro_atoms(os.path.join(d, "ion.gro"))
    mols = rt.molecules(os.path.join(d, "topol.top"))
    print("  presolvated: %d waters placed, topology %s, box %s, cell %s"
          % (nwat, mols, box[:3], cell))
    # Trp-cage is +1: genion replaces one water by a chloride
    check(mols.get("SOL") == nwat - 1, "the topology keeps the placed waters less one for the ion")
    check(all(abs(box[i] - cell[i] / 10.0) < 0.002 for i in range(3)),
          "the box is the cell of the solvated structure")


def main():
    rt.verbose = "-v" in sys.argv
    gmx = rt.find_gmx()
    if gmx is None:
        print("SKIP: gmx is not installed")
        return 77
    base = os.path.expanduser("~/.cache")
    os.makedirs(base, exist_ok=True)
    tmp = tempfile.mkdtemp(prefix="gmx-prep-opts-", dir=base)
    try:
        home = rt.Home(tmp, gmx)
        pdb = os.path.join(tmp, "two.pdb")
        two_chain_pdb(pdb)
        rcp = ("SOURCE=protein\nFF=amber99sb-ildn\nWATER=tip3p\nBOX=cubic\nDIST=1.0\n"
               "MERGE=0\nCONC=0\n")

        # the recipe writer
        for bad in ("CENTER=1 2\n", "CENTER=a b c\n", "CENTER=1 2 3 4\n", "CENTER=1;2;3\n"):
            rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=bad.encode())
            check(rc != 0, "md.gmxprep refuses %r" % bad.strip())
        rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=b"CENTER=1 2.5 -3\n")
        check(rc == 0 and "CENTER=1 2.5 -3" in out, "md.gmxprep keeps CENTER=1 2.5 -3")
        rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=b"")
        check(rc == 0 and "CENTER=\n" in out, "no CENTER by default")
        xml = ("<GromacsPrepareModel>\n<BoxCenter>1 2 3</BoxCenter>\n</GromacsPrepareModel>\n")
        rc, out = run([os.path.join(PARSERS, "md.gmxprep")], input=xml.encode())
        check(rc == 0 and "CENTER=1 2 3" in out, "md.gmxprep reads BoxCenter from the model XML")

        # centred, then off-centre
        results = {}
        for name, extra in (("centred", ""), ("off", "CENTER=2 2 5\n")):
            d = rt.prepare(gmx, home, tmp, name, rcp + extra, pdb)
            if d is None:
                return 1
            results[name] = d
            lines, box = gro_atoms(os.path.join(d, "ion.gro"))
            solute = [l for l in lines if l[5:10].strip() not in ("SOL", "NA", "CL")]
            c = cog(solute)
            print("  %s: %d solute atoms, centre %s, box %s"
                  % (name, len(solute), ["%.3f" % x for x in c], box[:3]))
            if name == "centred":
                check(all(abs(c[i] - box[i] / 2) < 0.01 for i in range(3)),
                      "no CENTER: the solute's centre is the box middle")
            else:
                check(all(abs(c[i] - v) < 0.01 for i, v in enumerate((2, 2, 5))),
                      "CENTER=2 2 5: the solute's centre is at 2 2 5 nm")

        if "--solvate-cli" in sys.argv:
            presolvated_test(gmx, home, tmp, sys.argv[sys.argv.index("--solvate-cli") + 1])

        d = results["centred"]
        itps = sorted(f for f in os.listdir(d) if f.endswith(".itp"))
        check(itps == ["posre_Protein_chain_A.itp", "posre_Protein_chain_B.itp",
                       "topol_Protein_chain_A.itp", "topol_Protein_chain_B.itp"],
              "one topology and one restraint file per chain: %s" % itps)

        # restraints: none, all chains, one chain
        n = {}
        for define in ("-DNOTHING", "-DPOSRES", "-DPOSRES_A", "-DPOSRES_B",
                       "-DPOSRES_A -DPOSRES_B"):
            n[define] = posre_blocks(gmx, d, define)
        print("  [position_restraints] (blocks, atoms):", n)
        check(n["-DNOTHING"] == (0, 0), "no define: no restraints")
        all_blocks, all_atoms = n["-DPOSRES"]
        check(all_blocks == 2 and all_atoms > 0, "POSRES restrains both chains")
        a, b = n["-DPOSRES_A"], n["-DPOSRES_B"]
        check(a[0] == 1 and b[0] == 1 and a[1] + b[1] == all_atoms and a[1] > 0,
              "POSRES_A and POSRES_B restrain one chain each, together the same atoms as POSRES")
        check(n["-DPOSRES_A -DPOSRES_B"] == (2, all_atoms),
              "POSRES_A POSRES_B together restrain both chains")

        # the index groups, per chain
        mdp = os.path.join(d, "g.mdp")
        open(mdp, "w").write("; ecce-group ChainA = mol:Protein_chain_A\n"
                             "; ecce-group ChainB = mol:Protein_chain_B\n")
        rc, out = run([os.path.join(PARSERS, "gromacs.mkndx"), "ion.gro", mdp,
                       os.path.join(d, "g.ndx"), "topol.top"], cwd=d)
        check(rc == 0, "gromacs.mkndx writes a group per chain: %s" % out.strip())
        groups, cur = {}, None
        for l in open(os.path.join(d, "g.ndx")):
            if l.startswith("["):
                cur = l.strip("[] \n")
                groups[cur] = []
            else:
                groups[cur] += l.split()
        check(len(groups.get("ChainA", [])) == len(groups.get("ChainB", [])) > 0
              and not set(groups["ChainA"]) & set(groups["ChainB"]),
              "the chain groups are disjoint and equal in size (%d atoms each)"
              % len(groups.get("ChainA", [])))
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("FAILED" if rt.failures else "OK")
    return 1 if rt.failures else 0


if __name__ == "__main__":
    sys.exit(main())
