#!/usr/bin/env python3
"""Missing Organizer thumbnails (#1238), through an isolated gateway session.

Check periodic structures without formula metadata, damaged JPEGs and empty
calculations. A displayed-image log proves the completion notice reached the
selected pane without a second selection. No screenshots are taken.
"""
import os
from pathlib import Path
import shutil
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import apps
import isolate
import xdisplay
from PIL import Image

ROOT = HERE.parent.parent


def main():
    if not Path(apps.WRAPPERS, "ecce").exists() or not shutil.which("Xvfb"):
        print("SKIP: installed ECCE and Xvfb required")
        return 77
    settings = isolate.apply(apps.INSTALL)
    print(isolate.describe(settings), flush=True)
    os.environ["ECCE_NO_REAP"] = "1"
    os.environ.pop("DISPLAY", None)
    failures = []

    def check(ok, what):
        print(("PASS: " if ok else "FAIL: ") + what, flush=True)
        if not ok:
            failures.append(what)

    with tempfile.TemporaryDirectory(prefix="thumbnail-", dir=ROOT) as work:
        work = Path(work)
        project = work / "data/users/local/p"
        project.mkdir(parents=True)
        shutil.copy(ROOT / "tools/screenshots/data/project.ecce-meta", project / ".ecce-meta")
        diamond = ROOT / "data/client/examples/genchem/molecular-and-covalent-solids/qe-diamond/stored/diamond"
        water = HERE / "fixtures/calc-water-opt"
        for name, source in (("diamond", diamond), ("damaged", water), ("empty", water)):
            shutil.copytree(source, project / name)
            parameters = project / name / "Parameters"
            thumb = parameters / "Thumbnail.jpeg"
            thumb.unlink(missing_ok=True)
            if name == "diamond":
                meta = parameters / ".ecce-meta"
                meta.write_text("".join(line for line in meta.read_text().splitlines(True)
                                        if "empiricalFormula" not in line))
            elif name == "damaged":
                thumb.write_bytes(b"not a JPEG")
            else:
                (parameters / "chemsys.mvm").unlink()
        commands = work / "commands"
        commands.write_text("")

        def drive(_display):
            for name in ("diamond", "damaged", "empty"):
                with commands.open("a") as handle:
                    handle.write("select file://%s\n" % (project / name))
                thumb = project / name / "Parameters/Thumbnail.jpeg"
                if name == "empty":
                    time.sleep(3)
                    continue
                end = time.monotonic() + 60
                while time.monotonic() < end:
                    try:
                        with Image.open(thumb) as im:
                            im.load()
                            if im.format == "JPEG":
                                break
                    except (OSError, ValueError):
                        pass
                    time.sleep(0.5)
                # Allow the stored-image notice to refresh this selection.
                time.sleep(3)

        env = {"ECCE_TEST_ORGANIZER": str(commands),
               "ECCE_LOCAL_DATA": str(work / "data"),
               "ECCE_TIMING": "1", "ECCE_NO_PROGRESS_BACKFILL": "1",
               "ECCE_TEST_ORGANIZER_NOLAUNCH": "1"}
        with xdisplay.Display() as display:
            result = apps.run(display, "gateway", windowTimeout=60, settle=10,
                              env=env, inspect=drive)
        log = result.log or ""
        check(result.sawWindow and not result.crashed, "session opened without a crash")
        for name in ("diamond", "damaged"):
            url = "file://" + str(project / name)
            check("Organizer: thumbnail requested " + url in log,
                  name + ": requested from the stored structure")
            check("Organizer: thumbnail displayed " + url in log,
                  name + ": selected button updated without reselecting")
            try:
                with Image.open(project / name / "Parameters/Thumbnail.jpeg") as im:
                    im.load()
                    extrema = im.convert("RGB").getextrema()
                    check(im.format == "JPEG" and im.size == (64, 64)
                          and any(hi - lo > 50 for lo, hi in extrema),
                          name + ": stored JPEG contains a rendered structure")
            except OSError:
                check(False, name + ": stored JPEG exists")
        empty = "file://" + str(project / "empty")
        check("Organizer: no structure for thumbnail " + empty in log
              and "Organizer: thumbnail requested " + empty not in log
              and not (project / "empty/Parameters/Thumbnail.jpeg").exists(),
              "no structure: retains generic icon without rendering")
        if failures:
            print(log[-18000:])
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
