#!/usr/bin/env python3
"""scripts/ecce_qsar_chem.py and `ecce-qsar table` (#961, descriptor table).

Oracles: analytic graph invariants, RDKit's published values for common
molecules, Open Babel's logP/TPSA/Gasteiger tables where its conventions are the
same (the part that needs obabel skips without it), and the stored water
calculation of tests/ecceqm for the quantum-chemical columns.
"""
import csv
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import ecce_qsar_chem as C  # noqa: E402

fails = []


def check(cond, msg):
    if not cond:
        fails.append(msg)
        print("FAIL:", msg)


def near(a, b, tol, msg):
    check(abs(a - b) <= tol, "%s: %s vs %s" % (msg, a, b))


D = C.all_descriptors

# analytic: Wiener numbers, Kier kappas of alkanes, Randic chi, Zagreb
near(D("CCCC")["weinerPath"], 10, 0, "Wiener n-butane")
near(D("CC(C)C")["weinerPath"], 9, 0, "Wiener isobutane")
near(D("CCCCC")["weinerPath"], 20, 0, "Wiener n-pentane")
near(D("c1ccccc1")["weinerPath"], 27, 0, "Wiener benzene")
h = D("CCCCCC")
near(h["Kier1"], 6.0, 1e-9, "kappa1 hexane")
near(h["Kier2"], 5.0, 1e-9, "kappa2 hexane")
near(h["Kier3"], 16 * 3 / 9.0, 1e-9, "kappa3 hexane")
near(h["chi1"], 2 / math.sqrt(2) + 3 / 2.0, 1e-9, "Randic chi hexane")
near(h["zagreb1"], 2 * 1 + 4 * 4, 0, "Zagreb M1 hexane")
near(h["zagreb2"], 2 * 2 + 3 * 4, 0, "Zagreb M2 hexane")
# Balaban J from the definition with numpy distances
try:
    import numpy as np
    for smi in ("CCCC", "c1ccccc1", "CC(C)CC(=O)O", "c1ccc2ccccc2c1"):
        m = C.parse(smi)
        n = len(m.atoms)
        A = np.full((n, n), 1e9)
        np.fill_diagonal(A, 0)
        for i, j, _, _ in m.bonds:
            A[i, j] = A[j, i] = 1
        for k in range(n):
            A = np.minimum(A, A[:, [k]] + A[[k], :])
        s = A.sum(1)
        q = len(m.bonds)
        J = q / (q - n + 2) * sum(1 / math.sqrt(s[i] * s[j]) for i, j, _, _ in m.bonds)
        near(D(smi)["balabanJ"], J, 1e-9, "Balaban J " + smi)
except ImportError:
    print("note: numpy missing, Balaban check skipped")

# RDKit values (MolLogP, MolMR, TPSA) as published for these molecules
RD = [("c1ccccc1", 1.6866, 26.442, 0.0),
      ("CCCCCC", 2.5870, 29.816, 0.0),
      ("CCO", -0.0014, 12.7598, 20.23),
      ("CC(=O)Nc1ccc(O)cc1", 1.3506, 42.4113, 49.33),
      ("CC(=O)Oc1ccccc1C(=O)O", 1.3101, 44.7103, 63.60),
      ("Cn1cnc2c1c(=O)n(C)c(=O)n2C", -1.0293, 51.1996, 61.82)]
for smi, lp, mr, tp in RD:
    d = D(smi)
    near(d["SlogP"], lp, 0.0015, "SlogP " + smi)
    near(d["SMR"], mr, 0.005, "SMR " + smi)
    near(d["TPSA"], tp, 0.005, "TPSA " + smi)

# counts and parsing
d = D("CC(=O)Nc1ccc(O)cc1")
check(d["a_heavy"] == 11 and d["a_nH"] == 9 and d["a_nO"] == 2, "paracetamol atoms")
check(d["b_rotN"] == 0 and d["aro_rings"] == 1 and d["chiral"] == 0, "paracetamol rotors")
check(D("C[C@H](N)C(=O)O")["chiral"] == 1, "alanine chiral centre")
check(D("CC(C)C")["chiral"] == 0 and D("OC1CCC(O)CC1")["chiral"] == 0, "no chiral centre")
check(D("C1=CC=CC=C1")["aro_rings"] == 1, "Kekule benzene is aromatic")
check(abs(D("C1=CC=CC=C1")["SlogP"] - D("c1ccccc1")["SlogP"]) < 1e-9, "Kekule = aromatic logP")
check(D("c1cc[nH]c(=O)c1")["aro_rings"] == 1, "pyridone aromatic")
check(D("CC[N+H](C)C")["FCharge"] == 1 and D("CC[NH+](C)C")["a_nH"] == 12, "[N+H] both orders")
check(D("C[NH3+].[Cl-]")["FCharge"] == 0, "salt with a dot")
for bad in ("", "C(C", "C1CC", "Xx", "c1ccc[", "CC)"):
    try:
        C.parse(bad)
        check(False, "no error for %r" % bad)
    except C.SmilesError:
        pass
check(D("CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC")["lip_violation"] >= 1, "lipinski violation")

# Open Babel, where its conventions agree (no N-H amines, no S/P in TPSA)
OB_SET = ["c1ccccc1", "CC(=O)Oc1ccccc1C(=O)O", "Cn1cnc2c1c(=O)n(C)c(=O)n2C", "CCO",
          "CC(C)Cc1ccc(cc1)C(C)C(=O)O", "OC(=O)c1ccccc1O", "Brc1ccccc1",
          "FC(F)(F)c1ccccc1", "CC(C)(C)c1ccccc1", "OCC(O)CO", "c1ccoc1", "CC#N",
          "C=CC=O", "O=C(O)CCCCC1CCCC1", "COc1ccccc1OC"]
