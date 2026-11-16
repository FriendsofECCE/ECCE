#!/usr/bin/env python3
"""Import as Calculation on a listed file, local folder (headless, #1092).

The ECCE_TEST_ORGANIZER command `importfile <file> <project-url>` is what the
right-click item runs after its project choice.  A stored ORCA output must
become a calculation in the project, with its final energy stored and the
file's origin in the annotation; a file no importer recognises must be refused
and leave nothing behind.

Same installed tree, isolation and services as run_tests.py (ECCE_TEST_HOME,
ECCE_TEST_WRAPPERS).  Exit 77 (skip) without an install or Xvfb.
"""

import os
import shutil
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import apps       # noqa: E402
import run_tests  # noqa: E402
import progress_box_test as pb  # noqa: E402

FIXTURES = os.path.join(HERE, "..", "parsers", "fixtures")
ORCA = os.path.abspath(os.path.join(FIXTURES, "orca", "h2o_sym.out"))
ENERGY = b"75.96098"
failures = []


def tree_has(root, needle):
    for base, _dirs, files in os.walk(root):
        for name in files:
            try:
                with open(os.path.join(base, name), "rb") as handle:
                    if needle in handle.read():
                        return True
            except OSError:
                pass
    return False


def run(display):
    data = tempfile.mkdtemp(prefix="ecce-impfile-data-")
    work = tempfile.mkdtemp(prefix="ecce-impfile-")
    try:
        project = os.path.join(data, "users", "local", "imp-test")
        os.makedirs(project)
        shutil.copy(pb.PROJECT_META, os.path.join(project, ".ecce-meta"))
        folder = os.path.join(work, "outputs")
        os.makedirs(folder)
        good = os.path.join(folder, "water.out")
        shutil.copy(ORCA, good)
        other = os.path.join(folder, "notes.txt")
        with open(other, "w") as handle:
            handle.write("nothing a code wrote\n")
        url = "file://" + project
        cmds = os.path.join(work, "commands")
        with open(cmds, "w") as handle:
            handle.write("importfile %s %s\nimportfile %s %s\n"
                         % (good, url, other, url))
        answers = os.path.join(work, "answers")
        state = {}

        def wait(_display):
            end = time.time() + 240
            while time.time() < end:
                text = ""
                if os.path.exists(answers):
                    with open(answers) as handle:
                        text = handle.read()
                state["answers"] = text
                if text.count("importfile") >= 2 and \
                        tree_has(project, ENERGY) and \
                        tree_has(project, b"Imported from " + good.encode()):
                    return
                time.sleep(1)

        apps.run(display, "organizer", windowTimeout=60, settle=2,
                 inspect=wait,
                 env={"ECCE_TEST_ORGANIZER": cmds,
                      "ECCE_TEST_ORGANIZER_ANSWERS": answers,
                      "ECCE_LOCAL_DATA": data})
        lines = state.get("answers", "").splitlines()
        first = [l for l in lines if l.startswith("importfile %s " % good)]
        second = [l for l in lines if l.startswith("importfile %s " % other)]
        if not first or not first[0].endswith(": done"):
            failures.append("import of the ORCA output: %r" % first)
        if not tree_has(project, ENERGY):
            failures.append("no final energy stored in the calculation")
        if not tree_has(project, b"Imported from " + good.encode()):
            failures.append("origin missing from the annotation")
        if not second or second[0].endswith(": done"):
            failures.append("unrecognised file was not refused: %r" % second)
        for l in first + second:
            print("      " + l)
        if not failures:
            print("ok    import as calculation: stored, annotated, refusal")
    finally:
        shutil.rmtree(data, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)


def main():
    def checkApp(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("import file", "; ".join(failures))
    run_tests.checkApp = checkApp
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
