#!/usr/bin/env python3
"""Render SVG icons through wx's own loader (wxBitmapBundle::FromSVGFile,
the NanoSVG renderer the app uses) at several pixel sizes.

    DISPLAY=:94 tools/screenshots/icons.py OUTDIR file.svg... [--sizes 16,24,32,48,64]

Writes OUTDIR/<name>-<size>.png.  Needs a display (Xvfb) for wx.App.
"""
import argparse
import os

import wx


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("outdir")
    ap.add_argument("svgs", nargs="+")
    ap.add_argument("--sizes", default="16,24,32,48,64")
    a = ap.parse_args()
    os.makedirs(a.outdir, exist_ok=True)
    app = wx.App()
    for path in a.svgs:
        name = os.path.splitext(os.path.basename(path))[0]
        for s in (int(v) for v in a.sizes.split(",")):
            b = wx.BitmapBundle.FromSVGFile(path, wx.Size(s, s))
            bmp = b.GetBitmap(wx.Size(s, s))
            if not bmp.IsOk():
                raise SystemExit("%s: no bitmap at %d" % (path, s))
            bmp.SaveFile(os.path.join(a.outdir, "%s-%d.png" % (name, s)),
                         wx.BITMAP_TYPE_PNG)


main()