if shutil.which("obabel"):
    def ob(smi, *props):
        r = subprocess.run(["obabel", "-:" + smi, "-otxt", "--append", " ".join(props)],
                           capture_output=True, text=True).stdout.split()
        return [float(x) for x in r]

    for smi in OB_SET:
        d = D(smi)
        lp, tp, mw = ob(smi, "logP", "TPSA", "MW")
        near(d["SlogP"], lp, 0.001, "OB logP " + smi)
        near(d["TPSA"], tp, 0.01, "OB TPSA " + smi)
        near(d["weight"], mw, 0.01, "OB MW " + smi)

    def ob_charges(smi):
        out = subprocess.run(["obabel", "-:" + smi, "-omol2", "-h", "--partialcharge",
                              "gasteiger"], capture_output=True, text=True).stdout
        q, on = [], False
        for ln in out.splitlines():
            if ln.startswith("@<TRIPOS>"):
                on = ln.startswith("@<TRIPOS>ATOM")
                continue
            f = ln.split()
            if on and f and f[5].split(".")[0] != "H":
                q.append(float(f[8]))
        return q

    # PEOE, 6 iterations as Open Babel; amide/aniline N and sulfones differ by
    # hybridisation convention (RDKit's is used), so they are not compared
    for smi in ("CCO", "c1ccccc1", "CC#N", "C=CC=O", "Brc1ccccc1", "c1ccoc1",
                "CC(=O)Oc1ccccc1C(=O)O",
                "COc1cc2c(cc1OC)C(=O)C(C2)CC1CCN(Cc2ccccc2)CC1"):
        mine, _ = C.gasteiger(C.parse(smi), 6)
        ref = ob_charges(smi)
        check(len(ref) == len(mine), "OB charge count " + smi)
        if len(ref) == len(mine):
            dev = max(abs(a - b) for a, b in zip(mine, ref))
            check(dev < 1.5e-3, "OB Gasteiger %s: max deviation %.4f" % (smi, dev))
else:
    print("note: obabel not installed, Open Babel comparison skipped")

# the table: a stored calculation, with its SMILES and measured value
tmp = tempfile.mkdtemp(prefix="qsar-desc-")
try:
    src = os.path.join(ROOT, "tests", "ecceqm", "fixtures", "h2o-geovib")
    folder = os.path.join(tmp, "set")
    os.makedirs(folder)
    calc = os.path.join(folder, "water")
    shutil.copytree(src, calc)
    ns = "http://www.emsl.pnl.gov/ecce:"
    with open(os.path.join(calc, ".ecce-meta"), "a") as f:
        f.write(".\t%sqsar.smiles\t\tO\n" % ns)
        f.write(".\t%sqsar.measured\t\t%s\n" % (ns, json.dumps({"IC50 (nM)": 12.5})))
    cli = os.path.join(ROOT, "scripts", "ecce_qsar.py")
    out = os.path.join(tmp, "t.csv")
    r = subprocess.run([sys.executable, cli, "table", folder, "--csv", out],
                       capture_output=True, text=True)
    check(r.returncode == 0, "table exit %s: %s" % (r.returncode, r.stderr))
    rows = list(csv.DictReader(open(out)))
    check(len(rows) == 1 and rows[0]["name"] == "water", "one row for the calculation")
    if rows:
        w = rows[0]
        near(float(w["IC50 (nM)"]), 12.5, 0, "measured value carried")
        near(float(w["weight"]), 18.015, 0.001, "water weight")
        check(w["a_nO"] == "1" and w["a_nH"] == "2", "water atoms")
        near(float(w["qm_HOMO"]), -0.286659366793 * C.EV, 1e-6, "HOMO from ORBENG/ORBOCC")
        near(float(w["qm_LUMO"]), 0.068269349111 * C.EV, 1e-6, "LUMO")
        near(float(w["qm_gap"]), (0.068269349111 + 0.286659366793) * C.EV, 1e-6, "gap")
        near(float(w["qm_dipole"]), 2.086240510421, 1e-9, "dipole")
        near(float(w["qm_qmin"]), -0.793631468726, 1e-7, "Mulliken min")
    # a table of names and SMILES with one unreadable row
    src_csv = os.path.join(tmp, "in.csv")
    with open(src_csv, "w") as f:
        f.write("name,smiles,pIC50\nA,CCO,5.1\nB,C(C,6.0\nC,c1ccccc1,\n")
    out2 = os.path.join(tmp, "t2.csv")
    r = subprocess.run([sys.executable, cli, "table", "--from-csv", src_csv,
                        "--groups", "counts,models", "--csv", out2],
                       capture_output=True, text=True)
    check(r.returncode == 0, "from-csv exit: " + r.stderr)
    rows = list(csv.DictReader(open(out2)))
    check([x["name"] for x in rows] == ["A", "B", "C"], "three rows kept")
    check(rows[1]["problems"].startswith("descriptors:") and rows[1]["SlogP"] == "",
          "bad SMILES listed, not dropped: %r" % rows[1]["problems"])
    check(rows[1]["pIC50"] == "6.0" and "SlogP" in rows[0] and "weinerPath" not in rows[0],
          "groups and measured columns")
finally:
    shutil.rmtree(tmp, ignore_errors=True)

if fails:
    print("%d FAILED" % len(fails))
    sys.exit(1)
print("qsar descriptors: ok")
