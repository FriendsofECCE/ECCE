"""
The set of codes to check, read straight out of ``data/client/cap/*.edml``.

There is deliberately no hand-maintained table of "which dialog goes with
which generator".  The ``.edml`` already is that table -- it names the theory
dialog, the runtype dialog, the input generator, and every
``<Theory category= name=>`` with its nested ``<runtype>`` list -- and it is
the file ECCE itself reads.  Deriving the suite's coverage from it means a
newly integrated code is checked the moment its ``.edml`` lands, with no
second list to forget to update, and a code whose ``.edml`` drifts from its
dialogs is caught by the same mechanism.
"""

import glob
import os
import re
import xml.etree.ElementTree as ET

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
CAP = os.path.join(REPO, "data", "client", "cap")
CODEREG = os.path.join(REPO, "scripts", "codereg")
PARSERS = os.path.join(REPO, "scripts", "parsers")


class Code(object):
    def __init__(self, path):
        self.edml = path
        self.name = os.path.splitext(os.path.basename(path))[0]
        text = open(path, errors="replace").read()
        # Several .edml files are not well-formed enough for a strict parse
        # (and XML comments there historically contained "--", see CLAUDE.md),
        # so read the few attributes needed with regexes rather than making
        # this suite's coverage hostage to that.
        self.theoryDialog = _attr(text, "theorydialog")
        self.runtypeDialog = _attr(text, "runtypedialog")
        gen = re.search(r"<InputGenerator>\s*([^<\s]+)", text)
        self.generator = gen.group(1) if gen else ""
        self.theories = _theories(text)
        self.itemKeys = sorted(set(
            re.findall(r'<item\s+key="([^"]+)"', text)))

    # -- resolved paths, or "" when the piece does not exist ---------------

    @property
    def theoryDialogPath(self):
        return _exists(CODEREG, self.theoryDialog)

    @property
    def runtypeDialogPath(self):
        return _exists(CODEREG, self.runtypeDialog)

    @property
    def generatorPath(self):
        return _exists(PARSERS, self.generator)

    @property
    def checkable(self):
        """Both halves of the dialog/generator pair exist on disk.

        Placeholder entries (``theorydialog="xxx"``) and the codes that are
        declared but not wired up are skipped -- ``CodeFactory::
        getFullySupportedCodes()`` would not offer them either.
        """
        return bool(self.theoryDialogPath and self.generatorPath)

    def categories(self):
        seen = []
        for category, _theory, _runtypes in self.theories:
            if category not in seen:
                seen.append(category)
        return seen

    def firstTheoryOf(self, category):
        for cat, theory, _runtypes in self.theories:
            if cat == category:
                return theory
        return ""

    def runtypesOf(self, category):
        found = []
        for cat, _theory, runtypes in self.theories:
            if cat == category:
                for runtype in runtypes:
                    if runtype not in found:
                        found.append(runtype)
        return found

    def __repr__(self):
        return "<Code %s>" % self.name


def _attr(text, attribute):
    match = re.search(attribute + r'="([^"]*)"', text)
    value = match.group(1) if match else ""
    # "xxx"/"yyy" are this tree's placeholders for a code with no dialog.
    return "" if value in ("", "xxx", "yyy") else value


def _exists(directory, name):
    if not name:
        return ""
    path = os.path.join(directory, name)
    return path if os.path.exists(path) else ""


def _theories(text):
    """[(category, name, [runtype, ...]), ...] in .edml order."""
    found = []
    for match in re.finditer(
            r'<Theory\s+category="([^"]+)"\s+name="([^"]+)"[^>]*>(.*?)</Theory>',
            text, re.S):
        runtypes = re.findall(r'<runtype>\s*([^<]+?)\s*</runtype>',
                              match.group(3))
        found.append((match.group(1), match.group(2), runtypes))
    return found


def allCodes():
    return [Code(p) for p in sorted(glob.glob(os.path.join(CAP, "*.edml")))]


def checkableCodes():
    return [c for c in allCodes() if c.checkable]
