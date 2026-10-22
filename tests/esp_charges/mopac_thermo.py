#!/usr/bin/env python3
"""MOPAC FORCE decks carry THERMO unless the calc stored 0, and the
CalcEd runtype summary no longer lists Gradient Norm / Thermochemistry."""
import os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
import mopac_deck

ROOT = mopac_deck.ROOT


def deck(runtype, extra):
    # mopac_deck.deck fixes RunType: Energy, so patch it per call.
    orig = mopac_deck.deck
    return orig(["RunType: " + runtype] + extra)


def main():
    fails = 0
    edml = open(os.path.join(ROOT, "data/client/cap/MOPAC.edml")).read()
    for gone in ("Gradient Norm", "Thermochemistry"):
        ok = gone not in edml
        print(("ok   " if ok else "FAIL ") + "summary lacks " + gone)
        fails += not ok
    ok = 'label="Max. Steps"' in edml
    print(("ok   " if ok else "FAIL ") + "summary keeps Max. Steps")
    fails += not ok
    for rt, extra, want in (("Vibration", [], True), ("GeoVib", [], True),
                            ("GeoVib", ["ES.Runtype.Vibration.Thermo: 1"], True),
                            ("GeoVib", ["ES.Runtype.Vibration.Thermo: 0"], False),
                            ("Energy", [], False)):
        text = deck(rt, extra)
        got = bool(re.search(r"\bTHERMO\b", text))
        ok = got == want
        print(("ok   " if ok else "FAIL ") + "%s %s THERMO=%s" % (rt, extra, got))
        fails += not ok
    return fails


if __name__ == "__main__":
    sys.exit(1 if main() else 0)
