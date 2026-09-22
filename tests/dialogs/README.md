# Code-registration dialog suite

This suite exists because of a second recurring, expensive bug shape in ECCE
-- the GUI-side sibling of the one `tests/parsers` covers:

> a `scripts/codereg/*theory.py` combo list and the `ai.<code>` subroutine
> that translates its selections are **two hand-maintained copies of one
> list**.  Every translator ends in `else { $result = ""; }`, so when they
> drift the `##token##` line is deleted and **the job runs without the
> setting the user picked** -- no error, no warning, nothing in the log.

Nine instances were found by eye in one afternoon (2026-09-21) across
`ged{03,09,16}theory.py`: letter `O` for zero (`MO6HF` vs `M06HF`), a missing
closing paren, wrong case, a stray space, a dialog entry with no generator
case at all, and a missing comma in a Python list, which silently
concatenated two adjacent string literals into one nonsense entry and made
both real functionals unselectable.

Finding them by eye does not scale and does not stay found.  Diffing the two
sets does both, in **both directions**: offered-but-unmapped is a dead
control, mapped-but-unoffered is a capability the UI cannot reach (which is
how Gaussian's double hybrids stayed hidden).

## What it actually runs

    data/client/cap/<Code>.edml
       |   names the theory dialog, the runtype dialog, the input generator,
       |   the template, and every <Theory category= name=> with its runtypes
       v
    scripts/codereg/<code>theory.py  ..  the REAL dialog, run headlessly once
       |                                 per declared theory category, with
       |                                 the REAL argv contract from
       |                                 globals.py; every widget it builds is
       |                                 recorded with its live choice list
       v                                 (union across categories)
    scripts/parsers/ai.<code>  ........  the REAL generator, traced for the
       |                                 finite set of strings it recognises
       v                                 for that GUIValues key
    set difference, both directions

Nothing is stubbed and nothing in `scripts/` is modified: the dialogs are
driven entirely through the interface they already have.

### The dialogs are already a testable interface

Each codereg dialog is a standalone process with a narrow contract that
predates this suite (`scripts/codereg/globals.py`):

    argv    <restoreFile> <udpPort> <GUIValues|NO_GUIValues> <Writable|...>
            <DebugOn|DebugOff> <category> <theory> <runtype> <calcName>
            <rxnFlag> <symmetry> <nElec> <mult> <nFrozen> <nOcc> <nVirt>
            <nModes>
    in      the restore file, a GUIValues dump (name|value|unit|...)
    out     one `name|value|unit|enabled|export|type` line per changed
            widget, over UDP

So no "test mode" had to be added to shipped code, and no new divergent code
path exists to drift from what users hit.  `harness.runDialog()` supplies the
argv, optionally writes a restore file, binds the socket, and returns both
the widget inventory and everything the dialog sent back.

### Headless, with no X server

`broadwayd` -- GTK3's own displayless backend, from `libgtk-3-bin`, present
on a stock Debian desktop -- runs wxPython 4.2.3 / wxWidgets 3.2.7 fine.  The
harness starts one for the run and tears it down after.  `xvfb-run` is used
if `broadwayd` is missing; `$DISPLAY` only if `ECCE_DIALOG_TESTS_USE_DISPLAY`
is set, because that flashes real windows on a real desktop.  With none of
the three the suite **skips** rather than fails.

Needs python3 and wxPython only: no build, no ECCE server, no gateway, no
chemistry codes.

## Running it

    tests/dialogs/run_tests.py                  # everything
    tests/dialogs/run_tests.py -v               # + every key that agreed
    tests/dialogs/run_tests.py --code NWChem
    tests/dialogs/run_tests.py --list
    tests/dialogs/run_tests.py --dump NWChem    # what the dialogs offer

Exit status is 0 only when every check passed.  Also wired into CTest:

    cd build-cmake && ctest -R dialogs --output-on-failure

## Coverage is derived, not listed

There is deliberately no hand-maintained table of which dialog goes with
which generator.  `codes.py` reads `data/client/cap/*.edml`, which already is
that table and is the file ECCE itself reads.  A newly integrated code is
checked the moment its `.edml` lands, and a code whose `.edml` drifts from
its dialogs is caught by the same mechanism.  Thirteen codes are covered
today; `--list` shows which and why the rest are skipped.

## The theory-summary check

The `.edml`'s `<TheorySummary><item key="..."/>` list is a **third**
hand-maintained copy of the same key set, after the dialog and the
generator. It drives the summary shown in CalcEd and the Organizer, so a key
listed there that no dialog ever builds is a summary row that can never show
a value.

Checked one way only: a dialog key absent from the summary is a setting
nobody chose to summarise, which is a decision rather than a bug.

This is why the suite runs each runtype dialog once per declared runtype
rather than sampling one — the runtype dialogs build completely different
controls per runtype (`ES.Runtype.GeomOpt.*` only exist for Geometry, the ESP
ones only for a partial-charge run), and sampling one runtype reported ~30
keys as unbuilt that were simply never looked for.

## The static lint pass

Before any dialog runs, every `scripts/codereg/*.py` is scanned for implicit
string concatenation inside a list literal -- a missing comma.  Python's AST
folds implicit concatenation into one constant before anything can inspect
it, so this works on the token stream.  Six are currently in the tree; see
"Known bugs" below.

## What makes it fail

* a value a dialog offers that the generator does not recognise, **when a
  miss is silent** -- the `else { $result = ""; }` shape, where the setting
  vanishes and the job runs with something other than what is on screen;
* a dialog that cannot be constructed at all;
* a missing comma in a list literal;
* a **stale** allowlist or `XFAIL` entry: every list is checked in both
  directions, so a documented exception that no longer applies is itself a
  failure.  The allowlists cannot rot silently.

Things it *reports* but does not fail on, under "FOR REVIEW":

* a value the generator recognises that no dialog offers -- real, but it
  misleads nobody; it is unreachable capability, not a wrong answer;
* a mismatch where the generator **forwards the string verbatim** (`$map{$x}
  || $x`, or an `else` that concatenates the raw value) or **dies** -- the
  code itself then rejects it loudly, which is a different severity from
  silently substituting a different method.

That split is deliberate: the failures that fire are always the
silently-wrong-answer kind, so a red run always means something users cannot
see.

## What it deliberately cannot check

* Keys with no finite accepted set: matched by regex (`$s =~ /marzari/i`),
  matched by substring (`ai.nwchem`'s `Cosmo()` mixes `eq "Water"` with
  `=~ "Methanol"` against dialog strings like `"Methanol: 33.0 Debye"`), or
  concatenated straight into the deck (`ai.gauss16:1092`).  These are
  reported as having no finite set rather than reporting every offered value
  as unmapped.  `-v` lists them.
* Whether a recognised string is the *right* keyword for the code.  The suite
  checks that the two halves agree, not that they agree on something the
  chemistry code accepts -- that is what `tests/parsers` fixtures and a real
  run are for.
* Numeric and free-text widgets: only fixed-choice widgets have two lists to
  diff.  The harness records all of them, so range and default checks could
  be added here later.
* Everything downstream of the generator.  This suite stops where
  `tests/parsers` starts.

## Bugs found on the first run

All fifteen were fixed in the same session; `cases.py` keeps the list as a
record.  They are worth reading as a description of what this suite is for:

| where | what |
| --- | --- |
| `nedtheory.py` vs `ai.nwchem` | dialog offered `BP86 (GGA)`, generator matched only `BP86 (hybrid)` -- **every NWChem DFT job with BP86 selected silently ran on the default LDA** |
| all four `ai.gauss*` | `"None"` in the combined-XC list had no case, so the method string collapsed to bare `r`/`u`/`ro` |
| `ged{03,09,16,98}theory.py` | `except wx.PyDeadObjectError` -- gone in Phoenix, so the except clause itself raised; **the Gaussian-03 theory dialog could not be opened** |
| `ged03theory.py` | `CheckDependency()` touched SCRF widgets built only for DFT -- a second dialog-killing bug that only became reachable once the one above was fixed |
| `amicatheory.py:349` | `wx.FlexGridSizer(0, 2)` has no Phoenix overload; **the Amica theory dialog could not be opened** |
| `ged{03,09,16}theory.py` x5 | missing comma made `"1-Butanol"` + `"Cyclohexane"` one `"1-ButanolCyclohexane"` entry; both solvents unselectable |
| `nedtheory.py:958` | missing comma in the local-correlation list |

Two of these were only reachable because a dialog is actually *run* here:
the Phoenix leftovers are invisible to `py_compile`, and the second ged03
bug was hidden behind the first.

## Adding to the suite

A new code needs **nothing**: it is picked up from its `.edml`.

When a check fails, it is either a real finding or something to write down in
`cases.py` **with a reason** -- never remove a check to make it pass.  A
genuinely harmless mismatch goes in `KNOWN_UNMAPPED` / `KNOWN_UNOFFERED`; an
open bug goes in `XFAIL` with enough detail that the next person can act on
it without re-deriving it.

## The restore round-trip cases

The second half of the dialog contract, and the other thing nothing
exercised: `calced` dumps a calculation's stored GUIValues to a file and the
dialog is launched in `GUIValues` mode, where `FinalizeSetting()` applies
those `name|value|unit|...` lines back onto the widgets.

That path is where issue #77 lived, in two layers, both silent and both
1000x:

* the stale **label** -- `SetUnit(unit)` overwrote the widget's own correct
  unit with whatever was persisted in the calc's stored data, so a Gaussian
  memory field saved before the GB-everywhere UX change read "Megawords"
  forever;
* the stale **number** -- even with the label fixed, a value saved under the
  old unit was still applied.  An ORCA calc saved when Memory Per Core meant
  MB restored 1000 into the field that now means GB, and `ai.orca` emitted
  `%maxcore 1000000`: 1 TB per core, from a dialog reading "1000 Gigabytes /
  core", with nothing typed in.

`cases.RESTORE_CASES` covers both, plus the two controls that stop a lazy
fix from passing: a *matching* unit must still restore its value, and an
ordinary unit-free combo must still restore normally.

The restore file is never hand-written.  The dialog is first run in
`NO_GUIValues` mode, which makes it emit its own state in exactly this
format, and the case's mutations are applied to those lines -- so the
fixture cannot drift out of the format the real thing produces.

Verified by negative control: reverting the `storedUnit != widgetUnit` guard
in `templates.py` makes both stale-unit cases fail immediately (ORCA
restores 512, clamped by its own `hardRange`, rather than 1; Gaussian
restores 1600 rather than 6).

## Verbose mode

    ./run_tests.py --debug

Launches every dialog with `DebugOn` and echoes its `[ecce-dialog]` lines
inline, under the code and category that produced them.  That is the same
verbose output `calced` can turn on in a real session, so what you see here
is what a user would be asked to send in.

The generators have a matching mode, `ECCE_AI_DEBUG=1`, documented in
GETTING_STARTED.md; it reports what every `##tag##` resolved to and warns
when a translator has no case for a value a dialog offered -- the failure
this suite exists to catch, reported at runtime instead.

## Where this could go next

The harness is more general than the checks built on it.  It records every
widget with its default, value, unit, enabled state and hard/soft range, and
it returns everything the dialog sends back over the socket.  Natural next
steps:

* assert defaults and ranges against the `.edml`'s own `<item key=>` list,
  which is a third hand-maintained copy of the same information;
* drive the dependency logic (`CheckDependency()`) by restoring states that
  should enable or disable other controls;
* extend the missing-comma lint to the other list-literal bug shapes.
