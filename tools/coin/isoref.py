"""Renderer-independent reference for the blended isosurface lobes (#166).

Reads <name>-iso.txt from SceneScript's `exportiso` (triangles in world space,
vertex normals, one packed RGBA per lobe, camera), casts one ray per pixel
centre against every triangle (Moller-Trumbore, all hits: front and back faces
of every lobe), sorts the hits by depth and composites them back to front over
a flat background.  No Inventor transparency code is involved.

Lighting model (assumed, not read from the scene graph): a headlight along the
view axis, two-sided, colour = ambient + diffuse*max(n.l,0), the normal flipped for
clockwise-wound triangles (GL two-sided lighting) with the packed colour
as diffuse and ambient 0.2*0.2 (OpenGL default material ambient times the
default global ambient).  It will not match the libraries bit for bit.
"""
import math
import numpy as np


def load(path):
    w = h = None
    cam = None
    lobes = []
    for line in open(path):
        t = line.split()
        if not t:
            continue
        if t[0] == "size":
            w, h = int(t[1]), int(t[2])
        elif t[0] == "camera":
            cam = [float(x) for x in t[1:]]
        elif t[0] == "lobe":
            lobes.append({"rgba": int(t[2]), "normals": t[3] == "1", "tri": []})
        elif t[0] == "tri":
            lobes[-1]["tri"].append([float(x) for x in t[1:]])
    for lb in lobes:
        a = np.array(lb["tri"], dtype=np.float64).reshape(-1, 3, 6)
        lb["P"], lb["N"] = a[:, :, :3], a[:, :, 3:]
    return w, h, cam, lobes


def _rot(axis, ang):
    a = np.array(axis, float)
    n = np.linalg.norm(a)
    if n == 0:
        return np.eye(3)
    x, y, z = a / n
    c, s = math.cos(ang), math.sin(ang)
    C = 1 - c
    return np.array([[c + x*x*C, x*y*C - z*s, x*z*C + y*s],
                     [y*x*C + z*s, c + y*y*C, y*z*C - x*s],
                     [z*x*C - y*s, z*y*C + x*s, c + z*z*C]])


def render(path, bg, size=None, ambient=0.04, alpha_override=None, only=None, ambient_gain=None):
    """Returns (image uint8 HxWx3, bitmask of lobes hit per pixel, nearest lobe per pixel)."""
    w, h, cam, lobes = load(path)
    px, py, pz, ax, ay, az, ang, fovy, aspect = cam
    R = _rot((ax, ay, az), ang)
    eye = np.array([px, py, pz])
    asp = w / h
    if asp < 1:                    # Inventor ADJUST_CAMERA: widen the height angle
        fovy = 2 * math.atan(math.tan(fovy / 2) / asp)
    th = math.tan(fovy / 2)
    hits_pix, hits_t, hits_col, hits_a, hits_lobe = [], [], [], [], []
    for li, lb in enumerate(lobes):
        rgba = lb["rgba"]
        col = np.array([(rgba >> 24) & 255, (rgba >> 16) & 255, (rgba >> 8) & 255]) / 255.0
        alpha = (rgba & 255) / 255.0
        if alpha_override is not None:
            alpha = alpha_override
        if only is not None and li != only:
            continue
        P = (lb["P"] - eye) @ R          # camera space: row v -> R^T v
        N = lb["N"] @ R
        # screen bounds of every triangle
        z = P[:, :, 2]
        ok = (z < -1e-4).all(axis=1)
        sx = P[:, :, 0] / (-z * th * asp)
        sy = P[:, :, 1] / (-z * th)
        for k in np.nonzero(ok)[0]:
            x0 = int(max(0, math.floor((sx[k].min() + 1) * 0.5 * w - 0.5)))
            x1 = int(min(w - 1, math.ceil((sx[k].max() + 1) * 0.5 * w - 0.5)))
            y0 = int(max(0, math.floor((1 - sy[k].max()) * 0.5 * h - 0.5)))
            y1 = int(min(h - 1, math.ceil((1 - sy[k].min()) * 0.5 * h - 0.5)))
            if x1 < x0 or y1 < y0:
                continue
            xs, ys = np.meshgrid(np.arange(x0, x1 + 1), np.arange(y0, y1 + 1))
            dx = ((xs + 0.5) / w * 2 - 1) * th * asp
            dy = (1 - (ys + 0.5) / h * 2) * th
            d = np.stack([dx, dy, -np.ones_like(dx)], axis=-1).reshape(-1, 3)
            a, b, c = P[k]
            e1, e2 = b - a, c - a
            pv = np.cross(d, e2)
            det = pv @ e1
            with np.errstate(divide="ignore", invalid="ignore"):
                inv = 1.0 / det
                tv = -a                       # ray origin is the camera, at 0
                u = (pv @ tv) * inv
                qv = np.cross(tv, e1)
                v = (qv * d).sum(axis=1) * inv
                t = (qv @ e2) * inv
            m = (np.abs(det) > 1e-12) & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > 0)
            if not m.any():
                continue
            idx = np.nonzero(m)[0]
            uu, vv = u[idx], v[idx]
            if lb["normals"]:
                n = (1 - uu - vv)[:, None] * N[k, 0] + uu[:, None] * N[k, 1] + vv[:, None] * N[k, 2]
            else:
                n = np.tile(np.cross(e1, e2), (len(idx), 1))
            n = n / np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-12)
            #  Two-sided lighting: a triangle wound clockwise on screen is
            #  lit with the flipped normal.  Light vector = +z (headlight).
            facing = np.sign(np.cross(e1, e2)[2])
            diff = np.maximum(facing * n[:, 2], 0.0)
            lit = np.clip(ambient + col[None, :] * diff[:, None], 0, 1)
            hits_pix.append((ys.reshape(-1)[idx] * w + xs.reshape(-1)[idx]))
            hits_t.append(t[idx])
            hits_col.append(lit)
            hits_a.append(np.full(len(idx), alpha))
            hits_lobe.append(np.full(len(idx), li))
    img = np.tile(np.array(bg, float) / 255.0, (h * w, 1))
    mask = np.zeros(h * w, int)
    top = np.full(h * w, -1)          # lobe of the nearest hit (purely geometric)
    if hits_pix:
        pix = np.concatenate(hits_pix)
        t = np.concatenate(hits_t)
        col = np.concatenate(hits_col)
        al = np.concatenate(hits_a)
        lo = np.concatenate(hits_lobe)
        order = np.lexsort((-t, pix))          # by pixel, farthest first
        pix, col, al, lo = pix[order], col[order], al[order], lo[order]
        first = np.r_[0, np.nonzero(np.diff(pix))[0] + 1]
        rank = np.arange(len(pix)) - np.repeat(first, np.diff(np.r_[first, len(pix)]))
        for r in range(rank.max() + 1):
            s = rank == r
            p = pix[s]
            img[p] = al[s, None] * col[s] + (1 - al[s, None]) * img[p]
        last = np.r_[first[1:], len(pix)] - 1
        top[pix[last]] = lo[last]
        for li in range(len(lobes)):
            mask[np.unique(pix[lo == li])] |= 1 << li
    return (np.clip(img, 0, 1) * 255 + 0.5).astype(np.uint8).reshape(h, w, 3), mask.reshape(h, w), top.reshape(h, w)
