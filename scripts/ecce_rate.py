#!/usr/bin/env python3
"""ecce-rate (#872): conventional transition-state-theory rate constants with
tunnelling from stored reactant, transition-state and product calculations.

Wx-free; reads stored calculation folders and talks to no ECCE session.  G, H and S
come from ecce-thermo (so every treatment option applies); the formulas are in
ecce_rates.py.  Run `ecce-rate --help`.
"""
import argparse
import importlib.machinery
import importlib.util
import json
import math
import os
import shutil
import subprocess
import sys

_here = os.path.dirname(os.path.abspath(__file__))


def _load(module, installed):
    """The sibling script: importable in the source tree, bin/<installed> after install."""
    sys.path.insert(0, _here)
    try:
        return __import__(module)
    except ImportError:
        l = importlib.machinery.SourceFileLoader(module, os.path.join(_here, installed))
        s = importlib.util.spec_from_loader(module, l)
        m = importlib.util.module_from_spec(s)
        l.exec_module(m)
        return m
    finally:
        sys.path.pop(0)


S = _load("ecce_si_export", "ecce-si-export")
# the formula library is a module, installed in <home>/scripts beside the parsers
sys.path.insert(0, os.path.join(os.path.dirname(_here), "scripts"))
R = _load("ecce_rates", "ecce_rates.py")

KJ_PER_HARTREE = 4.3597447222071e-18 * 6.02214076e23 / 1000.0
BARRIER_MAX = 1000.0   # kJ/mol; above this the energies cannot belong together
T_ZERO = 0.01     # ecce-thermo at this T gives H = E_el + ZPE (thermal terms ~1e-5 kJ/mol)
METHODS = ("none", "wigner", "eckart")
LABEL = {"none": "TST", "wigner": "Wigner", "eckart": "Eckart"}


def _fail(msg, code=1):
    sys.stderr.write("error: %s\n" % msg)
    sys.exit(code)


def _thermo_exe():
    for cand in (os.environ.get("ECCE_THERMO"),
                 os.path.join(os.environ.get("ECCE_HOME", ""), "bin", "ecce-thermo"),
                 os.path.join(_here, "ecce-thermo"), shutil.which("ecce-thermo")):
        if cand and os.path.isfile(cand):
            return cand
    return "ecce-thermo"


def thermo(paths, T, ss, treatment):
    """{path: (H, S, G)} (Hartree, J/mol/K, Hartree) from one ecce-thermo run."""
    uniq = sorted(set(paths))
    cmd = [_thermo_exe()] + ["file://" + p for p in uniq] + [
        "--T", repr(T), "--standard-state", ss, "--treatment", treatment, "--json"]
    r = subprocess.run(cmd, capture_output=True, text=True)
    i = r.stdout.find("[\n")
    if r.returncode or i < 0:
        _fail("ecce-thermo failed: " + (r.stdout.strip() or r.stderr.strip() or "no output"))
    out = {}
    for p, rec in zip(uniq, json.loads(r.stdout[i:])):
        t = next(iter(rec["treatments"].values()))
        out[p] = (t["H"], t["S"], t["G"])
    return out


def resolve(arg):
    """One stored calculation from a folder, or the single calculation below it."""
    found = [p for p in S.expand_calcs([arg]) if S.is_calc(p)]
    if len(found) != 1:
        _fail("%s: %s" % (arg, "no calculation found" if not found else
                          "%d calculations found, name one" % len(found)))
    return os.path.abspath(found[0])


def frequencies(calc):
    t = S._value_text(os.path.join(calc, "Props", "VIBFREQ"))
    if t is None:
        _fail("%s has no stored frequencies (Props/VIBFREQ); run a frequency calculation"
              % os.path.basename(calc))
    return [float(v) for v in t.split()]


def parse_T(spec):
    try:
        if ":" in spec:
            lo, hi, st = (float(x) for x in spec.split(":"))
            if st <= 0 or hi < lo:
                raise ValueError
            n = int(round((hi - lo) / st + 1e-9))
            return [lo + i * st for i in range(n + 1)]
        return [float(x) for x in spec.split(",")]
    except ValueError:
        _fail("--T is a temperature (298.15), a list (250,300) or LOW:HIGH:STEP (250:400:25)")


