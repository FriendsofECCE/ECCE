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
def home_window(i):
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


def house(i):
    # a free-standing house in three-quarter view: cream front with its
    # gable, darker side wall, red roof with a chimney, door and windows
    side = i.lin(16, 0, 27, 0, (0, "#e6cfa8"), (1, "#9a7448"))
    i.add('<path d="M16 15.500L27 13v11.500L16 27z" fill="%s" stroke="#6a4420" stroke-width=".6" stroke-linejoin="round"/>' % side)
    front = i.lin(4, 0, 16, 0, (0, "#fffaf0"), (1, "#ead6b2"))
    i.add('<path d="M4 15.500L10 9l6 6.500V27H4z" fill="%s" stroke="#8a5a30" stroke-width=".6" stroke-linejoin="round"/>' % front)
    roof = i.lin(10, 5, 24, 16, (0, "#ff9a6a"), (.55, "#e04a1c"), (1, "#9a2408"))
    i.add('<path d="M10 7.800L21.500 5.200 28.500 13.500 17 16.300z" fill="%s" stroke="#6a1804" stroke-width=".7" stroke-linejoin="round"/>' % roof)
    i.add('<path d="M2.800 16.300L10 7.800 17 16.300" fill="none" stroke="#7a1c06" stroke-width="1.700" stroke-linecap="round" stroke-linejoin="round"/>')
    ch = i.lin(21, 0, 24, 0, (0, "#d06040"), (1, "#7a2a14"))
    i.add('<path d="M21 4.800h2.800v4.800L21 10.400z" fill="%s" stroke="#4a1808" stroke-width=".5"/>' % ch)
    i.add('<rect x="20.600" y="4" width="3.600" height="1.200" fill="#5a2a18"/>')
    door = i.lin(0, 20, 0, 27, (0, "#b07040"), (1, "#5e3010"))
    i.add('<rect x="6.200" y="20" width="3.600" height="7" fill="%s" stroke="#3a1c06" stroke-width=".5"/>' % door)
    glass = i.lin(0, 17, 0, 22, (0, "#d8ecff"), (1, "#4a88d8"))
    i.add('<rect x="11.200" y="18" width="3.400" height="3.400" fill="%s" stroke="#5a3a1a" stroke-width=".6"/>' % glass)
    i.add('<path d="M19 18.200L23.500 17.200V21.400L19 22.400z" fill="%s" stroke="#4a2c10" stroke-width=".6"/>' % glass)


@icon
def home_free(i):
    i.shadow(16, 28.300, 13.500, 2, .25)
    house(i)


@icon
def home_grid(i):
    # the house standing on a grid floor: home in a 3-D scene
    fl = i.lin(0, 20, 0, 31, (0, "#e4eefc"), (1, "#a8c0e8"))
    i.add('<path d="M1 25L16 19.500 31 25 16 31z" fill="%s" stroke="#4a6aa8" stroke-width=".6" stroke-linejoin="round"/>' % fl)
    for t in (.25, .5, .75):
        x1, y1 = 1 + 15 * t, 25 - 5.5 * t
        x2, y2 = 16 + 15 * t, 31 - 6 * t
        i.add('<path d="M%.2f %.2fL%.2f %.2f" stroke="#6a8ac0" stroke-width=".5"/>' % (x1, y1, x2, y2))
        x1, y1 = 16 + 15 * t, 19.500 + 5.5 * t
        x2, y2 = 1 + 15 * t, 25 + 6 * t
        i.add('<path d="M%.2f %.2fL%.2f %.2f" stroke="#6a8ac0" stroke-width=".5"/>' % (x1, y1, x2, y2))
    i.add('<g transform="translate(5 1.500) scale(.68)">')
    house(i)
    i.add('</g>')


