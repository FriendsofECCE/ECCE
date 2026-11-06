#!/usr/bin/env python3
"""Mock-ups for #845 (Vibrational Frequencies panel redesign) and #846 (the
Spectrum window carrying the panel's spectrum functions).  Real wx widgets,
a drawn stand-in spectrum.  Throwaway tool, not part of the build.

    vib_panel_845.py OUTDIR      (needs a display, e.g. Xvfb)
"""
import math
import os
import sys

import wx

PEAKS = [(3900, 43), (3170, 40), (3080, 15), (3020, 80), (2990, 83),
         (1510, 5), (1470, 17), (1380, 3), (1297, 120), (1170, 53),
         (1060, 41), (930, 8), (420, 13), (300, 113), (261, 34)]
RAMAN = [(3900, 103), (3170, 115), (3080, 135), (3020, 109), (2990, 134),
         (1470, 18), (1297, 12), (1170, 5), (1060, 4), (930, 5), (300, 3)]
MODE = ("Mode 10 · 1297.0 cm⁻¹ · A′ · IR 120.4 km/mol · "
        "Raman 12.7 Å⁴/amu")
COMP = ("C–O stretch 44 %   H–O–C bend 15 %   C–C–O bend 12 %   "
        "C–H wag 9 %   C–C stretch 7 %   (others < 5 %)")


class Spectrum(wx.Panel):
    """Stand-in for the spectrum canvas: IR above, Raman below."""

    def __init__(self, parent, panes=2, size=(-1, 160), selected=1297):
        super().__init__(parent, size=size)
        self.panes, self.selected = panes, selected
        self.SetBackgroundColour(wx.WHITE)
        self.Bind(wx.EVT_PAINT, self.paint)

    def curve(self, dc, peaks, top, h, w, colour):
        x0, x1, ymax = 4300.0, 0.0, max(p[1] for p in peaks) * 1.15
        pts = []
        for i in range(w):
            nu = x0 + (x1 - x0) * i / max(w - 1, 1)
            y = sum(a * math.exp(-((nu - c) / 12.0) ** 2) for c, a in peaks)
            pts.append(wx.Point(i, int(top + h - h * y / ymax)))
        dc.SetPen(wx.Pen(colour, 2))
        dc.DrawLines(pts)
        sx = int(w * (x0 - self.selected) / (x0 - x1))
        dc.SetPen(wx.Pen(wx.BLACK, 3))
        a = dict(peaks).get(self.selected, 0)
        dc.DrawLine(sx, top + h, sx, int(top + h - h * a / ymax))

    def paint(self, _):
        dc = wx.PaintDC(self)
        w, h = self.GetClientSize()
        dc.SetPen(wx.Pen(wx.Colour(200, 200, 200)))
        ph = (h - 18) // self.panes
        self.curve(dc, PEAKS, 4, ph - 8, w, wx.Colour(0, 114, 178))
        if self.panes > 1:
            self.curve(dc, RAMAN, ph + 4, ph - 8, w, wx.Colour(0, 140, 100))
        dc.SetFont(wx.Font(8, wx.FONTFAMILY_SWISS, wx.FONTSTYLE_NORMAL,
                           wx.FONTWEIGHT_NORMAL))
        for nu in (4000, 3000, 2000, 1000, 0):
            x = int(w * (4300 - nu) / 4300.0)
            dc.DrawText(str(nu), x - 10, h - 15)


def boxed(parent, label):
    box = wx.StaticBox(parent, label=label)
    return box, wx.StaticBoxSizer(box, wx.VERTICAL)


def row(*items):
    s = wx.BoxSizer(wx.HORIZONTAL)
    for it in items:
        if isinstance(it, int):
            s.AddSpacer(it)
        else:
            s.Add(it, 0, wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, 6)
    return s


