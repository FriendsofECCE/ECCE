#!/usr/bin/env python3
"""Generate the style-E toolbar SVGs (Classic ECCE icons redrawn with light
gradients) into data/client/pixmaps/svg/candidates/e/.  32x32 viewBox.
NanoSVG: no <text>, no filters, every gradient userSpaceOnUse."""
import math
import os

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                   "data", "client", "pixmaps", "svg", "candidates", "e")


def stops(st):
    return "".join('<stop offset="%s" stop-color="%s"%s/>' % (
        s[0], s[1], (' stop-opacity="%s"' % s[2]) if len(s) > 2 else "")
        for s in st)


class Icon:
    def __init__(self):
        self.defs, self.body, self.n = [], [], 0

    def _id(self):
        self.n += 1
        return "g%d" % self.n

    def lin(self, x1, y1, x2, y2, *st):
        i = self._id()
        self.defs.append('<linearGradient id="%s" gradientUnits="userSpaceOnUse" '
                         'x1="%s" y1="%s" x2="%s" y2="%s">%s</linearGradient>'
                         % (i, x1, y1, x2, y2, stops(st)))
        return "url(#%s)" % i

    def rad(self, cx, cy, r, *st):
        i = self._id()
        self.defs.append('<radialGradient id="%s" gradientUnits="userSpaceOnUse" '
                         'cx="%s" cy="%s" r="%s" fx="%s" fy="%s">%s</radialGradient>'
                         % (i, cx, cy, r, cx, cy, stops(st)))
        return "url(#%s)" % i

    def sphere(self, cx, cy, r, hi, mid, lo, opacity=1, rim=None):
        g = self.rad(cx - .3 * r, cy - .35 * r, 1.25 * r, (0, hi), (.45, mid), (1, lo))
        o = ' opacity="%s"' % opacity if opacity != 1 else ""
        st = ' stroke="%s" stroke-width=".6"' % rim if rim else ""
        self.add('<circle cx="%s" cy="%s" r="%s" fill="%s"%s%s/>' % (cx, cy, r, g, st, o))

    def shadow(self, cx, cy, rx, ry, a=.28):
        g = self.rad(cx, cy, rx, (0, "#000000", a), (1, "#000000", 0))
        self.add('<ellipse cx="%s" cy="%s" rx="%s" ry="%s" fill="%s"/>' % (cx, cy, rx, ry, g))

    def add(self, s):
        self.body.append(s)

    def svg(self):
        return ('<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" '
                'viewBox="0 0 32 32"><defs>%s</defs>%s</svg>\n'
                % ("".join(self.defs), "".join(self.body)))


ICONS = {}


def icon(f):
    ICONS[f.__name__.rstrip("_")] = f
    return f


def floppy(i):
    body = i.lin(0, 4, 0, 29, (0, "#8b8bc4"), (1, "#383878"))
    i.add('<path d="M5.500 4H24l4 4v19a1.500 1.500 0 0 1-1.500 1.500h-21A1.500 1.500 0 0 1 4 27V5.500A1.500 1.500 0 0 1 5.500 4z" fill="%s" stroke="#272760" stroke-width="1"/>' % body)
    lab = i.lin(0, 5, 0, 16, (0, "#ffffff"), (1, "#d6d8ee"))
    i.add('<rect x="9" y="5" width="14" height="11" rx="1" fill="%s" stroke="#272760" stroke-width=".6"/>' % lab)
    sh = i.lin(0, 19, 0, 28, (0, "#b9bcd8"), (1, "#7c80b0"))
    i.add('<rect x="10" y="20" width="12" height="8" rx="1" fill="%s" stroke="#272760" stroke-width=".6"/>' % sh)
    i.add('<rect x="17.500" y="21.500" width="3" height="5" fill="#34346e"/>')