@icon
def home(i):
    # A house seen from the front, no perspective: shaded but flat-on
    i.shadow(16, 29.200, 12.500, 1.300, .22)
    ch = i.lin(20, 0, 24, 0, (0, "#d8684a"), (1, "#8a3018"))
    i.add('<rect x="20.500" y="5" width="3.600" height="8" fill="%s" stroke="#4a1808" stroke-width=".6"/>' % ch)
    i.add('<rect x="20" y="4.200" width="4.600" height="1.500" rx=".4" fill="#5a2414"/>')
    wall = i.lin(0, 14, 0, 29, (0, "#fffaf0"), (1, "#e2cca4"))
    i.add('<rect x="6" y="14.500" width="20" height="14" fill="%s" stroke="#8a5a30" stroke-width=".8"/>' % wall)
    roof = i.lin(0, 3, 0, 16, (0, "#ff9c6c"), (.6, "#e44c1c"), (1, "#a82a0a"))
    i.add('<path d="M2.500 15.500L16 3.500l13.500 12z" fill="%s" stroke="#6a1804" stroke-width=".9" stroke-linejoin="round"/>' % roof)
    i.add('<path d="M6 15.400L16 6.600l10 8.800" fill="none" stroke="#ffffff" stroke-opacity=".35" stroke-width=".8"/>')
    door = i.lin(0, 19, 0, 28.500, (0, "#b87444"), (1, "#5e3010"))
    i.add('<rect x="13.500" y="19.500" width="5" height="9" rx=".6" fill="%s" stroke="#3a1c06" stroke-width=".6"/>' % door)
    i.add('<circle cx="17.300" cy="24.300" r=".6" fill="#ffd27a"/>')
    glass = i.lin(0, 17, 0, 23, (0, "#e0f0ff"), (1, "#4a88d8"))
    for x in (8, 20.500):
        i.add('<rect x="%s" y="18" width="3.800" height="3.800" fill="%s" stroke="#5a3a1a" stroke-width=".7"/>' % (x, glass))
        i.add('<path d="M%s 19.900h3.800M%s 18v3.800" stroke="#5a3a1a" stroke-width=".5"/>' % (x, x + 1.9))


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
    # The resonance hybrid: hexagon with the delocalised ring inside, dark
    R = 12.500
    v = [(16 + R * math.sin(math.radians(60 * k)), 16 - R * math.cos(math.radians(60 * k))) for k in range(6)]
    g = i.lin(0, 3, 0, 29, (0, "#4a4a58"), (1, "#101018"))
    pts = " ".join("%.2f,%.2f" % q for q in v)
    i.add('<polygon points="%s" fill="#ffffff" fill-opacity=".45" stroke="%s" stroke-width="2.600" stroke-linejoin="round"/>' % (pts, g))
    i.add('<circle cx="16" cy="16" r="6.300" fill="none" stroke="%s" stroke-width="2.200"/>' % g)