def analyse(a):
    notes = []
    rs = [resolve(x) for x in a.reactants.split(",")]
    ts = resolve(a.ts)
    ps = [resolve(x) for x in a.products.split(",")] if a.products else []
    ns = {}
    for role, paths in (("reactant", rs), ("ts", [ts]), ("product", ps)):
        for p in paths:
            ns[p] = S.mode_counts(frequencies(p))[1]
    nts = ns[ts]
    if nts != 1:
        _fail("%s is not a transition state: it has %d imaginary mode(s), a TS needs exactly one. "
              "Optimise it as a saddle point (QST3/TS search) and compute its frequencies."
              % (os.path.basename(ts), nts))
    for p in rs + ps:
        if ns[p]:
            notes.append("warning: %s has %d imaginary mode(s); it is not a minimum"
                         % (os.path.basename(p), ns[p]))
    nu = min(f for f in frequencies(ts))
    m, mp = len(rs), len(ps)
    if m > 3:
        _fail("at most three reactants")
    if not ps:
        notes.append("no product given: no reverse rate or K_eq, and the Eckart barrier is "
                     "symmetric (reverse barrier = forward barrier)")

    # ZPE-corrected barriers: H at ~0 K is E_el + ZPE; always RRHO so that the
    # treatment of low modes does not alter the potential-energy barrier.
    h0 = thermo(rs + [ts] + ps, T_ZERO, a.standard_state, "rrho")
    e = lambda paths: sum(h0[p][0] for p in paths) * KJ_PER_HARTREE
    V1 = (h0[ts][0] * KJ_PER_HARTREE - e(rs))
    V2 = (h0[ts][0] * KJ_PER_HARTREE - e(ps)) if ps else None
    eck = None
    if V1 <= 0:
        notes.append("Eckart skipped: the ZPE-corrected barrier is %.2f kJ/mol (TS not above the "
                     "reactants)" % V1)
    elif ps and V2 <= 0:
        notes.append("Eckart skipped: the ZPE-corrected reverse barrier is %.2f kJ/mol" % V2)
    elif V1 > BARRIER_MAX or (ps and V2 > BARRIER_MAX):
        notes.append("Eckart skipped: a ZPE-corrected barrier above %d kJ/mol is not a "
                     "reaction barrier (check that the calculations use one method)" % BARRIER_MAX)
    else:
        eck = True
    meths = [x for x in METHODS if a.tunnelling in ("all", x) or x == "none"]
    if a.tunnelling in ("wigner", "eckart"):
        meths = ["none", a.tunnelling]

    rows = []
    for T in parse_T(a.T):
        g = thermo(rs + [ts] + ps, T, a.standard_state, a.treatment)
        sm = lambda paths, i: sum(g[p][i] for p in paths)
        dG = (g[ts][2] - sm(rs, 2)) * KJ_PER_HARTREE
        dH = (g[ts][0] - sm(rs, 0)) * KJ_PER_HARTREE
        dS = g[ts][1] - sm(rs, 1)
        row = {"T": T, "dG_act": dG, "dH_act": dH, "dS_act": dS, "kappa": {}, "k": {},
               "half_life": {}}
        kap = {"none": 1.0, "wigner": R.wigner(T, nu),
               "eckart": R.eckart_kappa(T, nu, V1, V2) if eck else None}
        for x in meths:
            row["kappa"][x] = kap[x]
            row["k"][x] = None if kap[x] is None else R.tst_rate(T, dG, m, a.standard_state, a.sigma, kap[x])
            row["half_life"][x] = None if row["k"][x] is None else R.half_life(row["k"][x], m, T, a.standard_state)
        if ps:
            dGr = (sm(ps, 2) - sm(rs, 2)) * KJ_PER_HARTREE
            row["dG_rxn"] = dGr
            row["dG_act_reverse"] = dG - dGr
            row["K_eq"] = math.exp(-dGr / (R.R_KJ * T))
            row["k_reverse"] = {x: None if kap[x] is None else
                                R.tst_rate(T, dG - dGr, mp, a.standard_state, a.sigma, kap[x])
                                for x in meths}
        rows.append(row)

    fits = {}
    if len(rows) >= 2:
        for x in meths:
            ks = [r["k"][x] for r in rows]
            if any(k is None or k <= 0 for k in ks):
                continue
            Ts = [r["T"] for r in rows]
            A, Ea = R.arrhenius_fit(Ts, ks)
            dH, dS = R.eyring_fit(Ts, ks, m, a.standard_state, a.sigma)
            fits[x] = {"A": A, "Ea_kJ_per_mol": Ea, "dH_act_kJ_per_mol": dH,
                       "dS_act_J_per_mol_K": dS}
    u = (nu * R.KJ_PER_CM) / (R.R_KJ * min(r["T"] for r in rows))
    if u > 4:
        notes.append("h*nu/kT = %.1f at the lowest T: Wigner is unreliable above about 4" % u)
    return {
        "reactants": [os.path.basename(p) for p in rs], "ts": os.path.basename(ts),
        "products": [os.path.basename(p) for p in ps], "standard_state": a.standard_state,
        "treatment": a.treatment, "molecularity": m, "sigma": a.sigma,
        "imaginary_frequency_cm-1": nu,
        "zpe_barrier_forward_kJ_per_mol": V1, "zpe_barrier_reverse_kJ_per_mol": V2,
        "unit": R.rate_unit(m, a.standard_state),
        "unit_reverse": R.rate_unit(mp, a.standard_state) if ps else None,
        "temperatures": rows, "fits": fits, "notes": notes,
    }, meths


