#!/usr/bin/env python3
"""
ECCE code-registration dialog suite.

Runs the real ``scripts/codereg`` dialogs headlessly, in every theory
category their ``.edml`` declares, and diffs what each dialog OFFERS against
what the matching ``ai.<code>`` generator RECOGNISES.

Why
---
A combo list in ``*theory.py`` and the ``eq "..."`` cases in ``ai.<code>``
are two hand-maintained copies of one list, and every translator ends in
``else { $result = ""; }``.  When they drift, the generated ``##token##``
line is deleted and **the job runs without the setting the user picked** --
no error, no warning, nothing in the log.  Nine instances were found by eye
in one afternoon (2026-09-21); this finds them mechanically, in both
directions, and keeps them found.

It also exercises something nothing else does: whether each dialog can be
constructed at all.  Two dialogs that raise during construction -- and so
cannot be opened by a user either -- were found the first time this ran.

    ./run_tests.py                  everything
    ./run_tests.py -v               also list keys that matched cleanly
    ./run_tests.py --code NWChem    one code
    ./run_tests.py --list           list codes
    ./run_tests.py --dump NWChem    print what the dialogs offer, and exit

Needs python3 + wxPython, and a headless display: ``broadwayd`` (from
libgtk-3-bin, present on a stock Debian desktop) or ``xvfb``.  No build, no
ECCE server, no gateway, no chemistry codes.  Exit status is 0 only when
every check passed.
"""

import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import cases as CASEDEFS                                       # noqa: E402
import codes as CODES                                          # noqa: E402
import harness                                                 # noqa: E402
import inventory                                               # noqa: E402
import resources                                               # noqa: E402
import siteconfig                                              # noqa: E402
import shellcmd                                                # noqa: E402
import restore                                                 # noqa: E402
import lint                                                    # noqa: E402
from perlmap import Fallback, Generator                        # noqa: E402


class Results(object):
    def __init__(self):
        self.failures = []
        self.review = []
        self.notes = []
        self.xfails = []
        self.xpasses = []
        self.checks = 0
        self.seenXfail = set()
        self.usedUnmapped = set()
        self.usedUnoffered = set()
        self.usedSummaryKey = set()

    def fail(self, where, message):
        self.failures.append("%s: %s" % (where, message))

    def check(self):
        self.checks += 1


def templateTags(code):
    """``##tag##`` names in the code's template, per its .edml <Template>."""
    match = re.search(r"<Template>\s*([^<\s]+)",
                      open(code.edml, errors="replace").read())
    if not match:
        return set()
    path = os.path.join(CODES.PARSERS, match.group(1))
    if not os.path.exists(path):
        return set()
    return set(re.findall(r"##([^#]+)##",
                          open(path, errors="replace").read()))


def checkCode(display, code, results, verbose=False):
    offers, failures, allNames, blanks = inventory.collect(display, code)

    for blank in blanks:
        results.check()
        results.fail(code.name, "combo opens blank:\n    " + blank)

    for failure in failures:
        # A dialog that will not construct is a finding in its own right.
        script = failure.split()[1] if len(failure.split()) > 1 else ""
        key = _xfailKey(code.name, _dialogOf(code, failure), "")
        if key in CASEDEFS.XFAIL:
            results.seenXfail.add(key)
            results.xfails.append((key, failure))
        else:
            results.fail(code.name, "dialog failed to construct:\n    " + failure)
        results.check()

    # The .edml's <TheorySummary><item key=...> list is a THIRD hand-maintained
    # copy of the same key set (after the dialog and the generator).  It drives
    # the theory summary shown in CalcEd and the Organizer, so a key here that
    # no dialog ever builds is a summary row that can never display a value.
    # Checked one way only: a dialog key absent from the summary is simply a
    # setting nobody chose to summarise, which is a decision, not a bug.
    for key in code.itemKeys:
        results.check()
        if key in allNames:
            continue
        if key in CASEDEFS.KNOWN_UNBUILT_SUMMARY_KEYS.get(code.name, {}):
            results.usedSummaryKey.add((code.name, key))
            continue
        results.review.append(
            "%s %s: LISTED IN %s's <TheorySummary> BUT NO DIALOG BUILDS IT\n"
            "      so that summary row can never show a value"
            % (code.name, key, os.path.basename(code.edml)))

    generator = Generator(code.generatorPath)
    tags = templateTags(code)

    for key, offer in sorted(offers.items()):
        trace = generator.acceptedForKey(key, tags)
        results.check()

        if key in CASEDEFS.FREEFORM_KEYS or not trace.enumerable:
            if verbose:
                results.notes.append(
                    "%-16s %-42s no finite accepted set (%s)"
                    % (code.name, key,
                       (trace.untraced + trace.freeform
                        or ["value used verbatim"])[0]))
            continue

        offered = set(offer.choices)
        unmapped = sorted(offered - trace.literals)
        unoffered = sorted(trace.literals - offered)

        allowUnmapped = CASEDEFS.KNOWN_UNMAPPED.get((code.name, key), {})
        allowUnoffered = CASEDEFS.KNOWN_UNOFFERED.get((code.name, key), {})

        for value in unmapped:
            if value in allowUnmapped:
                results.usedUnmapped.add((code.name, key, value))
                continue
            _report(results, code, key, value, trace, offer,
                    "OFFERED BY THE DIALOG BUT NOT RECOGNISED BY %s"
                    % os.path.basename(code.generatorPath))
        for value in unoffered:
            if value in allowUnoffered:
                results.usedUnoffered.add((code.name, key, value))
                continue
            _report(results, code, key, value, trace, offer,
                    "RECOGNISED BY %s BUT NOT OFFERED BY ANY DIALOG"
                    % os.path.basename(code.generatorPath), unreachable=True)

        if verbose and not unmapped and not unoffered:
            results.notes.append("%-16s %-42s %3d values agree"
                                 % (code.name, key, len(trace.literals)))


