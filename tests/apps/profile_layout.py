"""Overlap check for the Reaction overview's energy profiles.

Reads the layout lines the organizer hooks print ("layout P size W H",
"layout P text x y w h what: text", "layout P seg x1 y1 x2 y2 width what")
and reports every label that touches a line or another label, or leaves the
image.  A line counts with its drawn width.
"""


def parse(lines):
    """{profile: {"size": (w, h), "texts": [...], "segs": [...]}}"""
    out = {}
    for l in lines:
        f = l.split()
        if len(f) < 3 or f[0] != "layout":
            continue
        p = out.setdefault(f[1], {"size": None, "texts": [], "segs": [], "unplaced": False})
        if f[2] == "size":
            p["size"] = (int(f[3]), int(f[4]))
        elif f[2] == "text":
            p["texts"].append((int(f[3]), int(f[4]), int(f[5]), int(f[6]), " ".join(f[7:])))
        elif f[2] == "seg":
            p["segs"].append((float(f[3]), float(f[4]), float(f[5]), float(f[6]), int(f[7]), f[8]))
        elif f[2] == "unplaced":
            p["unplaced"] = True
    return out


def segHitsRect(s, r):
    """Liang-Barsky: does segment s touch the closed rectangle (x0, y0, x1, y1)?"""
    x0, y0, x1, y1 = r
    dx, dy = s[2] - s[0], s[3] - s[1]
    t0, t1 = 0.0, 1.0
    for p, q in ((-dx, s[0] - x0), (dx, x1 - s[0]), (-dy, s[1] - y0), (dy, y1 - s[1])):
        if p == 0:
            if q < 0:
                return False
            continue
        t = q / p
        if p < 0:
            t0 = max(t0, t)
        else:
            t1 = min(t1, t)
        if t0 > t1:
            return False
    return True


def overlaps(lines):
    """Every collision, as text; empty when the layout is clean."""
    bad = []
    for name, p in sorted(parse(lines).items()):
        if p["unplaced"]:
            bad.append("profile %s: a label found no free place" % name)
        if not p["texts"] or not p["segs"]:
            bad.append("profile %s: no layout" % name)
            continue
        W, H = p["size"]
        for i, (x, y, w, h, what) in enumerate(p["texts"]):
            if x < 0 or y < 0 or x + w > W or y + h > H:
                bad.append("profile %s: '%s' leaves the image" % (name, what))
            for s in p["segs"]:
                half = s[4] / 2.0
                if segHitsRect(s, (x - half, y - half, x + w + half, y + h + half)):
                    bad.append("profile %s: '%s' touches %s line" % (name, what, s[5]))
            for (x2, y2, w2, h2, what2) in p["texts"][i + 1:]:
                if x < x2 + w2 and x2 < x + w and y < y2 + h2 and y2 < y + h:
                    bad.append("profile %s: '%s' overlaps '%s'" % (name, what, what2))
    return bad
