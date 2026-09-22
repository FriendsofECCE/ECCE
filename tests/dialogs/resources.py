"""
Checks on the resource-graph files that decide which codes exist at all.

Three separate, silent gates decide whether a code is reachable, and CLAUDE.md
records that missing any of them "silently omits the code, with no error
anywhere":

  1. `data/client/cap/<Code>.edml` must declare both <InputGenerator> and
     <Template> -- that is what CodeFactory::getFullySupportedCodes() checks,
     and it drives CalcEd's code-switch buttons;
  2. `ResourceDescriptor.xml` AND `ResourceDescriptorRxn.xml` must each carry
     the code's <ResourceType> block *and* a <ContainsResource> entry on
     `project` -- that is what puts "New <Code> Calculation..." in the menu;
  3. the two files must agree, because which one is loaded depends on whether
     `bin/dirdyed` exists (ResourceDescriptor.C switches on it).

Gate 3 is the one that bites.  Before this check existed,
`ResourceDescriptor.xml` was missing `gaussian09_es`, `gaussian16_es` and
`orca_es` while the Rxn variant had them -- latent only because `dirdyed` is
installed on this build.  Dropping `dirdyed` would have made three live codes
vanish from the New Calculation menu at once, with nothing to explain it.

These checks need no dialog, no display and no build; they are pure file
consistency.
"""

import os
import re
import xml.etree.ElementTree as ET

import codes as CODES

CONFIG = os.path.join(CODES.REPO, "data", "client", "config")
DESCRIPTORS = ("ResourceDescriptor.xml", "ResourceDescriptorRxn.xml")


class Descriptor(object):
    def __init__(self, name):
        self.name = name
        self.path = os.path.join(CONFIG, name)
        self.error = None
        self.types = {}          # resource name -> DescriptorFile (may be "")
        self.contains = {}       # resource name -> [contained names]
        try:
            root = ET.parse(self.path).getroot()
        except ET.ParseError as exc:
            # XML comments may not contain "--", which the prose style used
            # throughout this tree uses constantly; that has silently made
            # these files non-well-formed before (MOPAC integration, #86).
            self.error = str(exc)
            return
        for element in root.iter("ResourceType"):
            name = element.get("name") or ""
            descriptor = element.find(".//DescriptorFile")
            self.types[name] = (descriptor.text or "").strip() \
                if descriptor is not None and descriptor.text else ""
            held = [c.get("name") for c
                    in element.findall(".//Contains/ContainsResource")]
            self.contains[name] = [h for h in held if h]

    def menuCodes(self):
        """The `*_es` types a project node offers -- the New Calc menu."""
        return [n for n in self.contains.get("project", [])
                if n.endswith("_es")]

    def referenced(self):
        found = set()
        for held in self.contains.values():
            found.update(held)
        return found


def check(report):
    """Run every resource-graph check, calling ``report(where, message)``."""
    loaded = {}
    for name in DESCRIPTORS:
        descriptor = Descriptor(name)
        loaded[name] = descriptor
        if descriptor.error:
            report(name, "is not well-formed XML: %s\n      (XML comments may "
                         "not contain '--', which has broken these files "
                         "before)" % descriptor.error)
    live = [d for d in loaded.values() if not d.error]
    if not live:
        return loaded

    # (a) the two files must offer the same codes
    if len(live) == 2:
        first, second = live
        menuFirst, menuSecond = set(first.menuCodes()), set(second.menuCodes())
        for missing, present, absent in (
                (menuFirst - menuSecond, first, second),
                (menuSecond - menuFirst, second, first)):
            for name in sorted(missing):
                report("resource graph",
                       "%s is offered by %s but NOT by %s.\n"
                       "      Which file is loaded depends on whether "
                       "bin/dirdyed exists, so this code appears or vanishes "
                       "from the New Calculation menu depending on an "
                       "unrelated binary."
                       % (name, present.name, absent.name))

    # (b) no dangling references
    for descriptor in live:
        for name in sorted(descriptor.referenced() - set(descriptor.types)):
            report(descriptor.name,
                   "<ContainsResource name=\"%s\"/> names a resource type "
                   "that file does not define" % name)

    # (c) every menu-registered code must actually be wired up
    byDescriptorFile = {}
    for code in CODES.allCodes():
        byDescriptorFile[os.path.basename(code.edml)] = code
    for descriptor in live:
        for name in descriptor.menuCodes():
            edml = descriptor.types.get(name, "")
            if not edml:
                report(descriptor.name,
                       "%s is in the New Calculation menu but names no "
                       "<DescriptorFile>" % name)
                continue
            code = byDescriptorFile.get(edml)
            if code is None:
                report(descriptor.name,
                       "%s names <DescriptorFile>%s</DescriptorFile>, which "
                       "is not in data/client/cap" % (name, edml))
                continue
            missing = []
            if not code.generator:
                missing.append("<InputGenerator>")
            if not _hasTemplate(code):
                missing.append("<Template>")
            if missing:
                report(descriptor.name,
                       "%s is in the New Calculation menu but %s lacks %s, so "
                       "CodeFactory::getFullySupportedCodes() will not accept "
                       "it" % (name, edml, " and ".join(missing)))
    return loaded


def wiredButUnreachable(loaded):
    """Codes that could be offered but are not -- informational.

    Deliberately not a failure: a code being disconnected is usually a
    decision, not a bug (see data/client/config/disabled-codes-archive.txt).
    Listing them keeps that decision visible rather than accidental.
    """
    offered = set()
    for descriptor in loaded.values():
        if descriptor.error:
            continue
        for name in descriptor.menuCodes():
            edml = descriptor.types.get(name, "")
            if edml:
                offered.add(edml)
    found = []
    for code in CODES.allCodes():
        edml = os.path.basename(code.edml)
        if edml in offered:
            continue
        if code.generator and _hasTemplate(code) and code.theoryDialog:
            found.append(code.name)
    return sorted(found)


def _hasTemplate(code):
    return bool(re.search(r"<Template>\s*\S",
                          open(code.edml, errors="replace").read()))
