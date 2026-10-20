#!/usr/bin/env python3
"""Style-E code icons for the Calculation Editor's code bar and the
Organizer's calculation tree: GMX, CP/2K, ORCA (an orca), Quantum ESPRESSO
(Q with an espresso cup), MOPAC in two variants.  Letters are strokes, not
<text> (NanoSVG draws none).  Output: data/client/pixmaps/svg/candidates/codes/."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from build_icons_e import Icon  # noqa: E402

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..",
                   "data", "client", "pixmaps", "svg", "candidates", "codes")
ICONS = {}


def icon(name):
    def deco(f):
        ICONS[name] = f
        return f
    return deco


def badge(i, top, bottom, edge):
    g = i.lin(0, 2, 0, 30, (0, top), (1, bottom))
    i.add('<rect x="1.500" y="2" width="29" height="28" rx="5" fill="%s" stroke="%s" stroke-width="1"/>' % (g, edge))
    i.add('<path d="M5 3.500h22" stroke="#ffffff" stroke-opacity=".45" stroke-width="1.200" stroke-linecap="round"/>')


def strokes(i, d, w=2.6, col="#ffffff", dark="#00000055"):
    i.add('<path d="%s" fill="none" stroke="%s" stroke-width="%s" stroke-linecap="round" stroke-linejoin="round" transform="translate(.6 .8)"/>' % (d, dark, w))
    i.add('<path d="%s" fill="none" stroke="%s" stroke-width="%s" stroke-linecap="round" stroke-linejoin="round"/>' % (d, col, w))


@icon("gmx")
def gmx(i):
    badge(i, "#5a8ee0", "#1c3f94", "#14286a")
    strokes(i, "M11.200 12.500C9.500 9.800 4 10 4 16c0 6 5.800 6.500 7.200 3.500V16.500H7.800", 2.4)
    strokes(i, "M13.500 22V10l3.500 7 3.500-7v12", 2.4)
    strokes(i, "M22.500 10l6.500 12M29 10l-6.500 12", 2.4)


@icon("cp2k")
def cp2k(i):
    badge(i, "#46b89a", "#0f5f62", "#0a3c46")
    w = 2.2
    strokes(i, "M10 6.200A3.600 4.400 0 1 0 10 11.800", w)
    strokes(i, "M12.800 14V4.400h2.800a2.600 2.600 0 0 1 0 5.200h-2.800", w)
    strokes(i, "M20.500 14.500l3-10.500", w)
    strokes(i, "M3.500 20.200a3.200 2.800 0 0 1 6.400.6c0 3-6.400 4.800-6.400 7.200h6.600", w)
    strokes(i, "M13.500 18v10M21 18l-7.500 5.500 7.500 4.500", w)


@icon("orca")
def orca(i):
    badge(i, "#8fd0f4", "#2e78c0", "#1c4e8c")
    w = i.rad(16, 30, 14, (0, "#ffffff", .5), (1, "#ffffff", 0))
    i.add('<ellipse cx="16" cy="29" rx="14" ry="3" fill="%s"/>' % w)
    body = i.lin(0, 5, 0, 25, (0, "#4a5262"), (.55, "#14161c"), (1, "#000000"))
    i.add('<path d="M3 19.500C4 14.500 10 12.500 16 12.500H19L21.500 5.500 23 12.500C26 13 28 14.200 29.500 16.500L28 18.200 30 22 26 20.800C22 24 14 25 8 24 5 23.500 3 22 3 19.500z" fill="%s" stroke="#000000" stroke-width=".8" stroke-linejoin="round"/>' % body)
    bel = i.lin(0, 19, 0, 25, (0, "#ffffff"), (1, "#c8d4e4"))
    i.add('<path d="M4 20.500C9 22 16 21.500 24 19.800 20 23.800 12 24.600 8 23.800 5.500 23.200 4 22 4 20.500z" fill="%s"/>' % bel)
    i.add('<path d="M17 13C19 14.500 21 14.500 23 13 21.500 16 18.500 16 17 13z" fill="#aeb6c4"/>')
    i.add('<ellipse cx="10.500" cy="16.300" rx="3" ry="1.300" transform="rotate(-22 10.500 16.300)" fill="#ffffff"/>')
    i.add('<circle cx="8" cy="18" r=".9" fill="#ffffff"/>')
    i.add('<path d="M12 21.500l3 3.500-4.500-1.500z" fill="#0a0a0e"/>')


@icon("qe")
def qe(i):
    # "QE" lettering like CP/2K's; espresso-brown badge for the name
    badge(i, "#c8936a", "#6b3a1c", "#45220c")
    w = 3
    strokes(i, "M9.500 9.200a5.300 6.800 0 1 0 .01 0z", w)
    strokes(i, "M11.300 18.500l3.400 4.600", w)
    strokes(i, "M26.500 9H19.300v13.800h7.200M19.300 15.900h5.800", w)


@icon("ecceqm")
def ecceqm(i):
    # "QM" lettering in the ECCE logo's colours: ECCE's own engine
    badge(i, "#3a2a78", "#120a30", "#0a0620")
    w = 2.8
    green = "#b8f0a0"
    strokes(i, "M8.800 8.800a4.800 6.400 0 1 0 .01 0z", w, green)
    strokes(i, "M10.500 17.800l3 4.200", w, green)
    strokes(i, "M17 22.500V9.200l4.300 7.500 4.300-7.500v13.300", w, green)


@icon("g16")
def g16(i):
    # the original: a large red G with a 1 over a 6 at its right
    badge(i, "#ffffff", "#e8e8ec", "#9a9aa8")
    red, dark = "#e01020", "#00000040"
    strokes(i, "M17.500 9.500C15.500 5.800 4.500 5.500 4.500 16c0 10.500 11 10.500 13 5.500V17h-5.500", 3.600, red, dark)
    strokes(i, "M22.500 7.800l2.300-1.600v8", 2.200, red, dark)
    strokes(i, "M28 17.600c-1-.9-4.400-.9-4.400 3.400v2.200a2.200 2.200 0 0 0 4.400 0v-.4a2.200 2.200 0 0 0-4.400 0", 2.200, red, dark)


@icon("mopac-a")
def mopac_a(i):
    badge(i, "#f0a04a", "#a8480c", "#6c2c04")
    w = 2.2
    strokes(i, "M3.800 14V4.500l3.200 5 3.200-5V14", w)
    strokes(i, "M16 9.200a3.600 4.700 0 1 0 7.200 0 3.600 4.700 0 1 0-7.200 0z", w)
    strokes(i, "M3.800 28V18.500h3a2.800 2.800 0 0 1 0 5.600h-3", w)
    strokes(i, "M11 28l3.200-9.500L17.400 28M12 25h4.400", w)
    strokes(i, "M26.500 19.500A3.600 4.600 0 1 0 26.500 26.500", w)


@icon("mopac-b")
def mopac_b(i):
    badge(i, "#f0a04a", "#a8480c", "#6c2c04")
    # a small bent molecule, ball and stick: the molecule motif for a semi-empirical code
    for (x, y) in ((8, 23), (24, 23)):
        i.add('<path d="M16 12L%s %s" stroke="#4a2008" stroke-width="4.400" stroke-linecap="round"/>' % (x, y))
        i.add('<path d="M16 12L%s %s" stroke="#f2e0c8" stroke-width="2.200" stroke-linecap="round"/>' % (x, y))
    i.sphere(8, 23, 4.200, "#ffffff", "#c8d4e4", "#6a7890", rim="#4a5870")
    i.sphere(24, 23, 4.200, "#ffffff", "#c8d4e4", "#6a7890", rim="#4a5870")
    i.sphere(16, 11, 6, "#ffd0d0", "#e02828", "#7c0808", rim="#5a0404")


def build():
    os.makedirs(OUT, exist_ok=True)
    for name, f in ICONS.items():
        i = Icon()
        f(i)
        with open(os.path.join(OUT, name + ".svg"), "w") as fh:
            fh.write(i.svg())
    # GROMACS keeps the study icon already chosen for it.
    import shutil
    shutil.copy(os.path.join(OUT, "..", "..", "study-gromacs.svg"),
                os.path.join(OUT, "gmx.svg"))
    print("wrote", len(ICONS), "icons to", os.path.normpath(OUT))


if __name__ == "__main__":
    build()
