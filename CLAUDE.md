# ECCE — context for Claude Code sessions in this repo

`FriendsofECCE/ECCE`, single active branch `main`. This is a
**modernization**, not a rewrite: porting a ~1200-file legacy scientific
C++/wxWidgets desktop app (wx 2.8 → 3.2, C++14 → 17, `build_ecce`/
recursive-make → CMake/CPack, Python 2 → 3) to build and run on current
Debian, keeping old behavior as the correctness anchor. This file is a
lean map of where things live and what to watch out for — it is
intentionally *not* a running log of past sessions. For that, see
"Where the history lives" at the bottom.

## Standing preferences
- Andy runs Debian ("trixie") on his machines — **beryllium** and
  **niobium**. Default to Debian conventions, not Ubuntu, for anything
  environment/package related, unless told otherwise.
- Releases are tagged (e.g. `v8.0.0-alpha.1`), not branched. Old branches
  (`develop`, `modernize-build`, `stable`, `master`, `make`) were
  consolidated into `main` and renamed to `archive/*` — no reason to
  branch from or compare against them.
- Build directory is `build-cmake` (not `cmake-build`) — `ninja` or
  `cmake --build .` from inside it.
- `GETTING_STARTED.md` (repo root) has the full build/package/install/
  first-login walkthrough. Don't reproduce it here.
- **Memory settings should be entered/labeled in GB everywhere, for
  every code** — Andy's explicit UX preference (2026-09-07), not each
  code's native convention. The wire format still has to match what
  each code's input file actually expects (words for GAMESS-UK, MB for
  ORCA's `%maxcore`, a `gb`/`GB` suffix NWChem and Gaussian both accept
  natively) — convert at the point closest to the generated input
  (`ai.<code>`/the `.tpl`), not by changing what the wire format itself
  accepts. See `scripts/codereg/{ged*,nedtheory,orcatheory,guktheory,
  metathry}.py` for the pattern. Issue #77 (G16 Memory field showing
  "Megawords" despite a verified-correct source and running process) is
  fixed as of `884593f` — the bug was never `calced`'s C++ side; it was
  `BoxSizerFrame.FinalizeSetting()` (`scripts/codereg/templates.py`)
  calling `SetUnit(unit)` on GUIValues restore, overwriting the
  widget's freshly-correct unit label with whatever was persisted in
  the calc's *stored* data (stale for any calc saved before the
  GB-everywhere UX change). Fix removes the `SetUnit()` call on
  restore — value persists, unit label doesn't. Not yet live-verified;
  if it still shows the wrong label after this fix, something new is
  wrong, not a repeat.

## Code map — where the role of each part is

### Getting a calculation set up (the "code registration" system)
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

#### New-code checklist (gotchas found integrating ORCA, issue #38)
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

#### Added from integrating MOPAC (issue #86) — the second code through
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
- **Keep `len(TEVEC) <= len(GEOMTRACE)`.** `GeomTracePropertyPanel`
  plots any `PropTSVector<Geometry Step>` alongside GEOMTRACE, and
  `OnPointClick` passes the curve index straight to `GTStepCmd` — an
  index past the last GEOMTRACE frame trips `PropTSVecTable::value()`'s
  bounds check and the atoms collapse to the origin. If a code prints a
  per-cycle energy trace but not per-cycle geometries, don't map it to
  TEVEC.
- **`ResourceDescriptor.xml`'s `project` `<Contains>` list is missing
  `gaussian16_es`, `gaussian09_es` and `orca_es`** — latent rather than
  live only because `ResourceDescriptor.C` switches to the `Rxn.xml`
  variant whenever `bin/dirdyed` exists, and it does on this build. If
  `dirdyed` is ever dropped, three codes vanish from the New-Calculation
  menu at once.
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

### The two background services ("the server")
Almost entirely new in this fork — not in the original app's docs.
Both per-user, both non-root, both started automatically by the
`ecce-<app>` wrapper scripts (generated by `CMakeLists.txt`):
- **`packaging/gateway/`** — ActiveMQ (Debian-packaged, `/usr/share/
  activemq`) JMS broker + the `java/` JMSDispatcher relay. How `gateway`
  launches other apps (`organizer`, `builder`, ...) and how they publish
  progress/results back.
