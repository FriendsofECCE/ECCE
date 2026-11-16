#!/usr/bin/env python3
"""Organizer file viewer: bounded tail, Load more, Find, Follow and binary."""
import os
from pathlib import Path
import re
import shutil
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import apps
import run_tests

failures = []


def check(ok, name, detail=""):
    print(("PASS " if ok else "FAIL ") + name + (" " + detail if not ok else ""))
    if not ok:
        failures.append(name)


def run(display):
    with tempfile.TemporaryDirectory(prefix="ecce-file-view-") as scratch:
        work = Path(scratch)
        data = work / "data"
        project = data / "users/local/files"
        project.mkdir(parents=True)
        shutil.copy(HERE.parents[1] / "tools/screenshots/data/project.ecce-meta", project / ".ecce-meta")
        log = project / "run.log"
        block = b"an output line for range testing\n"
        body = (block * (10 * 1024 * 1024 // len(block))) + b"LAST OUTPUT LINE\n"
        log.write_bytes(body)
        binary = project / "orbitals.gbw"
        binary.write_bytes(b"ORCA\0binary\r\n" + b"x" * 10000)
        commands, answers = work / "commands", work / "answers"
        commands.write_text("")
        url = log.as_uri()
        seen = []

        def command(line):
            with commands.open("a") as f:
                f.write(line + "\n")
            deadline = time.monotonic() + 40
            while time.monotonic() < deadline:
                rows = answers.read_text().splitlines() if answers.exists() else []
                if len(rows) > len(seen):
                    seen.append(rows[len(seen)])
                    return seen[-1]
                time.sleep(0.1)
            raise RuntimeError("no Organizer reply to " + line)

        def inspect(_display):
            initial = command("view " + url)
            check("binary=0" in initial and "bytes=65536" in initial and "read=73728" in initial and "readonly=1" in initial,
                  "10 MiB opens with only probe plus tail", initial)
            shot = work / "shown.txt"
            command("view-text " + str(shot))
            shown = shot.read_bytes()
            check(shown == body[-65536:], "shown text ends with the last line",
                  repr((len(shown), shown[:40], shown[-40:])))
            check("found ok" in command("view-find LAST OUTPUT LINE"), "Find searches the loaded tail")
            more = command("view-more")
            check("bytes=131072" in more and "follow=0" in more, "Load more prepends one chunk", more)
            check("follow=1" in command("view-follow on"), "Follow starts its timer")
            with log.open("ab") as f:
                f.write(b"APPENDED BY RUNNING JOB\n")
            time.sleep(4)
            command("view-text " + str(shot))
            check(shot.read_bytes().endswith(b"APPENDED BY RUNNING JOB\n"), "Follow picks up appended output")
            command("view-follow off")
            check("follow=1" in command("view-follow on"), "Follow can restart")
            log.write_bytes(b"OUTPUT RESTARTED\n")
            time.sleep(4)
            command("view-text " + str(shot))
            shown = shot.read_bytes()
            check(shown == b"OUTPUT RESTARTED\n", "Follow handles file truncation", repr(shown))
            command("view-follow off")
            command("view-close")
            menu = command("menulabels " + url)
            check(re.search(r"(?:\| |: )View(?: \||$)", menu) is not None, "file right-click menu offers View", menu)
            info = command("view " + binary.as_uri())
            check("binary=1" in info and "bytes=0" in info and "read=8192" in info,
                  "binary shows metadata after only the probe", info)
            check("follow=0" in command("view-follow on"), "binary cannot Follow")
            command("view-text " + str(shot))
            check(shot.read_bytes() == b"", "binary contents are not displayed")
            command("view-close")
            empty = project / "empty.log"
            empty.touch()
            info = command("view " + empty.as_uri())
            check("ok size=0" in info and "bytes=0" in info, "empty file displays without a range request", info)
            command("view-close")

        result = apps.run(display, "organizer", windowTimeout=60, settle=3,
                          env={"ECCE_TEST_ORGANIZER": str(commands),
                               "ECCE_TEST_ORGANIZER_ANSWERS": str(answers),
                               "ECCE_ORGANIZER_OPEN": project.as_uri(),
                               "ECCE_LOCAL_DATA": str(data)}, inspect=inspect)
        check(not result.crashed, "Organizer exits without a crash", (result.log or "")[-1500:])


def main():
    def check_app(display, name, results, verbose=False):
        run(display)
        if failures:
            results.fail("file viewer", ", ".join(failures))
    run_tests.checkApp = check_app
    sys.argv = [sys.argv[0], "--app", "organizer"] + sys.argv[1:]
    return run_tests.main()


if __name__ == "__main__":
    sys.exit(main())
