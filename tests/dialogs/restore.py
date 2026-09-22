"""
Round-trip a dialog through its own GUIValues restore path.

The other half of the dialog contract.  ``calced`` dumps the calculation's
stored GUIValues to a file, the dialog is launched in ``GUIValues`` mode, and
``EccePanel.FinalizeSetting()`` walks that file applying
``name|value|unit|enabled|export|type`` lines back onto the widgets.

That restore is where issue #77 lived, in two layers:

  * the stale *label*: ``SetUnit(unit)`` overwrote the widget's own freshly
    correct unit with whatever unit was persisted in the calc's stored data,
    so a Gaussian memory field saved before the GB-everywhere UX change kept
    reading "Megawords" forever;
  * the stale *number*: even with the label fixed, ``SetValue()`` still
    applied a value saved under the old unit.  An ORCA calc saved when
    "Memory Per Core" meant MB (default 1000) restored 1000 into the field
    that now means GB, and ai.orca emitted ``%maxcore 1000000`` -- 1 TB per
    core, from a dialog reading "1000 Gigabytes / core".  Confirmed live on a
    real job, with nothing typed into the field.

Both are silent, both are 1000x, and neither is visible in the dialog.  The
fix keeps the widget's default whenever the stored unit differs from the
widget's current one, and never restores the unit label.  Nothing exercised
that until now.

The restore file is not hand-written: the dialog is first run in
``NO_GUIValues`` mode, which makes it emit its own state in exactly this
format, and the case's mutations are applied to those lines.  So the fixture
cannot drift out of the format the real thing produces.
"""

import harness

FIELDS = ("name", "value", "unit", "enabled", "export", "type")


def defaultLines(display, script, **overrides):
    """The dialog's own freshly constructed state, in restore-file format."""
    overrides["restoreFlag"] = "NO_GUIValues"
    inventory = harness.runDialog(display, script, **overrides)
    lines = []
    for line in inventory["batch"]:
        line = line.rstrip("\n")
        if line.startswith("#") or line.count("|") != 5:
            continue
        if line not in lines:
            lines.append(line)
    return lines, inventory


def mutate(lines, changes):
    """Rewrite the value and unit of named fields.

    ``changes`` maps a GUIValues name to ``(value, unit)``; ``None`` for
    either leaves that field alone.  A name that matches nothing is an error,
    not a silent no-op -- a case that mutates a field the dialog does not
    build is testing nothing.
    """
    seen = set()
    out = []
    for line in lines:
        parts = line.split("|")
        name = parts[0]
        if name in changes:
            seen.add(name)
            value, unit = changes[name]
            if value is not None:
                parts[1] = str(value)
            if unit is not None:
                parts[2] = unit
        out.append("|".join(parts))
    missing = set(changes) - seen
    if missing:
        raise harness.DialogError(
            "restore case mutates field(s) the dialog does not build: %s"
            % ", ".join(sorted(missing)))
    return out


def roundTrip(display, script, changes, **overrides):
    """Run the dialog, mutate its own output, restore it, and report back.

    Returns ``(restored, before, after)``: the live widget state keyed by
    GUIValues name after the restore, the inventory of the defaults run, and
    the inventory of the restore run.
    """
    lines, before = defaultLines(display, script, **overrides)
    overrides.pop("restoreFlag", None)
    after = harness.runDialog(display, script, restore=mutate(lines, changes),
                              **overrides)
    if after.get("error"):
        raise harness.DialogError("restore run raised:\n%s" % after["error"])
    restored = {}
    for record in after["widgets"]:
        live = record.get("live") or {}
        name = live.get("name") or record["args"].get("name")
        if name:
            restored[name] = live
    return restored, before, after
