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
import gzip
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(
    os.path.dirname(os.path.abspath(__file__)))), "tests", "apps"))
from examples_preparsed_test import h5content

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
EXTENSIONS = (".qmout", ".eprint", ".pwout", ".out")   # the output of an example folder


def outputOf(folder):
    """The output file, also gzip-compressed (<name>.gz)."""
    for ext in EXTENSIONS:
        for name in sorted(os.listdir(folder)):
            plain = name[:-3] if name.endswith(".gz") else name
            if plain.endswith(ext) and plain != "g98.out":   # xtb companion
                return name
    return None


def gzipStoredOutput(calcDir, name):
    """A compressed example keeps its stored copy of the output compressed
    too (StoredCalcCopy writes X.gz as X); mtime 0 keeps the bytes stable."""
    for base, _dirs, files in os.walk(calcDir):
        if name in files:
            path = os.path.join(base, name)
            with open(path, "rb") as src, open(path + ".gz", "wb") as raw:
                with gzip.GzipFile(filename="", mode="wb", fileobj=raw,
                                   compresslevel=9, mtime=0) as dst:
                    shutil.copyfileobj(src, dst)
            os.remove(path)


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
    # PREPARSE_SETS: for runners that cannot pass arguments (run-offload apptest)
    sets = sys.argv[1:] or os.environ.get("PREPARSE_SETS", "").split() or sorted(SETS)
    arm = os.path.join(ROOT, "tests", "apps", "examples_arm.py")
    env = dict(os.environ, ECCE_NO_PREPARSED="1")
    for setId in sets:
        outputs = examplesOf(setId)
        cache = os.path.expanduser("~/.cache")
        os.makedirs(cache, exist_ok=True)
        out = tempfile.mkdtemp(prefix="preparse-", dir=cache)
        rc = subprocess.run([sys.executable, arm, out, setId, SETS[setId],
                             str(len(outputs))], env=env).returncode
        if rc != 0:
            return rc
        project = os.path.join(out, SETS[setId])
        for example, output in outputs.items():
            plain = output[:-3] if output.endswith(".gz") else output
            calc = os.path.splitext(plain)[0]
            topic = topicsOf(setId).get(example.split("/")[0], "")
            if "/" in example:
                parent = os.path.join(project, topic)
            else:
                parent = project
            dest = os.path.join(EXAMPLES, setId, example, "stored")
            oldH5 = os.path.join(dest, calc, "Outputs", "results.h5")
            kept = open(oldH5, "rb").read() if os.path.exists(oldH5) else None
            shutil.rmtree(dest, ignore_errors=True)
            shutil.copytree(os.path.join(parent, calc),
                            os.path.join(dest, calc), symlinks=True,
                            ignore=shutil.ignore_patterns("Thumbnail*", "*.lock"))
            if output.endswith(".gz"):
                gzipStoredOutput(os.path.join(dest, calc), calc + os.path.splitext(plain)[1])
            # HDF5 lays out equal content differently from run to run; keep
            # the old bytes when the content is the same, so git sees no change
            if kept is not None and os.path.exists(oldH5):
                fresh = oldH5 + ".fresh"
                os.rename(oldH5, fresh)
                with open(oldH5, "wb") as f:
                    f.write(kept)
                if h5content(oldH5) == h5content(fresh):
                    os.remove(fresh)
                else:
                    os.replace(fresh, oldH5)
            with open(os.path.join(dest, "stamp"), "w") as f:
                f.write(stamp(os.path.join(EXAMPLES, setId, example, output))
                        + "\n")
            print("stored", setId, example)
        shutil.rmtree(out, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
