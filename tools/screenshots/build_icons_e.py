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
    i.shadow(16, 30, 10, 1.4)
    cx = cy = 16
    R = 11.500
    i.sphere(cx, cy, 6, "#f4f6fa", "#98a0ae", "#3c4250", rim="#343a46")
    g = i.lin(3, 4, 29, 28, (0, "#8cc0ff"), (1, "#1848c8"))
    a0, a1 = math.radians(-35), math.radians(235)
    pt = lambda a, r=R: (cx + r * math.cos(a), cy + r * math.sin(a))
    x0, y0 = pt(a0)
    x1, y1 = pt(a1)
    d = "M%.2f %.2fA%s %s 0 1 1 %.2f %.2f" % (x0, y0, R, R, x1, y1)
    i.add('<path d="%s" fill="none" stroke="#0c2c78" stroke-width="5" stroke-linecap="round"/>' % d)
    i.add('<path d="%s" fill="none" stroke="%s" stroke-width="3.400" stroke-linecap="round"/>' % (d, g))
    tx, ty = -math.sin(a1), math.cos(a1)
    nx, ny = math.cos(a1), math.sin(a1)
    bx, by = x1 + 1.200 * tx, y1 + 1.200 * ty
    L, W = 8, 5.500
    pts = [(bx + L * tx, by + L * ty), (bx + W * nx, by + W * ny), (bx - W * nx, by - W * ny)]
    i.add('<path d="M%.2f %.2fL%.2f %.2fL%.2f %.2fz" fill="%s" stroke="#0c2c78" stroke-width="1" stroke-linejoin="round"/>' % (
        pts[0] + pts[1] + pts[2] + (g,)))


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
    # traced from structlib.png (22x22 grid, scaled to 32)
    C = {"A": (4.5, 4.6), "B": (11.4, 8.9), "D": (6.8, 12.9), "E": (16.8, 16.0), "F": (7.6, 19.0)}
    N = {"n1": (10.4, 4.6), "n2": (2.5, 10.0), "n3": (16.4, 10.7), "n4": (12.9, 19.3)}
    P = dict(C)
    P.update(N)
    bonds = [("A", "n1"), ("n1", "B"), ("B", "D"), ("D", "n2"), ("n2", "A"),
             ("B", "n3"), ("n3", "E"), ("E", "n4"), ("n4", "F"), ("F", "D")]
    H = [((11.6, 1.0), "n1"), ((4.6, 21.0), "F"), ((20.8, 17.6), "E")]
    k = 32 / 22.0
    i.add('<g transform="scale(%.4f)">' % k)
    i.shadow(11, 21.500, 8, 1, .15)
    for a, b in bonds + [(h[1], h[0]) for h in []]:
        (x1, y1), (x2, y2) = P[a], P[b]
        i.add('<path d="M%s %sL%s %s" stroke="#4a505a" stroke-width="2" stroke-linecap="round"/>' % (x1, y1, x2, y2))
        i.add('<path d="M%s %sL%s %s" stroke="#b4bac4" stroke-width=".8" stroke-linecap="round"/>' % (x1 - .2, y1 - .3, x2 - .2, y2 - .3))
    for (hx, hy), a in H:
        x, y = P[a]
        i.add('<path d="M%s %sL%s %s" stroke="#9aa0aa" stroke-width="1.100" stroke-linecap="round"/>' % (x, y, hx, hy))
        i.sphere(hx, hy, 1.200, "#ffffff", "#f0f0f4", "#b0b4be")
    for n, (x, y) in C.items():
        i.sphere(x, y, 2.300, "#e4e6ea", "#8c929c", "#3c424c")
    for n, (x, y) in N.items():
        i.sphere(x, y, 2.500, "#bfe0ff", "#2c92ee", "#0a4aa8")
    i.add('</g>')


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
    i.shadow(16, 30.5, 12, 1.2, .2)
    fr = i.lin(0, 2, 0, 30, (0, "#6aa4f6"), (1, "#1a42a8"))
    i.add('<rect x="2" y="2.500" width="28" height="27" rx="1.500" fill="%s" stroke="#122e78" stroke-width="1"/>' % fr)
    tb = i.lin(0, 4, 0, 7.500, (0, "#a9cdfb"), (1, "#4f86e0"))
    i.add('<rect x="4" y="4" width="24" height="3.500" fill="%s"/>' % tb)
    sky = i.lin(0, 7.500, 0, 27, (0, "#dcecff"), (1, "#ffffff"))
    i.add('<rect x="4" y="7.500" width="24" height="19.500" fill="%s"/>' % sky)
    gr = i.lin(0, 24, 0, 27, (0, "#d8e8d0"), (1, "#a8c8a0"))
    i.add('<rect x="4" y="24.500" width="24" height="2.500" fill="%s"/>' % gr)
    side = i.lin(19, 0, 26.500, 0, (0, "#d8b890"), (1, "#8a6444"))
    i.add('<path d="M19.500 17L26.500 15.500V24L19.500 26z" fill="%s" stroke="#6a4020" stroke-width=".6" stroke-linejoin="round"/>' % side)
    front = i.lin(8.500, 0, 19.500, 0, (0, "#fff4e0"), (1, "#e2c8a0"))
    i.add('<path d="M8.500 17h11v9h-11z" fill="%s" stroke="#8a5a30" stroke-width=".6"/>' % front)
    gab = i.lin(0, 10, 0, 17, (0, "#ff9a60"), (1, "#e0501c"))
    i.add('<path d="M7.500 17.500L14 10l6.500 7.500z" fill="%s" stroke="#7a2008" stroke-width=".7" stroke-linejoin="round"/>' % gab)
    roof = i.lin(14, 9, 24, 17, (0, "#ffb070"), (.5, "#ec5a20"), (1, "#a82a08"))
    i.add('<path d="M14 10L21 8l6.500 8-7 2z" fill="%s" stroke="#7a2008" stroke-width=".7" stroke-linejoin="round"/>' % roof)
    door = i.lin(0, 20, 0, 26, (0, "#b87040"), (1, "#6a3410"))
    i.add('<rect x="12" y="20.500" width="4" height="5.500" fill="%s" stroke="#4a2408" stroke-width=".5"/>' % door)
    i.add('<path d="M22 19v3.500M24 18.500v3.500" stroke="#f0e0c0" stroke-opacity=".6" stroke-width="1.200"/>')
    i.sphere(7, 24.500, 2.200, "#d8f0b8", "#4a9a30", "#1e5a14")


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
    # Kekule hexagon: three alternating double bonds
    R, r = 12.500, 8.200
    v = [(16 + R * math.sin(math.radians(60 * k)), 16 - R * math.cos(math.radians(60 * k))) for k in range(6)]
    w = [(16 + r * math.sin(math.radians(60 * k)), 16 - r * math.cos(math.radians(60 * k))) for k in range(6)]
    g = i.lin(0, 3, 0, 29, (0, "#4a4a58"), (1, "#101018"))
    pts = " ".join("%.2f,%.2f" % q for q in v)
    i.add('<polygon points="%s" fill="#ffffff" fill-opacity=".45" stroke="%s" stroke-width="2.600" stroke-linejoin="round"/>' % (pts, g))
    for k in (0, 2, 4):
        a, b = w[k], w[(k + 1) % 6]
        t = .16
        x1, y1 = a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1])
        x2, y2 = b[0] + t * (a[0] - b[0]), b[1] + t * (a[1] - b[1])
        i.add('<path d="M%.2f %.2fL%.2f %.2f" stroke="%s" stroke-width="2.200" stroke-linecap="round"/>' % (x1, y1, x2, y2, g))


