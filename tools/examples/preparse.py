#!/usr/bin/env python3
"""Regenerate the pre-parsed copies of the shipped examples.

    tools/examples/preparse.py          (needs the installed tree of this
                                         checkout, as tests/apps/run_tests.py:
                                         ECCE_TEST_HOME=<DESTDIR>/usr/local)

Runs the Organizer's import of each file-output example set with
ECCE_NO_PREPARSED=1, i.e. through the real parsers, in a fresh isolated state,
and stores each resulting calculation as
data/client/examples/<set>/[<topic>/]<example>/stored/<calc>/ with a `stamp` naming
the output it came from (StoredCalcCopy::stampOf).  Run it when an example
or a parser changes; tests/apps/examples_preparsed_test.py fails when a stored
copy no longer agrees with a parse.
"""
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
EXAMPLES = os.path.join(ROOT, "data", "client", "examples")

def setsOnDisk():
    """{set folder: Organizer project path}, as CalcMgr's exampleSets() makes
    it: Examples/<set name with _ for spaces>.  Sets without a stored copy
    (a GROMACS-only one) have no example here."""
    found = {}
    for setId in sorted(os.listdir(EXAMPLES)):
        text = os.path.join(EXAMPLES, setId, "set.txt")
        if os.path.exists(text):
            title = open(text).readline().strip()
            if examplesOf(setId):
                found[setId] = "Examples/" + title.replace(" ", "_")
    return found


def topicsOf(setId):
    """{topic folder: topic project name} from the set's set.txt."""
    topics = {}
    for line in open(os.path.join(EXAMPLES, setId, "set.txt")).read().splitlines()[2:]:
        if "|" in line and not line.startswith("#"):
            folder, name = line.split("|", 1)
            topics[folder] = name.replace(" ", "_")
    return topics
EXTENSIONS = (".qmout", ".eprint", ".out")   # the output of an example folder


def outputOf(folder):
    for ext in EXTENSIONS:
        for name in sorted(os.listdir(folder)):
            if name.endswith(ext):
                return name
    return None


def examplesOf(setId):
    """{example folder relative to the set: output file}; a GROMACS directory
    is never stored."""
    base = os.path.join(EXAMPLES, setId)
    topics = topicsOf(setId) if os.path.exists(
        os.path.join(base, "set.txt")) else {}
    found = {}
    for entry in sorted(os.listdir(base)):
        folder = os.path.join(base, entry)
        if not os.path.isdir(folder):
            continue
        group = ([entry + "/" + e for e in sorted(os.listdir(folder))
                  if os.path.isdir(os.path.join(folder, e))]
                 if entry in topics else [entry])
        for example in group:
            name = os.path.basename(example)
            if name.startswith("gromacs-") or (
                    entry in topics and example == entry):
                continue
            out = outputOf(os.path.join(base, example))
            if out:
                found[example] = out
    return found


FNV = 1469598103934665603


def stamp(path):
    with open(path, "rb") as f:
        data = f.read()
    h = FNV
    for b in data:
        h = ((h ^ b) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return "1 %x %d" % (h, len(data))


def main():
    SETS = setsOnDisk()
    sets = sys.argv[1:] or sorted(SETS)
    arm = os.path.join(ROOT, "tests", "apps", "examples_arm.py")
    env = dict(os.environ, ECCE_NO_PREPARSED="1")
    for setId in sets:
        outputs = examplesOf(setId)
        out = tempfile.mkdtemp(prefix="preparse-",
                               dir=os.path.expanduser("~/.cache"))
        rc = subprocess.run([sys.executable, arm, out, setId, SETS[setId],
                             str(len(outputs))], env=env).returncode
        if rc != 0:
            return rc
        project = os.path.join(out, SETS[setId])
        for example, output in outputs.items():
            calc = os.path.splitext(output)[0]
            topic = topicsOf(setId).get(example.split("/")[0], "")
            if "/" in example:
                parent = os.path.join(project, topic)
            else:
                parent = project
            dest = os.path.join(EXAMPLES, setId, example, "stored")
            shutil.rmtree(dest, ignore_errors=True)
            shutil.copytree(os.path.join(parent, calc),
                            os.path.join(dest, calc), symlinks=True,
                            ignore=shutil.ignore_patterns("Thumbnail*", "*.lock"))
            with open(os.path.join(dest, "stamp"), "w") as f:
                f.write(stamp(os.path.join(EXAMPLES, setId, example, output))
                        + "\n")
            print("stored", setId, example)
        shutil.rmtree(out, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
