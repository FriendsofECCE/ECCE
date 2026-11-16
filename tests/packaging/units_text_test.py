#!/usr/bin/env python3
"""User-visible text must use the real unit symbols (#998).

Greps the GUI string sources for ASCII spellings of angstrom, degree,
reciprocal centimetre etc.  File formats, parser output and log lines keep
their ASCII spellings; those places, and anything else legitimate, are listed
in units_allow.txt as "<path regex> <TAB> <line regex>" pairs.
No build, GUI or services needed.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

FORMS = re.compile(
    r"\(A\)|\(A,|A\^2|\bAngstroms?\b|\b2theta\b|\bcm-1\b|\beV/A\b|\(deg\)"
    r"|\bdegrees\b")
# A C++ line shows text to the user when it hands a literal to one of these.
GUI = re.compile(
    r"wxStaticText|SetToolTip|SetLabel|InsertColumn|SetColLabelValue|"
    r"\b_\(|FromUTF8|wxButton|wxCheckBox|wxRadioBox|SetValue\(|field\(")
STRING = re.compile(r'"([^"\\]|\\.)*"')


def allowed():
    out = []
    for line in (Path(__file__).parent / "units_allow.txt").read_text().splitlines():
        if line.strip() and not line.startswith("#"):
            p, _, l = line.partition("\t")
            out.append((re.compile(p), re.compile(l or ".")))
    return out


def scan():
    hits = []
    for pat in ("src/apps/**/*.C", "src/wxgui/**/*.C", "src/wxviz/**/*.C"):
        for f in sorted(ROOT.glob(pat)):
            for n, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
                if GUI.search(line) and any(
                        FORMS.search(m.group(0)) for m in STRING.finditer(line)):
                    hits.append((f, n, line.strip()))
    for f in sorted(ROOT.glob("data/client/cap/*.edml")):
        for n, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
            m = re.search(r'label="([^"]*)"', line)
            if m and FORMS.search(m.group(1)):
                hits.append((f, n, line.strip()))
    for f in sorted(ROOT.glob("scripts/codereg/*.dlg")):
        for n, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
            m = re.search(r'\blabel="([^"]*)"', line)
            if m and FORMS.search(m.group(1)):
                hits.append((f, n, line.strip()))
    for f in sorted(ROOT.glob("help/src/*.md")):
        for n, line in enumerate(f.read_text(errors="replace").splitlines(), 1):
            if re.search(r"\bcm-1\b|\bAngstroms?\b|A\^2|\b2theta\b|\beV/A\b", line):
                hits.append((f, n, line.strip()))
    return hits


def main():
    allow = allowed()
    bad = []
    for f, n, line in scan():
        rel = f.relative_to(ROOT).as_posix()
        if any(p.search(rel) and l.search(line) for p, l in allow):
            continue
        bad.append("%s:%d: %s" % (rel, n, line[:160]))
    if bad:
        print("ASCII unit spellings in user-visible text (use Å, Å², °, 2θ, cm⁻¹):")
        print("\n".join(bad))
        return 1
    print("units_text: ok")
    return 0


sys.exit(main())
