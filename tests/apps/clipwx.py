#!/usr/bin/env python3
"""Run a wxPython script with the clipping audit attached.

    clipwx.py SCRIPT ARGS...

Used by clip_test.py for the codereg dialogs: a few seconds after the app's
main loop starts, every top-level window is walked for controls that are cut
off, with the same checks as ewxWindowUtils::clipFindings() in the C++ apps,
and the findings are appended to $ECCE_CLIP_AUDIT in the same format.
$ECCE_CLIP_TAG names the window; $ECCE_CLIP_SHOTS gets a screenshot.
"""
import os
import runpy
import subprocess
import sys

import wx

ANY_BUTTON = (wx.Button, wx.ToggleButton)
FIXED = (wx.StaticText, wx.Button, wx.ToggleButton, wx.CheckBox,
         wx.RadioButton, wx.RadioBox, wx.Choice, wx.ComboBox, wx.SpinCtrl,
         wx.SpinCtrlDouble)


def scrolls(w):
    return (isinstance(w, (wx.ScrolledWindow, wx.ScrolledCanvas)) or
            w.GetClassName() in ("wxGrid", "wxListCtrl", "wxTreeCtrl",
                                 "wxHtmlWindow", "wxDataViewCtrl"))


def screen_rect(w):
    return wx.Rect(w.GetScreenPosition(), w.GetSize())


def client_rect(w):
    return wx.Rect(w.ClientToScreen((0, 0)), w.GetClientSize())


def describe(w):
    text = w.GetLabel()
    if not text and isinstance(w, wx.TextCtrl):
        text = w.GetValue()
    if not text:
        text = w.GetToolTipText() if w.GetToolTip() else ""
    text = " ".join(text.split())[:40]
    out = w.GetClassName() + (' "%s"' % text if text else "")
    p = w.GetParent()
    for _ in range(4):
        if not p:
            break
        name = " ".join((p.GetLabel() or p.GetName()).split())
        if name and name not in ("panel", "frame", "dialog"):
            out += ' in %s "%s"' % (p.GetClassName(), name[:30])
            break
        p = p.GetParent()
    return out


def shows_anything(bmp):
    if not bmp.IsOk():
        return False
    img = bmp.ConvertToImage()
    w, h = img.GetWidth(), img.GetHeight()
    if img.HasAlpha():
        return any(a > 32 for a in img.GetAlpha())
    d = bytes(img.GetData())
    for i in range(1, w * h):
        if (abs(d[3*i] - d[0]) + abs(d[3*i+1] - d[1]) +
                abs(d[3*i+2] - d[2])) > 40:
            return True
    return False


def audit(w, top, out):
    def add(kind, detail):
        r = screen_rect(w)
        out.append("%s\t%s\t%s\t%d,%d,%d,%d" % (kind, describe(w), detail,
                                                r.x, r.y, r.width, r.height))
    size = w.GetSize()
    if w is not top and size.x > 0 and size.y > 0:
        me = screen_rect(w)
        a = w.GetParent()
        while a:
            if scrolls(a):
                break
            area = client_rect(a)
            if (me.x < area.x - 1 or me.y < area.y - 1 or
                    me.GetRight() > area.GetRight() + 1 or
                    me.GetBottom() > area.GetBottom() + 1):
                over = max(area.x - me.x, me.GetRight() - area.GetRight(),
                           area.y - me.y, me.GetBottom() - area.GetBottom())
                add("outside-parent", "%dx%d, %d px outside %s's %dx%d client "
                    "area" % (size.x, size.y, over, a.GetClassName(),
                              area.width, area.height))
                break
            if a is top:
                break
            a = a.GetParent()
    ellipsized = (isinstance(w, wx.StaticText) and
                  (w.GetWindowStyleFlag() & wx.ST_ELLIPSIZE_MASK))
    if isinstance(w, FIXED) and not ellipsized and size.x > 0 and size.y > 0:
        best = w.GetBestSize()
        if size.x + 2 < best.x or size.y + 2 < best.y:
            add("smaller-than-best", "is %dx%d, needs %dx%d"
                % (size.x, size.y, best.x, best.y))
    if isinstance(w, wx.Button) or isinstance(w, wx.ToggleButton):
        bmp = w.GetBitmap() if hasattr(w, "GetBitmap") else wx.NullBitmap
        has_label = bool(w.GetLabel())
        has_bmp = bmp.IsOk()
        swatch = (not has_label and not has_bmp and w.GetParent() and
                  w.GetBackgroundColour() != w.GetParent().GetBackgroundColour())
        if not has_label and not has_bmp and not swatch:
            add("empty-button", "no label and no bitmap")
        elif not has_label and has_bmp and not shows_anything(bmp):
            add("blank-bitmap", "bitmap %dx%d draws nothing"
                % (bmp.GetWidth(), bmp.GetHeight()))
    value, pad = "", 0
    if isinstance(w, wx.TextCtrl) and not w.IsMultiLine() and \
            not (w.GetWindowStyleFlag() & wx.TE_PASSWORD):
        value = w.GetValue()
        if value:
            ext = w.GetTextExtent(value).x
            pad = w.GetSizeFromTextSize(ext, -1).x - ext
    elif isinstance(w, wx.ComboBox):
        value, pad = w.GetValue(), 36
    elif isinstance(w, wx.Choice) and w.GetSelection() != wx.NOT_FOUND:
        value, pad = w.GetStringSelection(), 36
    elif isinstance(w, wx.SpinCtrl):
        value, pad = str(w.GetValue()), 36
    if value:
        need = w.GetTextExtent(value).x + pad
        if size.x < need:
            v = value if len(value) <= 30 else value[:30] + "..."
            add("text-wider-than-field",
                'field is %d px wide, value "%s" needs %d' % (size.x, v, need))


def walk(w, top, out):
    if not w.IsShown():
        return
    if isinstance(w, wx.TopLevelWindow) and w is not top:
        return
    if w.IsShownOnScreen():
        audit(w, top, out)
    if scrolls(w) or isinstance(w, (wx.ComboBox, wx.SpinCtrl,
                                    wx.SpinCtrlDouble, wx.RadioBox)):
        return
    for c in w.GetChildren():
        walk(c, top, out)


def report():
    path = os.environ["ECCE_CLIP_AUDIT"]
    tag = os.environ.get("ECCE_CLIP_TAG", "dialog")
    for top in wx.GetTopLevelWindows():
        if not top.IsShown():
            continue
        out = []
        walk(top, top, out)
        shot = ""
        d = os.environ.get("ECCE_CLIP_SHOTS")
        if d:
            safe = "".join(c if c.isalnum() or c in ".-" else "_" for c in tag)
            shot = os.path.join(d, safe + ".png")
            if subprocess.call(["import", "-window", "root", shot],
                               stderr=subprocess.DEVNULL) != 0:
                shot = ""
        r = screen_rect(top)
        with open(path, "a") as f:
            f.write("WINDOW\t%s\t%d,%d,%d,%d\t%s\t%s\n"
                    % (tag, r.x, r.y, r.width, r.height, top.GetClassName(),
                       shot))
            for line in out:
                f.write("FINDING\t%s\t%s\n" % (tag, line))
        break


def main():
    script = sys.argv[1]
    sys.argv = sys.argv[1:]
    original = wx.App.MainLoop

    def mainloop(self, *a, **k):
        wx.CallLater(3500, report)
        return original(self, *a, **k)
    wx.App.MainLoop = mainloop
    runpy.run_path(script, run_name="__main__")


if __name__ == "__main__":
    main()
