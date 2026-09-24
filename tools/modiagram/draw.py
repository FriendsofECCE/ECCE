#!/usr/bin/env python3
"""Render a diagram model (from tools/modiagram/dump) as a picture.

    dump CharacterTables molecule.mol | draw.py out.png

WHY THIS EXISTS.  The diagram is drawn by a wxWidgets canvas inside
builder, so looking at a layout change meant building a package,
installing it, starting the services and opening a calculation.  This
draws the same model outside ECCE, so the *look* can be agreed before
it is written into the canvas.

The model comes from the real engine -- every level, label, degeneracy
and connection here was produced by MoFragments and MoDiagram.  The
LAYOUT is a second implementation and will drift from the canvas unless
one is kept in step with the other; that is the price of being able to
iterate, and it is worth paying only while the design is unsettled.
"""
import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

#  One colour per bonding/antibonding pair, so the two ends of an
#  interaction can be seen to belong together.  Chosen to stay
#  distinguishable in greyscale print and to colour-blind readers:
#  matplotlib's tab10 minus the red/green pair that causes trouble.
PAIR_COLOURS = ["#1f77b4", "#ff7f0e", "#9467bd", "#8c564b",
                "#17becf", "#e377c2", "#7f7f7f", "#bcbd22"]


def pairColour(level):
    """The colour for a level, black when it belongs to no pair."""
    if level.get("pair", -1) < 0:
        return "black"
    return PAIR_COLOURS[level["pair"] % len(PAIR_COLOURS)]


def read(stream):
    model = {"group": "", "note": "", "columns": {}, "levels": {}, "links": []}
    for raw in stream:
        f = raw.rstrip("\n").split("\t")
        if f[0] == "group":
            model["group"] = f[1]
        elif f[0] == "note":
            model["note"] = "" if f[1] == "-" else f[1]
        elif f[0] == "column":
            model["columns"][f[1]] = {
                "title": f[2], "hidden": int(f[3]),
                "hiddenMax": float(f[4]), "hiddenAbove": int(f[5])}
            model["levels"].setdefault(f[1], [])
        elif f[0] == "level":
            model["levels"].setdefault(f[1], []).append({
                "energy": float(f[3]), "occ": float(f[4]),
                "deg": int(f[5]), "label": f[6], "irrep": f[7],
                "annot": "" if f[8] == "-" else f[8],
                "character": int(f[9]), "pair": int(f[10]),
                "shareLeft": float(f[11]), "shareRight": float(f[12]),
                "shell": int(f[13]),
                "each": [float(x) for x in f[14:]]})
        elif f[0] == "link":
            model["links"].append(tuple(int(x) for x in f[1:4]))
    return model


#  Column centres in axes coordinates.
X = {"left": 0.24, "centre": 0.54, "right": 0.85}
#  Which side of a column its labels go: outward, so nothing is written
#  over the correlation lines in the middle.
OUT = {"left": -1, "centre": -1, "right": +1}
#  Half-width per column.  The fragment columns need more room than the
#  molecular one: chlorine's 3p TASOs in Td are four separate symmetry
#  sets -- A1, E, T1 and two T2 -- at ONE energy, which is twelve lines
#  and four labels on a single row.
HALFOF = {"left": 0.135, "centre": 0.105, "right": 0.135}
def slots(levels, index, half):
    """(x0, x1) for each orbital line of levels[index], and the group span.

    Levels sharing an energy are laid out side by side -- oxygen's 2p
    spans a1, b1 and b2 at ONE energy, and drawn on top of each other
    their labels overprint into a smudge.  A degenerate level is drawn
    as separate lines for the same reason: it is three orbitals, and one
    long line labelled "(3)" reads as one.
    """
    here = levels[index]["energy"]
    share = [i for i, l in enumerate(levels) if abs(l["energy"] - here) < 1e-9]
    total = sum(levels[i]["deg"] or 1 for i in share)
    before = sum(levels[i]["deg"] or 1 for i in share if i < index)

    width = 2 * half / max(total, 1)
    out = []
    for d in range(levels[index]["deg"] or 1):
        x0 = -half + (before + d) * width
        out.append((x0 + 0.12 * width, x0 + 0.88 * width))
    #  Which of the levels sharing this row this one is, so their labels
    #  can be staggered instead of printed on top of each other.
    return out, (before == 0), share.index(index), len(share)


def brace(ax, x, y0, y1, side, span):
    """A curly brace bracketing one level's orbitals.

    A degenerate set stays one level however far a finite-precision
    calculation spreads its members, or however slightly the geometry
    breaks the symmetry that makes them degenerate.  Drawn at their own
    heights with a brace, which is what says "these are one level"
    without pretending they share a number.
    """
    w = 0.012 * side
    mid = 0.5 * (y0 + y1)
    nose = 0.35 * (y1 - y0)
    pts = [(x, y0), (x + w, y0 + 0.001 * span), (x + w, mid - 0.15 * nose),
           (x + 2 * w, mid), (x + w, mid + 0.15 * nose),
           (x + w, y1 - 0.001 * span), (x, y1)]
    ax.add_line(Line2D([p[0] for p in pts], [p[1] for p in pts],
                       color="0.35", lw=1.0, solid_joinstyle="round"))
    return x + 2.4 * w


