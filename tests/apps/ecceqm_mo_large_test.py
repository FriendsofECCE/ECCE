#!/usr/bin/env python3
"""The MOs panel for an ECCE-QM calculation with 156 functions, headless.

    tests/apps/ecceqm_mo_large_test.py

Benzoic acid, DFT/def2-SVP: 156 spherical functions, so ecce-qm's parser
writes lines longer than the 2048 bytes PropertyInterpreter used to read
(tests/parsers/propinterp_test.py).

  stored by 9.1/9.2   MO (156x156) holding 152 values and ORBENG 134 of
                      156, no ORBOCC: what the cut lines left.  The panel
                      must say the data is missing; it used to throw
                      "Orbital energy vector is null" and end the Builder.
  stored now          the same shape through the fixed interpreter (synthetic
                      orbitals, a third occupied): 156 rows in the MO table, HOMO 52.

Same installed tree and isolation as orca_mo_test.py; also needs the build
tree's static libraries for the second case (ECCE_TEST_BUILD).  Exit 77
(skip) without an install or Xvfb.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)

import apps            # noqa: E402
import isolate         # noqa: E402
import xdisplay        # noqa: E402
import orca_mo_test as mo   # noqa: E402

FIXTURE = os.path.join(HERE, "fixtures", "ecceqm-benzoic-156")
SKIP = 77
NO_ENERGIES = "has no orbital energies"


def makeCalc(data, name, props):
    url = mo.makeCalc(data, name)
    calc = url[len("file://"):].rstrip("/")
    meta = os.path.join(calc, ".ecce-meta")
    text = open(meta).read().replace("\tORCA\n", "\tECCE-QM\n")
    with open(meta, "w") as handle:
        handle.write(text)
    for f in ("chemsys.mvm", "BasisSet.ecce_basisset"):
        shutil.copy(os.path.join(FIXTURE, "Parameters", f),
                    os.path.join(calc, "Parameters", f))
    for f in os.listdir(os.path.join(calc, "Props")):
        if not f.startswith("."):
            os.unlink(os.path.join(calc, "Props", f))
    for f in os.listdir(props):
        shutil.copy(os.path.join(props, f), os.path.join(calc, "Props", f))
    return url


def check(display, data, work, name, props, wantRows):
    url = makeCalc(data, name, props)
    out = os.path.join(work, name)
    result = mo.runBuilder(display, url, out, "motable tab\n", mode="detail")
    log = result.log or ""
    st = mo.readState(out, "tab")
    what = "%s: MOs panel" % name
    if result.crashed:
        mo.fail(what, "builder CRASHED (%s)" % result.signalName, log)
    elif "Orbital energy vector is null" in log:
        mo.fail(what, "the panel threw on the missing energies", log)
    elif wantRows is None:
        if NO_ENERGIES not in log and (st is None or st.get("rows") != "0"):
            mo.fail(what, "no message and not an empty table", log)
        else:
            print("ok    %s: no crash, the panel says the energies are "
                  "missing" % what)
    elif st is None:
        mo.fail(what, "the scene never reported the table", log)
    elif st.get("rows") != str(wantRows) or st.get("occHomo") != "52":
        mo.fail(what, "rows %s, HOMO %s; want %d rows, HOMO 52"
                % (st.get("rows"), st.get("occHomo"), wantRows), log)
    else:
        print("ok    %s: %d MOs, HOMO 52" % (what, wantRows))


def main():
    if not os.path.exists(os.path.join(apps.WRAPPERS, "ecce-builder")):
        print("SKIP: no ecce-builder under %s" % apps.WRAPPERS)
        return SKIP
    if not shutil.which("Xvfb"):
        print("SKIP: no Xvfb")
        return SKIP
    try:
        settings = isolate.apply(apps.INSTALL)
    except isolate.IsolationError as exc:
        print("refusing to run: %s" % exc)
        return 1
    print(isolate.describe(settings))
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ["GTK_THEME"] = "Adwaita"
    work = tempfile.mkdtemp(prefix="ecceqm-mo-",
                            dir=os.environ.get("ECCE_REALUSERHOME"))
    data = os.path.join(work, "data")
    os.makedirs(os.path.join(data, "users", "local"))
    os.environ["ECCE_LOCAL_DATA"] = data

    fresh = os.path.join(work, "props-now")
    gen = subprocess.run([sys.executable, os.path.join(
        ROOT, "tests", "parsers", "propinterp_test.py"), "--props", fresh,
        "--nbf", "156"], capture_output=True, text=True)
    with xdisplay.Display() as display:
        check(display, data, work, "stored-by-9.2",
              os.path.join(FIXTURE, "props-9.2"), None)
        if gen.returncode == 0:
            check(display, data, work, "stored-now", fresh, 156)
        elif gen.returncode == SKIP:
            print("skip  stored-now: no build tree for propinterp")
        else:
            mo.fail("stored-now", "propinterp_test.py failed",
                    gen.stdout + gen.stderr)
    shutil.rmtree(work, ignore_errors=True)
    print("FAILED: " + ", ".join(mo.failures) if mo.failures else "PASSED")
    return 1 if mo.failures else 0


if __name__ == "__main__":
    sys.exit(main())
