"""Read what ECCE stored for a finished calculation, and the raw trace.

Props/ on the data server is what the Builder's panels read, so it is the
thing under test.  The raw ecce.out trace in the run directory is written by
NWChem itself and serves as the independent reference for "is this the final
geometry's value".
"""

import math
import os
import re
import xml.etree.ElementTree as ET

BOHR = 0.529177210903


def _root(path):
    with open(path) as h:
        return ET.fromstring(h.read())


def floats(text):
    return [float(x) for x in text.split()]


class Props(object):
    def __init__(self, directory):
        self.dir = directory
        self.names = sorted(n for n in os.listdir(directory)
                            if not n.startswith(".")) if os.path.isdir(directory) else []

    def has(self, name):
        return name in self.names

    def value(self, name):
        return float(_root(os.path.join(self.dir, name)).text)

    def vector(self, name):
        return floats(_root(os.path.join(self.dir, name)).text)

    def steps(self, name):
        """[[values] per step] for a tsvector-like property."""
        root = _root(os.path.join(self.dir, name))
        return [root_text(s) for s in root.iter("step")]

    def frames(self, natoms):
        """GEOMTRACE as [(x, y, z) per atom] per frame, in Angstrom."""
        out = []
        for flat in self.steps("GEOMTRACE"):
            vals = floats(flat)
            out.append([tuple(vals[3 * i:3 * i + 3]) for i in range(natoms)])
        return out

    def strings(self, name):
        root = _root(os.path.join(self.dir, name))
        return [(s.text or "").strip() for s in root.iter("step")] or \
            (root.text or "").split()


def root_text(elem):
    return elem.text or ""


# --- the raw trace ---------------------------------------------------------

def trace_blocks(path):
    """{tag: [list of value-lines per occurrence]} from an ecce.out trace."""
    blocks = {}
    current = None
    with open(path, errors="replace") as h:
        for line in h:
            line = line.rstrip("\n")
            m = re.match(r"^(.*)%begin%(.*)%(\d+(?: \d+)*)%(\w+)$", line)
            if m:
                current = (m.group(2), [])
                continue
            m = re.match(r"^(.*)%end%(.*)%(\d+(?: \d+)*)%(\w+)$", line)
            if m and current:
                blocks.setdefault(current[0], []).append(current[1])
                current = None
                continue
            if current is not None:
                current[1].append(line)
    return blocks


def trace_last(blocks, tag):
    occ = blocks.get(tag)
    if not occ:
        return None
    return " ".join(occ[-1]).split()


# --- geometry --------------------------------------------------------------

def dist(a, b):
    return math.dist(a, b)


def angle(a, b, c):
    """Angle a-b-c in degrees."""
    u = [a[i] - b[i] for i in range(3)]
    v = [c[i] - b[i] for i in range(3)]
    cos = sum(x * y for x, y in zip(u, v)) / (math.hypot(*u) * math.hypot(*v))
    return math.degrees(math.acos(max(-1.0, min(1.0, cos))))
