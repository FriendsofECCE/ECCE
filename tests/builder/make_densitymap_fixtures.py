#!/usr/bin/env python3
"""Local-data-mode copies of the modiagram fixtures the Density Map tests need
(#215): n2 and ethene (benzene is the mocomp fixture), with .ecce-meta sidecars in place of the
data-server .davprops.  Run from the repository root."""
import json
import os
import re
import shutil

SRC = "tests/modiagram/fixtures"
DST = "tests/builder/densitymap-fixtures"
WATER = "tests/mocomp/fixtures/water"
FIXTURES = {"n2": "ecceqm-n2", "ethene": "ecceqm-c2h4"}

for name, src in FIXTURES.items():
    out = os.path.join(DST, name)
    shutil.rmtree(out, ignore_errors=True)
    for part in ("Parameters", "Props"):
        shutil.copytree(os.path.join(SRC, src, part), os.path.join(out, part),
                        ignore=shutil.ignore_patterns(".DAV", "SetupParams"))
    dav = os.path.join(SRC, src, ".davprops")
    props = json.load(open(dav)) if os.path.exists(dav) else {"": {"application": "ORCA"}}
    app = props[""]["application"]
    basis = props.get("Parameters/BasisSet.ecce_basisset", {})
    top = open(os.path.join(WATER, ".ecce-meta")).read()
    top = re.sub(r"(ecce:application\t\t).*", lambda m: m.group(1) + app, top)
    open(os.path.join(out, ".ecce-meta"), "w").write(top)
    par = open(os.path.join(WATER, "Parameters/.ecce-meta")).read()
    for key in ("name", "numFunctions", "numPrimitives", "coordsys", "gbs-type"):
        if key in basis:
            par = re.sub(r"(ecce:%s\t\t).*" % key, lambda m: m.group(1) + basis[key], par)
    open(os.path.join(out, "Parameters/.ecce-meta"), "w").write(par)
print("ok")
