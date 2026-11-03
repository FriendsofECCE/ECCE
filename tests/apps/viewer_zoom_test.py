#!/usr/bin/env python3
"""Synthetic trackpad bursts through the viewer's wx event route, under Xvfb."""
import argparse
import math
import os
from pathlib import Path
import subprocess
import sys
import tempfile

import xdisplay

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", nargs="?", default=str(ROOT / "build-cmake/viewer-scenes"))
    parser.add_argument("--baseline", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    binary = Path(args.binary).resolve()
    if not os.access(binary, os.X_OK):
        print("SKIP  viewer-scenes is not built")
        return 77
    try:
        xdisplay.findXvfb()
    except xdisplay.DisplayUnavailable as error:
        print("SKIP  " + str(error))
        return 77
    with tempfile.TemporaryDirectory(prefix="viewer-zoom-", dir=ROOT / "build-cmake") as temporary:
        work = args.output or Path(temporary)
        work.mkdir(parents=True, exist_ok=True)
        home = Path(temporary) / "home"
        home.mkdir()
        script = work / "zoom.scene"
        cases = {"vertical": ("8 15 vertical", 2 ** (1 / 8), 1),
                 "whole_steps": ("8 120 vertical", 2, 1),
                 "horizontal": ("8 15 horizontal", 1, 0),
                 "pinch": ("8 1.5 pinch", 1 / 1.5, 1),
                 "pinch_again": ("8 0.75 pinch", 1 / 0.75, 1),
                 "plus": ("1 + key", 2 ** (-1 / 8), 1),
                 "minus": ("1 - key", 2 ** (1 / 8), 1),
                 "keypad_plus": ("1 add key", 2 ** (-1 / 8), 1),
                 "keypad_minus": ("1 subtract key", 2 ** (1 / 8), 1),
                 "text_focus": ("1 + textkey", 1, 0)}
        if args.baseline:
            cases = {key: cases[key] for key in ("vertical", "horizontal")}
        command = "wheelburst" if args.baseline else "zoomburst"
        script.write_text("viewall\n" + "".join(
            f"{command} {name} {parameters}\n"
            for name, (parameters, _, _) in cases.items()))
        with xdisplay.Display() as display:
            env = dict(os.environ, HOME=str(home), ECCE_REALUSERHOME=str(home),
                       ECCE_HOME=str(ROOT), LIBGL_ALWAYS_SOFTWARE="1",
                       DISPLAY=":" + str(display.number), GDK_BACKEND="x11",
                       XDG_CACHE_HOME=str(Path(temporary) / "cache"))
            env.pop("WAYLAND_DISPLAY", None)
            result = subprocess.run([str(binary), str(work), str(script), "waterbox"],
                                    env=env, capture_output=True, text=True, timeout=180)
        if result.returncode:
            print("FAIL  viewer scene:\n" + (result.stdout + result.stderr)[-4000:])
            return 1
        problems = []
        for axis, (_, expected_ratio, expected_changes) in cases.items():
            text = (work / (axis + ".txt")).read_text()
            print(axis + ":\n" + text.rstrip())
            if args.baseline:
                continue
            values = dict(line.split() for line in text.splitlines()
                          if len(line.split()) == 2)
            if int(values["immediate_changes"]) != 0:
                problems.append(axis + " changed the camera during the input burst")
            if int(values["changes"]) != expected_changes:
                problems.append(axis + " camera changes: " + values["changes"])
            if not math.isclose(float(values["ratio"]), expected_ratio, rel_tol=2e-6):
                problems.append(axis + " camera ratio: " + values["ratio"])
        for problem in problems:
            print("FAIL  " + problem)
        if not problems:
            print("PASS  viewer wheel timing" if args.baseline else "PASS  viewer wheel accumulation")
        return bool(problems)


if __name__ == "__main__":
    sys.exit(main())
