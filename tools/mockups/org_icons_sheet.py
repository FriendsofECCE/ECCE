#!/usr/bin/env python3
"""Mock-up contact sheet (#210, wip/org-icons): old pixmap vs new icon, per
icon, at 1x and 2x, on a light and a dark background.  Needs wxPython and a
display (Xvfb).  Throwaway tool, not part of the build.

    org_icons_sheet.py OUT.png
"""
import os, sys, wx

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PIX = os.path.join(ROOT, "data/client/pixmaps")
SVG = os.path.join(PIX, "svg")

TOOLS = [("builder", "gwbuilder2"), ("basisset", "gwbst2"), ("editor", "gweditor2"),
         ("launcher", "gwlauncher2"), ("viewer", "gwviewer2"),
         ("periodictable", "gwpertab2"), ("machinebrowser", "gwmachinebrowser2"),
         ("organizer", "gworganizer2")]
STD = [("gohome", "go-home"), ("goup", "go-up"), ("back", "go-previous"),
       ("forward", "go-next"), ("editcut", "edit-cut"), ("editcopy", "edit-copy"),
       ("editpaste", "edit-paste"), ("rename", "document-edit"),
       ("editdelete", "edit-delete"), ("filefind", "edit-find"),
       ("reload", "view-refresh"), ("upload", "document-send"),
       ("download", "folder-download"), ("info", "dialog-information")]
THEMES = [("light", wx.Colour(246, 245, 244), wx.Colour(46, 52, 54)),
          ("dark", wx.Colour(48, 48, 48), wx.Colour(238, 238, 236))]


def old(name, scale=1):
    b = wx.Bitmap(os.path.join(PIX, name + ".xpm"), wx.BITMAP_TYPE_XPM)
    if scale > 1:  # what the toolkit does with a 1x bitmap on a 2x display
        im = b.ConvertToImage()
        im.Rescale(b.GetWidth() * scale, b.GetHeight() * scale, wx.IMAGE_QUALITY_BILINEAR)
        b = wx.Bitmap(im)
    return b


def themed(icon, px, ink):
    art = wx.ArtProvider.GetBitmap(icon + "-symbolic", wx.ART_OTHER, wx.Size(px, px))
    if not art.IsOk():
        return art
    im = art.ConvertToImage()
    if not im.HasAlpha():
        return art
    for x in range(im.GetWidth()):
        for y in range(im.GetHeight()):
            im.SetRGB(x, y, ink.Red(), ink.Green(), ink.Blue())
    return wx.Bitmap(im)


def svg(name, px):
    return wx.BitmapBundle.FromSVGFile(os.path.join(SVG, name + ".svg"),
                                       wx.Size(64, 64)).GetBitmap(wx.Size(px, px))


def main(out):
    app = wx.App()
    pad = 10
    colw = [80, 80, 144, 144]
    extra = 90            # old1x new1x old2x new2x cells
    panel = sum(colw) + 5 * pad + 60
    rows = [("ECCE tool buttons (64 px, 2x = 128 px)", None)] + \
           [(t, ("tool", t, o)) for t, o in TOOLS] + \
           [("Toolbar actions (22 px, 2x = 44 px): old pixmap vs theme icon "
             "(already on main)", None)] + \
           [(n, ("std", n, i)) for n, i in STD]
    heights = [26 if r[1] is None else (144 if r[1][0] == "tool" else 56) for r in rows]
    H = sum(heights) + 40
    W = 2 * panel + 3 * pad
    bmp = wx.Bitmap(W, H)
    dc = wx.MemoryDC(bmp)
    dc.SetBackground(wx.Brush(wx.WHITE)); dc.Clear()
    font = dc.GetFont(); font.SetPointSize(9); dc.SetFont(font)
    for pi, (tn, bg, ink) in enumerate(THEMES):
        x0 = pad + pi * (panel + pad)
        dc.SetBrush(wx.Brush(bg)); dc.SetPen(wx.TRANSPARENT_PEN)
        dc.DrawRectangle(x0 - 5, 0, panel + 10, H)
        dc.SetTextForeground(ink)
        y = 6
        dc.DrawText("%s theme   -   old 1x | new 1x | old 2x | new 2x" % tn, x0, y)
        y = 34
        for (label, spec), h in zip(rows, heights):
            if spec is None:
                dc.DrawText(label, x0, y + 4)
            else:
                kind, name, ref = spec
                if kind == "tool":
                    imgs = [old(ref), svg("tool-" + name, 64), old(ref, 2), svg("tool-" + name, 128)]
                else:
                    imgs = [old(name), themed(ref, 22, ink), old(name, 2), themed(ref, 44, ink)]
                x = x0
                for im, w in zip(imgs, colw):
                    if im.IsOk():
                        dc.DrawBitmap(im, x + (w - im.GetWidth()) // 2,
                                      y + (h - im.GetHeight()) // 2, True)
                    x += w + pad
                if kind == "std":
                    dc.DrawText(name, x, y + 20)
            y += h
    dc.SelectObject(wx.NullBitmap)
    bmp.SaveFile(out, wx.BITMAP_TYPE_PNG)


if __name__ == "__main__":
    main(sys.argv[1])