def _report(results, code, key, value, trace, offer, headline,
            unreachable=False):
    xkey = _xfailKey(code.name, key, value)
    detail = ("%s\n      %s\n      %-20s %s\n      dialog: %s\n"
              "      generator: %s"
              % (headline, "%r" % value,
                 "on a miss:", _explain(trace.severity, unreachable),
                 ", ".join(sorted(offer.sites)),
                 ", ".join(sorted(trace.sites)[:3])))
    where = "%s %s" % (code.name, key)
    if xkey in CASEDEFS.XFAIL:
        results.seenXfail.add(xkey)
        results.xfails.append((xkey, "%s: %s" % (where, detail)))
        return
    if unreachable or trace.severity != Fallback.SILENT:
        # Real, but not the catastrophic shape: a value the UI cannot reach,
        # or a generator that passes the string through / dies loudly.
        # Reported for review rather than failing the run, so that the
        # failures that DO fire are always the silent-wrong-answer kind.
        results.review.append("%s: %s" % (where, detail))
        return
    results.fail(where, detail)


def _explain(severity, unreachable):
    if unreachable:
        return ("nothing -- this is a capability the dialog cannot reach")
    return {
        Fallback.SILENT:
            "the setting is SILENTLY DROPPED and the job runs without it",
        Fallback.DEFAULTS_EMPTY:
            "nothing -- this resolver only emits a keyword for the "
            "non-default values, so an unmatched one IS the default",
        Fallback.PASSTHROUGH:
            "the string is passed through verbatim for the code to judge",
        Fallback.DIES:
            "the generator dies with an error (visible since #92)",
    }.get(severity, "unclear -- the generator's fallback could not be read")


def _dialogOf(code, failure):
    return (os.path.basename(code.theoryDialogPath)
            if " theory " in failure else
            os.path.basename(code.runtypeDialogPath or ""))


def _xfailKey(codeName, key, value):
    return (codeName, key, value)


def checkStaleAllowlists(results, skipCodes=()):
    """Every allowlist entry must still apply.  A documented exception that
    no longer holds is a failure, exactly as in tests/parsers.

    ``skipCodes`` are codes whose dialogs were not run at all this pass --
    retired ones, or a narrowed ``--code`` selection.  Their entries were
    never given the chance to apply, so silence there is not staleness.
    """
    for (codeName, key), values in CASEDEFS.KNOWN_UNMAPPED.items():
        if codeName in skipCodes:
            continue
        for value in values:
            if (codeName, key, value) not in results.usedUnmapped:
                results.fail("cases.py",
                             "stale KNOWN_UNMAPPED entry %s / %s / %r -- it no "
                             "longer applies; remove it"
                             % (codeName, key, value))
    for (codeName, key), values in CASEDEFS.KNOWN_UNOFFERED.items():
        if codeName in skipCodes:
            continue
        for value in values:
            if (codeName, key, value) not in results.usedUnoffered:
                results.fail("cases.py",
                             "stale KNOWN_UNOFFERED entry %s / %s / %r -- it no "
                             "longer applies; remove it"
                             % (codeName, key, value))
    for key in CASEDEFS.XFAIL:
        if key[0] in skipCodes:
            continue
        if key not in results.seenXfail:
            results.xpasses.append(
                "%s / %s / %r no longer reproduces -- remove it from XFAIL"
                % key)


