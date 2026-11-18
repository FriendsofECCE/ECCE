#!/usr/bin/env python3
"""Builder fragment script hooks, persisted structures and ORCA ghost export."""
import os
from pathlib import Path
import re
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import apps
import fixture
import run_tests

failures = []


def check(ok, what, detail=""):
    print("%s: %s" % ("PASS" if ok else "FAIL", what))
    if not ok:
        failures.append(what)
        print(detail)


def make_calculation(folder, atoms, bonds, charge):
    folder.mkdir(parents=True)
    (folder / "Parameters").mkdir()
    (folder / "Inputs").mkdir()
    (folder / "Outputs").mkdir()
    (folder / "Props").mkdir()
    metadata = {"application": "ORCA", "contenttype": "ecceCalculation",
                "resourcetype": "virtual_document", "state": "Ready",
                "configurationCollection": "Parameters", "charge": str(charge),
                "spinmultiplicity": "Singlet", "runtype": "Energy"}
    source = HERE / "fixtures" / "calc-polar-c6h6-g16" / ".ecce-meta"
    for line in source.read_text().splitlines():
        fields = line.split("\t\t", 1)
        if len(fields) != 2:
            continue
        key = fields[0].rsplit(":", 1)[-1]
        if key.endswith("Document") or key.endswith("Collection") or key in ("createdWith", "currentVersion"):
            metadata[key] = fields[1]
    configurations = ('<rdf:Bag xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" '
                      'xmlns:xlink="http://www.w3.org/1999/xlink">'
                      '<rdf:li><rdf:href xlink:href="Parameters/chemsys.mvm" '
                      'xlink:type="simple"></rdf:href></rdf:li></rdf:Bag>')
    (folder / ".ecce-meta").write_text("".join(
        ".\thttp://www.emsl.pnl.gov/ecce:%s\t\t%s\n" % item for item in metadata.items()) +
        ".\thttp://purl.oclc.org/NET/cmcs/schema/:hasConfigurations\t\t" + configurations + "\n")
    text = "title: counterpoise fixture\ntype: molecule\nfragment_attributes:\n  charge: %d\nend_fragment_attributes:\n" % charge
    text += "num_atoms: %d\natom_info: symbol cart\natom_list:\n" % len(atoms)
    text += "\n".join(atoms) + "\nnum_bonds: %d\nbond_list:\n" % len(bonds)
    text += "".join("%d %d 1.0 2\n" % bond for bond in bonds)
    (folder / "Parameters" / "chemsys.mvm").write_text(text)


def scripted(display, folder, lines, script_name):
    script = folder.parent / (script_name + ".script")
    script.write_text("\n".join(["wait 4000", "background 1 1 1"] + lines + ["wait 500", "quit"]) + "\n")
    result = apps.run(display, "builder", args=("-context", folder.as_uri() + "/"),
                      windowTimeout=60, settle=120,
                      env={"ECCE_BUILDER_SCRIPT": str(script), "ECCE_FEEDBACK_STDERR": "1"})
    check(not result.crashed, script_name + " Builder stays alive", result.log)
    return result.log or ""


def geometry(path):
    if not path.exists():
        return []
    text = path.read_text()
    block = text.split("* xyz", 1)[-1].splitlines()[1:]
    return [line.split()[0] for line in block if line.strip() and line.strip() != "*"]