def fmt(v, f="%.3e"):
    return "-" if v is None else f % v


def show(res, meths):
    print("%s  ->  TS %s%s" % (" + ".join(res["reactants"]), res["ts"],
                               "  ->  " + " + ".join(res["products"]) if res["products"] else ""))
    print("imaginary frequency %.1f cm-1; ZPE-corrected barrier %.2f kJ/mol forward%s"
          % (-res["imaginary_frequency_cm-1"] if res["imaginary_frequency_cm-1"] > 0 else
             res["imaginary_frequency_cm-1"], res["zpe_barrier_forward_kJ_per_mol"],
             "" if res["zpe_barrier_reverse_kJ_per_mol"] is None else
             ", %.2f kJ/mol reverse" % res["zpe_barrier_reverse_kJ_per_mol"]))
    print("treatment %s, standard state %s, sigma %d, molecularity %d; k in %s"
          % (res["treatment"], res["standard_state"], res["sigma"], res["molecularity"], res["unit"]))
    for n in res["notes"]:
        print("note: " + n)
    rows = res["temperatures"]
    print("\nForward")
    print("  %8s %10s %10s %10s" % ("T/K", "dG‡ kJ/mol", "dH‡ kJ/mol", "dS‡ J/mol/K")
          + "".join(" %10s %11s" % ("kappa " + LABEL[x] if x != "none" else "", "k " + LABEL[x])
                    for x in meths) + "  half-life/s (%s)" % LABEL[meths[-1]])
    for r in rows:
        print("  %8.2f %10.2f %10.2f %10.2f" % (r["T"], r["dG_act"], r["dH_act"], r["dS_act"])
              + "".join(" %10s %11s" % ("" if x == "none" else fmt(r["kappa"][x], "%.3f"),
                                        fmt(r["k"][x])) for x in meths)
              + "  %s" % fmt(r["half_life"][meths[-1]]))
    if res["products"]:
        print("\nReverse (k in %s) and equilibrium" % res["unit_reverse"])
        print("  %8s %10s %10s %11s" % ("T/K", "dG_rxn", "dG‡ rev", "K_eq")
              + "".join(" %11s" % ("k " + LABEL[x]) for x in meths))
        for r in rows:
            print("  %8.2f %10.2f %10.2f %11s" % (r["T"], r["dG_rxn"], r["dG_act_reverse"],
                                                  fmt(r["K_eq"]))
                  + "".join(" %11s" % fmt(r["k_reverse"][x]) for x in meths))
    if res["fits"]:
        print("\nFits over %.0f-%.0f K (tunnelling included in k)" % (rows[0]["T"], rows[-1]["T"]))
        for x in meths:
            f = res["fits"].get(x)
            if f:
                print("  %-7s Arrhenius A = %.3e %s, Ea = %.2f kJ/mol; Eyring dH‡ = %.2f kJ/mol, "
                      "dS‡ = %.2f J/mol/K" % (LABEL[x], f["A"], res["unit"], f["Ea_kJ_per_mol"],
                                              f["dH_act_kJ_per_mol"], f["dS_act_J_per_mol_K"]))


