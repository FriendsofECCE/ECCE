---
type: map
title: ".dlg grammar and the engine behind it: what each line means and which Python behaviour it reproduces"
area: codereg
paths: ["src/wxgui/details", "src/apps/codereg-dialog/ecce-dialog.cpp", "scripts/codereg/ecceqmtheory.dlg", "scripts/codereg/orcatheory.dlg", "scripts/codereg/orcaruntype.dlg", "tests/dialogs/grammar_test.dlg", "tests/dialogs/native_test.py", "tests/details/details_unit_test.cpp"]
issues: [411]
---
Engine: `src/wxgui/details/` = `DetailsRules` (line parser, expressions, ranges,
Python float formatting; no wx), `DetailsModel` (values, enabled/shown, the
CheckDependency pass, restore, Reset, the lines sent; no wx), `DetailsPanel`
(widgets mirroring the model, plus the UDP transport). `ecce-dialog.cpp` is only
argv and the frame. Tests: `details_unit` (ctest, headless, every construct on
`tests/dialogs/grammar_test.dlg`) and `dialogs_native` (`native_test.py --py X.py
--dlg X.dlg`: UDP lines byte for byte over category/theory/runtype from the
.edml x mult/rxn/nFrozen=-1 x restore cases, plus a diff of `ecce-dialog
--inventory X.dlg <17 argv>` against `_introspect.py`).

**Lines.** `# comment`; `list NAME = a|b|c`; `dialog title= help= minwidth= message=hide`;
`box label= cols= when=`; widgets `check radiopair combo calcpick int float text note`;
`force`, `status`. Items before the first box, or in a box whose `when` is false,
are not built.

**Expressions** (`when`, `enable_if`, `show_if`, `export_if`, `default_if`, `force`/`status when`):
`== != < <= > >=` (numbers compare as numbers), `x in @LIST`, `x not in @LIST`,
`&&`, `||`, `!`, parentheses. The left operand is looked up (argv field, then any
control's value by name; unknown is empty), the right one is a literal unless it
starts with `$` (a variable) or is `@N` (choice N of this combo, in `export_if`).
argv names: restore port restoreflag rw debug category theory runtype calc
calcname rxn symmetry pointgroup nelec mult nfrozen nocc nvirt nmodes.

**Build time (argv only):** `when=`, `default_if="cond => value; cond => value"`
(first true wins), `$name` inside `label soft hard default`. **Live (argv and
controls, file order after every change, like CheckDependency):** `enable_if`,
`show_if`, `force`, `status`.

**Defaults are names, never indexes** (see "A combo whose default is a bare
integer opens BLANK"): a default that is not in the list is reported in the
inventory `warnings` and falls back to the first choice.
`export_if` is added to `export=`; a setting that differs from its default
always exports.

**int / float:** `soft=` warns, `hard=` rejects (float: error, value reverts,
nothing sent; int: the spin box clamps), ranges in the templates.py syntax
`[lo..hi]`, `(lo..hi)`, either end optional, `$nocc` allowed. `unit=` is sent and
is what a stale stored unit is compared with. Float values the user edits are
reformatted as Python `str(float)` (`1e-06`, `1.0`). `default=` of a float is
the literal text sent; write it as Python would (`1.0`).

**Behaviours copied from the wx widgets, easy to lose:** combo/number/text/
calcpick report a change of enabled state as an edit (line sent after a nested
CheckDependency, so a later widget's line can precede it); check/radio do not;
the radio pair always exports; `force ... silent=1` sets the value without a line
(wx SetValue raises no event), without `silent` it behaves as an edit of that
control; depth of nested dependency passes is capped at 8.

**calcpick** = scripts/codereg/calcpick.py: choices `(none)` then `name (kind
structure)` from the tab file in `ECCE_QST_CANDIDATES` (3 fields per line),
stores the URL, empty unit, `menu_input`, default (none).

**text** = EcceTextInput: sends `name|value||enabled|export|text_input` (empty unit
always), sends again when its enabled state changes, like combo/number. inRange
messages print a bound like Python `str()`: an integer bound as `1`, a float
bound as `1.0`/`0.5`.

Not done yet: no C++ hook registry.

**Oracle notes.** Sizer-based Python widgets (combo, text) have no `IsEnabled`,
so the inventory diff skips `enabled` for them; the lines they send cover it.
A `status` goes to the message area on a false-to-true change; the Python NEB
dialog showed it as a label inside the box, so the native window has no empty
line there.
