#!/usr/bin/env python3
"""Check that the colours carrying meaning stay legible in light and dark themes.

    tests/look/contrast.py        exit 1 if any ratio is below 4.5:1

Everything else in the window chrome takes the GTK theme's own colours
(#210).  What is left are colours that say something: the run states, and
the good/unsure/bad status colours of the input checker and the message
logs.  Each has a light and a dark variant, and each variant must reach a
WCAG 2 contrast ratio of 4.5:1 against every background it is drawn on,
in the theme family it belongs to.

Run states whose icons share a shape (WxState::draw: triangle, circle,
diamond) are told apart by colour alone, so each such pair must also differ
by a CIEDE2000 of at least 10 with normal vision and with simulated
deuteranopia, protanopia and tritanopia (cvd.py), and by at least 10 in
lightness (L*): the icons are about a dozen pixels across, where hue alone
is hard to judge (submitted and running were once both L* 44).

The backgrounds are Adwaita's (GTK 3.24): a view's base colour and a
window's background colour.  The tables are read from the files the
program itself reads, so this cannot pass while the program shows
something else; and the run-state fallbacks compiled into WxState.C must
equal the shipped EcceGlobal, or two copies would drift apart.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cvd  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MINIMUM = 4.5

BACKGROUNDS = {
    "light": {"view": "#ffffff", "window": "#f6f5f4"},
    "dark": {"view": "#2d2d2d", "window": "#353535"},
}
TEXT = {"light": "#2e3436", "dark": "#eeeeec"}

# Pairs drawn with the same icon shape (LOADED shares COMPLETED's colour).
SAME_SHAPE = [("CREATED", "READY"), ("SUBMITTED", "RUNNING"),
              ("UNSUCCESSFUL", "FAILED"), ("UNSUCCESSFUL", "SYSTEM"),
              ("FAILED", "SYSTEM")]
MIN_DELTA_E = 10.0
MIN_DELTA_L = 10.0

STATES = ["CREATED", "READY", "SUBMITTED", "RUNNING", "COMPLETED", "KILLED",
          "UNSUCCESSFUL", "FAILED", "LOADED", "SYSTEM"]


def luminance(colour):
    value = colour.lstrip("#")
    channels = [int(value[i:i + 2], 16) / 255.0 for i in (0, 2, 4)]
    linear = [c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
              for c in channels]
    return 0.2126 * linear[0] + 0.7152 * linear[1] + 0.0722 * linear[2]


def ratio(a, b):
    la, lb = sorted((luminance(a), luminance(b)), reverse=True)
    return (la + 0.05) / (lb + 0.05)


def readEcceGlobal():
    path = os.path.join(ROOT, "data", "client", "config", "EcceGlobal")
    table = {}
    with open(path) as handle:
        for line in handle:
            match = re.match(r"\s*RUNSTATE\.(\w+)(\.DARK)?\s*:\s*(#[0-9a-fA-F]{6})",
                             line, re.IGNORECASE)
            if match:
                family = "dark" if match.group(2) else "light"
                table[(match.group(1).upper(), family)] = match.group(3).lower()
    return table


def readTable(path, name):
    """The "#rrggbb" strings of a C array `name`, in order."""
    with open(os.path.join(ROOT, path)) as handle:
        text = handle.read()
    match = re.search(re.escape(name) + r"[^=]*=\s*\{(.*?)\};", text, re.S)
    if not match:
        return None
    return [c.lower() for c in re.findall(r'"(#[0-9a-fA-F]{6})"', match.group(1))]


def main():
    failures = []
    checked = 0

    def check(what, fg, bg, where):
        nonlocal checked
        checked += 1
        value = ratio(fg, bg)
        line = "%-40s %s on %s (%s)  %.2f:1" % (what, fg, bg, where, value)
        if value < MINIMUM:
            failures.append(line)
            print("FAIL " + line)
        elif "-v" in sys.argv:
            print("ok   " + line)

    shipped = readEcceGlobal()
    for state in STATES:
        for family in ("light", "dark"):
            colour = shipped.get((state, family))
            if colour is None:
                failures.append("EcceGlobal has no %s colour for %s" % (family, state))
                print("FAIL " + failures[-1])
                continue
            for where, bg in BACKGROUNDS[family].items():
                check("run state %s (%s)" % (state.lower(), family), colour, bg, where)

    for family in ("light", "dark"):
        for a, b in SAME_SHAPE:
            ca, cb = shipped.get((a, family)), shipped.get((b, family))
            if ca is None or cb is None:
                continue
            checked += 1
            dl = abs(cvd.lab(cvd.linear(ca))[0] - cvd.lab(cvd.linear(cb))[0])
            line = "run states %s/%s (%s) dL* %.1f" % (a.lower(), b.lower(),
                                                      family, dl)
            if dl < MIN_DELTA_L:
                failures.append(line)
                print("FAIL " + line)
            elif "-v" in sys.argv:
                print("ok   " + line)
            for kind in cvd.KINDS:
                checked += 1
                value = cvd.delta(ca, cb, kind)
                line = "run states %s/%s (%s) %s  dE00 %.1f" % (
                    a.lower(), b.lower(), family, kind, value)
                if value < MIN_DELTA_E:
                    failures.append(line)
                    print("FAIL " + line)
                elif "-v" in sys.argv:
                    print("ok   " + line)

    #  WxState.C's tables are indexed by RUNSTATE, which starts with
    #  ILLEGAL and ends with the LAST sentinel; the ten real states sit
    #  between them in ResourceDescriptor order.
    order = ["CREATED", "READY", "SUBMITTED", "RUNNING", "COMPLETED",
             "LOADED", "KILLED", "UNSUCCESSFUL", "FAILED", "SYSTEM"]
    for family, name in (("light", "LIGHT_STATE_COLOURS"), ("dark", "DARK_STATE_COLOURS")):
        compiled = readTable("src/wxgui/wxtools/WxState.C", name)
        if not compiled or len(compiled) != len(order) + 2:
            failures.append("WxState.C: cannot read %s" % name)
            print("FAIL " + failures[-1])
            continue
        for state, colour in zip(order, compiled[1:-1]):
            if shipped.get((state, family)) != colour:
                failures.append("WxState.C %s %s is %s, EcceGlobal says %s"
                                % (name, state, colour, shipped.get((state, family))))
                print("FAIL " + failures[-1])

    themed = "src/wxgui/ewxClasses/ewxThemeColours.C"
    for family, index in (("light", 0), ("dark", 1)):
        text = readTable(themed, "STATUS_TEXT")
        tint = readTable(themed, "STATUS_TINT")
        if not text or not tint or len(text) != 6 or len(tint) != 6:
            failures.append("%s: cannot read STATUS_TEXT/STATUS_TINT" % themed)
            print("FAIL " + failures[-1])
            break
        for level, name in enumerate(("good", "unsure", "bad")):
            fg = text[2 * level + index]
            for where, bg in BACKGROUNDS[family].items():
                check("status text %s (%s)" % (name, family), fg, bg, where)
            check("status tint %s (%s)" % (name, family),
                  TEXT[family], tint[2 * level + index], "under theme text")

    if failures:
        print("contrast: %d of %d checks FAILED (minimum %.1f:1)"
              % (len(failures), checked, MINIMUM))
        return 1
    print("contrast: all %d checks pass (minimum %.1f:1, light and dark)"
          % (checked, MINIMUM))
    return 0


if __name__ == "__main__":
    sys.exit(main())
