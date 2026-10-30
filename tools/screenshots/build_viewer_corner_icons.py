#!/usr/bin/env python3
"""Writes the Viewer/Builder corner-button icon candidates (#670), style E.

    tools/screenshots/build_viewer_corner_icons.py

Home and magnifier are taken from candidates/e; the rest is drawn here.
Radial gradients use userSpaceOnUse (NanoSVG, see docs/claude/wx-viewer).
"""
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
E = os.path.join(ROOT, "data/client/pixmaps/svg/candidates/e")
OUT = os.path.join(ROOT, "data/client/pixmaps/svg/candidates/viewer-corner")


def svg(defs, body):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" '
            'viewBox="0 0 32 32"><defs>%s</defs>%s</svg>\n' % (defs, body))


def brackets(c1, c2, stroke):
    """Four corner brackets (the 'center' frame)."""
    d = 'M2 8V2h6M24 2h6v6M30 24v6h-6M8 30H2v-6'
    defs = ('<linearGradient id="br" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="32" y2="32">'
            '<stop offset="0" stop-color="%s"/><stop offset="1" stop-color="%s"/></linearGradient>'
            % (c1, c2))
    body = ('<path d="%s" fill="none" stroke="%s" stroke-width="3.2" stroke-linecap="round" '
            'stroke-linejoin="round"/><path d="%s" fill="none" stroke="url(#br)" '
            'stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>'
            % (d, stroke, d))
    return defs, body


def eye():
    bd, bb = brackets("#d98cf0", "#7a2fa8", "#4a1a6a")
    defs = bd + (
        '<linearGradient id="sc" gradientUnits="userSpaceOnUse" x1="0" y1="10" x2="0" y2="22">'
        '<stop offset="0" stop-color="#ffffff"/><stop offset="1" stop-color="#d4dae6"/></linearGradient>'
        '<radialGradient id="ir" gradientUnits="userSpaceOnUse" cx="14.5" cy="14" r="6">'
        '<stop offset="0" stop-color="#bfe6ff"/><stop offset=".55" stop-color="#3f8fe0"/>'
        '<stop offset="1" stop-color="#1a4a96"/></radialGradient>')
    body = (bb +
            '<path d="M4.5 16C8 10.2 12 8.8 16 8.8S24 10.2 27.5 16C24 21.8 20 23.2 16 23.2S8 21.8 4.5 16z" '
            'fill="url(#sc)" stroke="#3a4560" stroke-width="1.1" stroke-linejoin="round"/>'
            '<circle cx="16" cy="16" r="5.6" fill="url(#ir)" stroke="#16356e" stroke-width=".7"/>'
            '<circle cx="16" cy="16" r="2.4" fill="#0a1426"/>'
            '<circle cx="14.2" cy="13.9" r="1.3" fill="#ffffff" fill-opacity=".95"/>')
    return svg(defs, body)


