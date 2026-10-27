#!/usr/bin/env python3
"""[PROGRESS] entries of each code's .desc (#284), replayed through the real
eccejobmonitor over genuine output of that code.

    tests/progress/codes_test.py                 all cases
    tests/progress/codes_test.py --case orca     cases whose name contains "orca"
    tests/progress/codes_test.py --show <desc> <output-file> [parseType] [--file2 stdout]
                                                 print the last snapshot of any run

Post mode reads the file at once, so the monitor sends the first line's
snapshot and the last one; the checks are on the last, which holds every
stage's final fields and history.  Fixtures are real outputs (scratch paths
sanitised) in tests/progress/fixtures/<code>/ or tests/parsers/fixtures/.
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
MONITOR = os.path.join(ROOT, "scripts", "eccejobmonitor")
PARSERS = os.path.join(ROOT, "scripts", "parsers")
OWN = os.path.join(HERE, "fixtures")
SHARED = os.path.join(ROOT, "tests", "parsers", "fixtures")

failures = []


def check(ok, what):
    print("%s %s" % ("ok   " if ok else "FAIL ", what))
    if not ok:
        failures.append(what)


def replay(desc, out, parse_types="ALL", out2=None):
    """-> (number of jmPROG messages, {stage: text of its line}, history lines)."""
    work = tempfile.mkdtemp(prefix="ecce-progress-")
    try:
        results = os.path.join(work, "results.out")
        cmd = ["perl", MONITOR, "-commType", "stdio", "-monitoringMode", "post",
               "-jobOutputFile", out, "-parseDescriptorFile", desc,
               "-parseTypes", parse_types, "-outputFile", results,
               "-calcName", "p", "-logFile", os.path.join(work, "monitor.log"),
               "-filesToTransfer", "", "-importDir", work + os.sep,
               "-progress", "1"]
        if out2:
            cmd += ["-jobOutputFile2", out2]
        proc = subprocess.run(cmd, cwd=work, stdin=subprocess.DEVNULL,
                              capture_output=True, text=True, timeout=300)
        if proc.returncode != 0:
            return None, {}, proc.stderr[-400:]
        raw = open(results, "rb").read().decode("utf-8", "replace")
        texts, i = [], 0
        while True:
            i = raw.find("\001", i)
            if i < 0:
                break
            length = int(raw[i + 2:i + 6])
            payload = raw[i + 6:i + 6 + length].replace("\002", "\n")
            i += 6 + length
            if payload.startswith("jmPROG\0"):
                texts.append(payload.split("\0", 1)[1])
        stages = {}
        for line in (texts[-1].splitlines()[1:] if texts else []):
            name, _, rest = line.partition(" ")
            stages[name] = rest
        return len(texts), stages, texts[-1] if texts else ""
    finally:
        shutil.rmtree(work, ignore_errors=True)


def fields(stage_text):
    return dict(w.split("=", 1) for w in stage_text.split() if "=" in w)


def hist_len(stages, name):
    h = fields(stages.get(name + "~", ""))
    return len(next(iter(h.values())).split(",")) if h else 0


# name, desc, output file, expectations: {stage: {key: value-or-callable}},
# history lengths {stage: (min, max)}
CASES = []


def case(name, desc, out, want, hist=None, out2=None):
    CASES.append((name, desc, out, want, hist or {}, out2))


def fx(code, name):
    own = os.path.join(OWN, code, name)
    return own if os.path.exists(own) else os.path.join(SHARED, code, name)


def has(prefix):
    return lambda v: v.startswith(prefix)


def target(v):
    return "/" in v


case("orca opt+freq (analytic, HF)", "orca.desc", fx("orca", "h2o_optfreq.out"),
     {"opt": {"step": "4", "converged": "yes", "energy": has("-74.9659"),
              "maxgrad": target, "rmsstep": target, "dE": target},
      "freq": {"done": "yes", "zpe": has("0.0243"), "kind": "analytic",
              "point": "9/9", "phase": "assemble"},
      "scf": {"cycle": lambda v: int(v) >= 1, "dE": target}},
     {"opt": (4, 4)})
# Mode counts from the printed frequencies (#611): the translations and
# rotations are skipped, a negative frequency is imaginary.
case("orca frequency count", "orca.desc", fx("orca", "h2o_freq_raman.out"),
     {"freq": {"done": "yes", "real": "3", "imaginary": "0"}})
case("orca frequency count with an imaginary mode", "orca.desc",
     fx("orca", "ethanol_sym_optfreq.out"),
     {"freq": {"done": "yes", "real": "20", "imaginary": "1"}})
case("gaussian-16 frequency count", "gaussian-16.desc",
     fx("gaussian-16", "h2o_optfreq.log"),
     {"freq": {"done": "yes", "real": "3", "imaginary": "0"}})
case("gaussian-16 frequency count with an imaginary mode", "gaussian-16.desc",
     fx("gaussian-16", "hcn_irc.log"),
     {"freq": {"done": "yes", "real": "2", "imaginary": "1"}})
case("nwchem frequency count", "nwchem.desc",
     fx("nwchem", "h2o_freq_thermo.nwout"),
     {"freq": {"done": "yes", "real": "3", "imaginary": "0"}})
case("mopac frequency count (CH4, degenerate modes)", "mopac.desc",
     fx("mopac", "ch4_force.out"),
     {"freq": {"done": "yes", "real": "9", "imaginary": "0"}})

case("orca opt, not converged", "orca.desc", fx("orca", "optmax.out"),
     {"opt": {"step": "2", "converged": "no", "maxgrad": target}})
case("orca numerical frequencies", "orca.desc", fx("orca", "numfreq.out"),
     {"freq": {"point": "12/12", "kind": "numerical", "done": "yes",
               "zpe": has("0.0229")}})
# The CP-SCF solver also runs for NMR; that must not make a Frequencies stage.
case("orca NMR has no frequency stage", "orca.desc",
     fx("orca", "h2o_nmr_chelpg.out"), {"!freq": {}})

# A relaxed scan: one record per scan point; the optimisation's history is
# the whole job's, one descent per point (5 points, 39 steps).
case("orca relaxed scan (ethane dihedral, HF/STO-3G, 5 points)", "orca.desc",
     fx("orca", "scan.out"),
     {"scan": {"point": "5/5"}, "opt": {"converged": "yes", "step": "6"}},
     {"opt": (39, 39)})
# NEB-TS (HCN -> HNC, HF/3-21G, 4 images): 16 L-BFGS iterations 0..15, then the
# TS optimisation on the climbing image.  The thresholds are the NEB settings'.
case("orca NEB-TS (HCN -> HNC, HF/3-21G)", "orca.desc", fx("orca", "nebts.out"),
     {"neb": {"iter": "15", "erel": "0.097436", "maxforce": "0.004704/2.00e-02",
              "rmsforce": "0.001211/1.00e-02", "maxcforce": "0.000999/2.00e-03",
              "rmscforce": "0.000530/1.00e-03"},
      "opt": {"converged": "yes", "step": "3"}},
     {"neb": (16, 16)})
# ECCE-QM (made by the engine of this tree): an analytic RHF Hessian has no
# count, finite differences count displaced geometries.
case("ecceqm opt+freq (analytic RHF Hessian)", "ecceqm.desc",
     fx("ecceqm", "h2o_optfreq.qmout"),
     {"freq": {"kind": "analytic", "done": "yes", "real": "3", "zpe": has("0.0243")}})
case("ecceqm numerical frequencies (Raman input)", "ecceqm.desc",
     fx("ecceqm", "h2o_numfreq.qmout"),
     {"freq": {"kind": "numerical", "point": "18/18", "done": "yes"}})

case("gaussian-16 relaxed scan (H2O2 dihedral, 7 points)", "gaussian-16.desc",
     fx("gaussian-16", "h2o2_dihedral_scan.log"),
     {"scan": {"point": "7/7"}, "opt": {"converged": "yes"}},
     {"opt": (13, 20)})

case("gaussian-16 opt+freq (B3LYP/6-31G*, #P)", "gaussian-16.desc", fx("gaussian-16", "nh3_optfreq.log"),
     {"opt": {"step": "4", "converged": "yes", "maxgrad": target, "rmsgrad": target,
              "maxstep": target, "rmsstep": target, "dE": lambda v: "e-" in v},
      "freq": {"done": "yes", "zpe": has("0.0345"), "kind": "analytic", "pass": "5"},
      "scf": {"cycle": lambda v: int(v) >= 1, "converged": "yes"}},
     {"opt": (3, 4)})
case("gaussian-16 opt, not converged", "gaussian-16.desc", fx("gaussian-16", "nh3_optmax.log"),
     {"opt": {"step": "3", "converged": "no", "maxgrad": target}})
case("gaussian-09 desc on g16 output", "gaussian-09.desc", fx("gaussian-16", "nh3_optfreq.log"),
     {"opt": {"step": "4", "converged": "yes"}, "freq": {"done": "yes"}})

case("nwchem SCF geovib (stdout + trace)", "nwchem.desc", fx("nwchem", "h2o_scf_geovib.eprint"),
     {"opt": {"step": "3", "converged": "yes", "maxgrad": target, "rmsgrad": target,
              "maxstep": target, "rmsstep": target},
      "scf": {"cycle": lambda v: int(v) >= 1, "grad": target},
      "freq": {"done": "yes", "kind": "analytic", "zpe": has("0.0")}},
     {"opt": (4, 4)}, out2=fx("nwchem", "h2o_scf_geovib.nwout"))
case("nwchem DFT geovib (stdout + trace)", "nwchem.desc", fx("nwchem", "h2o_dft_geovib.eprint"),
     {"opt": {"converged": "yes", "maxgrad": target},
      "scf": {"dE": target, "energy": has("-75.")},
      "freq": {"done": "yes", "kind": "analytic", "zpe": has("0.022")}},
     out2=fx("nwchem", "h2o_dft_geovib.nwout"))
case("nwchem opt, not converged", "nwchem.desc", fx("nwchem", "h2o_scf_opt_capped.eprint"),
     {"opt": {"converged": "no", "step": "2"}},
     out2=fx("nwchem", "h2o_scf_opt_capped.nwout"))

case("mopac geovib (PM7 EF, then FORCE)", "mopac.desc", fx("mopac", "gv.out"),
     {"opt": {"step": "8", "converged": "yes",
              "grad": lambda v: v.endswith("/0.05000") and float(v.split("/")[0]) < 0.05},
      "scf": {"converged": "yes"},
      "freq": {"point": "9/9", "done": "yes"}},       # 3 atoms, 3 steps each
     {"opt": (8, 8)})
case("mopac opt, not converged", "mopac.desc", fx("mopac", "capped.out"),
     {"opt": {"step": "3", "converged": "no"}})
case("mopac 1SCF", "mopac.desc", fx("mopac", "o2_sp.out"),
     {"scf": {"converged": "yes"}})
case("mopac 1SCF with PL (SCF iterations printed)", "mopac.desc",
     fx("mopac", "o2_sp_pl.out"),
     {"scf": {"cycle": "8", "converged": "yes"}})

case("qe relax (pw.x 6.7)", "qe.desc", fx("qe", "h2o_relax.pwout"),
     {"opt": {"istep": "3", "converged": "yes", "maxgrad": lambda v: "/" not in v},
      "scf": {"cycle": lambda v: int(v) >= 1, "converged": "yes", "scferr": target}},
     {"opt": (4, 4)})
case("qe vc-relax", "qe.desc", fx("qe", "si_vcrelax.pwout"),
     {"opt": {"istep": "4", "converged": "yes"}})
case("qe scf only", "qe.desc", fx("qe", "si_scf.pwout"),
     {"scf": {"converged": "yes", "scferr": target}})
case("qe relax, step limit", "qe.desc", fx("qe", "capped.pwout"),
     {"opt": {"istep": "1", "converged": "no"}})
# An md run ends on the step limit line a relax also uses, so it leaves an
# "opt" stage with converged=no and no step; known, see qe.desc.
case("qe md (pw.x 6.7)", "qe.desc", fx("qe", "si_md.pwout"),
     {"md": {"point": "5/5"}})

case("cp2k geometry optimisation (NiO+U, UKS, OT, 3 steps)", "cp2k.desc",
     fx("cp2k", "nio_u_opt.cp2kout"),
     {"opt": {"step": "3", "converged": "no", "energy": has("-184.9534"),
              "maxgrad": target, "rmsgrad": target, "maxstep": target,
              "rmsstep": target},
      "scf": {"cycle": lambda v: int(v) >= 1, "converged": "yes", "scferr": target,
              "energy": has("-184.")}},
     {"opt": (3, 3)})
case("cp2k energy (silicon, OT)", "cp2k.desc", fx("cp2k", "si_scf.cp2kout"),
     {"scf": {"cycle": "35", "converged": "yes", "scferr": target,
              "energy": has("-7.2446")}},
     {"scf": (35, 35)})
case("cp2k energy, diagonalisation", "cp2k.desc", fx("cp2k", "si_diag.cp2kout"),
     {"scf": {"cycle": "8", "converged": "yes", "scferr": target}})

case("gromacs md (2025.2)", "gromacs.desc", fx("gromacs", "water_md.log"),
     {"md": {"point": "500/500"}})
case("gromacs minimisation", "gromacs.desc", fx("gromacs", "water_em.log"),
     {"steep": {"point": "4/200", "converged": "yes"}})
case("gromacs prepare (Trp-cage, 2025.2)", "gromacs.desc",
     fx("gromacs", "prepare.gmxlog"),
     {"prepare": {"point": "4/4", "tool": "genion", "done": "yes",
                  "atoms": "5913", "residues": "20", "waters": "1866",
                  "pos": "5", "neg": "6", "posname": "NA", "negname": "CL",
                  "charge0": "1.000000", "charge1": "0", "box": "dodecahedron",
                  "a": "4.392", "alpha": "60.00", "gamma": "90.00"}})


def run_case(name, desc, out, want, hist, out2):
    n, stages, text = replay(os.path.join(PARSERS, desc), out, "ALL", out2)
    check(n is not None and n >= 1, "%s: monitor ran, %s snapshot(s)" % (name, n))
    if not n:
        return
    for stage, keys in want.items():
        if stage.startswith("!"):          # "!freq": the stage must not exist
            check(stage[1:] not in stages, "%s: no %s stage" % (name, stage[1:]))
            continue
        got = fields(stages.get(stage, ""))
        check(stage in stages, "%s: stage %s present" % (name, stage))
        for key, expect in keys.items():
            v = got.get(key)
            ok = v is not None and (expect(v) if callable(expect) else v == expect)
            check(ok, "%s: %s %s = %s" % (name, stage, key, v))
    for stage, (lo, hi) in hist.items():
        n = hist_len(stages, stage)
        check(lo <= n <= hi, "%s: %s history has %d values" % (name, stage, n))
    print("  last snapshot:\n    " + text.replace("\n", "\n    "))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--case", action="append", default=[])
    ap.add_argument("--show", nargs="+")
    ap.add_argument("--file2", help="with --show: the stdout file (jobOutputFile2)")
    a = ap.parse_args()
    if not shutil.which("perl"):
        return 77
    if a.show:
        n, stages, text = replay(os.path.abspath(a.show[0]),
                                 os.path.abspath(a.show[1]),
                                 a.show[2] if len(a.show) > 2 else "ALL",
                                 os.path.abspath(a.file2) if a.file2 else None)
        print("%s snapshot(s)\n%s" % (n, text))
        return 0
    for c in CASES:
        if a.case and not any(s in c[0] for s in a.case):
            continue
        run_case(*c)
    print("FAILED: %s" % "; ".join(failures) if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
