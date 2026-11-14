#!/usr/bin/env python3
"""Launch keeps input lines of any length, in the stored deck and the run (#957).

Launch copies the stored deck to its cache line by line, sends that copy to
the run directory and stores it back over the deck.  A fixed 255-byte line
buffer stopped the copy at the first longer line, and the write-back made
the cut permanent.  Two ORCA decks with a 300- and a 5000-character line,
one without a final newline, are launched on localhost through the real
Launch with a stand-in ORCA that keeps the input it was given.  The stored
deck and the code's input must equal the deck byte for byte (the line copy
adds a missing final newline, nothing else).

    tests/launch/long_lines_test.py [--build build-cmake] [--keep]

Exit status 77 (CTest SKIP) without the built binaries.
"""

import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness                      # noqa: E402

failures = []


def check(ok, what):
    harness.say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def deck(final_newline):
    text = ("! HF def2-SVP\n"
            "# " + "a" * 298 + "\n"
            "* xyz 0 1\nH 0.0 0.0 0.0\nH 0.0 0.0 0.74\n*\n"
            "# " + "".join(chr(ord("A") + i % 26) for i in range(4998)) + "\n"
            "# END-957")
    return (text + "\n" if final_newline else text).encode()


# Stands in for ORCA: keeps a copy of the input the job script handed it.
FAKE_ORCA = """#!/bin/sh
cp "$1" received.inp
echo "ORCA TERMINATED NORMALLY"
"""


def storedDeck(root, calc, name, marker):
    for top, _, files in os.walk(root):
        if calc not in os.path.relpath(top, root).split(os.sep):
            continue
        for f in files:
            if f == name:
                path = os.path.join(top, f)
                with open(path, "rb") as h:
                    data = h.read()
                if marker in data:
                    return data
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    build = os.path.abspath(args.build)

    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "launchjob"), os.X_OK):
        harness.skip("launchjob is not built")

    s = harness.Session(build, "longlines", {}, local=True, keep=args.keep)
    fake = os.path.join(s.state, "fake-orca")
    with open(fake, "w") as f:
        f.write(FAKE_ORCA)
    os.chmod(fake, 0o755)
    with open(os.path.join(s.state, ".ECCE", "CONFIG.localhost"), "w") as f:
        f.write("ORCA: %s\n" % fake)
    try:
        if not s.services(True):
            check(False, "services came up")
            return 1
        for name, nl in (("longlines-nl", True), ("longlines-nonl", False)):
            harness.say(name)
            original = deck(nl)
            expected = original if nl else original + b"\n"
            path = os.path.join(s.state, name + ".deck")
            with open(path, "wb") as f:
                f.write(original)
            rundir = os.path.join(s.state, "run-" + name)
            os.makedirs(rundir, exist_ok=True)
            url, out = s.create(name, "orca_es", path, "orca.orcain", rundir)
            if not check(url is not None, "calculation created"):
                harness.say(out)
                continue
            rc, out = s.launch(url)
            if not check(rc == 0, "launched"):
                harness.say(out)
                continue
            state = s.waitState(url, seconds=180)
            harness.say("  final state: %s" % state)
            got = storedDeck(s.localData(), name, "orca.orcain",
                             b"! HF def2-SVP")
            check(got is not None, "stored deck found")
            if got is not None:
                check(got == expected, "stored deck equals the deck (%d of %d "
                      "bytes)" % (len(got), len(expected)))
            sent = os.path.join(rundir, "received.inp")
            if check(os.path.exists(sent), "the code received an input"):
                with open(sent, "rb") as h:
                    data = h.read()
                check(data == expected, "the code's input equals the deck (%d "
                      "of %d bytes)" % (len(data), len(expected)))
    finally:
        left = s.stop()
        if left:
            failures.append("processes left running: %r" % left)
    harness.say("FAILED (%d): %s" % (len(failures), "; ".join(failures))
                if failures else "PASSED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
