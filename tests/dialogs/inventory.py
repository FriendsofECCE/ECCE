"""
Collect the union of every choice a code's dialogs can offer.

A dialog's combo lists are built conditionally on ``EcceGlobals.Category``
(``ged16theory.py`` appends to ``xcFuncChoice`` only for some categories), so
one run does not see the whole dialog.  This runs it once per
``<Theory category= name=>`` pair declared in the code's ``.edml`` and takes
the union, recording for each offered string which contexts offered it.
"""

import collections

import harness


class Offer(object):
    """One GUIValues key, and everything the dialogs offer for it."""

    def __init__(self, key):
        self.key = key
        self.choices = []                     # in first-seen order
        self.contexts = collections.defaultdict(list)
        self.labels = set()
        self.sites = set()

    def add(self, choices, context, label, site):
        for choice in choices:
            if choice not in self.choices:
                self.choices.append(choice)
            self.contexts[choice].append(context)
        if label:
            self.labels.add(label)
        if site:
            self.sites.add(site)


def collect(display, code, verbose=False):
    """Run every dialog of ``code`` in every declared context.

    Returns ``(offers, failures, allNames)``; ``failures`` are dialogs that crashed or
    would not run, which are findings in their own right -- a dialog that
    cannot even be constructed headlessly is one the user cannot open either.
    """
    offers = {}
    failures = []
    allNames = set()
    for category in code.categories():
        theory = code.firstTheoryOf(category)
        runtypes = code.runtypesOf(category) or ["Energy"]
        jobs = [(code.theoryDialogPath, "theory", {})]
        if code.runtypeDialogPath:
            # EVERY runtype the .edml declares for this category, not just the
            # first. The runtype dialogs build entirely different controls per
            # runtype -- the ES.Runtype.GeomOpt.* widgets only exist for
            # Geometry, the ESP ones only for a partial-charge run -- so
            # sampling one runtype makes every other runtype's keys look as
            # though no dialog builds them.
            for runtype in runtypes:
                jobs.append((code.runtypeDialogPath, "runtype",
                             {"runType": runtype}))
        for path, kind, extra in jobs:
            if not path:
                continue
            context = "%s/%s" % (category, theory)
            if extra.get("runType"):
                context += "/" + extra["runType"]
            try:
                inv = harness.runDialog(display, path, category=category,
                                        theory=theory, **extra)
            except harness.DialogError as exc:
                failures.append("%s %s dialog [%s]: %s"
                                % (code.name, kind, context, exc))
                continue
            if inv.get("error"):
                failures.append("%s %s dialog [%s] raised:\n%s"
                                % (code.name, kind, context,
                                   _indent(inv["error"])))
                continue
            if getattr(display, "debug", False):
                # Echo the dialog's own [ecce-dialog] lines as they happen, so
                # --debug output is interleaved with the context that produced
                # it rather than dumped at the end.
                print("    --- %s [%s]" % (_base(path), context))
                for line in (inv.get("stderr") or "").splitlines():
                    if line.startswith("[ecce-dialog]"):
                        print("      %s" % line)
            for record in inv["widgets"]:
                live = record.get("live") or {}
                name = live.get("name") or record["args"].get("name")
                if name:
                    allNames.add(name)
            for widget in harness.choiceWidgets(inv):
                key = widget["name"]
                if not key:
                    continue
                offer = offers.setdefault(key, Offer(key))
                offer.add(widget["choices"], context, widget["label"],
                          "%s:%s" % (_base(path), widget["line"]))
    return offers, failures, allNames


def _base(path):
    import os
    return os.path.basename(path)


def _indent(text):
    return "\n".join("        " + line for line in text.splitlines())
