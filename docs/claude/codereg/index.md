---
type: index
title: "Code registration: adding and maintaining a computational code"
area: codereg
---
# Code registration: adding and maintaining a computational code

Read this before touching `data/client/cap/*.edml`, `scripts/codereg`, `scripts/parsers` (`ai.*`, `*.expt`, `*.desc`, `wr*GBS.pm`), `scripts/gensub`, `scripts/eccejobmonitor` or the input checker. Split out of CLAUDE.md 2026-10-04.

### Getting a calculation set up (the "code registration" system)
This is ECCE's central extension mechanism — how a new computational
chemistry code (NWChem, Gaussian, GAMESS-UK, ...) gets wired into the
GUI, and the thing most feature work in this codebase touches. See
`code_reg_slides.pdf` on the wiki (2008 PNNL deck, still architecturally
accurate) for the full pipeline; short version:

#### How a property gets from disk into the Properties menu
Worth knowing before concluding "property X can never display":
`PropertyTask::propertyNames()` (`src/dsm/edsiimpl/PropertyTask.C`)
lists the calc's `Props/` collection over WebDAV — so the menu is
driven by **what's actually on disk**, never a static list. Those keys
go to `PropertyPanelFactory::getPanelNamesForProperties()`
(`src/apps/builder/`), which matches them against
`data/client/config/PropertyPanelDescriptor.xml`, where a panel claims
keys by literal `name=`, or by `class=`/`type=`/`indexedBy=` resolved
through `data/client/config/properties`. `Builder::createPropertyPanel()`
then instantiates each matched panel — and **silently drops it, menu
entry and all, if `panel->isRelevant(propCalc)` returns false**
(`Builder.C`), which is why a missing menu item is not evidence that
extraction failed.
#### New-code checklist (gotchas found integrating ORCA, issue #38)
Six real bugs surfaced adding one new code, none obvious from reading
`ai.<code>`/`*.expt` alone — check these explicitly for the next one
(GAMESS-US, Dalton, ...) rather than re-discovering them live:
#### Added from integrating MOPAC (issue #86) — the second code through
  this checklist. Everything above held up; these are the gaps it found.