def main():
    def check_app(display, name, results, verbose=False):
        base = Path(fixture.stateDir()) / "counterpoise-localdata"
        base.mkdir(parents=True, exist_ok=True)
        project = base / "users" / "local" / "p"
        project.mkdir(parents=True)
        (project / ".ecce-meta").write_text(".\thttp://www.emsl.pnl.gov/ecce:contenttype\t\tecceProject\n")
        dimer = project / "water-dimer"
        make_calculation(dimer, ["O 0 0 0", "H 0.9584 0 0", "H -0.2392 0.9278 0",
                                 "O 2.85 0 0", "H 3.0892 0.9278 0", "H 3.0892 -0.9278 0"],
                         [(1, 2), (1, 3), (4, 5), (4, 6)], 0)
        ion = project / "sodium-water"
        make_calculation(ion, ["Na 0 0 0", "O 3.2 0 0", "H 3.44 0.93 0", "H 3.44 -0.93 0"],
                         [(2, 3), (2, 4)], 1)
        os.environ["ECCE_LOCAL_DATA"] = str(base)
        restore = fixture.settleUpgradeNotices()
        try:
            out = base / "dimer-inputs"
            log = scripted(display, dimer, [
                "fragments", "fragments auto", "fragment 1 atoms 1-3", "fragment 2 atoms 4,5,6",
                "fragment 1 charge 0 mult 1", "fragment 1 charge 2 mult 1", "fragment 1 atoms 99",
                "fragments", "counterpoise orca %s 1" % out,
                "fragment 3 atoms 4-6", "save", "wait 2500"], "dimer")
            check("fragment 1 atoms 1 2 3 charge 0 mult 1" in log and
                  "fragment 2 atoms 4 5 6 charge 0 mult 1" in log,
                  "water dimer query assigns two covalent molecules", log)
            check(re.search(r"fragment 1 charge 2 mult 1: FAIL: .*sum", log) is not None and
                  re.search(r"fragment 1 atoms 99: FAIL:", log) is not None,
                  "invalid charge sum and invalid atom are refused with messages", log)
            check(geometry(out / "fragment-1.inp") == ["O", "H", "H", "O:", "H:", "H:"] and
                  geometry(out / "fragment-2.inp") == ["O:", "H:", "H:", "O", "H", "H"],
                  "Builder export writes the two complementary water ghost geometries", log)
            stored = (dimer / "Parameters" / "chemsys.mvm").read_text()
            check("fragment_states: 2" in stored and
                  re.search(r"atom_fragment_list:\s*1\s+1\s+1\s+3\s+3\s+3\s", stored) is not None,
                  "Builder Save persists overridden assignments and fragment states", stored + "\n" + log)
            reopened = scripted(display, dimer, ["fragments", "counterpoise orca %s 1" %
                                                (base / "reopened-inputs")], "reopened")
            check("fragment 1 atoms 1 2 3 charge 0 mult 1" in reopened and
                  "fragment 3 atoms 4 5 6 charge 0 mult 1" in reopened and
                  "counterpoise orca: " in reopened and "FAIL:" not in reopened,
                  "saved fragment definitions are usable after reopening", reopened)
            check(geometry(base / "reopened-inputs" / "fragment-3.inp") ==
                  ["O:", "H:", "H:", "O", "H", "H"],
                  "reopened export retains the saved fragment ID override", reopened)
            ionic_out = base / "ionic-inputs"
            ionic = scripted(display, ion, ["fragments auto", "fragment 1 charge 1 mult 1",
                                            "fragments", "counterpoise orca %s 1" % ionic_out,
                                            "save", "wait 2500"], "ionic")
            check("fragment 1 atoms 1 charge 1 mult 1" in ionic and
                  "fragment 2 atoms 2 3 4 charge 0 mult 1" in ionic,
                  "Na+(H2O) accepts sodium charge +1 and neutral water", ionic)
            sodium_file = ionic_out / "fragment-1.inp"
            water_file = ionic_out / "fragment-2.inp"
            check(sodium_file.exists() and water_file.exists() and
                  "* xyz 1 1" in sodium_file.read_text() and
                  "* xyz 0 1" in water_file.read_text(),
                  "ionic monomer exports use their own charge and multiplicity", ionic)
        finally:
            restore()
            os.environ.pop("ECCE_LOCAL_DATA", None)
        if failures:
            results.fail("builder-counterpoise", "; ".join(failures))

    run_tests.checkApp = check_app
    sys.argv = [sys.argv[0], "--app", "builder"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    raise SystemExit(main())