- **`packaging/dataserver/`** — per-user Apache 2.4 + `mod_dav` (config:
  `httpd.conf.ecce`), the WebDAV "ECCE Server" — structure library,
  basis-set library, saved calculation data, and (as of this fork) help
  content and the help CGI backend all served from here, port 8096.

### GUI application layer
- `src/apps/*` — one directory per top-level app (`builder`, `organizer`,
  `calced`, `gateway`, `machregister`, `machbrowser`, `basistool`, ...).
- `src/wxgui/` — shared wx widget classes/toolkit used across apps.
- `src/wxviz/`, `src/inv/` — the 3D molecular viewer, built on Open
  Inventor (`SoWxRenderArea`/`SoWxExaminerViewer`/scene graph).
- `src/tdat/` — calculation/resource data model. Notably
  `src/tdat/properties/Prop*.C` — a family of copy-pasted-from-template
  classes (`PropTable`, `PropVector`, `PropTSVecTable`, ...) holding
  parsed results (MOs, energies, geometry traces). They share the same
  bounds-checked `value()` accessor shape closely enough that a bug found
  in one is worth grepping the siblings for.
- `src/dsm/`, `src/comm/`, `src/util/` — chemistry/XML data model, remote
  job/comm layer, general utilities. No wx dependency.

### Known pitfall classes (found more than once — check for siblings)
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
- **wx3.2/GTK3 layout reentrancy**: `wxWindow::DoSetSize` → `wxEVT_SIZE`
  → `Layout()` → reposition children → another `DoSetSize`, sometimes
  non-convergent (stack-overflow crash) or asynchronous (crashes well
  after `Show()`/`wxEVT_SHOW` return). Not present in GTK2/wx2.8; this is
  new surface area from the wx3.2 GTK3 backend. If a dialog crashes or
  hangs on construction or first `Show()`, suspect this before anything
  else — see `docs/HISTORY.md` for the working detection technique
  (backtrace-based reentrancy check in a `wxEventFilter`, not a fixed
  timer).
