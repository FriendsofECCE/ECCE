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
  metathry}.py` for the pattern. One label still doesn't match despite
  a verified-correct source and a verified-correct running process —
  see issue #77 before assuming a future "fix" here is wrong; the bug
  is downstream in `calced`'s C++ side, not in these Python dialogs.

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
- **`rdStandardGBS.pm`'s "NameBasis" format has two undocumented
  requirements** that every existing `*.expt`'s writer actually
  violates (silently broken there too, not just for a new code): the
  `basis "ao basis" <type>` line needs a literal trailing `print`
  keyword, and `<atom> library "<name>"` lines must NOT be indented —
  neither requirement matches the whitespace tolerance the format's
  own `NameBasis`/`EndNameBasis` markers get. Get either wrong and a
  named-library basis assignment silently translates to nothing, with
  no error.
- **No `CMakeLists.txt install()` changes needed** for a new code's
  own files — unlike the *other* `scripts/*` gotcha above, `scripts/
  parsers`, `scripts/codereg`, and `data/` are already installed as
  whole directories (`install(DIRECTORY ...)`), so new files under
  them are packaged automatically.
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
