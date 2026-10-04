# wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls

Read this before changing any dialog, panel, sizer, grid, ewx control or the Open Inventor viewer. Split out of CLAUDE.md 2026-10-04.

- **Open Inventor's redraw sensor is a ONE-SHOT that re-arms on render,
  and with a render callback installed nothing re-arms it.** This was
  #99: stepping a geometry trace moved the atoms once and then never
  again. `GTStepCmd` ran every step with changing coordinates,
  `SGFragment::getAtomCoordinates()` reads `TAtm` live so the scene
  always had fresh data, `touchChemDisplay()` and `sgfrag->touch()` were
  both called — and `SoWxRenderArea::renderCB` was entered for step 0
  and then *not once* for the thirteen steps after it. The redraw was
  never requested. Fixed by calling `SGViewer::refreshRenderArea()` at
  the end of `processStep()`, which forces a wx paint and does not
  depend on that sensor; applied to `GeomTracePropertyPanel` and
  `NModePanel`. **Two plausible theories were disproved on the way and
  should not be revisited**: the render cache (disabling caching
  process-wide changed nothing) and a stranded `p_redrawPending` in
  `OnPaint` (that retry path is fine — it never had a callback to
  service). If a viewer stops updating while the data demonstrably
  changes, instrument `renderCB` first: `ECCE_DEBUG_GEOMTRACE=1` prints
  `[RENDERCB]` lines alongside the step trace, and their *absence* is
  the finding.
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
- **The `.pjd` (DialogBlocks) files are reference only; never regenerate
  code from them.** They name plain wx classes, and PNNL's step that
  turned the output into ECCE's `ewx` subclasses is lost, so
  `dialogblocks --generate` replaces every `ewx` control with a plain
  wx one (63 references became 3 in machine registration). Three are
  also missing controls added by hand since: `NModesGUI` (#109),
  `CalcEdGUI` (Verify, Use Symmetry, Regenerate Input) and
  `WxMachineRegisterGUI` (#187). Edit the generated `.C`/`.H` directly;
  when #210 reworks a dialog, delete its `.pjd` in the same commit.
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
  **UPDATE 2026-09-22: that fix is correct but was not the cause of
  #99.** The `p_redrawPending` retry path works; it simply never had a
  callback to service, because the scene manager's redraw sensor is a
  one-shot that re-arms on render and nothing re-armed it. See the
  one-shot entry in the pitfall list above. Do not re-investigate
  `p_inPaint`/`p_redrawPending` for a "viewer stops updating" symptom
  without first checking whether `renderCB` is entered at all.
- **RESOLVED (#81, fixed `9a3004e`, confirmed live 2026-09-21): the
  Vibrational Frequencies panel's Animation/Vector radio box did not
  deliver its click event under wx3.2/GTK3.** Kept because the *cause*
  generalises to any `ewxRadioBox` (and any other ewx control with a
  pushed handler chain): `ewxRadioBox::Create()` does
  `PushEventHandler(new ewxHelpHandler(this))`, so a command event's
  route from the control to its panel is not the plain propagation wx
  documents, and a **static `EVT_RADIOBOX` table entry never arrives**.
  Symptom was total: no row swap, no vector/animate switch, and the Play
  button never appeared, since the whole display-mode switch is gated on
  that event.
  Fix: bind dynamically on the widget itself
  (`radbox->Bind(wxEVT_RADIOBOX, ...)`). **Use that pattern for any new
  ewx radio box rather than the static table** — the Graph/Table box
  added later for #109 is wired the same way and works.
  Two dead ends worth not repeating: an `EVT_UPDATE_UI` idle-poll
  fallback was added first and reported as making "no visible
  difference" — it was inert because wxGTK does not send
  `wxUpdateUIEvent` to ordinary child controls during idle unless they
  carry `wxWS_EX_PROCESS_UI_UPDATES`, which nothing set. Setting that
  style turns it into a real fallback, and it is kept alongside the Bind
  as layered defence (same approach as #78). And a reported *flicker*
  during the newly-working animation was **not reproducible on retest**;
  the `touchChemDisplay()` blank-then-rebuild mechanism proposed for it
  is an untested hypothesis for a symptom that may not exist — do not
  change that shared function on its strength without reproducing
  flicker first.

