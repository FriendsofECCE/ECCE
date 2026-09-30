"""Starting structures: reasonable, deliberately not optimised.

Each returns [(symbol, x, y, z)] in Angstrom.
"""

import math


def _d(deg):
    return math.radians(deg)


def diatomic(a, b, r):
    return [(a, 0.0, 0.0, 0.0), (b, 0.0, 0.0, r)]


def atom(sym):
    return [(sym, 0.0, 0.0, 0.0)]


def pyramid(center, ligand, r, polar, n=3):
    """`n` ligands at polar angle `polar` from +z, evenly spread in azimuth."""
    out = [(center, 0.0, 0.0, 0.0)]
    for k in range(n):
        phi = _d(360.0 * k / n)
        out.append((ligand, r * math.sin(_d(polar)) * math.cos(phi),
                    r * math.sin(_d(polar)) * math.sin(phi),
                    r * math.cos(_d(polar))))
    return out


def methane(r, polar):
    out = [("C", 0.0, 0.0, 0.0), ("H", 0.0, 0.0, r)]
    return out + pyramid("C", "H", r, polar)[1:]


def bent(center, ligand, r, angle):
    h = _d(angle / 2.0)
    return [(center, 0.0, 0.0, 0.0),
            (ligand, r * math.sin(h), 0.0, r * math.cos(h)),
            (ligand, -r * math.sin(h), 0.0, r * math.cos(h))]


def pf5():
    out = [("P", 0.0, 0.0, 0.0), ("F", 0.0, 0.0, 1.66), ("F", 0.0, 0.0, -1.66)]
    for k in range(3):
        phi = _d(120.0 * k)
        out.append(("F", 1.60 * math.cos(phi), 1.60 * math.sin(phi), 0.0))
    return out


def sf4_seesaw():
    e, ax, tilt = 1.58, 1.68, _d(3.5)
    half = _d(51.0)
    return [("S", 0.0, 0.0, 0.0),
            ("F", e * math.cos(half), e * math.sin(half), 0.0),
            ("F", e * math.cos(half), -e * math.sin(half), 0.0),
            ("F", ax * math.sin(tilt), 0.0, ax * math.cos(tilt)),
            ("F", ax * math.sin(tilt), 0.0, -ax * math.cos(tilt))]


def sf4_pyramid():
    """One F on the axis, three in a pyramid behind the sulfur, all slightly
    off symmetry so the optimiser is free to leave C3v."""
    out = [("S", 0.0, 0.0, 0.0), ("F", 0.03, 0.0, 1.62)]
    nudge = ((0.03, -0.02, 0.02), (-0.03, 0.02, -0.02), (0.02, 0.03, 0.03))
    for k in range(3):
        phi = _d(120.0 * k)
        r, p = 1.62, _d(100.0)
        dx, dy, dz = nudge[k]
        out.append(("F", r * math.sin(p) * math.cos(phi) + dx,
                    r * math.sin(p) * math.sin(phi) + dy,
                    r * math.cos(p) + dz))
    return out


def clf3_tshape():
    ax, tilt = 1.72, _d(3.0)
    return [("Cl", 0.0, 0.0, 0.0), ("F", 1.62, 0.0, 0.0),
            ("F", -ax * math.sin(tilt), 0.0, ax * math.cos(tilt)),
            ("F", -ax * math.sin(tilt), 0.0, -ax * math.cos(tilt))]


def clf3_planar():
    out = [("Cl", 0.0, 0.0, 0.0)]
    nudge = ((0.0, 0.0, 0.03), (0.0, 0.0, -0.03), (0.02, 0.02, 0.0))
    for k in range(3):
        phi = _d(120.0 * k)
        dx, dy, dz = nudge[k]
        out.append(("F", 1.68 * math.cos(phi) + dx, 1.68 * math.sin(phi) + dy, dz))
    return out


def linear_triatomic(a, b, r):
    return [(b, 0.0, 0.0, -r), (a, 0.0, 0.0, 0.0), (b, 0.0, 0.0, r)]


def polyene(n):
    """All-trans CH2=CH-(CH=CH)...-CH=CH2 with n carbons, planar zigzag."""
    short, long_, ang = 1.36, 1.46, 123.0
    pos = [(0.0, 0.0)]
    heading = 0.0
    for i in range(n - 1):
        r = short if i % 2 == 0 else long_
        pos.append((pos[-1][0] + r * math.cos(_d(heading)),
                    pos[-1][1] + r * math.sin(_d(heading))))
        heading += (180.0 - ang) * (1 if i % 2 == 0 else -1)
    out = [("C", x, y, 0.0) for x, y in pos]

    def unit(a, b):
        dx, dy = b[0] - a[0], b[1] - a[1]
        n_ = math.hypot(dx, dy)
        return dx / n_, dy / n_

    for i, p in enumerate(pos):
        if 0 < i < n - 1:
            u1, u2 = unit(p, pos[i - 1]), unit(p, pos[i + 1])
            bx, by = -(u1[0] + u2[0]), -(u1[1] + u2[1])
            nb = math.hypot(bx, by)
            out.append(("H", p[0] + 1.09 * bx / nb, p[1] + 1.09 * by / nb, 0.0))
        else:
            u = unit(p, pos[1] if i == 0 else pos[n - 2])
            for s in (+1, -1):
                t = _d(121.0 * s)
                hx = u[0] * math.cos(t) - u[1] * math.sin(t)
                hy = u[0] * math.sin(t) + u[1] * math.cos(t)
                out.append(("H", p[0] + 1.09 * hx, p[1] + 1.09 * hy, 0.0))
    return out


def benzene():
    out = []
    for k in range(6):
        a = _d(60.0 * k)
        out.append(("C", 1.42 * math.cos(a), 1.42 * math.sin(a), 0.0))
    for k in range(6):
        a = _d(60.0 * k)
        out.append(("H", 2.52 * math.cos(a), 2.52 * math.sin(a), 0.0))
    return out