@icon
def styles(i):
    # A protein cartoon as in the original: a red helix below and a blue
    # helix above, each a coiled ribbon (light front faces, dark back
    # faces), joined by a thin black coil.
    i.shadow(16, 30.500, 12, 1.200, .15)
    i.add('<path d="M3 9C1.500 4 7 2.500 9 5M24 7c5 0 6 5 3 7-2 1.500-1 4 1 5.500M27.500 20.500c1 3-1 6-5 6.500" fill="none" stroke="#141414" stroke-width="1.300" stroke-linecap="round"/>')

    def helix(x0, y0, x1, y1, turns, light, mid, dark, edge, amp):
        dx, dy = (x1 - x0) / turns, (y1 - y0) / turns
        L = (dx * dx + dy * dy) ** .5
        px, py = -dy / L * amp, dx / L * amp
        for k in range(turns):
            cx, cy = x0 + dx * (k + .5), y0 + dy * (k + .5)
            # back face (dark), then front face (light) of one turn
            i.add('<path d="M%.2f %.2fL%.2f %.2fL%.2f %.2fL%.2f %.2fz" fill="%s" stroke="%s" stroke-width=".5" stroke-linejoin="round"/>' % (
                cx - dx * .5 + px, cy - dy * .5 + py, cx - dx * .1 + px, cy - dy * .1 + py,
                cx + dx * .5 - px, cy + dy * .5 - py, cx + dx * .1 - px, cy + dy * .1 - py, dark, edge))
            g = i.lin(cx - px, cy - py, cx + px, cy + py, (0, light), (1, mid))
            i.add('<path d="M%.2f %.2fL%.2f %.2fL%.2f %.2fL%.2f %.2fz" fill="%s" stroke="%s" stroke-width=".5" stroke-linejoin="round"/>' % (
                cx - dx * .5 - px, cy - dy * .5 - py, cx - dx * .1 - px, cy - dy * .1 - py,
                cx + dx * .5 + px, cy + dy * .5 + py, cx + dx * .1 + px, cy + dy * .1 + py, g, edge))

    helix(9, 4, 26, 13, 4, "#9cc4ff", "#1f5ce0", "#0a2a80", "#06185a", 3.600)
    helix(3, 15, 22, 28, 4, "#ff9a90", "#e01818", "#7a0606", "#4a0202", 3.800)


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
    #  A simplified table, 10 columns x 4 periods and a detached f row, with
    #  cells of ~2 px at 22 px: the 18-column original blurred to grey.
    c, gap = 2.800, .5
    ox, oy = (32 - 10 * c) / 2, 7.200
    pal = {"s": ("#ff7a66", "#c82010"), "p": ("#ffd040", "#c88400"),
           "d": ("#5aa8ff", "#1450c0"), "f": ("#58d880", "#10802c")}
    cells = []
    for r in range(4):
        for col in range(10):
            if r == 0 and 0 < col < 9:
                continue
            if r in (1, 2) and 1 < col < 7:
                continue
            k = "s" if col < 2 else ("d" if col < 7 else "p")
            if r == 0:
                k = "s" if col == 0 else "p"
            cells.append((col, r, k, 0))
    for col in range(2, 9):
        cells.append((col, 4, "f", 1.0))
    for col, r, k, dy in cells:
        x, y = ox + col * c, oy + r * c + dy
        g = i.lin(0, y, 0, y + c, (0, pal[k][0]), (1, pal[k][1]))
        i.add('<rect x="%.2f" y="%.2f" width="%.2f" height="%.2f" fill="%s"/>'
              % (x + gap / 2, y + gap / 2, c - gap, c - gap, g))


@icon
def element(i):
    i.shadow(16, 30, 11, 1.4, .2)
    i.sphere(16, 16, 12, "#f4f4f6", "#9096a2", "#40454f", rim="#30343c")


#  Manipulator and Measure toolbars (Classic ECCE sphere22/rotation22 and
#  measure_*): candidates, not wired in until reviewed.
def red_x(i):
    for d in ("M5 5L27 27", "M27 5L5 27"):
        i.add('<path d="%s" stroke="#7a0000" stroke-width="3.6" stroke-linecap="round"/>' % d)
        i.add('<path d="%s" stroke="#ff3a2a" stroke-width="2.2" stroke-linecap="round"/>' % d)


def mesh_sphere(i):
    i.shadow(16, 29.5, 10, 1.3, .2)
    i.sphere(16, 16, 12, "#eef4ee", "#9cb0a0", "#3a4a40", opacity=.9, rim="#24302a")
    for ry in (12, 7.5, 3):
        i.add('<ellipse cx="16" cy="16" rx="%s" ry="12" fill="none" stroke="#1e5a32" '
              'stroke-width=".7" stroke-opacity=".8"/>' % ry)
    for dy, rx in ((0, 12), (-6.5, 10.1), (6.5, 10.1)):
        i.add('<ellipse cx="16" cy="%s" rx="%s" ry="%s" fill="none" stroke="#1e5a32" '
              'stroke-width=".7" stroke-opacity=".8"/>' % (16 + dy, rx, rx * .28))


