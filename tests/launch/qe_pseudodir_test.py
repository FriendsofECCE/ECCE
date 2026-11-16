#!/usr/bin/env python3
"""QE: the run machine's pseudopotential directory goes into pseudo_dir at launch (#989).

Part 1 runs qe.launchpp as Launch does: without pseudoDir the deck is byte
for byte what ai.qe wrote; with it only the pseudo_dir line changes.
Part 2 launches through launchjob on localhost with QuantumESPRESSOPseudoDir
in CONFIG.localhost pointing at a folder holding only some of the files:
the launch warning names the missing ones, and the stored deck carries the
machine's directory.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(REPO, "tests", "apps"))

import harness  # noqa: E402
from harness import say  # noqa: E402
import qe_guess_test  # noqa: E402

PARSERS = os.path.join(REPO, "scripts", "parsers")
failures = []


def check(ok, what):
    say("  %s %s" % ("ok  " if ok else "FAIL", what))
    if not ok:
        failures.append(what)
    return ok


def upfNames(text):
    m = re.search(r"ATOMIC_SPECIES\n((?:\s+\S+\s+\S+\s+\S+\n)+)", text)
    return [line.split()[2] for line in m.group(1).splitlines()] if m else []


def launchpp(folder, deck, params):
    with open(os.path.join(folder, "postParams"), "w") as f:
        f.write("inputFile: %s\n" % os.path.basename(deck))
        f.write("".join("%s: %s\n" % kv for kv in params.items()))
    subprocess.run(["perl", os.path.join(PARSERS, "qe.launchpp"), "-p", "postParams"],
                   cwd=folder, check=True)
    return open(deck).read()


def part1(work):
    say("qe.launchpp")
    deck = qe_guess_test.makeDeck(work, "pp", False)
    folder = os.path.dirname(deck)
    original = open(deck).read()
    names = upfNames(original)
    check(len(names) == 2, "ai.qe named two UPF files (%s)" % names)

    after = launchpp(folder, deck, {"numProcs": 1, "machineName": "localhost"})
    check(after == original, "no scratch, no pseudoDir: deck unchanged")

    after = launchpp(folder, deck, {"numProcs": 1, "machineName": "localhost",
                                    "pseudoDir": "/cluster/pseudo//"})
    check("  pseudo_dir = '/cluster/pseudo/'\n" in after, "pseudo_dir is the machine's")
    check(upfNames(after) == names, "the UPF names are unchanged")
    diff = [(a, b) for a, b in zip(original.splitlines(), after.splitlines()) if a != b]
    check(len(diff) == 1 and "pseudo_dir" in diff[0][0], "only the pseudo_dir line changed")
    again = launchpp(folder, deck, {"pseudoDir": "/cluster/pseudo"})
    check(again == after, "a relaunch gives the same deck")
    return names


def part2(build, work, names, keep):
    say("launch on localhost")
    harness.prerequisites(build, ())
    if not os.access(os.path.join(build, "launchjob"), os.X_OK):
        harness.skip("launchjob is not built")
    s = harness.Session(build, "qepseudo", {"QuantumESPRESSO": "/bin/true"},
                        local=True, keep=keep)
    try:
        if not s.services(True):
            check(False, "services came up")
            return
        remote = os.path.join(s.state, "cluster pseudo")
        os.makedirs(remote, exist_ok=True)
        open(os.path.join(remote, names[0]), "w").close()
        with open(os.path.join(s.state, ".ECCE", "CONFIG.localhost"), "a") as h:
            h.write("QuantumESPRESSOPseudoDir: %s\n" % remote)
        rundir = os.path.join(s.state, "run")
        os.makedirs(rundir, exist_ok=True)
        deck = qe_guess_test.makeDeck(work, "launch", False)
        url = qe_guess_test.runJob(s, "water-pseudo", deck, rundir)
        if not url:
            return
        rc, out = s.driver("launch", url, extra={"LAUNCHJOB_WARNING": "cancel"})
        check("launch warning:" in out, "launch warned")
        check(names[1] in out and names[0] not in out.split("launch warning:")[-1],
              "the warning names only the missing file %s" % names[1])
        check("launch cancelled" in out, "Cancel stopped the launch")
        if failures:
            say(out)
        calc = qe_guess_test.calcDir(s, url)
        stored = [os.path.join(d, f) for d, _, fs in os.walk(calc)
                  for f in fs if f == "qe.pwin"]
        text = open(stored[0]).read() if stored else ""
        check("pseudo_dir = '%s/'" % remote in text, "the stored deck has the machine's pseudo_dir")
        check(upfNames(text) == names, "and the same UPF names")

        # Directory missing on the machine: one sentence, still only a warning.
        with open(os.path.join(s.state, ".ECCE", "CONFIG.localhost"), "a") as h:
            h.write("QuantumESPRESSOPseudoDir: /cluster/pseudo\n")
        rc, out = s.driver("launch", url, extra={"LAUNCHJOB_WARNING": "cancel"})
        check("does not exist there" in out, "a missing directory is reported")
    finally:
        s.stop()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--build", default=os.path.join(REPO, "build-cmake"))
    ap.add_argument("--keep", action="store_true")
    args = ap.parse_args()
    work = os.path.join(os.path.abspath(args.build), "qe-pseudodir-work")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    try:
        names = part1(work)
        if len(names) == 2:
            part2(os.path.abspath(args.build), work, names, args.keep)
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)
    say("%d failure(s)" % len(failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
