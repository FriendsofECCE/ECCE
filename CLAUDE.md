# ECCE — context for Claude Code sessions in this repo

This repo (`FriendsofECCE/ECCE`, `develop` branch, local checkout on niobium)
is mid-modernization: porting a ~1200-file legacy scientific C++/wxWidgets
codebase (wx 2.8 → 3.2, C++14 → 17, `build_ecce`/recursive-make → CMake,
Python 2 → 3) rather than rewriting it. Full history and status lives in a
project doc on claude.ai (Project "ECCE", doc
`claude/ecce-modernization-status.md`) written by earlier Cowork sessions —
if you have web access or the user can paste it in, read that for complete
context. This file is a condensed briefing for the one open item that most
urgently needs *local shell execution* to make progress on, which a Cowork
session (file access only, no shell) could not do itself.

## Standing preference
Andy runs Debian 13 ("trixie") on his own machines (niobium included) —
default to Debian conventions, not Ubuntu, for anything environment/package
related, unless told otherwise.

## Where things are
- Build directory: `build-cmake` (NOT `cmake-build`). Rebuild with `ninja`
  from inside it, or `cmake --build .`.
- This investigation happened directly on branch `modernize-build` (NOT a
  separate `modernize-build-fixes` branch, despite what an earlier version
  of this doc said — that branch either never existed on this checkout or
  the work just landed directly on `modernize-build` instead). Fixes below
  are **committed** here, not sitting uncommitted.
- The wider modernization work (wx port, JMS gateway messaging, CMake/CPack
  packaging, Python 3 port) is documented in full in the claude.ai project
  doc (`claude/ecce-modernization-status.md`) — not reproduced here.

## `gateway` construction-time crash — FIXED, committed (2026-08-28)

`gateway` used to segfault/OOM on startup. Root cause, confirmed via gdb +
Valgrind: a genuine stack overflow, NOT heap corruption. `wxWindow::DoSetSize`
fires a `wxEVT_SIZE` event, whose default handler
(`wxWindowBase::InternalOnSize`) calls `Layout()`, which repositions
children, one of which resizing fires another size event — recursing
without ever converging (~12,470 repeating iterations / 100,000+ stack
frames before crashing).

**This is fixed and committed** (commit `aeb332f`, branch `modernize-build`).
Root cause, precisely: both `GatewayPrefs.C` and `GatewayPrefsGUI.C` had a
`wxEventFilter`-based guard (attempts 4 and 5 below) that suppressed
`wxEVT_SIZE` only for the duration of their `Fit()` call, then **re-enabled**
event processing *before* the follow-up `Layout()` call each made afterward
to "fix up final child positions." That unguarded `Layout()` — not `Fit()`
itself — was the actual trigger. Moving `g_suppress... = false` to *after*
`Layout()` in both files closed the gap. Confirmed via an unconditional
`FilterEvent` diagnostic trace (the technique the previous version of this
doc called for) that the filter mechanism was working correctly all
along — it just wasn't armed for the call that mattered. Verified crash-free
across 8+ repro runs after the fix, using the fast-repro recipe below.

Also committed:
- `d709f66` — `EcceDAVClient::getBody()` buffer overread, the real-but-
  unrelated bug already known from earlier sessions (diagnostic trace since
  removed).
- `94ed704` — guarded `getenv("ECCE_SYSDIR")` against `NULL` at its 5 call
  sites (`ResourceDescriptor.C`, `NWDirdy.C`, `MetaInputController.C`,
  `ESInputController.C`, `RunMgmt.C`). Found *while verifying* the crash
  fix: getting past the layout crash exposed that this build's flat
  single-platform CPack layout deliberately no longer sets `ECCE_SYSDIR`
  (old multi-platform subdir routing, not applicable here), so any of
  these call sites null-derefs the first time it runs.

**Reverted, not kept** (proven ineffective, and in one case actively
risky): attempt 1 (`Freeze()`/`Thaw()` in `Gateway.C` — confirmed the
recursion happened entirely inside its wrapped call) and attempt 2
(depth-capped `Layout()` override on `ewxFrame`/`ewxDialog` — worse than
just ineffective: it silently returns `true` without laying out anything
past depth 64, which could mask a *different* future bug as a subtly-broken
UI instead of a loud crash). Both `git checkout`'d back to clean before
committing the real fix.