@icon
def new(i):
    i.shadow(17, 29.5, 11, 1.8)
    pg = i.lin(5, 3, 25, 29, (0, "#ffffff"), (1, "#c9d0e4"))
    i.add('<path d="M6 2.500h13l8 8V28a1 1 0 0 1-1 1H6a1 1 0 0 1-1-1V3.500a1 1 0 0 1 1-1z" fill="%s" stroke="#6f7794" stroke-width="1"/>' % pg)
    fold = i.lin(19, 2.500, 25, 9, (0, "#ffffff"), (1, "#9aa4c4"))
    i.add('<path d="M19 2.500v6a2 2 0 0 0 2 2h6z" fill="%s" stroke="#6f7794" stroke-width="1" stroke-linejoin="round"/>' % fold)


@icon
def open_(i):
    i.shadow(16, 29.5, 13, 1.8)
    back = i.lin(0, 6, 0, 26, (0, "#3d6fd6"), (1, "#1a3f94"))
    i.add('<path d="M3 8a1.500 1.500 0 0 1 1.500-1.500h7l2.500 2.500H25a1.500 1.500 0 0 1 1.500 1.500V25H3z" fill="%s" stroke="#16337a" stroke-width="1"/>' % back)
    sheet = i.lin(8, 9, 24, 20, (0, "#ffffff"), (1, "#8fc0ff"))
    i.add('<path d="M8.500 20.500L10 10h13.500L22 20.500z" fill="%s" stroke="#2a58b8" stroke-width=".8"/>' % sheet)
    i.add('<path d="M11.500 13h9M11 16h9" stroke="#5b8ee0" stroke-width="1" stroke-linecap="round"/>')
    front = i.lin(0, 14, 0, 28, (0, "#7db6ff"), (.5, "#3b83f0"), (1, "#1f56c8"))
    i.add('<path d="M6.500 15.500H30L26 27.500H2.500z" fill="%s" stroke="#16337a" stroke-width="1" stroke-linejoin="round"/>' % front)
    i.add('<path d="M7.500 16.800H28.500" stroke="#ffffff" stroke-opacity=".55" stroke-width="1"/>')


@icon
def save(i):
    i.shadow(16, 30, 13, 1.5)
    floppy(i)


@icon
def saveas(i):
    i.shadow(14, 30, 12, 1.5)
    floppy(i)
    g = i.lin(18, 26, 30, 8, (0, "#e8a020"), (1, "#ffd966"))
    i.add('<path d="M16.500 27.500l1.200-5L28 12l3 3-10.300 10.500z" fill="%s" stroke="#8a5a00" stroke-width=".9" stroke-linejoin="round"/>' % g)
    i.add('<path d="M28 12l1.500-1.500a1.400 1.400 0 0 1 2 0l1 1a1.400 1.400 0 0 1 0 2L31 15z" fill="#e0564a" stroke="#8a2a20" stroke-width=".8"/>')
    i.add('<path d="M16.500 27.500l1.200-5 3.300 3.300z" fill="#2c2c2c"/>')


def arrow_green(i, mirror=False):
    t = ' transform="translate(32 0) scale(-1 1)"' if mirror else ""
    g = i.lin(0, 6, 0, 26, (0, "#8cf060"), (.55, "#34b020"), (1, "#14780c"))
    i.shadow(16, 28, 12, 1.5, .22)
    i.add('<path%s d="M4 15C4 8 13 6 21 8.500V4.500L30 12 21 19.500V15C16 13.500 11 15 10.500 19 10 22.500 15 24.500 22 24 18 28.500 8.500 28 5.500 23.500 3.500 20.500 4 17 4 15z" '
          'fill="%s" stroke="#0e5a08" stroke-width="1" stroke-linejoin="round"/>' % (t, g))


@icon
def undo(i):
    arrow_green(i, True)


@icon
def redo(i):
    arrow_green(i)


@icon
def select(i):
    i.shadow(14, 29, 8, 1.3, .2)
    g = i.lin(7, 3, 21, 26, (0, "#ffffff"), (1, "#b4b9c8"))
    i.add('<path d="M8 3v21l5.200-5 3.600 8 3.600-1.600-3.600-7.800H23z" fill="%s" stroke="#1c1c24" stroke-width="1.400" stroke-linejoin="round"/>' % g)