import re

#  A letter, then whatever belongs in its subscript, then any primes.
#  "T2" -> T_2, "A1g" -> A_{1g}, "A''" -> A'' , "E" -> E.
#  An optional leading count ("1a1", "2t2"), the letter, its subscript,
#  then any primes.  The leading digits are the MO's number and are NOT
#  part of the symbol, so they stay on the line.
#  A WHOLE token, anchored, or it matches inside ordinary words:
#  "Molecular orbitals" came out as "Molec_ular" because "cu" looks
#  like a letter and a subscript.
_IRREP = re.compile(r"^(\d*)([A-Za-z])([0-9]+[gu]?|[gu])?((?:'|\"|'')*)$")


def mathify(text):
    """Set irrep and group names the way they are written in print.

    a1, t2g, A'' are Mulliken symbols, and a diagram that prints them
    flat reads as output rather than as chemistry.  The subscript is
    not decoration: it is what distinguishes A1 from A2.
    """
    def one(m):
        lead, letter, sub, primes = m.groups()
        sub = sub or ""
        if not sub and not primes:
            return lead + letter
        body = letter
        if sub:
            body += "_{%s}" % sub if len(sub) > 1 else "_%s" % sub
        return "%s$%s$%s" % (lead, body, primes)
    #  Token by token, so only things that are entirely a symbol are
    #  set as one.
    return " ".join(_IRREP.sub(one, t) if _IRREP.match(t) else t
                    for t in text.split(" "))


def pretty(group):
    """T_d, C_2v, D_4h -- the way a chemist writes it."""
    if not group:
        return ""
    head, tail = group[:1].upper(), group[1:].lower()
    if not tail:
        return "$%s$" % head
    return "$%s_{%s}$" % (head, tail)