def checkResources(results, verbose=False):
    """Resource-graph consistency: which codes are reachable, and from where.

    Needs no display and no dialog run, so it goes first -- a code that is not
    registered cannot be reached however well its dialogs behave.
    """
    def report(where, message):
        results.check()
        results.fail(where, message)

    loaded = resources.check(report)
    results.check()

    #  siteconfig/: the machine list, the queue registry and the import
    #  verifypatterns.  Same shape of failure as the resource graph -- silent,
    #  or fatal at startup -- and equally checkable without a display.
    siteconfig.check(report)
    results.check()

    #  Shell commands built in C++ whose redirect depends on an inherited
    #  working directory. Costs nothing to run and caught a real
    #  all-codes launch failure the day it was written.
    shellcmd.check(report)
    results.check()

    unreachable = resources.wiredButUnreachable(loaded)
    for name in unreachable:
        if name not in CASEDEFS.NOT_IN_MENU:
            results.fail(
                "resource graph",
                "%s is fully wired (dialog, generator and template) but is "
                "not offered in the New Calculation menu by either descriptor."
                "\n      Either it was disconnected without being recorded, or "
                "a <ContainsResource> entry is missing.  If the disconnection "
                "is intended, add it to cases.NOT_IN_MENU with the reason."
                % name)
    for name, why in sorted(CASEDEFS.NOT_IN_MENU.items()):
        if name not in unreachable:
            results.fail(
                "cases.py",
                "NOT_IN_MENU lists %s, but it is reachable (or no longer "
                "fully wired).  Remove the entry, or find out who re-added "
                "it.\n      Recorded reason was: %s" % (name, " ".join(why.split())))
    if verbose:
        results.notes.append("not in the New Calculation menu: %s"
                             % ", ".join(unreachable))


def checkRestore(display, results, verbose=False):
    """Run every GUIValues restore round-trip case."""
    for case in CASEDEFS.RESTORE_CASES:
        results.check()
        where = "restore/%s" % case["name"]
        overrides = {k: v for k, v in case.items()
                     if k in harness.ARGV_FIELDS}
        try:
            got, _before, _after = restore.roundTrip(
                display, case["script"], case["changes"], **overrides)
        except harness.DialogError as exc:
            results.fail(where, "%s\n      %s" % (exc, _why(case)))
            continue
        for name, expected in case["expect"].items():
            if name not in got:
                results.fail(where, "the dialog built no widget named %s"
                             % name)
                continue
            live = got[name]
            for attribute, want in expected.items():
                if attribute not in live:
                    results.fail(
                        where,
                        "%s exposes no %r; this widget reports: %s"
                        % (name, attribute, ", ".join(sorted(live))))
                    continue
                have = live.get(attribute)
                if have != want:
                    results.fail(
                        where,
                        "%s.%s is %r after restore, expected %r\n      %s"
                        % (name, attribute, have, want, _why(case)))
        if verbose:
            results.notes.append("%-28s restored as expected" % case["name"])


def _why(case):
    return " ".join(case.get("why", "").split())


