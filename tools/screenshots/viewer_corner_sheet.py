#!/usr/bin/env python3
"""Sheet of the viewer-corner icon candidates at 16, 32 and 48 px, each next
to the old raster icon scaled to the same size (what a 200 % screen shows).

    DISPLAY=:94 tools/screenshots/viewer_corner_sheet.py OUT.png

SVGs go through wx (NanoSVG, as in the app); PIL composes the sheet.
"""
import os
import sys

import wx
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PIX = os.path.join(ROOT, "data/client/pixmaps")
CAND = os.path.join(PIX, "svg/candidates/viewer-corner")
PAIRS = [("center-view", "center_view16.png"), ("center-view-lock", "center_view_lock16.png"),
         ("center-view-unlock", "center_view_unlock16.png"), ("home-view", "home_view16.png"),
         ("set-home-view", "set_home_view16.png"), ("zoom-fit", "viewmagfit.png"),
         ("zoom-in", "viewmag+.png"), ("zoom-out", "viewmag-.png")]
SIZES = (16, 32, 48)
BG = (236, 236, 236)


def render(svg, s):
    b = wx.BitmapBundle.FromSVGFile(svg, wx.Size(s, s)).GetBitmap(wx.Size(s, s))
    img = b.ConvertToImage()
    w, h = img.GetWidth(), img.GetHeight()
    out = Image.frombytes("RGB", (w, h), bytes(img.GetData())).convert("RGBA")
    if img.HasAlpha():
        out.putalpha(Image.frombytes("L", (w, h), bytes(img.GetAlpha())))
    return out


def main():
    wx.App()
    cell = 56
    cols = 1 + 2 * len(SIZES)
    sheet = Image.new("RGB", (150 + cols * cell, 24 + len(PAIRS) * cell), BG)
    d = ImageDraw.Draw(sheet)
    for j, s in enumerate(SIZES):
        d.text((150 + (2 * j) * cell + 4, 6), "old %d" % s, fill=(0, 0, 0))
        d.text((150 + (2 * j + 1) * cell + 4, 6), "new %d" % s, fill=(0, 0, 0))
    for i, (name, old) in enumerate(PAIRS):
        y = 24 + i * cell
        d.text((6, y + cell // 2 - 5), name, fill=(0, 0, 0))
        o = Image.open(os.path.join(PIX, old)).convert("RGBA")
        for j, s in enumerate(SIZES):
            for k, im in enumerate((o.resize((s, s), Image.BICUBIC),
                                    render(os.path.join(CAND, name + ".svg"), s))):
                x = 150 + (2 * j + k) * cell + (cell - s) // 2
                sheet.paste(im, (x, y + (cell - s) // 2), im)
    sheet.save(sys.argv[1])


if __name__ == "__main__":
    main()
