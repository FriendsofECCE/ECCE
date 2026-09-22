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

    Returns ``(offers, failures, allNames, blanks)``; ``failures`` are dialogs
    that crashed or would not run, which are findings in their own right -- a
    dialog that cannot even be constructed headlessly is one the user cannot
    open either.  ``blanks`` are combos left with no selection at all, see
    below.
    """
    offers = {}
    failures = []
    allNames = set()
    blanks = []
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
                #  A combo whose default index is out of range for the list it
                #  actually ended up with.  wx.Choice.SetSelection() silently
                #  ignores an out-of-range index, so the control opens blank
                #  and GetValue() returns "" -- which the generators translate
                #  to nothing at all, with no error anywhere.  These lists are
                #  built conditionally on the category, so an index that is
                #  fine for one category can be past the end for another, and
                #  only running every context shows it.
                if widget["choices"] and widget["selection"] in (-1, None):
                    blanks.append(
                        "%s %s [%s]: %r has no selection -- default %r is out "
                        "of range for its %d choice(s): %s"
                        % (code.name, kind, context,
                           widget["label"] or widget["name"],
                           widget["default"], len(widget["choices"]),
                           ", ".join(widget["choices"][:6])))
                key = widget["name"]
                if not key:
                    continue
                offer = offers.setdefault(key, Offer(key))
                offer.add(widget["choices"], context, widget["label"],
                          "%s:%s" % (_base(path), widget["line"]))
    return offers, failures, allNames, blanks


def _base(path):
    import os
    return os.path.basename(path)


def _indent(text):
    return "\n".join("        " + line for line in text.splitlines())