- **`wxGrid::CreateGrid()`/`SetTable()` synchronously fires
  `wxEVT_GRID_SELECT_CELL`** on wx3.2/GTK3 — immediately, during
  construction, not deferred to the event loop the way it effectively
  was on wx2.8/GTK2. A grid built early in a panel's `CreateControls()`
  (e.g. `NModesGUI`'s vibration-mode grid) can reach an
  `EVT_GRID_SELECT_CELL`-bound handler in a subclass (`NModePanel::
  OnModeSelection` → `showMode()`) that dereferences sibling
  controls/members not constructed yet — including ones that don't
  exist until a *separate, later-running* function does (`p_slider`,
  only built in `NModePanel::Create()` itself, after the
  `NModesGUI::Create()`/`CreateControls()` call that triggers the
  event) — so reordering statements within the one function that
  triggered it isn't guaranteed to be enough; every dependency the
  handler touches needs to actually exist first. Reliably segfaulted
  `builder` on opening *any* job with vibrational (`VIB`) data (#78).
  Fixed by wiring up the panel's already-declared-but-dead `p_isValid`
  flag as a real "construction is fully finished" guard: the handler's
  target function returns immediately if not yet valid, and the flag
  is set `true` only once every control exists, right before the
  panel's own legitimate first call into that function. Fixed twice,
  independently, on two machines during the same investigation window
  (#78) — one pass guards in `OnModeSelection()` itself, the other
  guards inside `showMode()`; both landed and were kept as layered
  defense-in-depth rather than picking one, along with deferring
  `NModesGUI::CreateControls()`'s `CreateGrid()` call to the end of the
  function (belt-and-suspenders against the narrower null-button
  dereference that a first, incomplete pass at this fix hit). Check for
  the same shape (an early-constructed grid/combo/list whose "populate
  my data" call synchronously fires a selection/change event into a
  handler with unmet dependencies) in any other panel that builds a
  grid before finishing `CreateControls()`.
- **`wxEXPAND|wxALIGN_CENTER` on the same sizer item** — a documented wx
  footgun; alignment can suppress expand instead of being ignored.
  Combined with a widget that only learns its own size inside its first
  `OnPaint()` (e.g. `ElementButton`), this makes content render as
  near-invisible near-zero-size widgets. Hit in the periodic table,
  Basis Set Tool, and Builder's popup periodic table.
- **`wxFIXED_MINSIZE`** on a sizer item freezes it at whatever best-size
  it had *at the moment it was added* — a bug only when an empty
  placeholder is added first and filled with real content later.
- **`std::map`/`unordered_set` iterator invalidation**: `erase(it)`
  followed by a loop's own `it++`/`--it` touches a dangling iterator.
  Was found in 17 places codebase-wide in one audit
  (`GUIValues.C`, `AuthCache.C`, `DavEDSI.C`, others) — grep
  `erase(it)`/`erase(iter)` if you're touching map/set cleanup code.
  Correct pattern: `it = container.erase(it);`.
- **Uncontrolled format strings**: `wxLogError(msg.c_str(), 0)` treats
  dynamic text as a printf format — fixed 9 sites codebase-wide, pattern
  was always `wxLogError("%s", msg.c_str())`.
- **`EcceException::what()`** now correctly returns a pointer with
  exception-object lifetime (fixed from a dangling-stack-string bug) —
  if you see garbled `Throw Log:` text anywhere, that fix predates it and
  something new is wrong, not a repeat.
- **wx3.2 AUI port dropped the custom "ewxAUI" pane-caption buttons
  (take focus / pin / options / open) the original app was built
  against** (`src/apps/builder/EwxAuiCompat.H` documents this), and
  nothing replaced them as the trigger for `VizPropertyPanel::
  receiveFocus()` — which is what activates essentially every 3-D
  overlay in the viewer (vector/tensor arrows for dipole/quadrupole/
  gradient, Mulliken charge coloring, geometry-trace and vibration-mode
  animation, ...). Only the MOs panel had an independent workaround
  (its own "Compute" button calls `setFocus(true)` directly); every
  other `VizPropertyPanel` subclass was silently dead — correct
  extraction, correct data, zero visual output, no error. Fixed by
  binding `EVT_CHILD_FOCUS` on `Builder` (bubbles from any descendant
  control to the top-level frame) and walking up to the owning
  `VizPropertyPanel` — not `EVT_AUI_PANE_ACTIVATED`, which looks like
  the obvious stock-wx3.2 replacement but only fires when the pane's
  *own* bare window receives focus directly (`wxAuiManager::GetPane()`
  requires an exact pointer match, no ancestor walk), never when focus
  lands on a nested control inside it — the overwhelmingly common case.
  If a new property panel's viz still doesn't show after this fix,
  check whether it overrides `receiveFocus()`/`loseFocus()` at all
  before assuming the trigger is broken again.
- **`SoWxRenderArea::renderCB` silently drops a redraw** if Inventor's
  scene-graph-touch notification fires while a paint is already in
  flight (`p_inPaint`), with no retry — the sensor has already fired
  and won't fire again on its own. Manifests as geometry-trace/
  vibration step-through updating unreliably (works for the first
  step or two, then stops) and looped animation never visibly
  animating at all, while whatever reads the same step data via a pull
  model (e.g. the atom table) stays perfectly in sync — a strong tell
  that a symptom is this bug rather than a data problem. Fixed with a
  pending-redraw flag (`p_redrawPending`) that `OnPaint()` checks and
  acts on once the in-flight paint finishes.
- **OPEN, UNRESOLVED (2026-09-17): the Vibrational Frequencies panel's
  Animation/Vector radio box doesn't deliver its click event under
  wx3.2/GTK3** — confirmed live via strace (syscall-level, a synthetic
  click synced exactly with the trace window): the native GTK widget's
  own selected bullet toggles correctly, but `NModePanel::
  OnRadioboxSelected()` is never entered — zero evidence of it running,
  for a real click or a synced synthetic one. This is *not* the same
  bug as the `p_isValid`/`CreateGrid()` construction-ordering issue
  above (which is fixed) — this is later, after the panel is fully
  built and idle, on an ordinary click. Symptom: switching the radio
  does nothing (no row swap, no vector-arrows/animate-mode switch,
  Play button never appears) since the whole display-mode switch is
  gated on that event firing. An `EVT_UPDATE_UI`-based idle-poll
  workaround was added (`NModePanel::OnRadioboxUpdateUI`, compares
  `radbox->GetSelection()` each idle tick against a cached
  `p_lastRadioSel`) on the theory that this sidesteps whatever GTK
  signal wiring is failing for the click event specifically. Built,
  packaged, and live-tested same day — **no visible difference**, so
  this fix does not actually work, or doesn't work for the reason
  assumed. Don't trust the code comment above `OnRadioboxUpdateUI()`
  to mean this is fixed — re-verify live before believing it. Next
  session should treat the "native event never fires" diagnosis as
  solid (already re-derived twice, syscall-level) but budget fresh
  investigation for *why* the workaround didn't help — candidates not
  yet ruled out: whether `EVT_UPDATE_UI` is actually reaching this
  window at all (same category of failure as the click event, would
  need its own strace/instrumented-build check), whether
  `showAnimationMode()`/`showVectorMode()` are even the code path
  actually driving what's visible in the panel (unverified assumption
  going in), or a packaging/install mismatch (binary in the `.deb`
  not matching what was last built — check md5sum/mtime of
  `/opt/ecce/bin/builder` against `build-cmake/builder` before
  re-testing anything, this bit a previous round in the same session).
  This also blocks separately verifying whether `NModeStepCmd`'s
  redraw fix (`07e7bf9` plus this session's `ret=true` correction)
  actually animates once display-mode switching works at all — that
  is *still unverified end-to-end* despite the code-level trace in
  `NModeStepCmd.C`/`Builder::execute()` looking sound.
  **UPDATE 2026-09-21 — two of the three open candidates above are now
  answered, and a fix is in but NOT yet live-verified.** (a) The
  `EVT_UPDATE_UI` fallback was inert for a concrete reason: wxGTK does
  not send `wxUpdateUIEvent` to ordinary child controls during idle
  unless they carry `wxWS_EX_PROCESS_UI_UPDATES`, and nothing set it —
  so that workaround could never have fired, which fully explains "no
  visible difference" without needing a second mystery. (b)
  `showAnimationMode()`/`showVectorMode()` *are* the right code path,
  and the animation behind them is sound: `OnTimer` → `nextStep()` →
  `processStep()` → `NModeStepCmd` (which does hold the
  `touchChemDisplay()` redraw fix), so the animation was unreachable
  only because the Play button lives in the sizer
  `showAnimationMode()` reveals. Likely why the static `EVT_RADIOBOX`
  never arrives: `ewxRadioBox::Create()` does
  `PushEventHandler(new ewxHelpHandler(this))`, so the control has a
  pushed handler chain and the command event's route to this panel is
  not the plain propagation wx documents. Fixed by not relying on it —
  a dynamic `radbox->Bind(wxEVT_RADIOBOX, ...)` directly on the widget,
  plus setting `wxWS_EX_PROCESS_UI_UPDATES` so the existing idle poll
  becomes a real fallback instead of dead code. Both kept as layered
  defence, as with #78. Builds clean; **live verification still
  outstanding** — test by switching the radio to Animation and
  confirming the Play button appears and the molecule moves.

## Where the history lives
This file used to be a session-by-session diary and grew to ~2500 lines.
It's been split up:
- **`docs/HISTORY.md`** — the full former diary, verbatim: root causes,
  fix commits, verification notes, dead ends, for every bug investigated
  during this modernization effort through 2026-08-29. Read it when you
  need the detailed story behind something referenced above, or when
  debugging something that feels like it should already have been fixed.
- **`ECCE_modernization_status.md`** (repo root) — the earliest chapter
  (initial wx port completion, JMS gateway bring-up, Python 2→3 syntax
  pass), predates and isn't duplicated in `docs/HISTORY.md`.
- **claude.ai Project "ECCE"**, doc `claude/ecce-modernization-status.md`
  — the canonical, most up-to-date narrative across sessions, when
  accessible.
- **GitHub issue tracker** — the live source for what's currently open;
  search it first, most prior investigation is already recorded there
  too (per the wiki's own guidance).
- **Wiki** (`github.com/FriendsofECCE/ECCE/wiki`) — developer-facing
  orientation pages, kept in sync separately from this file: "Where to
  find things in the code" (a similar but more detailed map than the
  one above, aimed at someone adding a new computational code) and
  "Branches and releases."