@icon
def rotate(i):
    i.shadow(16, 29.5, 10, 1.5)
    i.sphere(16, 18, 8, "#f2f4f8", "#9aa1ae", "#454b58", rim="#3a404c")
    g = i.lin(2, 6, 30, 26, (0, "#7bb4ff"), (1, "#1c55d0"))
    d = "M3.500 21C1.500 12 9 4.500 17 4.500c6 0 11 3.500 12 9"
    i.add('<path d="%s" fill="none" stroke="#12347e" stroke-width="4.200" stroke-linecap="round"/>' % d)
    i.add('<path d="%s" fill="none" stroke="%s" stroke-width="2.800" stroke-linecap="round"/>' % (d, g))
    i.add('<path d="M23.500 8l7.500 2.500-5 6.500z" fill="%s" stroke="#12347e" stroke-width="1" stroke-linejoin="round"/>' % g)


@icon
def move(i):
    i.shadow(16, 29.5, 10, 1.3, .2)
    g = i.lin(0, 3, 0, 29, (0, "#6a7080"), (.5, "#2a2e38"), (1, "#0c0e14"))
    i.add('<path d="M16 2.500l5.500 6.500H18v5h5v-3.500l6.500 5.500-6.500 5.500V18.500h-5v5h3.500L16 29.500 10.500 23.500H14v-5H9v3.500L2.500 16.500 9 11v3.500h5V9h-3.500z" fill="%s" stroke="#000000" stroke-width=".8" stroke-linejoin="round"/>' % g)
    i.add('<path d="M16 4.500l3 3.500M16 4.500l-3 3.500" stroke="#ffffff" stroke-opacity=".4" stroke-width=".8"/>')


@icon
def zoom(i):
    i.shadow(16, 30, 11, 1.4, .2)
    h = i.lin(18, 18, 29, 29, (0, "#f0b14a"), (1, "#a86a14"))
    i.add('<path d="M18.500 20l3.500-3.500 7 7a2.500 2.500 0 0 1-3.500 3.500z" fill="%s" stroke="#6b3f08" stroke-width="1" stroke-linejoin="round"/>' % h)
    rim = i.lin(4, 3, 22, 21, (0, "#cfd6e6"), (1, "#5d6680"))
    i.add('<circle cx="12.500" cy="12" r="9" fill="%s" stroke="#434b63" stroke-width="1"/>' % rim)
    gl = i.rad(10, 8.500, 11, (0, "#ffffff"), (.5, "#bfe0ff"), (1, "#6ab0f0"))
    i.add('<circle cx="12.500" cy="12" r="7" fill="%s" stroke="#434b63" stroke-width=".6"/>' % gl)
    i.add('<path d="M7.500 10.500a5.500 5.500 0 0 1 4-3.500" fill="none" stroke="#ffffff" stroke-width="1.400" stroke-linecap="round" stroke-opacity=".9"/>')
    i.add('<path d="M2.500 27.500h5M5 25v5M9.500 27.500h3" stroke="#222222" stroke-width="1.500"/>')


@icon
def library(i):
    i.shadow(16, 30, 11, 1.4, .2)
    pts = [(16, 6), (26, 13), (22, 25), (10, 25), (6, 13)]
    for a, b in zip(pts, pts[1:] + pts[:1]):
        i.add('<path d="M%s %sL%s %s" stroke="#5a6070" stroke-width="2.400" stroke-linecap="round"/>' % (a + b))
        i.add('<path d="M%s %sL%s %s" stroke="#b8bfce" stroke-width=".9" stroke-linecap="round"/>' % (a[0] - .3, a[1] - .4, b[0] - .3, b[1] - .4))
    blue = ("#cfe6ff", "#3b8cf0", "#14449a")
    grey = ("#f1f3f6", "#9aa0ac", "#4a505c")
    for n, (x, y) in enumerate(pts):
        i.sphere(x, y, 4.200, *(blue if n in (0, 2, 4) else grey))