def side_panel(frame):
    p = wx.Panel(frame)
    v = wx.BoxSizer(wx.VERTICAL)
    top = row(wx.ToggleButton(p, label="Spectrum"), wx.ToggleButton(p, label="Table"),
              20, wx.ToggleButton(p, label="IR"), wx.ToggleButton(p, label="Raman"))
    top.GetChildren()[0].GetWindow().SetValue(True)
    top.GetChildren()[3].GetWindow().SetValue(True)
    v.Add(top, 0, wx.ALL, 6)
    v.Add(Spectrum(p, panes=1, size=(-1, 230)), 1, wx.EXPAND | wx.LEFT | wx.RIGHT, 6)
    mode = wx.TextCtrl(p, value=MODE + "\n" + COMP,
                       style=wx.TE_MULTILINE | wx.TE_READONLY | wx.TE_WORDWRAP,
                       size=(-1, 96))
    v.Add(mode, 0, wx.EXPAND | wx.ALL, 6)
    box, bs = boxed(p, "Mode in the viewer")
    bs.Add(row(wx.CheckBox(p, label="Show"), wx.RadioButton(p, label="Animation",
               style=wx.RB_GROUP), wx.RadioButton(p, label="Vectors")), 0, wx.ALL, 4)
    arrow = wx.SpinCtrlDouble(p, value="1.76", size=(100, -1), inc=0.1)
    arrow.SetDigits(2)
    arrow.SetValue(1.76)
    bs.Add(row(wx.StaticText(p, label="Arrow length"), arrow, wx.Button(p, label="", size=(28, 24)),
               wx.CheckBox(p, label="Reverse")), 0, wx.ALL, 4)
    v.Add(bs, 0, wx.EXPAND | wx.ALL, 6)
    v.Add(row(wx.Button(p, label="Open in window..."), wx.Button(p, label="Composition..."),
              wx.Button(p, label="Displace...")), 0, wx.ALL, 6)
    p.SetSizer(v)
    return p


def spectrum_window(frame):
    p = wx.Panel(frame)
    v = wx.BoxSizer(wx.VERTICAL)
    box, bs = boxed(p, "Spectrum")
    bs.Add(row(wx.StaticText(p, label="Width"), wx.TextCtrl(p, value="15", size=(50, -1)),
               wx.StaticText(p, label="cm⁻¹   Scale factor"),
               wx.TextCtrl(p, value="1.000", size=(60, -1)),
               wx.CheckBox(p, label="Sticks"), wx.CheckBox(p, label="High on left"),
               wx.StaticText(p, label="Range"), wx.TextCtrl(p, size=(60, -1)),
               wx.StaticText(p, label="to"), wx.TextCtrl(p, size=(60, -1)),
               wx.Button(p, label="Reset zoom")), 0, wx.ALL, 4)
    v.Add(bs, 0, wx.EXPAND | wx.ALL, 6)
    box2, cs = boxed(p, "Other spectra")
    cs.Add(row(wx.Button(p, label="Compare..."), wx.StaticText(p, label="(overlay or mode matching, #796)"),
               wx.CheckBox(p, label="Difference"), wx.Button(p, label="Experimental..."),
               wx.Button(p, label="Remove"), 30, wx.Button(p, label="Export .csv...")),
           0, wx.ALL, 4)
    v.Add(cs, 0, wx.EXPAND | wx.LEFT | wx.RIGHT, 6)
    v.Add(Spectrum(p, panes=2, size=(-1, 380)), 1, wx.EXPAND | wx.ALL, 6)
    sel = wx.TextCtrl(p, value="Selected:  " + MODE + "\n" + COMP,
                      style=wx.TE_MULTILINE | wx.TE_READONLY | wx.TE_WORDWRAP,
                      size=(-1, 56))
    v.Add(sel, 0, wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, 6)
    v.Add(row(wx.Button(p, label="Composition..."), wx.Button(p, label="Close")),
          0, wx.ALIGN_RIGHT | wx.ALL, 6)
    p.SetSizer(v)
    return p


def shoot(frame, path):
    frame.Show()
    frame.Raise()
    for _ in range(20):
        wx.Yield()
        wx.MilliSleep(50)
    w, h = frame.GetClientSize()
    dc = wx.ClientDC(frame)
    bmp = wx.Bitmap(w, h)
    mem = wx.MemoryDC(bmp)
    mem.Blit(0, 0, w, h, dc, 0, 0)
    mem.SelectObject(wx.NullBitmap)
    bmp.SaveFile(path, wx.BITMAP_TYPE_PNG)


def main():
    out = sys.argv[1]
    app = wx.App()
    f1 = wx.Frame(None, title="Vibrational Frequencies (panel)", size=(420, 640))
    side_panel(f1)
    shoot(f1, os.path.join(out, "845-panel.png"))
    f2 = wx.Frame(None, title="Vibrational Frequencies: spectrum", size=(1100, 700))
    spectrum_window(f2)
    shoot(f2, os.path.join(out, "846-window.png"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
