#!/usr/bin/env python3
"""Contact sheets from tests/plots/capture.py output.

    tests/plots/sheet.py DIR

Reads DIR/{before,after}-<case>-<panel>-{large,small}.png and writes one
DIR/sheet-<panel>.png per panel (a row per case: before and after at both
sizes) plus DIR/sheet.png with every panel's first case.
"""
import glob
import os
import sys

from PIL import Image, ImageDraw

SCALE = 0.5
CASES = ("g16-h2o-optfreq", "orca-h2o-opt", "nwchem-h2o-opt", "orca-h2o-sym",
         "synthetic")


def load(path):
    if not os.path.exists(path):
        return None
    img = Image.open(path).convert("RGB")
    return img.resize((int(img.width * SCALE), int(img.height * SCALE)),
                      Image.LANCZOS)


def row(directory, case, panel):
    cells = []
    for size in ("large", "small"):
        for tag in ("before", "after"):
            img = load(os.path.join(directory, "%s-%s-%s-%s.png"
                                    % (tag, case, panel, size)))
            cells.append((tag + " " + size, img))
    return cells


def sheet(rows, out):
    pad, head = 6, 16
    widths = [0] * 4
    heights = []
    for case, cells in rows:
        h = 0
        for i, (_label, img) in enumerate(cells):
            if img is not None:
                widths[i] = max(widths[i], img.width)
                h = max(h, img.height)
        heights.append(h + 2 * head + pad)
    canvas = Image.new("RGB", (sum(widths) + pad * 5,
                               sum(heights) + head * len(rows) + pad), "white")
    draw = ImageDraw.Draw(canvas)
    y = pad
    for (case, cells), h in zip(rows, heights):
        draw.text((pad, y), case, fill="black")
        y += head
        x = pad
        for i, (label, img) in enumerate(cells):
            draw.text((x, y - 2), label, fill=(90, 90, 90))
            if img is not None:
                canvas.paste(img, (x, y + 10))
            x += widths[i] + pad
        y += h - head
    canvas.save(out)


def main():
    directory = sys.argv[1]
    panels = {}
    for path in glob.glob(os.path.join(directory, "after-*-large.png")):
        base = os.path.basename(path)[len("after-"):-len("-large.png")]
        for case in CASES:
            if base.startswith(case + "-"):
                panels.setdefault(base[len(case) + 1:], []).append(case)
    overview = []
    for panel, cases in sorted(panels.items()):
        rows = [(c + "  " + panel, row(directory, c, panel))
                for c in sorted(cases)]
        sheet(rows, os.path.join(directory, "sheet-%s.png" % panel))
        overview.append(rows[0])
    sheet(overview, os.path.join(directory, "sheet.png"))
    print("%d sheets in %s" % (len(panels) + 1, directory))


if __name__ == "__main__":
    main()
