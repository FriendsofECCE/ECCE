#!/usr/bin/env python3
"""ai.ecceqm writes "localise false" when the Localised orbitals box (now in
Runtype Details, same key as before) is unticked, for every run type, and
nothing otherwise, so decks are as they were when it was in Theory Details."""
import os, shutil, subprocess, sys, tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
CASES = [(rt, v, want) for rt in ("Energy", "Geometry", "Vibration", "GeoVib")
         for v, want in (("0", "localise false"), ("1", None), (None, None))]


def deck(runtype, localise, extra=()):
    work = tempfile.mkdtemp(prefix="ecce-qmdeck-")
    try:
        shutil.copy(os.path.join(PARSERS, "ecceqm.tpl"), os.path.join(work, "t.tpl"))
        open(os.path.join(work, "w.frag"), "w").write(
            "# frag\nnum_atoms: 3\ntitle: w\natom_info: symbol\natom_list:\n"
            "O 0 0 0.1\nH 0 0.76 -0.47\nH 0 -0.76 -0.47\n")
        open(os.path.join(work, "w.basis"), "w").write("basis 6-31G*\nend\n")
        params = ["Category: SCF", "Theory: HF", "RunType: " + runtype, "Charge: 0",
                  "ChemSys.Multiplicity: 1"]
        if localise is not None:
            params.append("ES.Theory.ECCEQM.Localise: " + localise)
        params.extend(extra)
        open(os.path.join(work, "w.param"), "w").write("\n".join(params) + "\n")
        p = subprocess.run(["perl", os.path.join(PARSERS, "ai.ecceqm"), "-p", "-f", "-b",
                            "-n", "w", "-t", "t.tpl"], cwd=work, capture_output=True,
                           text=True, env=dict(os.environ, ECCE_HOME=ROOT))
        if p.returncode:
            raise RuntimeError(p.stderr)
        return open(os.path.join(work, "t.tpl")).read()
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    fails = 0
    for runtype, localise, want in CASES:
        got = [l for l in deck(runtype, localise).splitlines() if l.startswith("localise")]
        ok = got == ([want] if want else [])
        print("%-4s %s Localise=%s -> %s" % ("ok" if ok else "FAIL", runtype, localise, got))
        fails += not ok
    # Store orbitals at every point (#1114): only an unticked box is written, and only
    # for the run types that scan or follow a path.
    for runtype, v, want in (("IRC", "0", "point_orbitals no"), ("Geometry", "0", "point_orbitals no"),
                             ("IRC", "1", None), ("Geometry", None, None), ("Energy", "0", None)):
        got = [l for l in deck(runtype, None, ["ES.Runtype.PointOrbitals: " + v] if v else []).splitlines()
               if l.startswith("point_orbitals")]
        ok = got == ([want] if want else [])
        print("%-4s %s PointOrbitals=%s -> %s" % ("ok" if ok else "FAIL", runtype, v, got))
        fails += not ok
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
