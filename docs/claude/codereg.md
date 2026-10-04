# Code registration: adding and maintaining a computational code

Read this before touching `data/client/cap/*.edml`, `scripts/codereg`, `scripts/parsers` (`ai.*`, `*.expt`, `*.desc`, `wr*GBS.pm`), `scripts/gensub`, `scripts/eccejobmonitor` or the input checker. Split out of CLAUDE.md 2026-10-04.

## Getting a calculation set up (the "code registration" system)
This is ECCE's central extension mechanism — how a new computational
chemistry code (NWChem, Gaussian, GAMESS-UK, ...) gets wired into the
GUI, and the thing most feature work in this codebase touches. See
`code_reg_slides.pdf` on the wiki (2008 PNNL deck, still architecturally
accurate) for the full pipeline; short version:

- **EDML control file** (`data/client/cap/*.edml`) — one per code, an
  XML manifest naming every other piece below (`<InputGenerator>`,
  `<Template>`, `<ParseSpecification>`, `<BasisSetTranslationScript>`,
  theory/runtype categories, GUI dialog script names). `data/client/
  config/ResourceDescriptor.xml`/`ResourceDescriptorRxn.xml` reference a
  code's `.edml` to put a "New \<Code\> Calculation..." entry in the
  menu — that's what makes a code actually reachable.
  `CodeFactory::getFullySupportedCodes()` (`src/dsm/xml/CodeFactory.C`)
  is the authoritative "is this code wired up" check (needs both
  `<InputGenerator>` and `<Template>`).
- **`scripts/codereg`** — one `*theory.py` + `*runtype.py` pair per code
  (the "Theory/Runtype Details" dialogs). Python 3 / wxPython Phoenix.
  `globals.py`/`templates.py` are the shared base every script imports —
  most framework-level bugs live there, not in the per-code scripts.
- **`scripts/parsers`** — Perl. `ai.<code>`/`std2<Code>` generates the
  input file before submission; `*.expt` parses full job output into the
  `.frag`/`.basis`/`.param` files ECCE reads back. The **`*.desc`** file
  tells ECCE when a property appears in output, which script extracts
  it, and `Frequency=first|last|all` (get this wrong → silently wrong
  geometry step, see #6/#7). `data/client/config/properties` maps
  property keys to their GUI representation (~250 entries; find one
  close to what you need rather than inventing a new shape).
- **`scripts/gensub`** / **`scripts/eccejobmonitor`** — job-submission
  script generation and remote progress reporting.

### How a property gets from disk into the Properties menu
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
- **Most property keys are deliberately *not* panel triggers** — a
  panel is triggered by one key and reads its companions itself. `MO`
  triggers `MoPanel`, which then reads `ORBENG`/`ORBENGBETA`/`ORBOCC`/
  `ORBOCCBETA`/`ORBSYM`/`MOBETA` directly; `VIB` triggers `NModePanel`,
  which reads `VIBFREQ`/`VIBIR`/`VIBRAM`; `MULLIKEN` triggers
  `MullikenPanel`, which reads `MLKNSHELL`. Cross-referencing emitted
  keys against `PropertyPanelDescriptor.xml` alone therefore reports a
  pile of false "no display path" gaps — grep `src/apps/builder/*.C`
  for the key before believing one.
- A `.desc` bracket group's key names are **labels, not the stored
  key** — what actually gets stored is whatever the parser script
  `print`s as `key: NAME`. The two can disagree (see `[TGRADCPVEC]` in
  `nwchem.desc`, whose script emits `EGRADVEC`), so audit the scripts'
  emissions, not the `.desc` brackets.
- **NWChem's `.desc` `Begin` patterns (`%begin%`/`%end%`/`task_*`) don't
  match raw NWChem stdout at all, and look bizarre if you try** — they
  match a *separate*, machine-tagged trace file that NWChem writes via
  its own built-in `ecce_print <file>` directive (still supported as of
  NWChem 7.2.3, confirmed live), wired up by `nwch.tpl`'s `ecce_print
  ##parseFile##` line and `nwchem.launchpp` (rewrites that line to an
  absolute path at launch time). This *is* the file `eccejobmonitor`
  actually reads for NWChem — auditing `nwchem.desc` against plain
  stdout instead will wrongly conclude the whole file is dead.
- **The one gotcha that has bitten this project repeatedly**: every file
  under `scripts/*` needs its own `install()` rule in `CMakeLists.txt`,
  or it's simply absent from the packaged `.deb` — correct in the repo,
  "command not found" only when installed. Hit independently for
  `std2NWChem`, `processmachine`, `gensub`, `eccejobmonitor`, and
  `scripts/codereg`. If a script mysteriously isn't found only in the
  packaged app, check `CMakeLists.txt` first.

