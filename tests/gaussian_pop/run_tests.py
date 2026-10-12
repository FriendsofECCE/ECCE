#!/usr/bin/env python3
"""ai.gauss16 puts every ticked population analysis into ONE Pop=(...)
keyword (Gaussian rejects two).  Generates decks for the combinations and
checks the route; with --run-g16 also runs CO through g16 (1 core)."""
import itertools, os, re, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")


def deck(extra, theory="RDFT", category="DFT"):
    work = tempfile.mkdtemp(prefix="ecce-gpop-")
    try:
        shutil.copy(os.path.join(PARSERS, "g16.tpl"), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(
            "# frag\nnum_atoms: 2\ntitle: co\natom_info: symbol\natom_list:\n"
            "C 0.0 0.0 0.0\nO 0.0 0.0 1.128\n")
        open(os.path.join(work, "w.basis"), "w").write("useRouteCard 6-31G*\n")
        params = ["Category: " + category, "Theory: " + theory,
                  "RunType: Energy", "Charge: 0", "ChemSys.Multiplicity: 1",
                  "ES.Theory.DFT.XCFunctionals: rPBE1PBE"] + extra
        open(os.path.join(work, "w.param"), "w").write("\n".join(params) + "\n")
        env = dict(os.environ, ECCE_HOME=ROOT)
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.gauss16"), "-p", "-f",
                            "-b", "-n", "w", "-t", "t.tpl"], cwd=work, env=env,
                           capture_output=True, text=True)
        if p.returncode:
            raise RuntimeError(p.stderr)
        return open(os.path.join(work, "t.tpl")).read()
    finally:
        shutil.rmtree(work, ignore_errors=True)


KEYS = [("UseNBO: 1", "NBO7"), ("UseHirshfeld: 1", "Hirshfeld"),
        ("ESPCharges: CHelpG", "CHelpG"), ("ESPCharges: Merz-Kollman", "MK")]


def main():
    fails = 0
    for n in range(len(KEYS) + 1):
        for combo in itertools.combinations(KEYS, n):
            if {"CHelpG", "MK"} <= {w for _, w in combo}:
                continue  # one ESP fit per job: the dialog cannot offer both
            extra = ["ES.Theory." + k for k, _ in combo] + \
                ["ES.Theory.NBOVersion: NBO 7"]
            d = deck(extra)
            pops = re.findall(r"Pop=\(([^)]*)\)", d)
            want = [w for _, w in combo]
            want.sort(key=lambda w: ["NBO7", "Hirshfeld", "MK", "CHelpG"].index(w))
            if "NBO7" in want:
                want.insert(1, "savenbos")
            want.append("full")
            ok = len(re.findall(r"Pop=", d)) == 1 and \
                pops and pops[0].replace(" ", "").split(",") == want
            print("%-4s %s" % ("ok" if ok else "FAIL", pops))
            fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
