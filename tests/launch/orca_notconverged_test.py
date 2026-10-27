#!/usr/bin/env python3
"""An ORCA optimisation that stops at its cycle limit ends Unsuccessful (#614).

ORCA ends normally after "The optimization did not converge but reached the
maximum number of optimization cycles"; its structure and energy must not
pass as a result.  A distorted water, HF/def2-SVP, MaxIter 2, through the
real Launch, gensub, eccejobmonitor and eccejobstore: the state is
unsuccessful and the reason names the unconverged optimisation.

    tests/launch/orca_notconverged_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without ORCA 6 or the built binaries.
"""

import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness                      # noqa: E402
from orca_guess_test import check, failures, findOrca, say   # noqa: E402

DECK = """! HF def2-SVP Opt
%geom MaxIter 2 end
* xyz 0 1
O   0.000   0.000   0.300
H   0.000   0.950  -0.600
H   0.000  -0.600  -0.700
*
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    orca = findOrca()
    if not orca:
        harness.skip("ORCA 6 is not installed under /opt/orca")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "launchjob"), os.X_OK):
        harness.skip("launchjob is not built")

    s = harness.Session(build, "orcanotconv", {"ORCA": orca}, local=True,
                        keep=args.keep)
    rundir = os.path.join(s.state, "run")
    os.makedirs(rundir, exist_ok=True)
    deck = os.path.join(s.state, "deck")
    with open(deck, "w") as f:
        f.write(DECK)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        url, out = s.create("water-maxiter", "orca_es", deck, "orca.orcain", rundir)
        if not check(url is not None, "calculation created"):
            say(out)
            return 1
        rc, out = s.launch(url)
        if not check(rc == 0, "launched"):
            say(out)
            return 1
        state = s.waitState(url, seconds=300)
        check(state == "unsuccessful",
              "an optimisation at its cycle limit ends unsuccessful (last: %s)" % state)
        rc, why = s.driver("reason", url)
        check("did not converge" in why,
              "the reason names the unconverged optimisation (%s)" % why.strip())
    finally:
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)
    say("FAILED (%d): %s" % (len(failures), "; ".join(failures)) if failures
        else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
