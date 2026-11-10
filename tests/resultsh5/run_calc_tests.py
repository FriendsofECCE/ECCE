#!/usr/bin/env python3
"""results.h5 from parsed calculations (#690).

    tests/resultsh5/run_calc_tests.py <results_calc_test> [<ecce-results>]

Stored calculations (ECCE-QM, ORCA, Gaussian 16, NWChem; closed and open
shell) are copied to a scratch folder, results.h5 is stored in each copy, and
results_calc_test checks tr(PS) = N and C'SC = 1, the size, and that the
Props path and the file path give one density.  Real ORCA (HF/cc-pVQZ with a
g shell, CrO) and xtb output goes through the parsers and the same
conversion.  `ecce-results write` is run on one copy for the back-fill route.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
FLATTEN = os.path.join(ROOT, "tests", "basisflatten", "fixtures")
XTB = os.path.join(ROOT, "tests", "parsers", "fixtures", "xtb")
T = os.path.join(ROOT, "tests")
CALCS = [os.path.join(T, p) for p in (
    "ecceqm/fixtures/h2o-geovib", "ecceqm/fixtures/o2-roks", "ecceqm/fixtures/o2-roks-loc",
    "builder/densitymap-fixtures/ethene", "tdat/densityfields-fixtures/o2",
    "mocomp/fixtures/water", "mocomp/fixtures/cocl4uks", "mocomp/fixtures/ch3",
    "mocomp/fixtures/benzene", "mocomp/fixtures/crco6",
    "apps/fixtures/calc-orca-water-ccpvdz",
    "apps/fixtures/calc-water-vib")]
# NWChem output stored with 3-digit coefficients: its orbitals cannot pass the
# checks, so the file is written, marked, and ignored by the views.
EXPECT_FAILED = [os.path.join(T, "apps", "fixtures", "calc-water-opt")]
G16 = os.path.join(T, "apps", "fixtures", "calc-water-vib")   # Gaussian 16, properties only
ORCA_META = os.path.join(T, "apps", "fixtures", "calc-orca-water-ccpvdz", ".ecce-meta")
XTB_CASES = [("h2o", 8), ("so2", 18)]
EX = os.path.join(ROOT, "data", "client", "examples")
# Views reading vibrations and traces from results.h5 against the Props:
# (stored example, keys that must come from the file).  An example stored
# without results.h5 gets one from `ecce-results write` first.
VIEW_CASES = [
    ("getting-started/orca-benzene/stored/benzene", "VIBFREQ,VIBIR,VIB,GEOMTRACE,TEVEC"),
    ("organic/conformations/ecceqm-butane-scan/stored/butane_scan",
     "SCANGEOM,SCANENERGY,SCANCOORD,GEOMTRACE,TEVEC"),
    ("computational/h2-dissociation/orca-h2_ccsd_scan/stored/h2_ccsd_scan",
     "SCANGEOM,SCANENERGY,SCANCOORD"),
    ("getting-started/ecceqm-water/stored/water", "VIBFREQ,VIBIR,VIB"),
    ("organic/reactions/orca-sn2_irc/stored/sn2_irc", "IRCGEOM,IRCENERGY,IRCCOORD"),
    # Gaussian 16 traces reach the file only with their Angstrom label (#946).
    # IRCGEOM has one structure fewer than IRCENERGY/IRCCOORD (no TS frame),
    # so the file drops those companions and they stay on the Props.
    ("workflows/sn2-cl-ch3br/g16-irc/stored/sn2_irc", "IRCGEOM,GEOMTRACE"),
    ("workflows/sn2-cl-ch3br/g16-product/stored/sn2_product", "GEOMTRACE,TEVEC"),
]

# Shipped with results.h5 and the small Props only (#883): the raw output and
# the trace/vibration Props are removed from a copy, and the views must get
# the full calculation's values with the parsed layout.
FILE_ONLY_CASES = [
    "organic/conformations/ecceqm-butane-scan/stored/butane_scan",
    "getting-started/orca-benzene/stored/benzene",
    "computational/h2-dissociation/orca-h2_ccsd_scan/stored/h2_ccsd_scan",
    "workflows/sn2-cl-ch3br/g16-irc/stored/sn2_irc",
    "organic/reactions/orca-sn2_irc/stored/sn2_irc",
]
FILE_KEYS = ("VIBFREQ", "VIBIR", "VIBRAM", "VIB", "SCANGEOM", "SCANENERGY", "SCANCOORD",
             "IRCGEOM", "IRCENERGY", "IRCCOORD", "GEOMTRACE", "TEVEC")


def strip_to_file(calc):
    props = os.path.join(calc, "Props")
    for k in FILE_KEYS:
        if os.path.exists(os.path.join(props, k)):
            os.remove(os.path.join(props, k))
    meta = os.path.join(props, ".ecce-meta")
    if os.path.exists(meta):
        lines = [l for l in open(meta) if l.split("\t", 1)[0] not in FILE_KEYS]
        open(meta, "w").writelines(lines)
    out = os.path.join(calc, "Outputs")
    for f in os.listdir(out):
        if f != "results.h5" and not f.endswith(".ecce_run_log"):
            os.remove(os.path.join(out, f))


def run(cmd, env, **kw):
    r = subprocess.run(cmd, env=env, capture_output=True, text=True, **kw)
    sys.stdout.write(r.stdout)
    sys.stderr.write(r.stderr)
    return r.returncode


def main():
    if len(sys.argv) < 2 or not os.access(sys.argv[1], os.X_OK):
        print("usage: run_calc_tests.py <results_calc_test> [<ecce-results>]")
        return 2
    exe = os.path.abspath(sys.argv[1])
    cli = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else None
    tmp = tempfile.mkdtemp(prefix="resultsh5-calc-")
    env = dict(os.environ, ECCE_HOME=ROOT, HOME=tmp, ECCE_REALUSERHOME=tmp,
               ECCE_REALUSER="resultsh5")
    rc = 0
    try:
        copies = []
        for src in CALCS + EXPECT_FAILED:
            dst = os.path.join(tmp, os.path.basename(src))
            shutil.copytree(src, dst, ignore=shutil.ignore_patterns(".DAV"))
            os.makedirs(os.path.join(dst, "Outputs"), exist_ok=True)
            if src == G16:
                # the fixture has no .ecce-meta: a local calculation reads its
                # facts from there (as tests/apps/orca_mo_test.py does)
                meta = open(ORCA_META).read().replace("\tORCA\n", "\tGaussian-16\n")
                open(os.path.join(dst, ".ecce-meta"), "w").write(meta)
            copies.append(dst if src not in EXPECT_FAILED else "failed:" + dst)
        rc |= run([exe, "fixture"] + copies, env)
        rc |= run([exe, "emptyspin"], env)

        if cli:
            # back-fill an existing calculation, then read it with the checker
            target = copies[0]
            h5 = os.path.join(target, "Outputs", "results.h5")
            if os.path.exists(h5):
                os.remove(h5)
            rc |= run([cli, "write", target], env)
            if not os.path.exists(h5):
                print("FAIL: ecce-results write stored no results.h5")
                rc = 1
            else:
                print("ok:   ecce-results write stored results.h5 (%d bytes)" % os.path.getsize(h5))
            out = os.path.join(tmp, "elsewhere.h5")
            rc |= run([cli, "write", target, out], env)
            rc |= run([cli, "show", out], env)

            # NWChem: the eprint's 3-decimal coefficients fail the checks; the
            # kept nwchem.movecs (same orbitals) gives full precision (#944)
            nw = os.path.join(tmp, "nwchem-methane-movecs")
            shutil.copytree(os.path.join(HERE, "fixtures", "nwchem-methane-movecs"), nw)
            for keep, want in ((True, "checks passed"), (False, "checks FAILED")):
                if not keep:
                    os.remove(os.path.join(nw, "Outputs", "nwchem.movecs"))
                r = subprocess.run([cli, "write", nw, os.path.join(tmp, "nw.h5")],
                                   env=env, capture_output=True, text=True)
                sys.stdout.write(r.stdout)
                ok = want in r.stdout
                print("%s NWChem %s movecs: %s" % ("ok:  " if ok else "FAIL:",
                                                  "with" if keep else "without", want))
                rc |= 0 if ok else 1

        if cli:
            args = []
            for i, (rel, keys) in enumerate(VIEW_CASES):
                dst = os.path.join(tmp, "view%d-%s" % (i, os.path.basename(rel)))
                shutil.copytree(os.path.join(EX, rel), dst, ignore=shutil.ignore_patterns(".DAV"))
                os.makedirs(os.path.join(dst, "Outputs"), exist_ok=True)
                if not os.path.exists(os.path.join(dst, "Outputs", "results.h5")):
                    rc |= run([cli, "write", dst], env)
                args.append(dst + ("=" + keys if keys else ""))
            rc |= run([exe, "views"] + args, env)

            args = []
            for i, rel in enumerate(FILE_ONLY_CASES):
                full = os.path.join(tmp, "full%d-%s" % (i, os.path.basename(rel)))
                bare = os.path.join(tmp, "bare%d-%s" % (i, os.path.basename(rel)))
                shutil.copytree(os.path.join(EX, rel), full, ignore=shutil.ignore_patterns(".DAV"))
                if not os.path.exists(os.path.join(full, "Outputs", "results.h5")):
                    rc |= run([cli, "write", full], env)
                shutil.copytree(full, bare)
                strip_to_file(bare)
                args.append(full + "=" + bare)
            rc |= run([exe, "fileonly"] + args, env)

        # real ORCA blocks: CrO/def2-SVP (f functions, a metal) and HF/cc-pVQZ (g)
        for block, nb, xyz, nel in (
                ("cro_mo_block.txt", "cro.nb", "cro.xyz", "32"),
                ("hf_qz_mo_block.txt", "hf_qz.nb", "hf_qz.xyz", "10")):
            perl = subprocess.run(
                ["perl", os.path.join(ROOT, "scripts", "parsers", "orca.mo"),
                 "MO", "Energy", "SCF", "RHF"],
                stdin=open(os.path.join(FLATTEN, block)), capture_output=True, text=True)
            parsed = os.path.join(tmp, block + ".parsed")
            open(parsed, "w").write(perl.stdout)
            rc |= run([exe, "orca", parsed, os.path.join(FLATTEN, nb),
                       os.path.join(FLATTEN, xyz), nel], env)

        for name, nel in XTB_CASES:
            molden = os.path.join(XTB, name + "_gfn2.molden")
            parsed = subprocess.run(
                ["perl", os.path.join(ROOT, "scripts", "parsers", "xtb.mo"), ".",
                 "Energy", "SE", "GFN2-xTB"],
                input="ECCE-XTB-MOLDEN\n" + open(molden).read(),
                capture_output=True, text=True, check=True).stdout
            out = os.path.join(tmp, name + ".xtb")
            open(out, "w").write(parsed)
            rc |= run([exe, "xtb", molden, out, str(nel)], env)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    print("RESULT", "FAIL" if rc else "OK")
    return 1 if rc else 0


if __name__ == "__main__":
    sys.exit(main())