### Fast repro (for regression-checking the fix, or investigating the new bug below)
```
cd ~/tmp/ecce/ECCE/build-cmake
export ECCE_HOME=/opt/ecce ECCE_REALUSER=$(id -un) ECCE_REALUSERHOME=$HOME HOST=$(hostname) DISPLAY=$DISPLAY ECCE_SYSDIR=""
ulimit -s 8192   # caps stack to 8MB -> fails fast (SIGSEGV) instead of ~20s/~2GB OOM
./gateway
```
Before the fix: instant SIGSEGV, 100,000+ stack frames. After: clean
`exit(1)` on an unrelated, expected error (see "DataServers / EDSI data
server" below) — every time, across repeated runs.

**Always re-export that full env (including `ECCE_SYSDIR=""` now) immediately
before every invocation in a fresh terminal** — a missing `ECCE_HOME`
produces an unrelated `Ecce.C:140` assertion (`"You Must Define ECCE_HOME"`)
during static initialization, before `main()` even runs, which was mistaken
for a new crash signature once already and wasted real time. A missing
`ECCE_SYSDIR` now fails gracefully (see the `94ed704` fix above) rather than
segfaulting, but still export it to reach the real code paths cleanly.

### DataServers / EDSI data server — separate, unported piece of infra
`gateway` fully constructs its GUI (the crash-prone path above) *before*
`GatewayApp::OnInit()` (`GatewayApp.C:142`/`164`) checks connectivity to the
EDSI/WebDAV "ECCE Server" listed in `/opt/ecce/siteconfig/DataServers`. That
config's default entry is still the real PNNL production URL
(`http://eccetera.emsl.pnl.gov:8096/Ecce`), unreachable from a dev sandbox,
so `EDSIServerCentral::checkServerSetup()` (`EDSIServerCentral.C:193-216`,
untouched legacy code) hard `exit(1)`s with a `cerr` message — no dialog, no
offline mode. This is old, pre-existing behavior, not a regression.

Architecturally: old ECCE's "server" was actually **two** independent
pieces, both started by `build/server_admin/start_ecce_server.ecce` — an
Apache 2.2 + `mod_dav` **data server** (`start_ecce_data_server.ecce`, via
`apachectl`; a vendored `httpd-2.2.25.tar.bz2` sits unused in
`build/3rdparty-dists/` alongside the account/htaccess CGI scripts in
`build/server_admin/`) and an ActiveMQ **message server**
(`start_ecce_message_server.ecce`). The modernization session's JMS gateway
work (see the claude.ai project doc) ported and packaged the *message*
server only (`ecce-gateway-start`) — the *data* server half has never been
touched. Standing it up for real is a separate task comparable in scope to
that JMS porting work, not something to fold into this crash investigation.

`EDSIFactory` does support a `file://` (or empty-protocol) scheme via
`FileEDSI` (`src/dsm/edsiimpl/FileEDSI.C`) — just `stat()`s a local
directory, no network — which is how old ECCE could plausibly run without a
real server. Pointing `DataServers`' default `<Url>` at a local `file://`
path would let `gateway` get past this check without standing up Apache,
but `DataServers` is root-owned; doing this needs explicit sign-off before
touching it (asked, not yet done as of this writing).

## `GatewayPrefs::Show()` reentrancy crash — FIXED, committed (2026-08-28)

Found 2026-08-28, same evening as the construction-time fix above, while
trying to visually verify that fix on a real display. **A different bug
from the construction-time one** — same root cause *class* (wx3.2/GTK3
`DoSetSize`/`Layout` reentrancy), different trigger. Not a regression from
the construction-time fix; it's a pre-existing latent bug nobody had
reached before, because `gateway` always died at the `DataServers` check
(previous section) before a user could ever click Preferences.

**This is fixed and committed** (commit `3c50f03`, branch `modernize-build`).
Root cause: `Gateway.C:477`'s `p_prefsDlg->Show(true)` (the real "open
Preferences" handler, not a test artifact) triggers the same
`DoSetSize -> wxEVT_SIZE -> InternalOnSize -> Layout() ->
RepositionChildren -> DoSetSize` cycle as the construction-time bug, but
starting *asynchronously*, well after `Show()` (and `wxEVT_SHOW`) have
already returned/fired — confirmed via a targeted gdb capture (a
conditional Python breakpoint that only stops on genuine `Layout()`
self-reentrancy, not just any call, avoiding the need to unwind the full
crash stack) that the storm begins inside GTK's own recursive
size-allocate cascade, reached through `wxPizza::size_allocate_child()`.

Four timing-based release strategies were tried first and all rejected —
see "History: four failed release strategies" below for the detail. The
key realization that unblocked this: **stop guessing when the trigger
happens and detect it directly**. `SizeEventSuppressor::FilterEvent`
(the same class already used for the construction-time fix) now also
swallows a `wxEVT_SIZE` whenever a call to `Layout()` is already on the
stack when it arrives — i.e. it's genuinely reentrant, not just any
resize — checked via glibc's raw `backtrace()`/`backtrace_symbols()`
(`<execinfo.h>`). An earlier attempt at this exact idea using
`wxStackWalker` crashed with `SIGBUS` from this call context (not further
diagnosed *why* — just confirmed unsafe here); raw `backtrace()` bypasses
wx's wrapper entirely and worked cleanly. Symbol names come back mangled
(e.g. `_ZN12wxWindowBase6LayoutEv`), but a mangled name still contains the
original identifier as a length-prefixed substring, so a plain `strstr()`
for `"Layout"` matches correctly with no demangling needed (confirmed via
`nm -D` on the installed `libwx_gtk3u_core-3.2.so.0` that these symbols
are present in the dynamic symbol table before relying on this).

This mechanism needed no changes to `GatewayPrefs.H` at all (no `Show()`
override, no timer, no new member state) — it's a pure addition inside
the existing global `wxEventFilter`, always active, reacting to actual
reentrancy rather than any fixed time window.

**Verified**: 20/20 safely-capped runs of a standalone test harness
driving the real `Show(true)` path (including three 30-second runs), plus
12/12 runs of the actual `gateway` binary confirming the construction-time
fix still works with no regression (important since the new check runs on
every `wxEVT_SIZE` process-wide, not just within `GatewayPrefs`).

### Test harness (reusable for future work here)
Not committed — lived at `.../scratchpad/GatewayPrefsVisualTest.C` in the
session that wrote it, gone once that session ends. A ~15-line wxApp
subclass, trivial to rewrite: `Ecce::initialize()`, `new GatewayPrefs(NULL)`,
`dlg->Show(true)`, `SetTopWindow(dlg)`. Bypasses `GatewayApp::OnInit()`'s
server checks entirely (per `GatewayPrefs.H`'s own header comment: "this
dialog can become a standalone app"), compiled and linked manually against
the already-built `gateway` object files (`GatewayPrefs.C.o`,
`GatewayPrefsGUI.C.o`, `StateButton.C.o`) and static libs, same compiler
flags `ninja -t commands` shows for the real `gateway` target.

**Safety note, hard-learned**: the first run of this harness had no memory
cap at all and grew to 40GB+ RSS, freezing the machine — this exact
failure mode was already documented above ("multiple GB of RSS...
OOM-killed after ~20s") and should have been guarded against from the
start. Every run after that used a hard, kernel-enforced cap:
```
systemd-run --user --scope -p MemoryMax=1G -p MemoryHigh=768M -p TasksMax=64 --collect \
  timeout -s KILL 15 bash -c "ulimit -s 8192; ulimit -v 2000000; '<binary>'"
```
`ulimit` alone (as used for the construction-time crash) is insufficient
for a fresh, unverified code path — it caps the *stack*, but an unbounded
heap-growth variant of this same bug class would sail past it. Use the
`systemd-run` wrapper for any further live testing of a fresh hypothesis
in this codebase, not just `ulimit`. One exception: capturing a full,
deep crash backtrace with gdb needs the *opposite* — gdb itself needs more
memory/time than the crashing process to unwind 100,000+ frames, so a
tight cap starves gdb, not just the target (see "the breakthrough" below
for how this was worked around).

### History: four failed release strategies, before the working fix
Kept for precedent — if a similar reentrancy bug shows up elsewhere (see
"Also still worth doing" below), these are already-ruled-out approaches:

**Attempt 1 — synchronous `Show()` wrapper**: arm the existing
`g_suppressSizeEventsDuringFit` guard, call the base `Show()`, then disarm
synchronously — same pattern as the construction-time fix. Failed:
confirmed via trace that `OnInit()` (and therefore this synchronous
wrapper) completes and returns *before* the actual storm starts, so
nothing was suppressed by the time it happened.

**Attempt 2 — disarm via `CallAfter()`** (runs once the event loop has
drained everything currently queued) instead of disarming immediately.
Failed: the trace showed the flow working as designed — the callback
fires, calls one manual `Layout()` while still suppressed, disarms — but
a *separate*, later storm still starts right after, with suppression
already off. One `CallAfter` round wasn't enough.

**Attempt 3 — bind `wxEVT_SHOW` as the release signal, plus a 2s
safety-net `wxTimer`**, based on reading wxWidgets' GTK3 backend source
(`src/gtk/toplevel.cpp`, `wxTopLevelWindowGTK::Show()`/
`GTKDoAfterShow()`/`GTKUpdateClientSizeIfNecessary()` — see "Is this a wx
version issue?" below). Theory: `Show(true)` on wxGTK3 can defer the
actual `gtk_widget_show()` while it round-trips with the window manager
for `_NET_FRAME_EXTENTS`, and only once that completes does it send
`wxEVT_SHOW` — so binding release to that event should bracket the real
trigger. Failed anyway. A follow-up gdb capture (see below) later showed
why: the real trigger happens well after `wxEVT_SHOW` fires, not bounded
by it as the theory assumed.

**Attempt 4 — fixed 1.5s timer as the sole release mechanism** (dropping
the idle-quiescence debounce that was tried in between and also failed —
it detected a real but misleading lull before the actual GTK cascade even
started, releasing too early). This one is the most important negative
result: **non-deterministic**. 10/13 runs survived in one batch; a
separate batch was 5/8. Since only the timer duration changes between
runs, and 1.5s isn't a reliably safe threshold, no fixed duration can be
trusted — the real trigger's timing genuinely varies run to run.

### The breakthrough: catching the trigger directly instead of timing it
After attempt 3 failed, rather than keep guessing release points, tried
to find the true entry point into the storm directly:
- Breaking on `wxWindowBase::InternalOnSize` and capturing the very first
  hit of the whole program wasn't useful — it's ordinary construction-time
  noise (a `wxStaticBox` being created), not the storm; a hit counter
  can't distinguish routine calls from the start of runaway recursion.
- Capturing the *bottom* of the stack at crash time (`bt -40`, oldest
  frames) hit a practical wall: gdb itself needs real memory/CPU to
  unwind and symbolize 100,000+ frames, directly in tension with the
  safety caps this bug requires — a short timeout cut gdb off mid-unwind,
  a long one let gdb itself run out of virtual memory
  (`./gdb/utils.c:666: internal-error: virtual memory exhausted`).
- **What worked**: a gdb Python breakpoint (`gdb -x script.py`) on
  `wxWindowBase::Layout` whose `stop()` method walks up to 100 frames via
  `gdb.newest_frame()`/`.older()` and only actually stops if `"Layout"`
  appears in 2+ frame names — i.e. only on genuine self-reentrancy, not
  just any call. This catches the *first* reentrant call, at a shallow
  depth (tens of frames, not 100,000+), so gdb can unwind and print it
  instantly with no memory pressure at all. This single technique is what
  unblocked the whole investigation — see `/tmp/reentrant_break.py` in
  that session's transcript if reconstructing it.
- That capture showed the real trigger: the storm starts inside
  `wxPizza::size_allocate_child()` (wx's internal GTK container class),
  reached through GTK's own recursive native `size_allocate` signal
  cascade (`gtk_widget_size_allocate_with_baseline` calling itself down
  through nested containers — normal GTK behavior), which at some depth
  calls back into wx's C++ `wxEVT_SIZE` dispatch and recurses
  independently on top of it. Confirmed via the `[TEST]` markers that
  this happens *after* `OnInit()` (and therefore `Show()` and
  `wxEVT_SHOW`) have already returned — explaining why attempts 1-3 all
  released too early, and why attempt 4's timer was racing something with
  inherently variable timing (matches this being driven by GTK/window-
  manager negotiation, not anything in ECCE's own control flow).
- This same technique also correctly identified a *false positive*: a
  legitimate 5-deep nested `wxGridSizer` → `wxStaticBoxSizer` →
  `wxBoxSizer` layout pass also has "2+ Layout frames," but terminates
  cleanly after 48 total frames with no further recursion — i.e. ordinary
  nested layout work, not the pathological storm. The eventual fix (next
  section) swallows this case too, same as it swallows the real storm,
  which turned out to be fine: sizers position children via direct
  `SetDimension`/`DoSetSize` calls during the same `Layout()` pass, not by
  relying on the swallowed event's own cascade, so correctness doesn't
  depend on that inner event actually re-dispatching.

### Is this a wx version issue? Yes, plausibly — but not fully confirmed
Researched whether this class of bug is a known wx3.2/GTK3 regression
(GTK3 didn't exist when wx2.8 was released in 2008, so none of this
machinery existed under the old toolkit ECCE originally targeted):
- wxWidgets GitHub issues [#17585](https://github.com/wxWidgets/wxWidgets/issues/17585)
  ("GTK+3.20 sizing failures") and [#22668](https://github.com/wxWidgets/wxWidgets/issues/22668)
  ("GTK3 Refresh or Layout bug when hiding and showing multiple panels")
  document real, acknowledged GTK3-specific sizing/layout bugs in
  wxWidgets, including cases where `SetSize()` calls get silently
  overridden after `Fit()`, and layout bugs specifically tied to
  `Show()`/`Hide()` cycles on GTK3.
- wxWidgets' own GTK3 backend (`src/gtk/toplevel.cpp`, confirmed against
  the `v3.2.8` tag — matches the installed `libwxgtk3.2-1t64 3.2.8+dfsg-2`
  on niobium) has machinery that has no GTK2/wx2.8 equivalent:
  `wxTopLevelWindowGTK::GTKDoAfterShow()`, `GTKUpdateClientSizeIfNecessary()`,
  `m_pendingFittingClientSizeFlags`, and a documented deferred-`Show()`
  path that waits for a `_NET_REQUEST_FRAME_EXTENTS` window-manager
  property notification before actually realizing the window. This is
  genuinely new surface area introduced by the GTK3 backend.
- wx's own GTK3 `size_allocate` signal callback already has a recursion
  guard (`wxRecursionGuard setInSizeAllocate(g_inSizeAllocate)`) — but
  that protects GTK's *native* signal callback from reentering itself, a
  different code path from the one actually crashing here (our storm is
  entirely within wx's own C++-level `wxEVT_SIZE`/`InternalOnSize`/
  `Layout()` event dispatch, which has no equivalent guard). This may
  explain why wx's own upstream fixes for the GTK3 issues above don't
  cover this specific case.
- **Caveat**: the wx-source research above came from an AI-summarized
  fetch of `toplevel.cpp`, not a byte-exact reading of the full ~180-line
  `Show()` function — treat the specific claims about exactly when
  `wxEVT_SHOW` fires as directionally credible, not verified fact. Worth
  re-reading that function directly (not summarized) before trusting a
  fix design on it again.
- **Bottom line**: strong circumstantial evidence this class of bug is
  substantially a wx3.2/GTK3 regression rather than something pre-existing
  in ECCE's code that wx2.8/GTK2 simply tolerated — but the exact trigger
  for *this* widget tree's non-convergence hasn't been pinned down
  precisely enough to say that with full certainty, or to know whether an
  upstream wx patch/newer wx point release might already fix it.

### Still open after this fix
Both known reentrancy crashes (construction-time and `Show()`-triggered)
are now fixed and committed. What's left:
- **Manual interactive resize testing** — still not done. Both fixes are
  verified via safely-capped automated runs (no crash across many
  repetitions), not by an actual human resizing the real toolbar and
  Preferences dialog on screen, per the original investigation's
  checklist. Still blocked on either a working `DataServers` setup or the
  standalone test harness (see above) — the harness never actually
  `Raise()`s/focuses the window in a way that's been confirmed visually
  correct, only confirmed non-crashing.
- **`FilterEvent` now runs `backtrace()`/`backtrace_symbols()` on every
  `wxEVT_SIZE` process-wide** (when not already in the manually-suppressed
  construction window) — correctness is verified, but performance/overhead
  during rapid real interactive resizing (a user actively dragging a
  window edge, which can fire many `wxEVT_SIZE` events per second) hasn't
  been measured. `backtrace_symbols()` calls `malloc` internally and does
  real symbol-table work each time; worth profiling if resizing ever feels
  laggy once there's a real display session to check on.
- Consider whether the two guards in `GatewayPrefs.C`
  (`g_suppressSizeEventsDuringFit` for construction,
  `isReentrantSizeEvent()` for everything else) should be consolidated —
  the second one is actually more general and might have been sufficient
  alone for construction too, just not verified against that path
  specifically since the manually-scoped guard there already works and
  wasn't worth risking a change to.

## Also still worth doing (from the original investigation, unchanged)
This same `Fit()`/`SetSizeHints()` structure exists across dozens of other
`*GUI.C` files in ~15 other `ECCE_GUI_APPS` (confirmed via grep — e.g.
`src/apps/builder/*GUI.C`, `src/apps/machbrowser/*GUI.C`,
`src/apps/calced/*GUI.C`, and many more), since they're all generated by the
same wx designer tool. **None of them have ever been reported crashing**,
and the actual bugs found here were specific to `GatewayPrefs`'s particular
widget tree not converging (construction-time) and GTK's first-`Show()`
realize behavior (the new bug above) — not "`Fit()`+`SetSizeHints()` is
inherently dangerous." Treat this as a watch-list item if any other app is
ever reported freezing/crashing on startup or on opening a dialog, not as
something to preemptively patch without evidence.