def draw(model, path):
    fig, ax = plt.subplots(figsize=(13, 8.5))
    ax.set_xlim(0, 1)
    ax.axis("off")

    every = [l["energy"] for side in model["levels"]
             for l in model["levels"][side]]
    if not every:
        ax.text(0.5, 0.5, model["note"] or "nothing to draw", ha="center")
        fig.savefig(path, dpi=110, bbox_inches="tight")
        return

    lo, hi = min(every), max(every)
    pad = 0.08 * (hi - lo or 1.0)
    ax.set_ylim(lo - pad, hi + pad)

    #  Energy axis, for the molecular orbitals only.  The fragment
    #  columns are placed by order and spacing, not on this scale.
    ax.annotate("", xy=(0.015, hi + pad), xytext=(0.015, lo - pad),
                arrowprops=dict(arrowstyle="->", color="0.4", lw=1))
    #  The axis belongs to the molecular orbitals.  With no MO column
    #  there is nothing on it but the tabulated fragment energies, and
    #  saying "Hartree" over a column of eV values would be a lie.
    haveMOs = bool(model["levels"].get("centre"))
    ax.text(0.015, hi + pad,
            "  E / Hartree" if haveMOs else "  E / eV  (valence IE)",
            va="bottom", ha="left", fontsize=9, color="0.35")
    span = hi - lo
    for step in (0.02, 0.05, 0.1, 0.2, 0.5, 1, 2, 5, 10, 20, 50):
        if span / step <= 10:
            break
    v = step * round(lo / step)
    while v <= hi + pad:
        if lo - pad <= v <= hi + pad:
            ax.plot([0.008, 0.022], [v, v], color="0.4", lw=0.8)
            #  Rounded to the step, so a tick that should read 0 does
            #  not come out as -5.55e-17.
            ax.text(0.028, v, "%g" % (round(v, 10) + 0.0), va="center",
                    fontsize=8, color="0.35")
        v += step

    for side in ("left", "centre", "right"):
        levels = model["levels"].get(side, [])
        column = model["columns"].get(side, {})
        if not levels:
            continue

        ax.text(X[side], hi + 0.55 * pad, mathify(column.get("title", "")),
                ha="center", va="bottom", fontsize=11, color="0.25")

        half = HALFOF[side]
        #  Every label already placed in this column, as (y, step), so a
        #  new one can be pushed out past ALL of them and not just past
        #  the previous one.  Nitrite has four levels within 0.05
        #  Hartree and a single nudge left three of them on top of each
        #  other.
        placed = []
        for i, level in enumerate(levels):
            spans, leftmost, rank, sharing = slots(levels, i, half)
            y = level["energy"]

            #  Their own heights when they differ enough to see; the
            #  mean when they do not, which is every ordinary case.
            each = level["each"] or [y] * len(spans)
            split = (len(each) > 1 and
                     max(each) - min(each) > 0.012 * span)
            ys = each if split else [y] * len(spans)

            colour = pairColour(level)
            for (x0, x1), ly in zip(spans, ys):
                ax.add_line(Line2D([X[side] + x0, X[side] + x1], [ly, ly],
                                   color=colour, lw=2.4))

            labelY = y
            if split:
                brace(ax, X[side] + spans[-1][1] + 0.004,
                      min(each), max(each), +1, span)
                labelY = 0.5 * (min(each) + max(each))

            #  Electrons: one pair of slots per orbital, up then down.
            per = int(round(level["occ"] / (level["deg"] or 1)))
            for (x0, x1), ly in zip(spans, ys):
                cx = X[side] + 0.5 * (x0 + x1)
                for k in range(min(per, 2)):
                    dx = (-1 if k == 0 else 1) * 0.006
                    up = (0.02 if k == 0 else -0.02) * span
                    ax.annotate("", xy=(cx + dx, ly + up),
                                xytext=(cx + dx, ly - up),
                                arrowprops=dict(arrowstyle="->", color="tab:blue",
                                                lw=1.4))

            out = OUT[side]
            text = level["label"]
            if text and text != "?":
                if sharing == 1:
                    #  Out one notch at a time until this label clears
                    #  every other one at a similar height.
                    step = 0
                    while any(abs(labelY - py) < 0.030 * span and ps == step
                              for py, ps in placed):
                        step += 1
                    placed.append((labelY, step))
                    ax.text(X[side] + out * (half + 0.012 + 0.075 * step),
                            labelY, mathify(text),
                            ha="right" if out < 0 else "left",
                            va="center", fontsize=9, color=colour)
                else:
                    #  Alternating above and below the row, so four
                    #  symmetry sets at one energy stay legible.  They
                    #  really are at one energy -- the split into
                    #  irreps is a classification, not an energy
                    #  splitting -- so they are NOT fanned out
                    #  vertically, which would imply an order that does
                    #  not exist.
                    mid = X[side] + 0.5 * (spans[0][0] + spans[-1][1])
                    above = (rank % 2 == 0)
                    ax.text(mid,
                            (max(ys) + 0.018 * span) if above
                            else (min(ys) - 0.018 * span),
                            mathify(text), ha="center",
                            va="bottom" if above else "top", fontsize=8)
            if level["annot"]:
                #  Under the label, at the label's own offset, so it
                #  moves with it rather than staying put and colliding
                #  with whatever the label was pushed clear of.
                shift = 0.075 * (placed[-1][1] if placed else 0)
                ax.text(X[side] + out * (half + 0.012 + shift),
                        labelY - 0.026 * span, level["annot"],
                        ha="right" if out < 0 else "left",
                        va="center", fontsize=7.5, color="0.55")

        if column.get("hidden"):
            ax.text(X[side], lo - 0.6 * pad,
                    "%d core orbital%s below %.2f Hartree, not shown"
                    % (column["hidden"], "" if column["hidden"] == 1 else "s",
                       column["hiddenMax"]),
                    ha="center", fontsize=8, color="0.5")
        if column.get("hiddenAbove"):
            ax.text(X[side], hi + 0.55 * pad,
                    "%d higher virtual%s not shown"
                    % (column["hiddenAbove"],
                       "" if column["hiddenAbove"] == 1 else "s"),
                    ha="center", fontsize=8, color="0.5")

    #  Correlation lines, under the levels.
    for leftIdx, centreIdx, rightIdx in model["links"]:
        centre = model["levels"]["centre"]
        if not (0 <= centreIdx < len(centre)):
            continue
        cy = centre[centreIdx]["energy"]
        for side, idx in (("left", leftIdx), ("right", rightIdx)):
            levels = model["levels"].get(side, [])
            if not (0 <= idx < len(levels)):
                continue
            fy = levels[idx]["energy"]
            fx = X[side] + (HALFOF[side] if side == "left" else -HALFOF[side])
            cx = (X["centre"] - HALFOF["centre"] if side == "left"
                  else X["centre"] + HALFOF["centre"])
            tint = pairColour(centre[centreIdx])
            ax.add_line(Line2D([fx, cx], [fy, cy],
                               color="0.7" if tint == "black" else tint,
                               alpha=0.45, lw=0.9, ls=(0, (4, 3)), zorder=0))

    ax.set_title(pretty(model["group"]), fontsize=15, fontweight="bold")
    if model["note"]:
        fig.text(0.02, 0.015, model["note"], fontsize=7.5, color="0.45",
                 wrap=True)
    fig.savefig(path, dpi=110, bbox_inches="tight", facecolor="white")


if __name__ == "__main__":
    draw(read(sys.stdin), sys.argv[1])
    print("wrote", sys.argv[1])