def padlock(locked):
    if locked:
        bd, bb = brackets("#d98cf0", "#7a2fa8", "#4a1a6a")
        body_stops = ("#ffd86a", "#c8860c")
        sh_stops = ("#e6ecf6", "#7f8aa4")
        body_edge, sh_edge = "#6b3f08", "#434b63"
        shackle = 'M11.5 15v-3.5a4.5 4.5 0 0 1 9 0V15'
    else:
        bd, bb = brackets("#d4d4d4", "#8c8c8c", "#5a5a5a")
        body_stops = ("#e2e2e2", "#8e8e8e")
        sh_stops = ("#eeeeee", "#9a9a9a")
        body_edge, sh_edge = "#555555", "#555555"
        shackle = 'M11.5 15v-3.5a4.5 4.5 0 0 1 9 0V12'
    defs = bd + (
        '<linearGradient id="pb" gradientUnits="userSpaceOnUse" x1="0" y1="14" x2="0" y2="25">'
        '<stop offset="0" stop-color="%s"/><stop offset="1" stop-color="%s"/></linearGradient>'
        '<linearGradient id="ps" gradientUnits="userSpaceOnUse" x1="11" y1="0" x2="21" y2="0">'
        '<stop offset="0" stop-color="%s"/><stop offset="1" stop-color="%s"/></linearGradient>'
        % (body_stops + sh_stops))
    body = (bb +
            '<path d="%s" fill="none" stroke="%s" stroke-width="3.6" stroke-linecap="round"/>'
            '<path d="%s" fill="none" stroke="url(#ps)" stroke-width="2.2" stroke-linecap="round"/>'
            '<rect x="8" y="14.5" width="16" height="10.5" rx="1.6" fill="url(#pb)" '
            'stroke="%s" stroke-width="1"/>'
            '<path d="M9.5 16.2h13" stroke="#ffffff" stroke-opacity=".45" stroke-width=".9"/>'
            '<circle cx="16" cy="19" r="1.5" fill="%s"/><path d="M16 19.5v3" stroke="%s" '
            'stroke-width="1.4" stroke-linecap="round"/>'
            % (shackle, sh_edge, shackle, body_edge, body_edge, body_edge))
    return svg(defs, body)


def badge(kind, cx, cy, r):
    """Green round badge with a check, plus or minus."""
    g = ('<radialGradient id="bg" gradientUnits="userSpaceOnUse" cx="%s" cy="%s" r="%s">'
         '<stop offset="0" stop-color="#8fe08a"/><stop offset="1" stop-color="#1f8a2a"/></radialGradient>'
         % (cx - 1.5, cy - 2, r + 1))
    k = r / 6.0
    if kind == "check":
        p = "M%.1f %.1fl%.1f %.1f %.1f %.1f" % (cx - 4 * k, cy + 0.2 * k, 2.6 * k, 2.6 * k, 4.8 * k, -5.8 * k)
    elif kind == "plus":
        p = "M%.1f %.1fh%.1fM%.1f %.1fv%.1f" % (cx - 3.5 * k, cy, 7 * k, cx, cy - 3.5 * k, 7 * k)
    else:
        p = "M%.1f %.1fh%.1f" % (cx - 3.5 * k, cy, 7 * k)
    return g, ('<circle cx="%s" cy="%s" r="%s" fill="url(#bg)" stroke="#12601a" stroke-width="1"/>'
               '<path d="%s" fill="none" stroke="#fff" stroke-width="2.2" stroke-linecap="round" '
               'stroke-linejoin="round"/>' % (cx, cy, r, p))


def from_e(name):
    t = open(os.path.join(E, name)).read()
    m = re.match(r'(<svg[^>]*>)<defs>(.*?)</defs>(.*)</svg>', t, re.S)
    return m.group(2), m.group(3)


def home(check):
    defs, body = from_e("home.svg")
    if check:
        bdefs, bbody = badge("check", 25, 25, 6)
        defs, body = defs + bdefs, body + bbody
    return svg(defs, body)


def zoom(kind):
    defs, body = from_e("zoom.svg")
    body = re.sub(r'<path d="M2\.500 27\.500h5.*?/>', '', body)   # the e/ sign mark
    if kind == "fit":
        extra = ('<path d="M3 20.5V28h7.5" fill="none" stroke="#1f8a2a" stroke-width="1.8" '
                 'stroke-linecap="round" stroke-linejoin="round" stroke-dasharray="2.2 2.2"/>')
    else:
        bdefs, extra = badge(kind, 7, 25, 5)
        defs += bdefs
    return svg(defs, body + extra)


FILES = {
    "center-view.svg": eye(), "center-view-lock.svg": padlock(True),
    "center-view-unlock.svg": padlock(False), "home-view.svg": home(False),
    "set-home-view.svg": home(True), "zoom-fit.svg": zoom("fit"),
    "zoom-in.svg": zoom("plus"), "zoom-out.svg": zoom("minus"),
}

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for n, t in FILES.items():
        with open(os.path.join(OUT, n), "w") as f:
            f.write(t)