@icon
def styles(i):
    i.shadow(16, 30.5, 12, 1.2, .15)
    # blue beta-strand arrow with a twist (light face, then dark face)
    bl = i.lin(0, 18, 0, 30, (0, "#8cb8ff"), (1, "#1c46c8"))
    bd = i.lin(0, 18, 0, 30, (0, "#2a56d8"), (1, "#0c2070"))
    i.add('<path d="M2 28L14 22l1.500 3.200z" fill="%s" stroke="#0a1c6a" stroke-width=".7" stroke-linejoin="round"/>' % bd)
    i.add('<path d="M14 22L20 19l4-.500-1.500-4.500 8 7.500-8.500 6.500.5-4.500-4.500 1-5 2.500z" fill="%s" stroke="#0a1c6a" stroke-width=".8" stroke-linejoin="round"/>' % bl)
    i.add('<path d="M20 19l4-.500 1.700 1.500-5 2.500z" fill="%s" opacity=".55"/>' % bd)
    # red helix ribbon
    i.add('<g transform="translate(4 2) scale(.88) rotate(-32 13 11)">')
    back = i.lin(0, 5, 0, 17, (0, "#a01010"), (1, "#500404"))
    front = i.lin(0, 5, 0, 17, (0, "#ffa090"), (.5, "#e02020"), (1, "#8a0808"))
    for k in range(4):
        x = 1.500 + 5.600 * k
        i.add('<path d="M%s 7.500Q%s 11 %s 14.500" fill="none" stroke="#3a0000" stroke-width="3.600" stroke-linecap="butt"/>' % (x + 4, x + 4.500, x + 2))
        i.add('<path d="M%s 7.500Q%s 11 %s 14.500" fill="none" stroke="%s" stroke-width="2.400"/>' % (x + 4, x + 4.500, x + 2, back))
    for k in range(4):
        x = 1.500 + 5.600 * k
        i.add('<path d="M%s 6.500Q%s 10.500 %s 15.500" fill="none" stroke="#4a0000" stroke-width="4.600" stroke-linecap="round"/>' % (x, x + 1.500, x + 4))
        i.add('<path d="M%s 6.500Q%s 10.500 %s 15.500" fill="none" stroke="%s" stroke-width="3.200" stroke-linecap="round"/>' % (x, x + 1.500, x + 4, front))
    i.add('</g>')


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
    # 18 columns x 7 periods + detached f-block, square cells, canvas 32 wide
    c = 1.700
    ox, oy = (32 - 18 * c) / 2, 6.500
    pal = {"s": ("#ff9a8a", "#d83a2a"), "p": ("#ffe070", "#d49a00"),
           "d": ("#8cc4ff", "#2a6ad0"), "f": ("#90e8a8", "#1e9a48")}
    def cell(col, row, k, dy=0):
        x, y = ox + col * c, oy + row * c + dy
        g = i.lin(0, y, 0, y + c, (0, pal[k][0]), (1, pal[k][1]))
        i.add('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="%s"/>' % (x + .1, y + .1, c - .2, c - .2, g))
    for r in range(7):
        for col in range(18):
            if r == 0 and 0 < col < 17:
                continue
            if r in (1, 2) and 1 < col < 12:
                continue
            if col in (0, 1):
                k = "s"
            elif col == 17 and r == 0 or (col >= 12):
                k = "p" if not (r == 0) else "s"
            else:
                k = "d"
            if r == 0 and col == 17:
                k = "p"
            if r >= 5 and col == 2:
                k = "f"
            cell(col, r, k)
    for r in (0, 1):
        for col in range(14):
            cell(col + 2.5 - 0.5 + 0, 7 + r, "f", 0.8)


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