HELP = """\
Reactants, TS and products are stored calculation folders (or a folder holding
exactly one calculation); two reactants or products are joined by a comma, no
space.  The TS must have exactly one imaginary mode (otherwise ecce-rate
refuses); reactants and products without imaginary modes (otherwise a warning).

Formulas
  k = kappa * sigma * (kB T/h) * (c0)^(1-m) * exp(-dG‡/RT)
    dG‡ = G(TS) - sum G(reactants) from ecce-thermo at T, the --treatment and
    the --standard-state (so the rotational symmetry numbers of every species
    are already in G; --sigma is the further reaction-path degeneracy).
    m = number of reactants.  c0 = 1 mol/L for 1M (k in M^(1-m) s^-1) or
    p0/kT in molecule cm-3 for 1atm (k in cm^(3(m-1)) molecule^-(m-1) s^-1).
  Wigner: kappa = 1 + (h nu‡/kB T)^2 / 24, nu‡ the imaginary frequency.
  Eckart: the unsymmetric Eckart barrier V(x) = A y/(1+y) + B y/(1+y)^2,
    y = exp(2 pi x/L), with forward and reverse barriers V1, V2 (ZPE-corrected
    TS minus reactants or products; electronic energy + ZPE from ecce-thermo at
    ~0 K, RRHO) and its length fixed by nu‡ (Johnston and Heicklen 1962).
    kappa = exp(V1/kT) * integral T(E) exp(-E/kT) dE/kT from max(0, V1-V2) up,
    T(E) the exact transmission probability of that barrier.  Without a
    product the barrier is symmetric (V2 = V1) and a note says so.  The same
    kappa is used for the reverse reaction.
  Half-life: ln2/k for one reactant; for two, pseudo-first order with the second
    held at the standard concentration (1 M, or 1 atm); not given for three.
  Reverse: k_rev uses dG‡_rev = dG‡ - dG_rxn and the product molecularity;
    K_eq = exp(-dG_rxn/RT) (dimensionless, standard state as chosen), so
    k/k_rev = K_eq * c0^(m_prod - m).
  Fits over a --T range (k with tunnelling): Arrhenius ln k = ln A - Ea/RT and
    Eyring ln(k c0^(m-1)/(sigma T)) = ln(kB/h) + dS‡/R - dH‡/RT.  A fit absorbs
    the T-dependence of dH‡, dS‡ and kappa.
Exit 0 done, 1 refused or failed.

  ecce-rate proj/ClCH3Br proj/TS proj/product --T 250:400:25
  ecce-rate proj/A,proj/B proj/TS --standard-state 1atm --tunnelling eckart --json
"""


def main(argv=None):
    ap = argparse.ArgumentParser(
        prog="ecce-rate", formatter_class=argparse.RawDescriptionHelpFormatter,
        usage="ecce-rate REACTANT[,REACTANT2] TS [PRODUCT[,PRODUCT2]] [--T 298.15 | --T LOW:HIGH:STEP]\n"
              "                [--standard-state 1M|1atm] [--treatment SPEC] [--sigma N]\n"
              "                [--tunnelling none|wigner|eckart|all] [--json]",
        description="Conventional TST rate constants with Wigner and Eckart tunnelling.",
        epilog=HELP)
    ap.add_argument("reactants")
    ap.add_argument("ts")
    ap.add_argument("products", nargs="?")
    ap.add_argument("--T", default="298.15", help="K: 298.15, 250,300 or LOW:HIGH:STEP")
    ap.add_argument("--standard-state", choices=("1M", "1atm"), default="1M")
    ap.add_argument("--treatment", default="qrrho-grimme:100",
                    help="as ecce-thermo, one only (default qrrho-grimme:100)")
    ap.add_argument("--sigma", type=int, default=1, help="reaction-path degeneracy (default 1)")
    ap.add_argument("--tunnelling", choices=("none", "wigner", "eckart", "all"), default="all")
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args(argv)
    if "," in a.treatment or a.treatment == "all":
        _fail("--treatment takes one treatment")
    if a.sigma < 1:
        _fail("--sigma is a positive whole number")
    res, meths = analyse(a)
    if a.json:
        print(json.dumps(res, indent=2))
    else:
        show(res, meths)
    return 0


if __name__ == "__main__":
    sys.exit(main())
