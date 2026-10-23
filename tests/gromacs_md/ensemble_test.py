#!/usr/bin/env python3
"""NVE, NVT and NPT (#478): the model flags the Dynamics page's Ensemble row
sets (UseNVT, UseNPT; NPT is both) through md.gmxtask, checked on the .mdp:
tcoupl and pcoupl lines, and that pressure coupling never comes without a
thermostat.

    tests/gromacs_md/ensemble_test.py          needs perl only
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import run_tests as rt  # noqa: E402
import run_settings_test as rs  # noqa: E402

ENSEMBLES = (("NVE", 0, 0), ("NVT", 1, 0), ("NPT", 1, 1))


def main():
    for name, nvt, npt in ENSEMBLES:
        dyn = (rt.dyn(500, True, npt=bool(npt))
               .replace("<UseNVT> 1", "<UseNVT> %d" % nvt))
        ok, mdp = rs.mdp_of(rs.EQ, dynamics=dyn)
        k = rs.keys(mdp)
        rt.check(ok == 0 or ok is True or mdp, "%s: md.gmxtask ran" % name)
        has_t = "tcoupl" in k and k["tcoupl"][0].lower() not in ("no", "none")
        has_p = "pcoupl" in k and k["pcoupl"][0].lower() not in ("no", "none")
        rt.check(has_t == bool(nvt), "%s: tcoupl %s (%s)" % (name, "set" if nvt else "absent", k.get("tcoupl")))
        rt.check(has_p == bool(npt), "%s: pcoupl %s (%s)" % (name, "set" if npt else "absent", k.get("pcoupl")))
        if npt:
            rt.check(has_t, "NPT keeps a thermostat")
    return 1 if rt.failures else 0


if __name__ == "__main__":
    rt.verbose = True
    sys.exit(main())
