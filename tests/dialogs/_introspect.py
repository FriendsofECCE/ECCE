"""
Runs INSIDE a codereg dialog process; never imported by the test runner.

The bootstrap does three things, in this order:

  1. import ``templates`` (which imports ``globals``, which constructs the
     ``wx.App`` and parses ``sys.argv`` -- so ``sys.argv`` must already be
     exactly what the real dialog would have been launched with);
  2. monkeypatch every ``Ecce*`` widget class in ``templates`` that takes a
     ``name=`` parameter, so each construction records its arguments, and
     patch ``EccePanel.FinalizeSetting`` to snapshot live widget state once
     the panel is fully built;
  3. ``runpy`` the real dialog script, unmodified, as ``__main__``.

Nothing in ``scripts/codereg`` is altered on disk.  The dialog scripts build
their choice lists conditionally (``ged16theory.py`` appends to
``xcFuncChoice`` depending on ``EcceGlobals.Category``), which is exactly why
this introspects a real, running dialog rather than parsing the Python source.

``EcceFrame.Finalize`` is neutered: in ``GUIValues`` mode it would call
``app.MainLoop()`` and never return.  The snapshot is taken in
``FinalizeSetting`` instead, which runs at the end of ``AddButtons()`` --
after the restore-from-file pass, so a restored value is visible here.

Communication is by environment variable so that ``sys.argv`` stays pristine:
    ECCE_HARNESS_SCRIPT   absolute path of the dialog script to run
    ECCE_HARNESS_OUT      where to write the JSON inventory
"""

import inspect
import json
import os
import runpy
import sys
import traceback

SCRIPT = os.environ["ECCE_HARNESS_SCRIPT"]
OUT = os.environ["ECCE_HARNESS_OUT"]
CODEREG = os.path.dirname(os.path.abspath(SCRIPT))

sys.path.insert(0, CODEREG)
os.chdir(CODEREG)

import templates                                              # noqa: E402
from templates import EcceGlobals, wx                         # noqa: E402

# widget object -> the record dict describing it, in construction order
RECORDS = []
LIVE = []


def _capture(cls, name):
    original = cls.__init__
    try:
        sig = inspect.signature(original)
    except (TypeError, ValueError):
        return

    def patched(self, *args, **kwargs):
        # Record BEFORE construction: if the constructor raises we still know
        # which widget was being built, which is most of the diagnosis.
        rec = {
            "widget": name,
            "line": _dialog_lineno(),
            "args": _jsonable(kwargs),
        }
        RECORDS.append(rec)
        original(self, *args, **kwargs)
        LIVE.append((self, rec))

    patched.__name__ = "__init__"
    cls.__init__ = patched


def _dialog_lineno():
    """Line in the dialog script that constructed this widget."""
    for frame in inspect.stack():
        if os.path.abspath(frame.filename) == os.path.abspath(SCRIPT):
            return frame.lineno
    return None


def _jsonable(obj):
    if isinstance(obj, dict):
        return {k: _jsonable(v) for k, v in obj.items()}
    if isinstance(obj, (list, tuple)):
        return [_jsonable(v) for v in obj]
    if isinstance(obj, (str, int, float, bool)) or obj is None:
        return obj
    return repr(obj)


def _snapshot():
    """Read live state back off each constructed widget.

    Everything is wrapped: these are real wx objects in a half-torn-down
    process, and a widget that cannot answer is worth recording as such
    rather than losing the whole inventory to one exception.
    """
    for widget, rec in LIVE:
        live = {}
        for key, getter in (
            ("name", "GetName"),
            ("value", "GetValue"),
            ("selection", "GetSelection"),
            ("stringSelection", "GetStringSelection"),
            ("enabled", "IsEnabled"),
        ):
            fn = getattr(widget, getter, None)
            if fn is None:
                continue
            try:
                live[key] = _jsonable(fn())
            except Exception:
                pass
        # The real strings the user can pick, straight off the wx.Choice --
        # not the `choices=` argument, so a later Append() would show up.
        combo = getattr(widget, "combo", None)
        if combo is not None:
            try:
                live["choices"] = [combo.GetString(i)
                                   for i in range(combo.GetCount())]
            except Exception:
                pass
        for attribute, key in (("unitString", "unit"),
                               ("hardRange", "hardRange"),
                               ("softRange", "softRange")):
            try:
                live[key] = _jsonable(getattr(widget, attribute))
            except Exception:
                pass
        try:
            live["default"] = _jsonable(widget.default)
        except Exception:
            pass
        try:
            live["export"] = _jsonable(widget.export)
        except Exception:
            pass
        rec["live"] = live


def _dump(error=None):
    _snapshot()
    payload = {
        "script": SCRIPT,
        "argv": sys.argv,
        "globals": {
            "Category": EcceGlobals.Category,
            "Theory": EcceGlobals.Theory,
            "RunType": EcceGlobals.RunType,
            "RestoreFlag": EcceGlobals.RestoreFlag,
            "SymmetryGroup": EcceGlobals.SymmetryGroup,
            "NumElectrons": EcceGlobals.NumElectrons,
        },
        "widgets": RECORDS,
        "batch": list(EcceGlobals.BatchValues),
        "error": error,
    }
    with open(OUT, "w") as handle:
        json.dump(payload, handle, indent=1)


for _name in dir(templates):
    _cls = getattr(templates, _name)
    if not (isinstance(_cls, type) and _name.startswith("Ecce")):
        continue
    try:
        _params = inspect.signature(_cls.__init__).parameters
    except (TypeError, ValueError):
        continue
    if "name" in _params:
        _capture(_cls, _name)

# Snapshot at the end of every panel build, so the inventory survives even if
# the dialog later dies; and neuter the two calls that would block forever.
_origFinalizeSetting = templates.EccePanel.FinalizeSetting


def _finalizeSetting(self):
    _origFinalizeSetting(self)
    _dump()


templates.EccePanel.FinalizeSetting = _finalizeSetting
templates.EcceFrame.Finalize = lambda self: None

try:
    runpy.run_path(SCRIPT, run_name="__main__")
except SystemExit:
    pass
except BaseException:
    _dump(error=traceback.format_exc())
    raise
_dump()
