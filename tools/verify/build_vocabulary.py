#!/usr/bin/env python3
"""
Harvest the words ECCE itself can put in an input deck (GitHub #148).

    tools/verify/build_vocabulary.py [--check]

Writes data/client/config/verify/<Code>.words, one token per line.
--check regenerates into memory and fails if the committed file is out
of date, so the vocabulary cannot silently drift from the generators.

WHY THIS IS NOT A LIST OF WHAT THE CODES ACCEPT
-----------------------------------------------
Collecting every keyword Gaussian or ORCA accepts is a losing game:
ORCA 6 has well over a thousand, Gaussian's route options run to
hundreds, the basis libraries to hundreds more, and all of it moves
between versions. Worse, an incomplete list of that kind produces
FALSE ALARMS -- it condemns every valid keyword it has not heard of --
and a checker that cries wolf gets ignored, including when it is
right.

So this collects something different and much smaller: the words
**ECCE can emit**. That set is closed and it is already in this
repository, in the `ai.<code>` generators, the `.tpl` templates and
the basis-name tables. Anything outside it is, by definition, text a
human typed -- which is exactly what is worth drawing attention to,
and exactly what Final Edit produces.

And it degrades the right way. A token this does not know is reported
UNSURE, never BAD: "I do not recognise this" is a true statement about
a hand-typed keyword, and an incomplete harvest means more amber
rather than false red.

The completeness test is free: every deck ECCE has ever generated
should be recognised in full. tests/verify checks exactly that.
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PARSERS = os.path.join(ROOT, "scripts", "parsers")
OUT = os.path.join(ROOT, "data", "client", "config", "verify")

#  code -> (generator, template, basis writer)
SOURCES = {
    "Gaussian-16": ("ai.gauss16", "g16.tpl", "wrGaussian16GBS.pm"),
    "Gaussian-09": ("ai.gauss09", "g09.tpl", "wrGaussian09GBS.pm"),
    "ORCA":        ("ai.orca",    "orca.tpl", "wrORCAGBS.pm"),
    "NWChem":      ("ai.nwchem",  "nwch.tpl", "wrNWChemGBS.pm"),
    "MOPAC":       ("ai.mopac",   "mopac.tpl", None),
}

#  A literal that is plainly a filesystem path, not deck text.
#
#  This used to reject any literal containing a slash or a space,
#  which threw away "/GEN " -- how ai.gauss16 puts GEN on the route
#  card -- and "def2/J", ORCA's auxiliary basis. Both then showed up
#  as unrecognised words on real decks. Detect paths as paths, and
#  filter the rest per TOKEN, after splitting, where a slash or a
#  space cannot survive anyway.
PATH = re.compile(r"""^(
      [./~] .* /                  # ./x, /x/y, ~/x
    | .* / .* \.\w{1,4}$          # a/b.ext
    )$""", re.VERBOSE)

#  Tokens that are not keywords.
NOISE = re.compile(r"""^(
      \W*                         # punctuation only
    | \d+(\.\d+)?                 # bare numbers
    )$""", re.VERBOSE)


def literals(path):
    """Every quoted literal in a script, which is where its output lives."""
    if not os.path.exists(path):
        return set()
    text = open(path, errors="replace").read()
    found = set()
    for match in re.finditer(r'"([^"\n]{1,60})"', text):
        found.add(match.group(1))
    for match in re.finditer(r"'([^'\n]{1,60})'", text):
        found.add(match.group(1))
    return found


def tokenise(value, splitSlash=True):
    """Split a literal the way that code's keyword line would read it.

    Gaussian uses '/' to separate method from basis, so both halves are
    tokens. ORCA does NOT -- "def2/J" is a single auxiliary-basis
    keyword -- and splitting it produced two words ECCE was then said
    not to recognise, on every ORCA deck that used one.
    """
    if PATH.match(value.strip()):
        return
    for piece in re.split(r"[\s=(),]+", value):
        piece = piece.strip()
        if not piece or NOISE.match(piece):
            continue
        halves = piece.split("/") if splitSlash else [piece]
        for half in halves:
            half = half.strip()
            if half and not NOISE.match(half):
                yield half.lower()


def vocabulary(code):
    generator, template, basis = SOURCES[code]
    words = set()
    splitSlash = code.startswith("Gaussian")

    #  The basis WRITER is a source in its own right, not only through
    #  its name table: it is what puts "Gen", "5D", "7F" and the rest
    #  on a Gaussian route card. Harvesting only the name table left
    #  "GEN" unrecognised on every deck that supplies its own basis --
    #  which is most of them.
    sources = [generator, template]
    if basis:
        sources.append(basis)

    for name in sources:
        for value in literals(os.path.join(PARSERS, name)):
            for token in tokenise(value, splitSlash):
                words.add(token)

    #  The template's own bare words: a .tpl is mostly unquoted text,
    #  and its literal keywords never appear in quotes anywhere.
    path = os.path.join(PARSERS, template)
    if os.path.exists(path):
        for line in open(path, errors="replace"):
            line = re.sub(r"##\w+##", " ", line)      # resolved tags
            for token in tokenise(line, splitSlash):
                words.add(token)

    #  Point group names, which a Gaussian route card carries as
    #  Symmetry=(PG=D2H,Loose) once the user has chosen one. They come
    #  from ECCE's own PointGroups file rather than a list written
    #  here, so a group added there cannot start showing up amber.
    groups = os.path.join(ROOT, "data", "client", "config", "PointGroups")
    if os.path.exists(groups):
        for line in open(groups, errors="replace"):
            line = line.strip()
            if not line or line.startswith("#") or ":" not in line:
                continue
            name = line.split(":", 1)[0].strip()
            if name:
                words.add(name.lower())
                #  Gaussian spells the axial groups with a star.
                words.add(name.lower().replace("inf", "*"))

    return words


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true",
                    help="fail if a committed file is out of date")
    args = ap.parse_args()

    os.makedirs(OUT, exist_ok=True)
    stale = []
    for code in sorted(SOURCES):
        words = sorted(vocabulary(code))
        body = "\n".join(words) + "\n"
        path = os.path.join(OUT, code + ".words")
        if args.check:
            current = open(path).read() if os.path.exists(path) else ""
            if current != body:
                stale.append(code)
            continue
        open(path, "w").write(body)
        print("%-14s %4d words" % (code, len(words)))

    if args.check:
        for code in stale:
            print("STALE  %s.words does not match the generators" % code)
        return 1 if stale else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