def checkLint(results):
    """Static lint over every codereg script, before any dialog runs."""
    seen = set()
    for path, line, message in lint.lintDialogs(CODES.CODEREG,
                                                CASEDEFS.KNOWN_CONCATENATION):
        results.check()
        key = (os.path.basename(path), line)
        seen.add(key)
        where = "%s:%s" % (os.path.basename(path), line)
        if key in CASEDEFS.CONCATENATION_XFAIL:
            results.xfails.append((("lint", key), "%s: %s" % (where, message)))
            CASEDEFS.XFAIL[("lint", key)] = CASEDEFS.CONCATENATION_XFAIL[key]
            results.seenXfail.add(("lint", key))
        else:
            results.fail(where, message)
    generatorPath = os.path.join(os.path.dirname(CODES.CODEREG),
                                 "parsers", "ai.nwchem")
    if os.path.exists(generatorPath):
        for number, value in lint.camOnXcLine(generatorPath):
            results.check()
            results.fail("ai.nwchem:%s" % number,
                         "\"cam\" run onto the xc line -- nwchem rejects this "
                         "with \"xc_input: invalid format\" and the job dies.\n"
                         "      %s\n"
                         "      cam is a dft directive of its own; put it on "
                         "its own line." % value)

    for key in CASEDEFS.CONCATENATION_XFAIL:
        if key not in seen:
            results.xpasses.append(
                "%s:%s no longer has a missing comma -- remove it from "
                "CONCATENATION_XFAIL" % key)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--code", action="append",
                        help="only this code (repeatable)")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--dump", metavar="CODE",
                        help="print what a code's dialogs offer, then exit")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("--debug", action="store_true",
                        help="launch every dialog with DebugOn and print its "
                             "[ecce-dialog] troubleshooting output")
    args = parser.parse_args()

    allCodes = CODES.checkableCodes()
    if args.list:
        for code in CODES.allCodes():
            state = "checked" if code.checkable else "skipped (no dialog or "\
                                                    "no generator)"
            print("%-20s %-18s %-16s %s"
                  % (code.name, code.theoryDialog or "-",
                     code.generator or "-", state))
        return 0

    selected = [c for c in allCodes
                if not args.code or c.name in args.code
                or (args.dump and c.name == args.dump)]
    #  Retired codes are left on disk so their old calculations still open,
    #  but they are not maintained -- checking them only buries live findings
    #  under ones nobody intends to fix.  Naming one explicitly still works.
    if not args.code:
        selected = [c for c in selected if c.name not in CASEDEFS.RETIRED]
    if args.dump:
        selected = [c for c in allCodes if c.name == args.dump]
        if not selected:
            print("no such code: %s" % args.dump, file=sys.stderr)
            return 2

    try:
        display = harness.Display(debug=args.debug).__enter__()
    except harness.HarnessUnavailable as exc:
        print("SKIP: %s" % exc)
        return 0

    results = Results()
    try:
        print("display: %s" % display.kind)
        if args.dump:
            code = selected[0]
            offers, failures, allNames, blanks = inventory.collect(display, code)
            for failure in failures:
                print("FAILED TO RUN: %s" % failure)
            for key, offer in sorted(offers.items()):
                print("\n%s   [%s]" % (key, ", ".join(sorted(offer.labels))))
                for choice in offer.choices:
                    print("    %-50s %s"
                          % (choice, ",".join(sorted(set(offer.contexts[choice])))))
            return 0

        if not args.code:
            checkResources(results, verbose=args.verbose)
            checkLint(results)
            checkRestore(display, results, verbose=args.verbose)
        for code in selected:
            print("  %-20s %s" % (code.name, code.theoryDialog), flush=True)
            checkCode(display, code, results, verbose=args.verbose)
        if not args.code:
            ran = set(c.name for c in selected)
            checkStaleAllowlists(
                results,
                skipCodes=set(CASEDEFS.RETIRED) | (
                    set(c.name for c in allCodes) - ran))
    finally:
        display.__exit__(None, None, None)

    return report(results, args.verbose)


def report(results, verbose):
    if verbose and results.notes:
        print("\n--- detail " + "-" * 58)
        for note in results.notes:
            print("  " + note)

    if results.review:
        print("\n--- FOR REVIEW (not failures) " + "-" * 39)
        print("  Real mismatches, but not the silently-wrong-answer shape:\n"
              "  a value the UI cannot reach, or a generator that forwards\n"
              "  the string verbatim rather than dropping it.\n")
        for item in results.review:
            print("  * " + item + "\n")

    if results.xfails:
        print("\n--- KNOWN BUGS (reproduced, not failing the run) " + "-" * 21)
        for key, detail in results.xfails:
            print("  * %s" % detail)
            print(_wrap(CASEDEFS.XFAIL[key]))

    if results.xpasses:
        print("\n--- STALE XFAILS " + "-" * 52)
        for item in results.xpasses:
            print("  * " + item)

    if results.failures:
        print("\n--- FAILURES " + "-" * 56)
        for item in results.failures:
            print("  * " + item + "\n")

    ok = not results.failures and not results.xpasses
    print("\n%s  %d checks, %d failures, %d known bugs, %d for review"
          % ("PASS" if ok else "FAIL", results.checks, len(results.failures),
             len(results.xfails), len(results.review)))
    return 0 if ok else 1


def _wrap(text):
    lines = [line.strip() for line in text.strip().splitlines()]
    return "\n".join("        " + line for line in lines)


if __name__ == "__main__":
    sys.exit(main())