@icon
def depthcue(i):
    i.shadow(16, 30, 12, 1.2, .15)
    i.sphere(24, 21, 3.400, "#ffd0d0", "#e04040", "#8a1010", .38)
    i.sphere(18, 17, 5.200, "#ffd0d0", "#e03030", "#880c0c", .68)
    i.sphere(9.500, 12, 7.500, "#ffe0e0", "#e82020", "#800808", 1)


@icon
def spin(i):
    g = i.lin(0, 3, 0, 29, (0, "#8cf060"), (.55, "#30a81c"), (1, "#106a0a"))
    for rot in (0, 180):
        i.add('<g transform="rotate(%d 16 16)"><path d="M6 17A10 10 0 0 1 20.500 7.500L19 4 28.500 7.500 24.500 16.500 23 12.500A6.500 6.500 0 0 0 11 16.500z" fill="%s" stroke="#0c5008" stroke-width=".9" stroke-linejoin="round"/></g>' % (rot, g))


@icon
def home(i):
    i.shadow(16, 30, 12, 1.3, .2)
    fr = i.lin(0, 3, 0, 29, (0, "#5b95f0"), (1, "#1d49b0"))
    i.add('<rect x="3" y="3" width="26" height="25" rx="1.600" fill="%s" stroke="#14327c" stroke-width="1"/>' % fr)
    sky = i.lin(0, 8, 0, 27, (0, "#e6f3ff"), (1, "#a9d0f8"))
    i.add('<rect x="5" y="8" width="22" height="18" fill="%s"/>' % sky)
    i.add('<rect x="5" y="5" width="22" height="2" fill="#ffffff" fill-opacity=".5"/>')
    wall = i.lin(0, 14, 0, 26, (0, "#ffe0b0"), (1, "#f09040"))
    i.add('<path d="M9.500 15.500h13V26h-13z" fill="%s" stroke="#9a4a10" stroke-width=".8"/>' % wall)
    roof = i.lin(0, 8, 0, 16, (0, "#ff8a50"), (1, "#c8300c"))
    i.add('<path d="M7 16.500L16 8.500l9 8z" fill="%s" stroke="#7a1a04" stroke-width=".9" stroke-linejoin="round"/>' % roof)
    i.add('<rect x="14" y="19.500" width="4" height="6.500" fill="#7a3a10"/>')


def arrow(i, x1, y1, x2, y2, light, dark, w=2.4):
    dx, dy = x2 - x1, y2 - y1
    L = math.hypot(dx, dy)
    ux, uy = dx / L, dy / L
    hx, hy = x2 - 5.5 * ux, y2 - 5.5 * uy
    px, py = -uy, ux
    g = i.lin(x1, y1, x2, y2, (0, light), (1, dark))
    i.add('<path d="M%s %sL%s %s" stroke="%s" stroke-width="%s" stroke-linecap="round"/>' % (x1, y1, hx, hy, g, w))
    i.add('<path d="M%s %sL%s %sL%s %sz" fill="%s" stroke="%s" stroke-width=".6" stroke-linejoin="round"/>' % (
        x2, y2, hx + 3.4 * px, hy + 3.4 * py, hx - 3.4 * px, hy - 3.4 * py, g, dark))


@icon
def axes(i):
    arrow(i, 14, 17, 28.5, 17, "#ff8a80", "#c01010")
    arrow(i, 14, 17, 14, 3, "#8cf078", "#128a10")
    arrow(i, 14, 17, 5, 27, "#8ab4ff", "#0c3fc8")
    i.sphere(14, 17, 2, "#ffffff", "#c8c8c8", "#606060")


@icon
def rings(i):
    i.shadow(16, 30, 11, 1.2, .15)
    i.add('<path d="M16 3.500l11 6.250v12.500L16 28.500 5 22.250V9.750z" fill="#ffffff" fill-opacity=".35" stroke="#1c1c28" stroke-width="2.400" stroke-linejoin="round"/>')
    g = i.rad(14, 13.5, 9, (0, "#ffe2a0"), (.5, "#ff9a1c"), (1, "#c05a00"))
    i.add('<circle cx="16" cy="16" r="6" fill="%s" stroke="#7a3800" stroke-width=".9"/>' % g)