### New-code checklist (gotchas found integrating ORCA, issue #38)
Six real bugs surfaced adding one new code, none obvious from reading
`ai.<code>`/`*.expt` alone — check these explicitly for the next one
(GAMESS-US, Dalton, ...) rather than re-discovering them live:
- **Don't trust `ai.<code>`'s own comments for the `.frag`/`.param`/
  `.basis` format** — verify against the actual C++ writers instead:
  `ESInputController.C`'s `write_cs()`/`write_setup()`/
  `write_gbsconfig()` (`CalcEd::write_setup` in particular — the real
  key list, e.g. `Category`/`Theory`/`RunType`/`Charge`/
  `ChemSys.Multiplicity`, plus whatever `GUIValues::dumpKeyVals()`
  exports from the theory/runtype dialogs). A sibling code's own
  scripts can be stale or written against slightly different
  assumptions than what the GUI actually emits today.
- **Two parallel resource-graph files, not one**: `ResourceDescriptor.
  xml` *and* `ResourceDescriptorRxn.xml` (used for reaction-study
  projects) each need the new code's full `<ResourceType>` block *and*
  a `<ContainsResource name="..._es"/>` entry on `project` — missing
  either makes the "New Calculation" menu (and, in the Rxn file,
  the CalcEd code-switch toolbar) silently omit the code, with no
  error anywhere. `CodeFactory::getFullySupportedCodes()` (used by
  CalcEd's code-switch *buttons*) auto-discovers `.edml` files by
  directory scan; the "New..." *menu* (`CalcMgr::getContextMenu`/
  `SessionContextPanel.C`) instead reads the clicked node's own
  `ResourceType::getContains()` — two different mechanisms, only one
  of which is automatic.
- **`<DataFiles>` filenames need a distinctive extension**, not
  something generic like `.in`/`.out` — Apache has no built-in MIME
  mapping for those, so the uploaded file's `Content-Type` silently
  becomes `DefaultType text/plain`, and ECCE's mimetype-filtered
  "find the primary input file" lookup then can't match it against
  the `.edml`'s declared mimetype *even though the file exists on
  disk* — looks exactly like a save failure, isn't one. Add a matching
  `AddType` line to `packaging/dataserver/httpd.conf.ecce` (see the
  other codes' `.g16in`/`.nw`/`.gki`-style entries there). This is a
  per-user template resolved into `$STATEDIR/httpd.conf` fresh on
  every `ecce-dataserver-start` — but only when the dataserver isn't
  already running (early exit if the port's listening), so testing a
  `httpd.conf.ecce` change needs `ecce-dataserver-stop` first, not
  just an app relaunch.
- **`<LaunchPreprocessor>` is required, unconditionally** —
  `Launch::postProcessInput()` shells out to `JCode::launchPPScript()`
  with no empty-string check, so a code without one fails launch with
  a bare `-p postParams` (empty command name) "command not found",
  which reads like a shell/PATH problem but isn't. Even a trivial
  script is needed if there's nothing to post-process. This script,
  not the input generator, is also the *correct* place for genuinely
  launch-time-only info the input generator can't know at edit time
  (processor count, scratch dir path) — it receives a `postParams`
  dictionary with the real values (see `nwchem.launchpp` for the
  pattern: read `-p <paramfile>`, rewrite the already-generated input
  file in place, idempotently).
- **`rdStandardGBS.pm`'s "NameBasis" format used to have two
  undocumented requirements — FIXED in `78bb8d0`, don't "re-fix" the
  `*.expt` writers for it.** The original PNNL parser matched
  `/^basis \"(\w\w) basis\" (\w+) print/` and
  `/^(\w+)\s+library\s+(\".+\")$/i`, so the `basis "ao basis" <type>`
  line needed a literal trailing `print` keyword and `<atom> library
  "<name>"` lines could not be indented — which every existing
  `*.expt`'s writer violated, silently translating named-library basis
  assignments to nothing with no error. `78bb8d0` relaxed both
  (`(\s+print)?`, and `^\s*` on each), so indented lines and a missing
  `print` are now both accepted. What *is* still required: the library
  line must end immediately after the quoted name (no trailing text),
  and the coordinants token must be a bare word (`spherical`/
  `cartesian`). Verified against the current file 2026-09-21.
- **No `CMakeLists.txt install()` changes needed** for a new code's
  own files — unlike the *other* `scripts/*` gotcha above, `scripts/
  parsers`, `scripts/codereg`, and `data/` are already installed as
  whole directories (`install(DIRECTORY ...)`), so new files under
  them are packaged automatically. (Confirmed still true adding MOPAC.)

### Added from integrating MOPAC (issue #86) — the second code through
  this checklist. Everything above held up; these are the gaps it found.
- **`scripts/gensub` needs a `sub <lccode>()` per code**, or the job
  simply cannot be launched — `$fct = $lccode` is a bare dispatch on
  the lowercased application type. Not mentioned anywhere in the list
  above and easy to miss, since it isn't in the per-code file set.
- **Let the *code* dictate the input filename, not the checklist.** The
  "use a distinctive extension, not `.in`/`.out`" rule above is about
  Apache MIME mapping, and following it naively broke MOPAC: MOPAC
  derives its output name by stripping a known extension **by
  substring**, so `mopac.mopin` makes it write a file literally named
  `mopac    in.out`. Pick whatever extension the code demands, add the
  `AddType` for *that*; and if the code forces a generic output name,
  symlink the declared name onto it in `gensub` **before** the run —
  renaming afterwards is too late, live monitoring needs to tail the
  file during the job.
- **Test the `.desc` against a molecule with degenerate vibrations**
  (CH₄, benzene), never just water. MOPAC's `DESCRIPTION OF VIBRATIONS`
  collapses degenerate modes (7 stanzas for CH₄) while `NORMAL
  COORDINATE ANALYSIS` lists all 9 — sourcing VIBFREQ and VIB from the
  two different blocks silently misaligns them, the same trap as ORCA's
  VIBFREQ/VIBIR. C₂ᵥ water cannot show this.
- **Run *every* runtype's output through the simulation, not just the
  richest one.** MOPAC prints `FINAL HEAT OF FORMATION =` for an
  optimization but `HEAT OF FORMATION =` for a FORCE job, so a
  Vibration-only job extracted no energy at all — invisible in the
  optimization output, which looks perfect.
- **XML comments may not contain `--`**, which the prose style used
  throughout this file uses constantly. It silently made `MOPAC.edml`
  and both `ResourceDescriptor` files non-well-formed.
- **The MO diagram needs the code's MO coefficients and basis, and
  their AO order verified — not any population analysis from the
  code.** ECCE computes Löwdin shares (S^½c) and overlap populations
  itself, from `Props/MO` and an overlap matrix it rebuilds from the
  stored basis, so a code never has to print them. What it must supply
  is the coefficient table plus the basis, with `.edml` `MOOrdering`
  correct for every l up to d/f. Check with the `[MOLOC]` line's
  `norm=` (cᵀSc, `ECCE_DEBUG_MOSYM_LOG=<file>`): it must be 1.000 for
  every orbital, degenerate sets and d-heavy ones included. A norm off
  1 means ECCE is pairing coefficients with the wrong functions, and
  every share, overlap population and line built on them is wrong
  (Cr(CO)₆/ORCA, 2026-09-28: 0.7–2.9 on the Cr p/d sets). A
  semiempirical code writes no basis; ECCE rebuilds a Slater one, but
  NDDO coefficients belong to an orthogonal basis (S = I), so check
  which S such a code's numbers actually assume.
- **Keep `len(TEVEC) <= len(GEOMTRACE)`.** `GeomTracePropertyPanel`
  plots any `PropTSVector<Geometry Step>` alongside GEOMTRACE, and
  `OnPointClick` passes the curve index straight to `GTStepCmd` — an
  index past the last GEOMTRACE frame trips `PropTSVecTable::value()`'s
  bounds check and the atoms collapse to the origin. If a code prints a
  per-cycle energy trace but not per-cycle geometries, don't map it to
  TEVEC.
- **Keep `ResourceDescriptor.xml` and `ResourceDescriptorRxn.xml` in
  step.** `ResourceDescriptor.C` uses the `Rxn` variant whenever
  `bin/dirdyed` exists, so a code missing from the plain file's
  `project` `<Contains>` list is invisible on this build and vanishes
  from the New-Calculation menu only where `dirdyed` is absent.
- **A `.desc` parse-type's `Begin` value is also its hash key, AND its
  match priority** — `scripts/eccejobmonitor`'s `PDFileRead()` keys its
  whole parse-type table by the literal `Begin` string (`$parseHandle =
  $pdBuf{$PD_KEY_BEGIN}`), *silently* dropping any later entry whose
  `Begin` collides with one already read ("silently ignore duplicate
  parse handles" — no error, no log). Making two entries' `Begin`
  values textually different but functionally identical (e.g. wrapping
  one in a no-op `(?:...)`) avoids that collision, but creates a worse
  problem if both are meant to match the *same real line*:
  `PDMatchBegin()` returns the first parse type whose `Begin` matches a
  given line and never checks the rest for that line, so whichever
  entry happens to be checked first **permanently starves the other of
  ever matching, for the entire run** (confirmed live: one matched
  10/10 real occurrences, the other matched zero). If a code's output
  has only one marker phrase serving two logical purposes (e.g. "the
  converged energy" vs "energy at every optimization step", where
  NWChem happens to have two distinct marker phrases for these but not
  every code does) — don't split them into two `.desc` entries at all.
  Use one combined `[KEY1][KEY2]` entry at `Frequency=all`, and have
  the script unconditionally emit both keys on every invocation; a
  plain overwriting value naturally ends up holding the last (e.g.
  converged) value once matching stops, while a step-vector-typed key
  accumulates the full per-step trace from the same invocations.

- **`End`-line starvation: an `End` that lands on another entry's
  `Begin` line makes that entry unable to EVER fire.** `eccejobmonitor`
  consumes the `End`-matching line as part of the block it closes, so
  that line is gone before the other entry's `Begin` is ever tested
  against it. No error, job completes normally, the property is simply
  always absent. Found twice in one file on 2026-09-21, in `orca.desc`:
  `[VIBFREQ]`'s `End=NORMAL MODES` ate `[VIB]`'s `Begin=NORMAL MODES`,
  and `[SHIELDTENSOR]`'s `End=CHEMICAL SHIELDING SUMMARY` ate
  `[ISOSHIELD][ANISOSHIELD]`'s `Begin`. The `[VIB]` one was
  particularly costly because `PropertyPanelDescriptor.xml` gates the
  "Vibrational Frequencies" panel on `VIB`, not `VIBFREQ` — so every
  ORCA frequency job extracted correct frequencies and then showed no
  vibration panel at all. Note it also hides itself in chains: `[VIB]`
  had the identical bug against `[VIBIR]` one link further down, latent
  only because `[VIB]` never fired, so fixing one entry can expose the
  next. Fix by ending on a *separator* line instead of the next
  section's header — an anchored unindented dashes run
  (`End=^-{4,}\s*$`) works and can't match the indented separators
  inside other blocks — or by re-anchoring the starved `Begin` onto
  something past the other entry's `End` (e.g. the table's own column
  header). `mopac.desc`'s header warns about this hazard; whenever two
  entries read adjacent sections of one output, check the boundary.
- **A `.desc` entry's `Skip=N` counts the `Begin`-matching line itself**,
  not N lines *after* it (`scripts/eccejobmonitor`'s Begin/Skip/End
  line-feeding algorithm decrements `lineSkip` starting from the Begin
  match's own iteration). Getting this off by one leaves whatever
  separator/dashed/blank line immediately follows `Begin` as the
  *first* line fed to the parse script — harmless for a script that
  tolerantly skips non-matching lines, but silently produces **zero**
  output, every single invocation, for any script that does `if
  (matches) {...} else { last }` on its first read (a common, natural
  pattern for "read atom rows until the shape changes"). This was the
  root cause of ORCA's `GEOMTRACE`/`VIBFREQ` properties never
  extracting through the real monitor pipeline despite their `Begin`
  regex matching correctly in isolation — found only by simulating
  eccejobmonitor's actual algorithm against real captured output, not
  by hand-testing the parser script with manually-picked line ranges
  (which is exactly what let it hide through an earlier live-debugging
  session). If a new `.desc` entry's script produces no output despite
  a confirmed-matching `Begin`, simulate the real Skip/End feed before
  suspecting the regex or the script's own logic.

Separately (found the same way, but not code-registration-specific,
so don't expect it to recur per-code): `VDoc::isCurrentVdoc()` had a
version-string parsing bug that broke saving *any* calc's input file
on this build, misread as a new-code-specific issue at first because
it was hit while testing one. Already fixed — if a save fails with
"input file copy to DAV failed" / 409 Conflict on a build after this
fix, it's a genuinely new problem, not a repeat of that one.

### The input checker (`scripts/parsers/verifyinput`, #148)
Run automatically whenever `CalcEd::generateInput()` writes a deck and
when a calculation is opened, and on demand from CalcEd's **Verify**
button; the result is a lamp beside that button and, on a click, a
dialog showing the deck with the offending lines marked.
`tests/verify` covers both the script and the dialog's construction.
- **It checks the SHAPE of a deck and must never check a keyword.**
  Per code: section order and the blank lines between them, Link 0
  directives preceding the route, the title/charge/geometry positions,
  block open/close (`* xyz`…`*`, `… end`, `&namelist`…`/`), and the
  basis block — declared primitive counts matching what is listed, and
  every element in the geometry actually covered. This tree has been
  wrong about keyword *validity* from reading manuals repeatedly
  (NWChem rejects its own documented `disp grimme3`; ORCA takes
  `6-31++G**` and refuses `6-31++G`; Gaussian wants `GD3BJ`, ECCE
  wrote `GD3-BJ`), and a checker that calls a correct deck wrong is
  *worse than no checker* — the user stops reading it, including when
  it is right. Anything not decidable from the file is `UNSURE`, which
  is a real verdict, not a weak `BAD`.
- **Gaussian ships `testrt`, its own route-card parser — use it in
  `tests/verify`, never in the shipped script.** ECCE submits to
  remote machines, so the checking client is the machine least likely
  to have Gaussian installed; a check that degrades to UNSURE on most
  installs is not a check (Andy, 2026-09-25). In the suite it is the
  oracle the reverse-engineered route rules are validated against, and
  it skips when absent. It rejects unbalanced parentheses and stray
  characters, and **accepts `Freq=()`** — which ECCE emits on nearly
  every deck, so a paren rule written from intuition would have
  condemned the whole corpus. Compare only route-card findings with
  it; a broken basis block has a perfectly good route card.
- **Check what is INSIDE a section, not only that it exists.** Every
  rule was about section presence and closure until a hand-typed `s`
  in an NWChem deck passed all of them (reported live 2026-09-25). A
  geometry known to be Cartesian must have three coordinates on every
  line; that one rule catches the whole stray-character class in
  Gaussian, ORCA and NWChem at once.
- **Charge/multiplicity/electron parity is the one "forbidden
  combination" that needs no knowledge of any code** — it is
  arithmetic on the deck, identical for all four, and every code
  rejects a violation well into the run. Unknown element → say
  nothing rather than accuse a good deck.
- **A hand edit is the case the checker most needs to see and the one
  it missed.** `processEditCompletion()` wrote Final Edit's result
  back to DAV and nothing re-checked it. Also: the Verify button must
  **not** `doSave()` first — that regenerates the input file and
  silently destroys the user's hand edit, then reports the clean
  regenerated deck as fine.
- **A "row of primitives" check must be anchored.** Counting numbers
  found anywhere in the line accepts the *next shell's header*
  (`S   1  1.00` holds two numbers), so a shell declaring five
  primitives and listing three read as complete. Found by the suite,
  not by inspection.
- Findings are `LEVEL|LINE|CHECK|MESSAGE` on stdout; exit 1 means at
  least one `BAD`. **`execout()` returns false on that exit status**,
  so `InputVerifier` deliberately ignores its return value and decides
  from the parsed output instead — a deck with faults in it is the
  script working, not the script failing.
- An empty finding list means *nothing was checked*, and the lamp goes
  **blank, not green**. A light that cannot distinguish "looked and
  found nothing" from "did not look" is the first one a user believes.
  The dialog smoke test asserts this; it caught the bug once already.
- Do not move the automatic run into `enableLaunch()`/`enableAllFields()`
  — those fire on every edit, and a check costs a WebDAV fetch of the
  input file plus a process.


## Pitfalls found more than once (check for siblings)
- **A code's theory NAME must not equal its CATEGORY.** `CalcEd::
  getTheoryName()` reverses `populateTheories()`'s display convention —
  the combo shows `name()` for every theory except one literally named
  `"None"`, where it shows `category()` — and it used to detect that
  case by comparing the label against the category. For Quantum
  ESPRESSO, whose theory is `category="PW" name="PW"`, that returned
  `("PW","None")`: a theory in no `.edml`. **Two unrelated-looking
  faults followed from it**, which is why it took a while to see:
  `JCode::theoryNeedsBasis()` returns `true` for a theory it cannot
  find, so the Basis Set Tool stayed enabled for a plane-wave code
  *despite* `needsBasis="false"` being correct; and `populateRuntypes()`
  found nothing, giving "No runtypes are supported for the given
  code/theory combination" with nothing to edit or launch. Fixed by
  asking the code whether a `"None"`-named theory exists rather than
  inferring it from the label. Audited: QE was the only code where name
  equals category, which is why it survived every other integration —
  but check it when adding one.
- **CalcEd used to discard the input generator's error message.**
  `execout()` captures the generator's stdout+stderr into `message`, and
  the failure branch overwrote it with a generic "input parsing command
  ... failed". Every `ai.<code>` validates in its main flow and dies with
  something specific and actionable — `ai.qe`'s periodicity check even
  names the Builder panel to use — and none of it reached the screen.
  Fixed (the generator's output now leads the message), but the lesson
  generalises: when a script's diagnostics are the only explanation of a
  failure, check that whatever shells out to it actually shows them.
  Same shape as `eccejobmaster` logging to `/dev/null`.
- **A parser with nothing able to request its input is dead code, and
  the suite will not tell you.** `orca.desc` parsed `CHELPG Charges`
  into ESPCHARGE from the day ORCA was integrated, but neither
  `ai.orca` nor the runtype dialog could ever put `CHELPG` on the route
  card, so the property never appeared for any job (#88). The parse
  type simply never fired, which looks identical to "no fixture
  exercises it". When adding extraction for a property, check that
  something can *ask the code to produce it* — and when a parse type
  never fires, establish which of the two it is (see
  `tests/parsers/cases.py`'s `KNOWN_DEAD` vs `UNCOVERED`, and the gate
  that now forces every non-firing entry to be classified).
- **"The code accepted the keyword" is not "the keyword works."** All
  eleven ORCA correlated/double-hybrid keywords passed ORCA's input
  check; two of them then died at runtime — double hybrids route their
  correlation through RI-MP2 and need a `<basis>/C` auxiliary basis
  (`ERROR: RI-MP2 needs an AuxC basis but none was defined!`, exit 55).
  Offering them without it would have shipped decks that always fail,
  *after* reaching a queue. Run a real job, not a syntax check. The same
  discipline found that GROMACS's double-row energy blocks and QE's
  header-carried units both silently produce wrong values rather than
  none.
- **`ai.<code>`'s template engine silently swallows `die()`** (#92).
  `modifyInputFile` resolves every `##tag##` through `eval "&$subname"`,
  so a `die` inside a resolver — or anything it calls — is caught, the
  tag's line vanishes, and the script **exits 0 with a quietly
  incomplete input deck**. All 10 `ai.*` scripts share this engine and
  nine contain `die`s. Confirmed reachable in `ai.mopac`:
  `mopac.tpl`'s `##MOPACSecondJob##` → `MOPACSecondJob` →
  `HamiltonianKeyword`'s "unsupported theory" `die`, so MOPAC's
  reduced-scope guards cannot report anything and emit a deck missing
  its keyword line instead. Put validation in the **main flow** before
  generation starts, never inside a resolver — `ai.qe`'s
  `&verifyPeriodic` is the pattern. **FIXED for all ten generators in
  `a6d0512`**: each `eval` site now re-raises a real error while leaving
  "Undefined subroutine" silent, so a resolver's `die` reaches the user.
  Expect it to surface errors that were previously invisible.
- **Never point an `ai.<code>` script's `-t` at the repo's own
  template.** `cleanup()` ends with `mv -f tmpfile "$TplFILE"`, so the
  generated deck **overwrites the template you passed in**. Harmless in
  normal operation, where the `.tpl` is staged into the run directory
  first, but running one by hand for testing silently destroys
  `scripts/parsers/<code>.tpl`. Copy the template into a scratch
  directory and point `-t` at the copy. (Learned by clobbering
  `mopac.tpl` and restoring it from git.)
- **Codereg dialogs are never told which elements the structure
  contains.** They are standalone processes whose entire input is
  `globals.py`'s fixed argv (calc name, category, theory, runtype,
  symmetry group, electron/orbital/normal-mode counts). So no
  per-element UI is possible in a dialog — anything element-dependent
  (QE's pseudopotential-per-species being the live case) has to be
  resolved in `ai.<code>`, which does read the `.frag`. Worth knowing
  before designing any such dialog.
- **A dialog's choice string must match the generator's expected string
  EXACTLY, or selecting it silently emits nothing.** The
  `scripts/codereg/*theory.py` combo lists and the `ai.<code>` subs that
  translate them (`$xcFun eq "..."`, `$exchFun eq "..."`) are two
  hand-maintained copies of the same list, and every `ai.<code>`
  translation sub ends in an `else { $result = ""; }` — so a mismatch
  produces an empty keyword, the `##token##` line is deleted, and the
  job runs *without* the functional the user picked. No error anywhere.
  Found **nine times in one afternoon** (2026-09-21) across
  `ged{03,09,16}theory.py`: letter `O` for zero (`MO6HF` vs `M06HF`), a
  missing closing paren (`"MPW1PBE (hybrid"`), wrong case (`MPW3PBE` vs
  `MPw3PBE`), a stray space (`"Becke 89"` vs `Becke89`), a dialog
  offering a functional with no generator case at all (`M11`), and
  ged03 offering `BP86 (hybrid)`/`M06L (nonlocal)` where its generator
  wants `BP86 (GGA)`/`M06L (meta-GGA)`.
  **Also check for missing commas in the Python list**: adjacent string
  literals are implicitly concatenated, so
  `"N12SX (range)" "MN12SX (range)"` silently became one nonsense
  dropdown entry in all three dialogs while both real functionals
  became unselectable.
  Detect mechanically rather than by eye — extract the dialog's list and
  the generator's `eq "..."` cases and diff the two sets, checking both
  directions (offered-but-unmapped is a dead control; mapped-but-unoffered
  is a capability the UI can't reach, which is how Gaussian's three
  double hybrids stayed hidden). Watch out that `xcFuncDefault` is an
  **index** into the list, so append rather than insert, and verify the
  defaults still resolve to the same entry afterwards.
- **The basis-set writers decide *how* a basis reaches the deck, and every
  bug in them is silent.** `TGBSConfig::dump()` always writes a
  `NumericalBasis` section (explicit exponents and coefficients) and
  *additionally* a `NameBasis` section when the basis can be named; each
  code's `wr<Code>GBS.pm` then chooses. Things learned the hard way
  (2026-09-23):
  - **Gaussian's writer printed ECCE's own name, not the translated
    one**, so `%NameToBasis` was consulted only as a yes/no test and its
    value discarded. That works wherever the spellings coincide and
    produces a deck Gaussian *rejects* where they do not — `midi!` has
    to be `midix`, `dz (dunning)` → `d95`, `sv (dunning-hay)` → `d95v`.
    Fixed; ORCA's writer always printed the value.
  - **Name tables must be verified by RUNNING the code.** ORCA accepts
    `6-31++G**` but rejects `6-31++G` and `6-31++G*`. Gaussian's
    `def2SVPP` is *not* def2-SVPP — it is def2-SV(P) (18 functions for
    water against def2SVP's 24), so a plausible-looking mapping silently
    substitutes a smaller basis. Record the basis-function count beside
    each entry so a future substitution shows up as a changed number.
  - **A Gen section / `%basis` block does not need primitives** — each
    element group may name a basis the code ships. Naming is decided
    **per element**: an element carrying an ECP keeps explicit output so
    it cannot disagree with the separately written ECP, everything else
    is named. Before this, one unmappable element forced every element
    to be written out in full.
  - **NWChem needs no table**: ECCE's names *are* its library names
    (both EMSL's). It has a blocklist instead, measured with
    `tests/basisload/nwchem_library_check.py` against NWChem 7.2.3: only
    `aug-cc-pwCV*` (no hydrogen in NWChem's set), `aug-pV7Z`/`aug-mcc-pV8Z`
    (unloadable), 5Z and up and `d-aug-cc-pVQZ` (unsettled) stay; every
    other `aug-`/`d-aug-`/`-pCV`/`IGLO` name spans the same space as
    ECCE's set. Beware that ECCE's explicit form repeats primitives, so
    NWChem drops near-dependent vectors and its energy can differ from the
    named form by up to 7e-4 Eh (QZ) until `lindep:tol` is tightened.
  - **ORCA is spherical-only.** It has no cartesian basis keyword at
    all, so `wrORCAGBS.pm` ignoring `$coordinants` is correct, not a bug.
    Gaussian's writer passes it and `ai.gauss16` emits `5D 7F`/`6D 10F`.
  - Perl randomises hash iteration per process, so these writers emitted
    **elements in a different order every run** until the keys were
    sorted — the same calculation producing a byte-different deck each
    time. `tests/basis` covers all of this now.
- **A combo whose default is a bare integer opens BLANK when the list is
  built conditionally.** `wx.Choice.SetSelection()` ignores an
  out-of-range index without complaint, so the control shows nothing
  selected and `GetValue()` returns `""` — which every generator
  translates to no keyword at all. Same silent-emission class as a
  dialog string that doesn't match the generator's, same cause: one
  list, several lengths, a default written against the longest. Found
  in every Gaussian runtype dialog at once
  (`ES.Runtype.GeomOpt.InitialHessian`, `default = 1` against a
  one-entry list for all but a handful of theories) — and the same file's
  `CheckDependency()` already carried the `len()==2` guard the
  constructor was missing. Always write these as
  `choices.index("Name")`, never an integer; `xcFuncDefault` is the
  same hazard (a stale `36` was selecting Mod. Perdew-Wang 1K where
  B3LYP was intended). `tests/dialogs` now fails on any combo left with
  no selection, in every category/theory/runtype context.
- **Verify a code's keyword list by running the code, not by reading its
  manual.** Sweeping all 55 functionals `nedtheory.py` offers through
  NWChem 7.2.3 found three that abort the job every time (CAM-B3LYP and
  LC-wPBE put `cam` on the `xc` line, where it is a directive of its
  own; plain `hcth147` is deprecated and fatal, it wants
  `hcth147@tz2p`). The same sweep showed the documented spellings for
  dispersion (`disp grimme3`, a trailing `bj`) are rejected outright —
  only `disp vdw <1..4>` works — and that D3/D3BJ with a functional
  lacking parameters is **fatal rather than ignored**, which is why
  `ai.nwchem` validates the pairing in the main flow. Where a support
  table like that is needed, keep it in the generator alone and let it
  report; putting a copy in the dialog recreates the
  two-hand-maintained-lists bug this file already warns about twice.
- **Retired codes are not maintained, and the suite no longer checks
  them.** `tests/dialogs/cases.py`'s `RETIRED` (Gaussian-03,
  Gaussian-98, GAMESS-UK, Amica) is skipped unless named with `--code`.
  It is *not* the same as `NOT_IN_MENU`, which only means "absent from
  the New Calculation menu" — Polyrate and GROMACS are in that one and
  are maintained. **MetaDyn is not retired**: its `.edml` declares
  `codeName="NWChem"`, so it is NWChem's plane-wave metadynamics front
  end, and it is what `QuantumESPRESSO.edml` was modelled on.
- **A `.desc` entry's `Begin` wording can silently stop matching between
  versions of the same code**, not just between different codes.
  `gaussian-16.desc`'s `MULLIKEN` entry had `Begin= Mulliken atomic
  charges\:`, copied from `gaussian-09.desc`/`gaussian-03.desc` — but
  Gaussian 16 actually prints "Mulliken charges:" (no "atomic").
  `Gaussian-16.expt` (the full-output post-hoc parser) has no Mulliken
  handling of its own, so this property is *only* ever captured live
  via `eccejobmonitor` — the mismatched `Begin` regex meant it was
  silently never extracted, for every G16 job, ever (#80). Same
  no-error/job-completes-normally shape as the other `.desc` bugs
  here. Fixed with an alternation (`Mulliken (atomic )?charges\:`)
  rather than assuming the new wording fully replaced the old one.
  **Caveat that applies to any fix in this family**: because there's
  no reparse-from-saved-output path for a live-monitor-only property,
  fixing the regex does *not* retroactively recover data for
  already-completed jobs parsed under the old pattern — only a fresh
  run monitored under the fixed `.desc` will have the property. If a
  "we fixed the parser but the old job still shows nothing" report
  comes in, check whether the job actually predates the fix before
  assuming it didn't work.
- **`eccejobmonitor`'s "enable all parse types" check is case-sensitive**
  (`scripts/eccejobmonitor`, `PDTypesEnable()`) — it only recognizes
  lowercase `all`, but `Launch.C` hardcodes `"parseTypes ALL"`
  (uppercase) when invoking it. The mismatch makes it silently delete
  every not-yet-enabled parse descriptor from its live-monitoring match
  table, so any `Frequency=all`-style trace property (`GEOMTRACE`, and
  presumably any other code's equivalent) never gets extracted during
  a run — no error, the job completes normally, only the trace data is
  missing. Scalar single-value properties (`TE`, ...) are unaffected,
  so this looks exactly like a per-property parsing bug and not the
  systemic one it is. Fixed script-side (case-insensitive) rather than
  touching `Launch.C`, so it doesn't need a C++ rebuild — if a fresh
  build still drops a trace property with an otherwise-correct
  `Begin`/`End` match, this fix predates it and something new is wrong.

## Memory fields: the #77 unit-label fix

  Issue #77 (G16 Memory field showing
  "Megawords" despite a verified-correct source and running process) is
  fixed as of `884593f` — the bug was never `calced`'s C++ side; it was
  `BoxSizerFrame.FinalizeSetting()` (`scripts/codereg/templates.py`)
  calling `SetUnit(unit)` on GUIValues restore, overwriting the
  widget's freshly-correct unit label with whatever was persisted in
  the calc's *stored* data (stale for any calc saved before the
  GB-everywhere UX change). Fix removes the `SetUnit()` call on
  restore — value persists, unit label doesn't. **Live-verified and
  closed 2026-09-22** (confirmed more than once on screen). If a wrong
  unit label ever shows again, something new is wrong, not a repeat.

