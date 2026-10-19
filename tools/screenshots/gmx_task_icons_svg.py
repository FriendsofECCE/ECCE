#!/usr/bin/env python3
"""Writes candidate GROMACS Organizer icons as multi-colour 64x64 SVGs in the
style of data/client/pixmaps/svg/tool-*.svg (radial/linear gradients, light
from the top left, Okabe-Ito colours):

    tools/screenshots/gmx_task_icons_svg.py [OUTDIR]

taskgmx_<step>_a: pictogram only; _b: shaded task frame around the pictogram.
studygromacs_{a,b,c}: study icon candidates.
"""
import os, sys

DEFS = """<defs>
<radialGradient id="sW" cx="35%" cy="30%" r="75%"><stop offset="0" stop-color="#fff" stop-opacity=".95"/><stop offset=".3" stop-color="#f0f0f0"/><stop offset="1" stop-color="#6a6a6a"/></radialGradient>
<radialGradient id="sO" cx="35%" cy="30%" r="75%"><stop offset="0" stop-color="#fff" stop-opacity=".95"/><stop offset=".3" stop-color="#D55E00"/><stop offset="1" stop-color="#5f2a00"/></radialGradient>
<radialGradient id="sB" cx="35%" cy="30%" r="75%"><stop offset="0" stop-color="#fff" stop-opacity=".95"/><stop offset=".3" stop-color="#0072B2"/><stop offset="1" stop-color="#00263d"/></radialGradient>
<radialGradient id="sh"><stop offset=".5" stop-color="#000" stop-opacity=".32"/><stop offset="1" stop-color="#000" stop-opacity="0"/></radialGradient>
<linearGradient id="frame" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#F6A56B"/><stop offset=".45" stop-color="#D55E00"/><stop offset="1" stop-color="#6b2d00"/></linearGradient>
<linearGradient id="panel" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset="1" stop-color="#cfd8e0"/></linearGradient>
<linearGradient id="glass" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#ffffff"/><stop offset=".4" stop-color="#e8f1f8"/><stop offset="1" stop-color="#7d93a5"/></linearGradient>
<linearGradient id="merc" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#F59A55"/><stop offset=".4" stop-color="#D55E00"/><stop offset="1" stop-color="#7a3500"/></linearGradient>
<linearGradient id="cell" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#9fd3f0" stop-opacity=".55"/><stop offset="1" stop-color="#0072B2" stop-opacity=".25"/></linearGradient>
<linearGradient id="bolt" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#FFF6A0"/><stop offset=".5" stop-color="#F0E442"/><stop offset="1" stop-color="#E69F00"/></linearGradient>
<linearGradient id="fold" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#F6A56B"/><stop offset=".5" stop-color="#D55E00"/><stop offset="1" stop-color="#7a3500"/></linearGradient>
<linearGradient id="curve" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#56B4E9"/><stop offset="1" stop-color="#0072B2"/></linearGradient>
</defs>"""

def ball(x, y, r, g):
    return ('<circle cx="%g" cy="%g" r="%g" fill="url(#sh)"/>'
            '<circle cx="%g" cy="%g" r="%g" fill="url(#%s)"/>' % (x+r*.27, y+r*.45, r*1.3, x, y, r, g))

def bond(x1, y1, x2, y2, w=3.4):
    return ('<line x1="%g" y1="%g" x2="%g" y2="%g" stroke="#7a7a7a" stroke-width="%g" stroke-linecap="round"/>'
            '<line x1="%g" y1="%g" x2="%g" y2="%g" stroke="#e6e6e6" stroke-width="%g" stroke-linecap="round"/>'
            % (x1, y1, x2, y2, w, x1-.5, y1-.5, x2-.5, y2-.5, w*.45))

def water(cx, cy, s=1.0, o="sO"):
    ox, oy = cx, cy - 3*s
    h1, h2 = (cx-11*s, cy+8*s), (cx+11*s, cy+8*s)
    return (bond(ox, oy, *h1, 3.6*s) + bond(ox, oy, *h2, 3.6*s) + ball(*h1, 5*s, "sW") +
            ball(*h2, 5*s, "sW") + ball(ox, oy, 8*s, o))

def prepare():
    return ('<path d="M12 22L26 10H56V38L42 50H12z" fill="url(#cell)" stroke="none"/>'
            '<path d="M26 10V36L12 50M26 36H56" stroke="#0072B2" stroke-opacity=".6" stroke-width="2" fill="none" stroke-dasharray="3 2"/>'
            '<path d="M12 22L26 10H56V38L42 50H12z M12 22H42V50 M42 22L56 10" stroke="#0072B2" stroke-width="2.6" stroke-linejoin="round" fill="none"/>'
            + water(28, 32, .85))