@icon
def styles(i):
    i.shadow(16, 30, 12, 1.2, .15)
    r = i.lin(0, 8, 0, 26, (0, "#ff8a80"), (.5, "#d81818"), (1, "#7c0808"))
    for d in ("M3.500 17L22 5.500", "M5 26L24 14"):
        i.add('<path d="%s" stroke="#660000" stroke-width="5.200" stroke-linecap="round"/><path d="%s" stroke="%s" stroke-width="3.600" stroke-linecap="round"/>' % (d, d, r))
    for (x, y, rr) in ((24, 6, 4.2), (13, 12, 4), (26, 20, 4.4)):
        i.sphere(x, y, rr, "#d0e0ff", "#2a6af0", "#0c2a9a", rim="#0a2070")


@icon
def background(i):
    i.shadow(16, 30, 12, 1.2, .2)
    g = i.lin(3, 4, 29, 28, (0, "#f8dc9c"), (1, "#c88838"))
    i.add('<path d="M16 4C7 4 2.500 10 2.500 16c0 7 6 12.500 12 12.500 3.500 0 3-3 2.500-4.500-.8-2.500 1-4.500 3.500-4.500h4.500C28.500 19.500 30 17.500 30 15 30 8.500 23.500 4 16 4z" fill="%s" stroke="#7a4a10" stroke-width="1"/>' % g)
    i.add('<ellipse cx="9" cy="23.500" rx="2.800" ry="2.400" fill="#ffffff" stroke="#7a4a10" stroke-width=".7"/>')
    for (x, y, b) in ((9.5, 12.5, "#2a7aea"), (15, 8.5, "#e03030"),
                      (21.5, 10, "#f2c418"), (24.5, 16, "#2ab43c")):
        g2 = i.rad(x - 1, y - 1, 4, (0, "#ffffff"), (.4, b), (1, b))
        i.add('<circle cx="%s" cy="%s" r="2.800" fill="%s" stroke="#5a3a10" stroke-opacity=".5" stroke-width=".5"/>' % (x, y, g2))


@icon
def textcolor(i):
    g = i.lin(0, 3, 0, 25, (0, "#7cb4f0"), (1, "#1c5aa8"))
    i.add('<path d="M16 3.500L5 25h4l2-4.600h10L23 25h4L16 3.500zm0 8.200l3.600 7.400h-7.200z" fill="%s" stroke="#123c70" stroke-width=".9" stroke-linejoin="round" fill-rule="evenodd"/>' % g)
    b = i.lin(4, 0, 28, 0, (0, "#ff5050"), (.5, "#f2c418"), (1, "#2ab43c"))
    i.add('<rect x="4" y="27" width="24" height="3.500" rx=".8" fill="%s" stroke="#444444" stroke-width=".6"/>' % b)


@icon
def periodic(i):
    rows = ["N.......N", "MM..SNNNN", "MM..MSNNN", "MMMMMSSNN", "MMMMMMSSN"]
    cols = {"M": ("#6ab4f0", "#0b5fb0"), "S": ("#ffd060", "#d08000"), "N": ("#6ee0b0", "#0a9060")}
    for r, row in enumerate(rows):
        for c, k in enumerate(row):
            if k == ".":
                continue
            y = 3 + r * 5.2
            g = i.lin(0, y, 0, y + 4.4, (0, cols[k][0]), (1, cols[k][1]))
            i.add('<rect x="%s" y="%s" width="3.200" height="4.400" rx=".5" fill="%s"/>' % (2 + c * 3.3, y, g))


@icon
def element(i):
    i.shadow(16, 30, 11, 1.4, .2)
    i.sphere(16, 16, 12, "#f4f4f6", "#9096a2", "#40454f", rim="#30343c")


def build():
    os.makedirs(OUT, exist_ok=True)
    for name, f in ICONS.items():
        i = Icon()
        f(i)
        with open(os.path.join(OUT, name + ".svg"), "w") as fh:
            fh.write(i.svg())
    print("wrote", len(ICONS), "icons to", os.path.normpath(OUT))


if __name__ == "__main__":
    build()