@icon
def manip_sphere(i):
    mesh_sphere(i)


@icon
def manip_nosphere(i):
    mesh_sphere(i)
    red_x(i)


def wheel(i):
    g = i.lin(0, 15, 0, 17, (0, "#c8ccd4"), (1, "#5a606c"))
    i.add('<path d="M3 16H29" stroke="#3a3e48" stroke-width="2.4" stroke-linecap="round"/>')
    i.add('<path d="M3 16H29" stroke="%s" stroke-width="1.2" stroke-linecap="round"/>' % g)
    i.sphere(5.5, 16, 3.3, "#ffffff", "#a8acb4", "#4a4e58", rim="#30343c")
    i.sphere(26.5, 16, 3.3, "#ffffff", "#a8acb4", "#4a4e58", rim="#30343c")
    i.add('<ellipse cx="16" cy="16" rx="5" ry="13" fill="none" stroke="#1a1c22" stroke-width="2.6"/>')
    i.add('<ellipse cx="16" cy="16" rx="5" ry="13" fill="none" stroke="#7a808c" stroke-width="1.1"/>')
    i.add('<path d="M20.4 9.5l1.8 -3.6l1 4z" fill="#1a1c22"/>')
    i.sphere(16, 16, 2.4, "#ffffff", "#a8acb4", "#4a4e58", rim="#30343c")


@icon
def manip_wheel(i):
    wheel(i)


@icon
def manip_nowheel(i):
    wheel(i)
    red_x(i)


def ink(i):
    return i.lin(0, 4, 0, 28, (0, "#4a4e58"), (1, "#0c0e14"))


def length_glyph(i, y=16, x0=3, x1=29, h=6):
    g = ink(i)
    i.add('<path d="M%s %sv%sM%s %sv%s" stroke="#0c0e14" stroke-width="1.6"/>'
          % (x0, y - h / 2, h, x1, y - h / 2, h))
    i.add('<path d="M%s %sH%s" stroke="%s" stroke-width="2"/>' % (x0 + 4, y, x1 - 4, g))
    for xa, sgn in ((x0 + .8, 1), (x1 - .8, -1)):
        i.add('<path d="M%s %sl%s %sv%sz" fill="%s" stroke="#0c0e14" stroke-width=".6" '
              'stroke-linejoin="round"/>' % (xa, y, 5.5 * sgn, -3.2, 6.4, g))


def stick(i, x1, y1, x2, y2, w=2.6, s=1.0):
    g = i.lin(x1, y1, x2, y2, (0, "#e8eaee"), (1, "#7c828e"))
    i.add('<path d="M%s %sL%s %s" stroke="#30343c" stroke-width="%s" stroke-linecap="round"/>'
          % (x1, y1, x2, y2, (w + 1.2) * s))
    i.add('<path d="M%s %sL%s %s" stroke="%s" stroke-width="%s" stroke-linecap="round"/>'
          % (x1, y1, x2, y2, g, w * s))


def oxygen(i, x, y, r):
    i.sphere(x, y, r, "#ffb0a8", "#e02818", "#7a0a04", rim="#4a0602")


def hydrogen(i, x, y, r):
    i.sphere(x, y, r, "#ffffff", "#d8dade", "#7a7e88", rim="#4a4e58")