def optimize():
    return ('<path d="M6 54H58M6 54V8" stroke="#7f8a96" stroke-width="2.6" stroke-linecap="round" fill="none"/>'
            '<path d="M10 12C20 12 22 50 34 50S48 28 58 26" stroke="url(#curve)" stroke-width="4" stroke-linecap="round" fill="none"/>'
            '<path d="M17 22L22 34" stroke="#D55E00" stroke-width="2.4" stroke-dasharray="3 2.4" fill="none"/>'
            + ball(14, 17, 6, "sO") + ball(34, 44, 5.2, "sB"))

def equilibrate():
    t = ''.join('<line x1="38" y1="%d" x2="%d" y2="%d" stroke="#555" stroke-width="2"/>' % (y, 38+(7 if i%2==0 else 4), y)
                for i, y in enumerate(range(10, 40, 5)))
    return ('<rect x="25" y="5" width="13" height="42" rx="6.5" fill="url(#glass)" stroke="#6b7c8a" stroke-width="1.6"/>'
            '<rect x="29" y="18" width="5" height="32" fill="url(#merc)"/>'
            + t +
            '<circle cx="32" cy="51" r="9.5" fill="url(#sh)" opacity=".6"/>'
            '<circle cx="31.5" cy="50" r="9" fill="url(#sO)"/>'
            '<path d="M47 33c3-3 5 3 8 0M47 40c3-3 5 3 8 0" stroke="#0072B2" stroke-width="2.2" fill="none" stroke-linecap="round"/>')

def dynamics():
    ghosts = ''.join('<g opacity="%s">%s</g>' % (o, water(x, y, .65, "sB"))
                     for x, y, o in ((12, 50, .3), (24, 38, .5)))
    return ('<path d="M6 54C12 40 16 58 22 44S30 40 36 30S44 28 52 14" stroke="#0072B2" stroke-width="2.4" stroke-dasharray="3 2.4" fill="none" stroke-linecap="round"/>'
            + ghosts + water(44, 22, 1.0))

def energy():
    return ('<path d="M36 4L14 34H27L22 60L50 24H35z" fill="url(#bolt)" stroke="#8a5a00" stroke-width="2" stroke-linejoin="round"/>'
            '<path d="M34 9L19 31H30" stroke="#fff" stroke-opacity=".7" stroke-width="2" fill="none" stroke-linecap="round"/>')

STEPS = (("prepare", prepare), ("optimize", optimize), ("equilibrate", equilibrate),
         ("dynamics", dynamics), ("energy", energy))

def svg(body):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 64 64">\n'
            + DEFS + "\n" + body + "\n</svg>\n")

def framed(inner):
    return ('<rect x="3" y="7" width="58" height="50" rx="8" fill="url(#sh)" transform="translate(1.5,2.5)"/>'
            '<rect x="3" y="6" width="58" height="50" rx="8" fill="url(#frame)"/>'
            '<path d="M6 12Q6 7 12 7H52" stroke="#fff" stroke-opacity=".6" stroke-width="1.6" fill="none"/>'
            '<rect x="9" y="12" width="46" height="38" rx="4" fill="url(#panel)"/>'
            '<g transform="translate(14.4,17.6) scale(.64)">' + inner + '</g>')

def study():
    G = '<path d="M45 22A16 16 0 1 0 47 38V33H33" stroke="#fff" stroke-width="6" fill="none" stroke-linecap="round" stroke-linejoin="round"/>'
    a = ('<circle cx="34" cy="35" r="27" fill="url(#sh)"/><circle cx="32" cy="32" r="27" fill="url(#sO)"/>'
         '<circle cx="32" cy="32" r="27" fill="none" stroke="#fff" stroke-opacity=".25"/>' + G)
    b = ('<path d="M4 14Q4 9 9 9H24L30 16H55Q60 16 60 21V50Q60 55 55 55H9Q4 55 4 50z" fill="url(#fold)"/>'
         '<path d="M6 20H58" stroke="#fff" stroke-opacity=".5" stroke-width="2"/>'
         '<g transform="translate(18,22) scale(.55)">' + water(14, 22, 1.0, "sB") + '</g>')
    c = ''
    for i, (dx, dy) in enumerate(((0, 0), (7, 7), (14, 14))):
        c += ('<rect x="%d" y="%d" width="34" height="34" rx="4" fill="url(#%s)" stroke="#5f2a00" stroke-opacity=".5"/>'
              % (6+dx, 6+dy, "frame" if i == 2 else "fold"))
    c += '<path d="M24 38C28 28 30 46 36 34S42 30 46 26" stroke="#fff" stroke-width="3" fill="none" stroke-linecap="round"/>'
    return (("a", a), ("b", b), ("c", c))

def main():
    d = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)),
        "..", "..", "data", "client", "pixmaps", "svg", "candidates")
    os.makedirs(d, exist_ok=True)
    for n, f in STEPS:
        open(os.path.join(d, "taskgmx_%s_a.svg" % n), "w").write(svg(f()))
        open(os.path.join(d, "taskgmx_%s_b.svg" % n), "w").write(svg(framed(f())))
    for k, b in study():
        open(os.path.join(d, "studygromacs_%s.svg" % k), "w").write(svg(b))
main()
