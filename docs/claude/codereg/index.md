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
The gaps the second code found; the ORCA checklist held up.
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
Each of these bug shapes has hit more than one code or file; when one
turns up, grep the other codes for the same shape.
### Memory fields: the #77 unit-label fix
Memory is entered and labelled in GB for every code; each code's own
unit is produced in `ai.<code>`/the `.tpl`, closest to the input deck.

## Entries

### Getting a calculation set up (the "code registration" system)

- [A GROMACS MD study has the application type of its tasks; NWChem's study has its own](gromacs-study-shares-its-tasks-application-type.md)
- [EDML control file](edml-control-file.md)
- [`scripts/codereg`](scripts-codereg.md)
- [`scripts/parsers`](scripts-parsers.md)
- [`scripts/gensub`](scripts-gensub.md)

### How a property gets from disk into the Properties menu

- [Most property keys are deliberately *not* panel triggers](most-property-keys-are-deliberately-not-panel.md)
- [A `.desc` bracket group's names are labels, not the stored key](labels-not-the-stored-key.md)
- [NWChem's `.desc` `Begin` patterns match the `ecce_print` trace file, not NWChem's stdout](nwchems-desc-begin-patterns-begin-end-task.md)
- [The one gotcha that has bitten this project repeatedly](the-one-gotcha-that-has-bitten-this.md)

### New-code checklist (gotchas found integrating ORCA, issue #38)

- [Don't trust `ai.<code>`'s own comments for the `.frag`/`.param`/ `.basis` format](dont-trust-ai-code-s-own-comments.md)
- [Keep `ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` in step](keep-resourcedescriptor-xml-and-resourcedescriptorrxn-xml-in.md)
- [`<DataFiles>` filenames need a distinctive extension](datafiles-filenames-need-a-distinctive-extension.md)
- [`<LaunchPreprocessor>` is required, unconditionally](launchpreprocessor-is-required-unconditionally.md)
- [ECCE-QM is a bundled code: where it is wired, and the per-code flags it introduced](ecce-qm-is-a-bundled-code.md)
- [`rdStandardGBS.pm` accepts indented NameBasis lines and an optional `print`; do not re-fix the `*.expt` writers](rdstandardgbs-pms-namebasis-format-used-to-have.md)
- [No `CMakeLists.txt install()` changes needed](no-cmakelists-txt-install-changes-needed.md)
- [An `.expt`'s `.param` keys and `.frag` attributes are read literally; a wrong key is dropped silently (#235)](expt-param-keys-are-read-literally.md)
- [A spherical-only code must say `<SphericalOnly>` in its EDML, or its basis is recorded Cartesian (#239)](spherical-only-code-must-say-so-in-edml.md)

### Added from integrating MOPAC (issue #86) — the second code through

- [`scripts/gensub` needs a `sub <lccode>()` per code](scripts-gensub-needs-a-sub-lccode-per.md)
- [Let the *code* dictate the input filename, not the checklist](let-the-code-dictate-the-input-filename.md)
- [Test the `.desc` against a molecule with degenerate vibrations](test-the-desc-against-a-molecule-with.md)
- [Run *every* runtype's output through the simulation, not just the richest one](run-every-runtypes-output-through-the-simulation.md)
- [XML comments may not contain `--`](xml-comments-may-not-contain.md)
- [The MO diagram needs the code's MO coefficients and basis, with their AO order verified, not its population analysis](the-mo-diagram-needs-the-codes-mo.md)
- [Keep `len(TEVEC) <= len(GEOMTRACE)`](keep-lentevec-lengeomtrace.md)
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
- [An empty finding list leaves the checker's lamp blank, not green](blank-not-green.md)
- [Do not move the checker's automatic run into `enableLaunch()`/`enableAllFields()`](do-not-move-the-automatic-run-into.md)

### Pitfalls found more than once (check for siblings)

- [Verify keywords, basis names and output formats by running the code, not by reading its manual](verify-by-running-the-code.md)
- [A code's theory NAME must not equal its CATEGORY](a-codes-theory-name-must-not-equal.md)
- [CalcEd used to discard the input generator's error message](calced-used-to-discard-the-input-generators.md)
- [A parser with nothing able to request its input is dead code, and the suite will not tell you](a-parser-with-nothing-able-to-request.md)
- [ORCA double hybrids pass the input check and need a `<basis>/C` auxiliary basis to run](the-code-accepted-the-keyword-is-not.md)
- [`ai.<code>`'s template engine silently swallows `die()`](ai-code-s-template-engine-silently-swallows.md)
- [Never point an `ai.<code>` script's `-t` at the repo's own template](never-point-an-ai-code-scripts-t.md)
- [Codereg dialogs are never told which elements the structure contains](codereg-dialogs-are-never-told-which-elements.md)
- [CalcEd's own ES.Theory.UseSymmetry key is lost whenever p_GUIValues is replaced](calced-owns-use-symmetry-key.md)
- [A dialog's choice string must match the generator's expected string EXACTLY, or selecting it silently emits nothing](a-dialogs-choice-string-must-match-the.md)
- [The basis-set writers decide *how* a basis reaches the deck, and every bug in them is silent](the-basis-set-writers-decide-how-a.md)
- [A Gaussian basis keyword is selectable only if the library has an entry; tools/basissets/g16_basis_dump.py makes one from the keyword](a-gaussian-basis-keyword-needs-a-library-entry.md)
- [A combo whose default is a bare integer opens BLANK when the list is built conditionally](a-combo-whose-default-is-a-bare.md)
- [NWChem aborts on three offered functionals and rejects its documented dispersion spellings](verify-a-codes-keyword-list-by-running.md)
- [Retired codes are not maintained, and the suite no longer checks them](retired-codes-are-not-maintained-and-the.md)
- [A `.desc` entry's `Begin` wording can silently stop matching between versions of the same code](a-desc-entrys-begin-wording-can-silently.md)
- [`eccejobmonitor`'s "enable all parse types" check is case-sensitive](eccejobmonitors-enable-all-parse-types-check-is.md)

### Memory fields: the #77 unit-label fix

- [The memory field's unit label comes from the dialog, never from stored GUIValues (#77)](live-verified-and-closed-2026-09-22.md)

### Machine configuration (CONFIG files)

- [`CONFIG.<machine>` is the site file, then the user file, merged per key (C++ and gensub alike)](config-machine-is-site-then-user-per-key.md)
- [`GENSUB_EXPLAIN=1 gensub` prints each CONFIG key's effective value and where it came from](gensub-explain-config-provenance.md)
- [`ConfigFile` reads and writes `CONFIG.<m>`; `MachineConfigDraft` is what Register Machines edits](configfile-writer-and-draft.md)
- [Register Machines is a tabbed frame over a draft; `ECCE_MACHREG_SCRIPT` drives the real window](register-machines-frame-and-test-hook.md)
- [Register Machines asks the scheduler (discover, test) and previews the job script from the unsaved form](register-machines-queue-tools.md)
- [Queue defaults, the memory unit is MB, and the Launcher's Machine settings button](queue-defaults-and-launcher-machine-settings.md)
- [`ecce -admin -remote` saves on the central server over ssh (`ecce-site-admin`), publishes, then refreshes the client's copy](site-admin-from-a-client.md)
- [The Launcher lists a machine for a code when its Machines line lists the code or CONFIG gives it a path](launcher-machine-list-per-code.md)
- [`eccejobmonitor` polls every 2 s for a local job without a queue manager, 10 s otherwise](eccejobmonitor-poll-interval.md)
- [A parser script can be handed half a line: post mode delivers a final line with no newline](parser-scripts-must-survive-a-final-line-cut-short.md)
- [ORCA 6.1.1: no orca_scf next to orca (Find), and the Intel Mac build needs macOS 12.3](orca-6-companion-file-and-macos-minimum.md)