def distance_glyph(i, s=1.0, ox=0, oy=0):
    #  Two atoms with a double-headed arrow between them.
    P = lambda x, y: (ox + x * s, oy + y * s)
    A, B = P(4.5, 16), P(27.5, 16)
    oxygen(i, A[0], A[1], 4 * s)
    oxygen(i, B[0], B[1], 4 * s)
    x0, x1, y = A[0] + 4.8 * s, B[0] - 4.8 * s, A[1]
    g = i.lin(0, y - 2, 0, y + 2, (0, "#5a8cf0"), (1, "#0c3fa8"))
    i.add('<path d="M%.2f %.2fH%.2f" stroke="%s" stroke-width="%s"/>'
          % (x0 + 3 * s, y, x1 - 3 * s, g, 1.8 * s))
    for xa, sgn in ((x0, 1), (x1, -1)):
        i.add('<path d="M%.2f %.2fl%.2f %.2fv%.2fz" fill="%s" stroke="#0c2c78" '
              'stroke-width="%s" stroke-linejoin="round"/>'
              % (xa, y, 3.6 * sgn * s, -2.6 * s, 5.2 * s, g, .5 * s))


def angle_glyph(i, s=1.0, ox=0, oy=0):
    #  Water with the H-O-H angle marked.
    P = lambda x, y: (ox + x * s, oy + y * s)
    O, H1, H2 = P(16, 9), P(5, 25), P(27, 25)
    stick(i, O[0], O[1], H1[0], H1[1], s=s)
    stick(i, O[0], O[1], H2[0], H2[1], s=s)
    R = 9 * s
    import math
    a1 = math.atan2(H1[1] - O[1], H1[0] - O[0])
    a2 = math.atan2(H2[1] - O[1], H2[0] - O[0])
    p1 = (O[0] + R * math.cos(a1), O[1] + R * math.sin(a1))
    p2 = (O[0] + R * math.cos(a2), O[1] + R * math.sin(a2))
    i.add('<path d="M%.2f %.2fA%.2f %.2f 0 0 0 %.2f %.2f" fill="none" stroke="#1848c8" '
          'stroke-width="%s" stroke-linecap="round"/>' % (p1 + (R, R) + p2 + (1.8 * s,)))
    hydrogen(i, H1[0], H1[1], 3.6 * s)
    hydrogen(i, H2[0], H2[1], 3.6 * s)
    oxygen(i, O[0], O[1], 5 * s)


def torsion_glyph(i, s=1.0, ox=0, oy=0):
    #  H2O2 along its O-O bond: one H up in front, one down behind, and the
    #  dihedral between the two O-H bonds marked.
    P = lambda x, y: (ox + x * s, oy + y * s)
    O1, O2 = P(11, 17), P(21, 15)
    H1, H2 = P(6, 5), P(27, 26)
    stick(i, O2[0], O2[1], H2[0], H2[1], s=s)       # behind
    hydrogen(i, H2[0], H2[1], 3.2 * s)
    stick(i, O1[0], O1[1], O2[0], O2[1], s=s)
    oxygen(i, O2[0], O2[1], 4.6 * s)
    stick(i, O1[0], O1[1], H1[0], H1[1], s=s)
    oxygen(i, O1[0], O1[1], 4.6 * s)
    hydrogen(i, H1[0], H1[1], 3.6 * s)
    a, b = P(8.5, 10.5), P(25, 22.5)
    i.add('<path d="M%.2f %.2fC%.2f %.2f %.2f %.2f %.2f %.2f" fill="none" stroke="#1848c8" '
          'stroke-width="%s" stroke-linecap="round"/>'
          % (a + P(20, 2) + P(30, 12) + b + (1.6 * s,)))
    #  arrowhead at b
    i.add('<path d="M%.2f %.2fl%.2f %.2fl%.2f %.2fz" fill="#1848c8"/>'
          % (b[0] + 1.2 * s, b[1] + 1.8 * s, -3.6 * s, -0.6 * s, 2.2 * s, -3 * s))


@icon
def measure_length(i):
    distance_glyph(i)


@icon
def measure_angle(i):
    angle_glyph(i)


@icon
def measure_torsion(i):
    torsion_glyph(i)


@icon
def measure_clear(i):
    distance_glyph(i, .55, 7, -2)
    angle_glyph(i, .5, 1, 13)
    torsion_glyph(i, .5, 15, 13)
    red_x(i)

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