Separately (found the same way, but not code-registration-specific,
so don't expect it to recur per-code): `VDoc::isCurrentVdoc()` had a
version-string parsing bug that broke saving *any* calc's input file
on this build, misread as a new-code-specific issue at first because
it was hit while testing one. Already fixed — if a save fails with
"input file copy to DAV failed" / 409 Conflict on a build after this
fix, it's a genuinely new problem, not a repeat of that one.

#### The input checker (`scripts/parsers/verifyinput`, #148)
Run automatically whenever `CalcEd::generateInput()` writes a deck and
when a calculation is opened, and on demand from CalcEd's **Verify**
button; the result is a lamp beside that button and, on a click, a
dialog showing the deck with the offending lines marked.
`tests/verify` covers both the script and the dialog's construction.
### Pitfalls found more than once (check for siblings)
### Memory fields: the #77 unit-label fix

## Entries

### Getting a calculation set up (the "code registration" system)

- [EDML control file](edml-control-file.md)
- [`scripts/codereg`](scripts-codereg.md)
- [`scripts/parsers`](scripts-parsers.md)
- [`scripts/gensub`](scripts-gensub.md)

### How a property gets from disk into the Properties menu

- [Most property keys are deliberately *not* panel triggers](most-property-keys-are-deliberately-not-panel.md)
- [labels, not the stored key](labels-not-the-stored-key.md)
- [NWChem's `.desc` `Begin` patterns (`%begin%`/`%end%`/`task_*`) don't match raw NWChem stdout at all, and look bizarre if you try](nwchems-desc-begin-patterns-begin-end-task.md)
- [The one gotcha that has bitten this project repeatedly](the-one-gotcha-that-has-bitten-this.md)

### New-code checklist (gotchas found integrating ORCA, issue #38)

- [Don't trust `ai.<code>`'s own comments for the `.frag`/`.param`/ `.basis` format](dont-trust-ai-code-s-own-comments.md)
- [Two parallel resource-graph files, not one](two-parallel-resource-graph-files-not-one.md)
- [`<DataFiles>` filenames need a distinctive extension](datafiles-filenames-need-a-distinctive-extension.md)
- [`<LaunchPreprocessor>` is required, unconditionally](launchpreprocessor-is-required-unconditionally.md)
- [`rdStandardGBS.pm`'s "NameBasis" format used to have two undocumented requirements — FIXED in `78bb8d0`, don't "re-fix" the `*.expt` writers for it](rdstandardgbs-pms-namebasis-format-used-to-have.md)
- [No `CMakeLists.txt install()` changes needed](no-cmakelists-txt-install-changes-needed.md)

### Added from integrating MOPAC (issue #86) — the second code through

- [`scripts/gensub` needs a `sub <lccode>()` per code](scripts-gensub-needs-a-sub-lccode-per.md)
- [Let the *code* dictate the input filename, not the checklist](let-the-code-dictate-the-input-filename.md)
- [Test the `.desc` against a molecule with degenerate vibrations](test-the-desc-against-a-molecule-with.md)
- [Run *every* runtype's output through the simulation, not just the richest one](run-every-runtypes-output-through-the-simulation.md)
- [XML comments may not contain `--`](xml-comments-may-not-contain.md)
- [The MO diagram needs the code's MO coefficients and basis, and their AO order verified — not any population analysis from the code](the-mo-diagram-needs-the-codes-mo.md)
- [Keep `len(TEVEC) <= len(GEOMTRACE)`](keep-lentevec-lengeomtrace.md)
- [Keep `ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` in step](keep-resourcedescriptor-xml-and-resourcedescriptorrxn-xml-in.md)
- [A `.desc` parse-type's `Begin` value is also its hash key, AND its match priority](a-desc-parse-types-begin-value-is.md)
- [`End`-line starvation: an `End` that lands on another entry's `Begin` line makes that entry unable to EVER fire](end-line-starvation-an-end-that-lands.md)
- [A `.desc` entry's `Skip=N` counts the `Begin`-matching line itself](a-desc-entrys-skip-n-counts-the.md)

### The input checker (`scripts/parsers/verifyinput`, #148)

- [It checks the SHAPE of a deck and must never check a keyword](it-checks-the-shape-of-a-deck.md)
- [Gaussian ships `testrt`, its own route-card parser — use it in `tests/verify`, never in the shipped script](gaussian-ships-testrt-its-own-route-card.md)
- [Check what is INSIDE a section, not only that it exists](check-what-is-inside-a-section-not.md)
- [Charge/multiplicity/electron parity is the one "forbidden combination" that needs no knowledge of any code](charge-multiplicity-electron-parity-is-the-one.md)
- [A hand edit is the case the checker most needs to see and the one it missed](a-hand-edit-is-the-case-the.md)
- [A "row of primitives" check must be anchored](a-row-of-primitives-check-must-be.md)
- [`execout()` returns false on that exit status](execout-returns-false-on-that-exit-status.md)
- [blank, not green](blank-not-green.md)
- [- Do not move the automatic run into `enableLaunch()`/`enableAllFields()` — th](do-not-move-the-automatic-run-into.md)

### Pitfalls found more than once (check for siblings)

- [A code's theory NAME must not equal its CATEGORY](a-codes-theory-name-must-not-equal.md)
- [CalcEd used to discard the input generator's error message](calced-used-to-discard-the-input-generators.md)
- [A parser with nothing able to request its input is dead code, and the suite will not tell you](a-parser-with-nothing-able-to-request.md)
- ["The code accepted the keyword" is not "the keyword works."](the-code-accepted-the-keyword-is-not.md)
- [`ai.<code>`'s template engine silently swallows `die()`](ai-code-s-template-engine-silently-swallows.md)
- [Never point an `ai.<code>` script's `-t` at the repo's own template](never-point-an-ai-code-scripts-t.md)
- [Codereg dialogs are never told which elements the structure contains](codereg-dialogs-are-never-told-which-elements.md)
- [A dialog's choice string must match the generator's expected string EXACTLY, or selecting it silently emits nothing](a-dialogs-choice-string-must-match-the.md)
- [The basis-set writers decide *how* a basis reaches the deck, and every bug in them is silent](the-basis-set-writers-decide-how-a.md)
- [A combo whose default is a bare integer opens BLANK when the list is built conditionally](a-combo-whose-default-is-a-bare.md)
- [Verify a code's keyword list by running the code, not by reading its manual](verify-a-codes-keyword-list-by-running.md)
- [Retired codes are not maintained, and the suite no longer checks them](retired-codes-are-not-maintained-and-the.md)
- [A `.desc` entry's `Begin` wording can silently stop matching between versions of the same code](a-desc-entrys-begin-wording-can-silently.md)
- [`eccejobmonitor`'s "enable all parse types" check is case-sensitive](eccejobmonitors-enable-all-parse-types-check-is.md)

### Memory fields: the #77 unit-label fix

- [Live-verified and closed 2026-09-22](live-verified-and-closed-2026-09-22.md)

### Machine configuration (CONFIG files)

- [`CONFIG.<machine>` is the site file, then the user file, merged per key (C++ and gensub alike)](config-machine-is-site-then-user-per-key.md)
- [`GENSUB_EXPLAIN=1 gensub` prints each CONFIG key's effective value and where it came from](gensub-explain-config-provenance.md)
- [`ConfigFile` reads and writes `CONFIG.<m>`; `MachineConfigDraft` is what Register Machines edits](configfile-writer-and-draft.md)
- [Register Machines is a tabbed frame over a draft; `ECCE_MACHREG_SCRIPT` drives the real window](register-machines-frame-and-test-hook.md)
- [`ecce -admin -remote` saves on the central server over ssh (`ecce-site-admin`), publishes, then refreshes the client's copy](site-admin-from-a-client.md)
- [The Launcher lists a machine for a code when its Machines line lists the code or CONFIG gives it a path](launcher-machine-list-per-code.md)
