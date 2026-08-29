# ECCE — context for Claude Code sessions in this repo

This repo (`FriendsofECCE/ECCE`, `main` branch, local checkout on niobium)
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

## Branch structure (as of 2026-08-29)
This repo now uses a single active branch, `main` — it's the GitHub default
and the only branch you should ever need to work on or push to. Releases are
marked with tags (e.g. `v7.3.4-beta` was the last one cut), not a separate
release branch. If anything here still says `develop` or `modernize-build`,
treat that as referring to `main` — those names were merged into it.

Everything that used to be scattered across `develop` (the long-running but
stale nominal default), `modernize-build` (where all of this modernization
work actually happened, forked from `develop`'s tip with zero divergence),
`stable`, `master`, and `make` has been consolidated: `main` is exactly
`modernize-build`'s former tip, pushed under the new name and made the
default. The old branches weren't deleted, just renamed to `archive/develop`,
`archive/modernize-build`, `archive/stable`, `archive/master`, `archive/make`
— full history is still there if anything needs digging up, but there's no
reason to branch from or compare against them going forward. A same-session
audit found `stable` had nothing `develop` was missing except two Quick Basis
Menu entries (deliberately not ported over — "when in doubt, stick with
develop, not stable" was the standing call) and was otherwise strictly
behind (older XC-functional mappings, a real bug in `std2Gaussian-16` calling
the wrong writer function, a smaller vendored basis-set library) — so nothing
of substance was lost in the consolidation.

## Where things are
- Build directory: `build-cmake` (NOT `cmake-build`). Rebuild with `ninja`
  from inside it, or `cmake --build .`.
- All the modernization work in this file happened directly on `main` (until
  today named `modernize-build` — see "Branch structure" above). Fixes below
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

**This is fixed and committed** (commit `aeb332f`, branch `main`, formerly `modernize-build`).
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

### DataServers / EDSI data server — FIXED, implemented (2026-08-28)
`gateway` fully constructs its GUI (the crash-prone path above) *before*
`GatewayApp::OnInit()` checks connectivity to the EDSI/WebDAV "ECCE Server"
listed in `/opt/ecce/siteconfig/DataServers`, in
`EDSIServerCentral::checkServerSetup()` (`EDSIServerCentral.C:193-216`,
untouched legacy code, hard `exit(1)`s with a `cerr` message on failure — no
dialog, no offline mode).

Architecturally, old ECCE's "server" was actually **two** independent
pieces, both started by `build/server_admin/start_ecce_server.ecce` — an
Apache + `mod_dav` **data server** and an ActiveMQ **message server**. The
message server was already ported in an earlier session (`ecce-gateway-start`).
The data server is now ported too — a per-user, non-root Debian `apache2`
2.4 instance (not the vendored `httpd-2.2.25.tar.bz2`, which risks a real
`mod_ssl`-vs-OpenSSL-3.x build break for no functional gain; ECCE's config
needs nothing 2.2 has that 2.4 lacks). New files:
`packaging/dataserver/httpd.conf.ecce` (2.2→2.4-ported config, LoadModule
block added since Debian's apache2 is DSO-based and the legacy template
assumed a static build) and `packaging/dataserver/ecce-dataserver-{start,
stop,status,adduser}` (mirroring `ecce-gateway-*`'s idioms exactly). Runs
under `$ECCE_REALUSERHOME/.ECCE/dataserver`, port 8096, seeded from the
already-shipped `data/admin/dataserver/server_config/data_server_files.tar`.
`CMakeLists.txt` repoints `siteconfig/DataServers` (both `<Url>` and
`<BasisSet>`, same `file(READ)`/`REGEX REPLACE`/`file(WRITE)` pattern as
`jndi.properties`) to `localhost`, flips `ECCE_AUTO_ACCOUNTS` off in
`siteconfig/site_runtime` (the Perl CGI self-service account flow is
deliberately deferred — a missing CGI degrades gracefully in the client,
confirmed by tracing `EcceDAVClient::doPost()`; use `ecce-dataserver-adduser`
instead), and adds `apache2, apache2-utils` to `CPACK_DEBIAN_PACKAGE_DEPENDS`
(`dpkg-shlibdeps` only catches linked `.so`s, not a spawned subprocess).

**Two real bugs found and fixed while verifying, both worth knowing if this
ever needs revisiting**:
1. The vendored seed data's own `.htaccess` files (under `Ecce/users/`,
   `Ecce/share/`) use legacy `Order`/`Allow`/`Deny` syntax — Apache 2.4
   refuses to load them without `mod_access_compat` (`Invalid command
   'Allow'`). Loaded that module rather than rewriting vendored data files.
2. **The actual root cause of `checkServerSetup()` still failing after
   everything else was correct**: `Options Indexes` was set on the `Ecce/`
   directory (matching the legacy config) but `mod_autoindex` was never
   loaded — without it, a bare directory request with no `index.html`
   returns 404 instead of a listing. `EDSIServerCentral`'s very first check
   is a HEAD against the bare `<Url>` from `DataServers`
   (`http://localhost:8096/Ecce`, no trailing slash) — Apache 301-redirects
   that to `/Ecce/` (a behavior `EcceDAVClient::execute()` already has
   built-in retry logic for, confirmed reading the code — a "GDB 4/26/02
   Hack to support Apache2" comment shows this exact problem was already
   solved once before, decades ago), but the *retried* request against
   `/Ecce/` was **also** 404ing because of the missing `mod_autoindex` —
   which `DavEDSI::resourceExists()` correctly reports as "doesn't exist"
   (empty error message, not a connection failure — confirmed via a
   temporary diagnostic trace, since removed), exactly matching the generic
   "cannot establish a connection" message this investigation started from.
   Fixed by loading `mod_autoindex`.

**Verified end-to-end against the real `gateway` binary**, not just raw
`curl`: `ecce-dataserver-start` + `ecce-dataserver-adduser -b <user> <pass>`,
then the exact repro command from "Fast repro" above — `gateway` no longer
hits the hard `exit(1)` path at all; it runs past `checkServerSetup()` into
real application UI, confirmed both by process still running past the repro
window and by an actual screenshot showing a genuine "ECCE Authentication"
dialog (username pre-filled, waiting for a password) on screen. Also
confirmed: the negative path (data server stopped → `gateway` still cleanly
`exit(1)`s with the original message, so the fix isn't a no-op) and start
idempotency (running `ecce-dataserver-start` twice doesn't re-seed over live
data).

Not yet done: actually typing a password into that dialog and confirming
what happens next in the GUI flow (a `Gtk-CRITICAL: gtk_editable_get_chars`
warning appears periodically while the dialog is up — not fatal, dialog
still renders and accepts input, but worth a look if anything downstream of
login misbehaves). `EDSIFactory` also supports a `file://` (or
empty-protocol) scheme via `FileEDSI` for a no-server-at-all setup, if ever
wanted instead — not needed now that the real data server works.

## `GatewayPrefs::Show()` reentrancy crash — FIXED, committed (2026-08-28)

Found 2026-08-28, same evening as the construction-time fix above, while
trying to visually verify that fix on a real display. **A different bug
from the construction-time one** — same root cause *class* (wx3.2/GTK3
`DoSetSize`/`Layout` reentrancy), different trigger. Not a regression from
the construction-time fix; it's a pre-existing latent bug nobody had
reached before, because `gateway` always died at the `DataServers` check
(previous section) before a user could ever click Preferences.

**This is fixed and committed** (commit `3c50f03`, branch `main`, formerly `modernize-build`).
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

## Post-login usability bugs found by actually using the app (2026-08-28)

Once login worked end-to-end (previous sections), several real bugs
surfaced simply by clicking around the running GUI. All fixed except the
last, and none related to the wx3.2/GTK3 layout-reentrancy class above —
worth keeping separate for that reason.

### Organizer: missing xpm icon — FIXED
Clicking Organizer threw an error about a missing xpm file. Root cause: a
letter-transposition typo in `data/client/config/ResourceDescriptorRxn.xml`
line 3161 — `<Selected>tasconkrxmetadyn.xpm</Selected>` vs the correct
`taskconrxmetadyn.xpm` (matching the real file in
`data/client/pixmaps/`, and matching the `<Normal>`/`<Expanded>`/
`<SelectedExpanded>` entries in the same block, which all had it right).
Fixed by correcting the one typo'd entry. Verified via grep across every
`ResourceDescriptor*.xml` that this was the only missing-icon reference (24
total icon references checked, zero missing after the fix).

### Gateway: crash on clicking Help — FIXED
`BrowserHelp::initialize()` (`src/util/genutil/BrowserHelp.C:138`) throws an
uncaught `NullPointerException` if the `ECCE_HELP` env var isn't set — and
nothing in `Gateway.C`'s `wxID_HELP` handler catches it, so the whole app
dies. This build's wrapper scripts never set `ECCE_HELP` at all (nothing
did, historically — the old tcsh launcher scripts this build deliberately
doesn't use apparently did). Fixed two ways together:
1. `CMakeLists.txt`'s per-app wrapper `file(WRITE)` block now exports
   `ECCE_HELP=file:///opt/ecce/data/client/WebHelp/` by default (only if
   unset, so it's still user-overridable).
2. That path needs real content to point at: added a new CMake block
   (mirrors the existing ActiveMQ-tarball-extraction pattern) that extracts
   the already-vendored-but-never-wired-up
   `data/admin/dataserver/help/eccehelp.tar` into
   `${CMAKE_BINARY_DIR}/ecce-help-extracted/EcceHelp/` at build time and
   installs it to `data/client/WebHelp/EcceHelp/` — confirmed via `tar tf`
   that `homepage.html` sits at the tarball's root. `BrowserHelp::URL()`
   prepends `ECCE_HELP` directly for any non-`http://` help reference (see
   `data/client/config/help.urls`'s own comment), so this now resolves to
   real local file:// help content instead of crashing.

Not ported (deliberately, matches the "degrades gracefully" pattern used
for the data server's CGI account-creation flow): the `cgi-bin/help/
cshelp`/`toolhelp` dynamic help backend some `help.urls` entries reference —
those specific entries will 404 gracefully rather than resolve, everything
else (the bulk of `help.urls`, including Gateway's own homepage) works.

### Periodic Table: window opens but content area completely blank — FIXED
`ecce-pertable` opened a correctly-sized window (confirmed via `xwininfo`:
640×467, not a zero-size bug) with menu bar intact, but the entire element
grid was blank — no visible buttons, just faint single-pixel dots at each
element's grid position (confirmed via screenshot; the *positions* were
already correct, matching the periodic table's real shape, which is what
pointed at a per-item sizing bug rather than a data-loading or layout-
positioning one).

Root cause: `PerTabPanel.C:162`, each `ElementButton`'s
`wxGridBagSizer::Add()` call passed `wxEXPAND|wxALL|wxALIGN_CENTER`.
Combining `wxEXPAND` with an alignment flag on the same item is a
documented wx footgun — alignment can suppress the expand behavior instead
of being (as the docs say it should be) ignored in favor of it. Each
`ElementButton` only learns its own real size *inside its first
`OnPaint()`* (`ElementButton::OnPaint()` calls `SetMinSize()` based on
measured text extents — a chicken-and-egg pattern: it needs to be painted
to know its size, and needs a size to be painted), so before that first
paint its best/min size defaults to near-zero — and with `wxALIGN_CENTER`
in the mix, the sizer centered that near-zero-size item within its
(correctly large, growable) cell instead of expanding it to fill it. Fixed
by dropping `wxALIGN_CENTER`, leaving just `wxEXPAND|wxALL` — confirmed via
rebuild + screenshot that the full 118-element table now renders correctly,
colored and labeled, filling the window.

The panel's *other* `wxALIGN_CENTER`+`wxEXPAND` combination
(`PerTabPanel.C:148`, the outer `wxBoxSizer` holding the whole grid) was
left alone — confirmed by the same screenshot that it isn't causing a
problem (a plain `wxBoxSizer` apparently doesn't hit the same wx quirk a
`wxGridBagSizer` does here). Not changed without evidence it's broken.

**Worth checking**: `MiniPerTab.C` (`src/apps/builder/`) reuses this same
`PerTabPanel` in `isMini` mode for the Builder's embedded periodic table —
this fix should apply there too, but hasn't been visually re-verified
against Builder specifically yet.

### Builder: crash when building molecules — FIXED (2026-08-28)

First attempt at a coredump failed: `ulimit -c unlimited` in the launching
shell hit `bash: ulimit: core file size: cannot modify limit: Operation not
permitted` — the shell's *hard* limit was already stuck at 0 (confirmed via
`ulimit -Hc`), and a non-root process can only lower a hard limit, never
raise it back, regardless of `/etc/security/limits.d/*` being correctly
configured for `infinity` (that only applies to freshly-spawned sessions
that go through PAM, not this already-running shell). Worked around
without needing root or a relogin: `systemd-run --user -p
LimitCORE=infinity --pipe --collect <cmd>` sets the resource limit directly
on a *fresh* transient unit, bypassing the stuck shell entirely. (Note:
must be a plain `systemd-run --user` transient *service*, not `--scope` —
`--scope` rejected `-p LimitCORE=...` with "Unknown assignment" on this
systemd version; some exec-related unit properties aren't settable on
scopes.) With that, a real coredump was captured on the next repro.

Root cause: `Builder::showMessage()` (`Builder.C:2176`, now fixed) —
```cpp
wxLogError(msg.c_str(), 0);      // msg used AS the format string
```
`wxLogError`/`wxLogWarning`/`wxLogMessage` treat their first argument as a
**printf-style format string**, not literal text — but `msg` here is
dynamic, runtime-generated content (confirmed via the crash backtrace:
`Builder::execute()` → `Builder::reportError(ex.what())` → `showMessage()`,
i.e. an arbitrary `EcceException::what()` message from a failed
builder command, which can legitimately contain a literal `%`, e.g. a
percentage or stray symbol in a validation-error message). A `%s` (or
`%o`, etc.) inside that text with no corresponding vararg reads whatever
garbage happens to be in the next unfilled argument slot — a classic
uncontrolled-format-string bug. When that garbage is an invalid pointer
(as it was in the field), `wxFormatString::AsWChar()` segfaults trying to
dereference it. The crash backtrace pinpointed this precisely (extracted
via `coredumpctl dump <PID> -o core; gdb build-cmake/builder core` — note:
`coredumpctl gdb` alone resolves against the *installed*, CPack-stripped
`/opt/ecce/bin/builder`, which has no debug symbols per
`CPACK_STRIP_FILES ON`; pointing gdb at the unstripped `build-cmake/`
binary with the same core file resolves full symbols and source lines):
```
#1  wxFormatString::AsWChar()
#2  wxFormatString::operator wchar_t const*()
#3  wxLogger::Log<int>(...)
#4  Builder::showMessage(msg, error=true) at Builder.C:2179
#5  Builder::reportError(msg) at Builder.C:970
#6  Builder::execute(cmd, batch=false) at Builder.C:2246   <- catch (EcceException&) { reportError(ex.what()); }
#7  Builder::processSelectionChange(sel) at Builder.C:1550
```

Fixed by always passing a literal `"%s"` format with the message as its
substituted argument: `wxLogError("%s", msg.c_str())`. Grepped for the same
anti-pattern across the whole tree (`wxLog(Error|Warning|Message)\(` calls
without a literal format string) and found **8 more identical sites**, all
clearly copy-pasted from the same origin — one still had the original
author's own comment nailing exactly why: `ewxNumericValidator.C:834`,
`// GDB 3/4/12  Added 0 arg to suppress compiler warning`. That stray `, 0`
across all 9 sites was never a real second argument; it was added purely
to silence `-Wformat-security`'s "format string is not a string literal"
warning without addressing what the warning was actually about. Fixed all
9, same pattern, in `Builder.C` (×4), `TrajectoryPanel.C` (×3),
`BuilderApp.C` (×1), `ewxNumericValidator.C` (×1).

**Verified two ways**:
1. Real repro, real fix: rebuilt `builder`+`ninja` (whole tree) clean after
   the fix, no new warnings introduced.
2. Standalone mechanism proof (not full build's-worth): a tiny wx console
   app calling both the buggy and fixed pattern with `"...95% of maximum
   (%s)..."` (memory-capped via `systemd-run` per the standing safety
   practice below) — buggy version corrupts/misinterprets the message
   (confirmed two ways: a `%o`-shaped message silently ate a wrong
   character range, and a `%s`-shaped message read garbage from an
   unfilled vararg slot instead of crashing *this specific run* — true
   undefined behavior, non-deterministic depending on what's in that
   register/stack slot, matching this repo's established pattern of UB
   bugs not reproducing identically every run); fixed version preserves
   the message verbatim in both cases.

This is unrelated to the wx3.2/GTK3 layout-reentrancy bug class
documented above — a plain uncontrolled-format-string bug, not
wx-version-specific, that just happened to go unnoticed until dynamic,
runtime-generated error text with a `%` in it hit one of these 9 call
sites for the first time.

## `EcceException::what()` dangling pointer — FIXED (2026-08-28)

Found while investigating the builder crash above: `what()`
(`src/util/exceptions/EcceException.C`) built a local `std::string
baseMessage` and returned `baseMessage.c_str()` — a pointer into a stack
object destroyed the moment `what()` returns. Every caller of `ex.what()`
across the whole codebase was reading freed stack memory, which happened to
look correct most of the time (undefined behavior, not consistently wrong)
but explains both the garbled `Throw Log:` text seen while investigating
the crash above (binary garbage instead of the real exception message) and
plausibly contributed to the original crash's non-determinism. Fixed by
returning `runtime_error::what()` directly — the base class already owns a
copy of the message with a lifetime tied to the exception object, no
temporary needed.

## Structure Library: blank list after selecting any library — FIXED (2026-08-28)

`Builder`'s "Import from structure library" panel populated the `Libraries`
dropdown correctly (reading top-level folder names via `EDSIFactory`/
`FileEDSI`, confirmed working via live gdb tracing) but the list of
structures/folders below it stayed completely blank no matter which library
was selected — confirmed via gdb that `Resource::getChildren()` DID
correctly resolve every child (folder or `.mvm` file) and that
`wxListCtrl::InsertItem()` WAS being called the right number of times; the
data pipeline was fully correct.

Root cause: `StructLib.C`'s `refreshStructures()` builds each row as a
`wxListItem` and calls `InsertItem(item)` (the overload that inserts at
`item.m_itemId`) without ever calling `item.SetId(...)` — `m_itemId`
defaults to an invalid index. A minimal standalone repro (`wxListCtrl` in
`wxLC_LIST` mode, same `wxImageList`/icon setup) reproduced this exactly:
`./src/generic/listctrl.cpp(4609): assert ""item.m_itemId >= 0"" failed in
InsertItem()`. In a RelWithDebInfo build the assert doesn't abort, so the
app just silently drops every item instead of crashing. The function's own
comment ("all new items put into the list appear at index = 0... insert
them in reverse order") shows the ORIGINAL intent was always index 0 — it
was just never actually set. Fixed by adding `item.SetId(0);` at both
`InsertItem()` call sites (folder branch and file branch). Confirmed no
other `wxListCtrl::InsertItem(wxListItem&)` call sites exist without an
explicit ID elsewhere in the tree (`ewxFileCtrl.C` already sets
`item.m_itemId = 0` directly; `WxTableView.C` uses the different
`InsertItem(long, wxString, int)` overload that takes the index as a plain
argument, unaffected).

## `SparseBits` — real crash + two latent logic bugs — FIXED (2026-08-28)

A second, distinct builder crash (SIGSEGV) surfaced once the structure
library fix above let real structures actually get imported and rendered.
Root-caused via a real coredump (see "Getting a coredump" note below) +
`gdb bt` against the unstripped `build-cmake/builder` binary:
```
SparseBits::operator^=      SparseBits.C:295 -> clearBit(*it)
  -> std::unordered_set::erase(iterator)   <- crash here
ChemSSSR::findFragments      ChemSSSR.C:1646
ChemSSSR::ChemSSSR (ctor)    ChemSSSR.C:585
ChemDisplay::generateDoubleBondList / computeBBox
```
Ring-perception (SSSR) on a freshly-imported 52-atom structure crashed
inside `SparseBits`, `src/inv/moiv/SparseBits.C`. Two independent bugs
stacked together:
1. `operator^=` (line 291, now fixed): `bool set = (*it == (true ^
   testBit(*it)));` compares an arbitrary **integer bit index** (`*it`,
   e.g. 0-51) against a **bool** XOR result — nonsensical, and only ever
   true by accident when the index happens to be 0 or 1. The correct XOR
   logic is simply `!testBit(*it)` (rhs's bit is known true for every
   `*it` in `rhs.p_set`, so the XOR result is just "not already set here").
   The bug meant `clearBit()` got called for indices NOT actually present
   in the set far more often than intended.
2. `clearBit()` and `setBitTo(..., false)` (now fixed): both did
   `p_set.erase(p_set.find(index))` — if `index` isn't present, `find()`
   returns `end()`, and **erasing `end()` is undefined behavior**. That's
   the actual segfault. Fixed by using safe erase-by-key,
   `p_set.erase(index)` (a no-op if absent), everywhere this pattern
   appeared.

Also found and fixed while reading this same ~30-line block:
`operator|=` iterates `rhs.p_set.begin()` but its loop condition compared
against `p_set.end()` (`this`'s end iterator, not `rhs`'s) — comparing
iterators from two different `unordered_set` instances is undefined
behavior. Fixed to compare against `rhs.p_set.end()`. Not the crash seen
here, but the same function block, same bug class, same file — fixed
alongside the other two rather than left for a future session to rediscover.

Verified via `ninja builder` (clean rebuild) + reproducing the exact import
that crashed before, with a coredump-enabled launch (see below) as a
safety net — no crash, no new coredump.

### Getting a coredump when `ulimit -c unlimited` fails with "Operation not permitted"
Hit this mid-investigation: the launching shell's *hard* core-size limit
was already 0 (`ulimit -Hc` → `0`), and a non-root process can only lower a
hard limit, never raise it — `/etc/security/limits.d/*` being correctly
configured for `infinity` doesn't matter for an *already-running* shell
that predates that config taking effect. Fix that doesn't need root or a
relogin: launch through a **transient systemd unit**, not `ulimit`:
```
systemd-run --user -p LimitCORE=infinity --pipe --collect \
  env <the usual ECCE_HOME=... etc> ./builder
```
Must be a plain `systemd-run --user` **service** (no `--scope`) —
`--scope` rejected `-p LimitCORE=...` with "Unknown assignment" on this
systemd version; some exec-related unit properties aren't settable on
scopes. Once captured, resolve full symbols/source lines against the
**unstripped build-tree binary**, not the installed one (CPack strips the
latter — see the first builder-crash section above for the same gotcha):
`coredumpctl dump <PID> -o core.dump; gdb build-cmake/builder core.dump`.

### `perf` profiling needs `perf_event_paranoid` lowered
`perf record -p <pid>` failed outright with a permissions error even after
installing `linux-perf` — Debian's default `kernel.perf_event_paranoid=3`
blocks it entirely for non-root. Fixed (session-only, not persisted) via
`sudo sysctl kernel.perf_event_paranoid=1`. Add `kernel.perf_event_paranoid
= 1` to `/etc/sysctl.conf` if this needs to survive a reboot.

## Builder 3D viewer: rotation/insertion never redrew until an unrelated event — PARTIALLY FIXED (2026-08-28)

Two symptoms, same root cause: newly-added structures (via "Import from
Structure Library") and camera rotation (both the dedicated toolbar
"Rotate" mode and right-click-drag in Select mode) updated their
**underlying state correctly** (confirmed: atom count and the Rotation
X/Y/Z fields at the bottom of the viewer updated) but the **3D viewport
never visibly repainted** until some unrelated later event (switching
modes, then clicking) forced a repaint — at which point it "jumped" to
show the fully-accumulated state all at once.

Root cause: this is Open Inventor's own `SoSceneManager` render-callback
mechanism (`SoWxRenderArea::renderCB`,
`src/inv/wxinv/SoWxRenderArea.C:1038`) not firing reliably during
interactive drag/command-execution under wx3.2/GTK3 — matching a
previous developer's own diagnosis, left as a comment right at the exact
spot that needed it (`SGViewer::processEvent`,
`src/wxviz/viewer/SGViewer.C`): `// @todo This wasn't needed. Some
callback function is not working! //  Refresh(false);` — i.e. someone
already found this exact gap and gave up rather than root-causing why the
"working" mechanism wasn't firing. `renderCB` itself, when it *does* fire,
correctly does `renderArea->Refresh(false); renderArea->Update();` — that
became the template for the fix.

Fixed two call sites to explicitly force what `renderCB` was supposed to
be doing automatically:
- `SGViewer::processEvent()` (mouse-move/drag handling) — re-enabled the
  commented-out call, now `p_renderArea->Refresh(false);
  p_renderArea->Update();` (targeting the actual `wxGLCanvas` child
  directly, not `this`/the wrapping `wxPanel` — see flicker note below).
- `Builder::execute()` (`src/apps/builder/Builder.C`, after a command
  successfully modifies the scene graph) — added a new public
  `SGViewer::refreshRenderArea()` wrapper (declared in `SGViewer.H`) since
  `p_renderArea` itself is `protected` on the base `SoWxViewer` and not
  reachable from `Builder.C`, which only holds a `p_viewer` pointer by
  composition, not inheritance.

**Structure-library insertion is fully fixed and confirmed** ("Insertion
works perfectly" — user, live-tested). **Rotation is now live/responsive
but flickers** — confirmed via the user's own memory that old
(pre-wx3.2-port) ECCE did NOT flicker, so this is a wx3.2/GTK3 regression,
not inherent to the old immediate-mode renderer. Four variants tried
live, in order, none eliminated the flicker without a worse regression:
1. `Refresh(false)` on `this` (the wrapping `wxPanel`, not the render
   area) — compiled, but was refreshing the wrong window (an old
   commented-out leftover verbatim).
2. `p_renderArea->Refresh(false)` alone — **rotation became responsive**,
   flickers.
3. `p_renderArea->scheduleRedraw()` instead of `Refresh()` — regression:
   back to no live update at all. `scheduleRedraw()` only marks something
   dirty for the *existing* (broken) auto-redraw path to pick up later; it
   doesn't force an actual paint the way `Refresh()`+`Update()` does.
4. `p_renderArea->Refresh(false); p_renderArea->Update();` (matching
   `renderCB`'s own pattern exactly) — still flickers, no better than
   plain `Refresh()`.
5. Same as #4 but guarded with a new `p_inPaint` reentrancy check
   (mirroring the guard `renderCB` itself uses) — regression: flicker
   *and* dropped/jumping frames, worse than #4.

**Left in place: variant #4** (`Refresh()`+`Update()`, no guard) — the
best of the tried options: fully responsive, matches the codebase's own
existing pattern for this exact purpose, flicker is a cosmetic regression
rather than a functional blocker. The `isInPaint()` public accessor added
for variant #5 was removed again since nothing uses it in the final state.

**Not yet root-caused**: *why* `renderCB` doesn't fire reliably in the
first place (the deeper question, unanswered) — worth checking whether
`p_sceneMgr`'s attached scene graph actually includes the camera node
`rotateCamera()`/`spinCamera()` mutate, since Inventor's field-notification
system only triggers a scene manager's callback for changes within the
graph it's actually watching. Also unexplored: whether GTK3's compositing
model changed how `wxGLCanvas`'s double-buffered `SwapBuffers()`
interacts with an externally-forced `wxWindow::Update()` in a way GTK2
tolerated but GTK3 doesn't — the user's confirmation that old ECCE never
flickered points at exactly this class of explanation. A profiling
session (`perf record` while dragging, see note above for the
`perf_event_paranoid` gotcha) showed essentially **zero** time in any
`SoGL`/OpenGL/`SoWxRenderArea` code during rotation — all measurable cost
was in wx's own event-dispatch/idle-event machinery
(`wxWindowBase::SendIdleEvents`, `wxToolBarBase::UpdateWindowUI`,
`wxEventHashTable::HandleEvent`) — so the flicker is very unlikely to be a
raw rendering-performance problem; it's specifically a paint-timing/
compositing issue.

## Help: page opened but broken images / dead links — FIXED (2026-08-28)

Once the earlier `ECCE_HELP` crash fix (see above) had Help actually
opening a page, two further problems surfaced:
1. **Every embedded image was broken.** The vendored help HTML uses
   *absolute* paths for images, e.g. `<img src="/EcceHelp/media/foo.gif">`
   — correct only when served over HTTP with `/EcceHelp` as a real URL
   root (its original PNNL deployment target). Pointing `ECCE_HELP` at a
   raw `file:///opt/ecce/data/client/WebHelp/` URL (the previous fix)
   made the *page itself* load fine (relative links work), but a browser
   resolves that same absolute `/EcceHelp/...` path against the
   filesystem root, not the help tree — nothing there, so every image
   404'd locally.
2. **Some links 404 for a different, already-known reason**: entries like
   `/cgi-bin/EcceHelp/toolhelp?...` need the dynamic CGI help backend,
   which is deliberately not packaged (same "deferred, degrades
   gracefully" pattern as the data server's CGI account-creation flow,
   documented earlier in this file) — expected, not a new bug.

Fixed #1 by serving the help content through the **data server's Apache
instance** (already running on every GUI app launch) instead of a raw
`file://` URL, rather than rewriting every absolute path in vendored HTML:
- `packaging/dataserver/httpd.conf.ecce`: added an `Alias /EcceHelp
  "/opt/ecce/data/client/WebHelp/EcceHelp"` (+ matching `<Directory>`
  block) — a literal path is correct here since this is fixed,
  package-installed content under `ECCE_HOME`, not per-user state like
  the rest of this config (which uses `##DATAROOT##`).
- `CMakeLists.txt`: `ECCE_HELP` default changed from
  `file:///opt/ecce/data/client/WebHelp/` to `http://localhost:8096/`
  (port 8096 = the data server's fixed port, confirmed against
  `ecce-dataserver-start`'s own `PORT=8096`). `mod_alias` was already
  loaded (needed for the pre-existing `ScriptAlias /cgi-bin/`), so no new
  Apache module needed.

As a side effect, #2's failure mode improves too: a `/cgi-bin/...` help
link now hits Apache and gets a proper HTTP 404 instead of a raw browser
"file not found" error — same underlying limitation, cleaner failure.

Not yet re-verified live (found and fixed together with the pipe-hang bug
below, in the same package rebuild) — check images render and the
homepage's internal links resolve correctly on the next Help click.

## Gateway freeze (real hang, not a repaint bug) clicking "Viewer" — FIXED (2026-08-28)

Distinct from every other "state updates but doesn't repaint" bug in this
file — this was a **genuine, unrecoverable hang**: the whole Gateway
window stopped responding to everything (not just the clicked button),
showed a busy-cursor, and needed `kill -9` from a shell to escape (a
desktop-level "force quit" wasn't enough — the process wasn't merely slow
to repaint, its main thread was truly blocked in a kernel syscall).
Confirmed via a live `gdb -p <pid> -batch -ex "thread apply all bt"`
snapshot: the main thread was stuck in `open64()`, called from wx/GTK
internals by way of unresolved (stripped-binary) ECCE frames.

**Two bugs, stacked**: a genuine missing-executable bug that made the
hang inevitable, plus the blocking-`open()` bug that turned "child fails
to start" into "gateway hangs forever" instead of a clean error.

### Bug 1: "Viewer" invoked a command this build never packaged
Traced the full `Gateway → JMS "ecce_get_app" → WxJMSMessageDispatch::
getAppMCB()/appExec()` app-launch pipeline (`src/wxgui/jms/
WxJMSMessageDispatch.C`) end to end. `GatewayApp.C:113` registers
`Gateway` itself as the one process allowed to spawn other apps
(`registerMyselfAsAppExecer()`, comment: *"only gateway calls this"*) —
so this isn't a missing-daemon problem (a `launcher` process not running
was an early, wrong hypothesis, ruled out once `registerMyselfAsAppExecer`
was found). `appExec()` (`WxJMSMessageDispatch.C:604`) `fork()`s and
`execv("/bin/sh", ["-c", "<InvokeArg> -pipe <authpipe>"])`, where
`<InvokeArg>` comes straight from `ResourceDescriptor.xml`'s
`<Tool name="CalculationViewer"><InvokeArg>viewer</InvokeArg>`. **`viewer`
doesn't exist anywhere in this build** (`which viewer` → nothing) — it's
one of the legacy `scripts/*` csh wrappers this modernization already
deliberately left unpackaged (same category as `ecce`, `ebuilder`, per
the existing `ECCE_GUI_APPS` wrapper comment in `CMakeLists.txt`), and
nobody had reached this specific code path before to notice. The shell's
`execv` of a nonexistent command fails immediately, so the child never
gets far enough to open its end of the auth pipe — which is what made
bug 2 below actually manifest.

`scripts/viewer` (csh) turned out to do something simple once read: it's
not a separate app at all, just `builder` re-exec'd with two extra env
vars (`ECCE_INVOKE_VIEWER`, `ECCE_INVOKE_FROMECCE`) that put it into a
read-only/view mode — confirmed still fully intact, unmodified legacy
logic in `BuilderApp.C` (`getenv("ECCE_INVOKE_VIEWER") ? CALCVIEWER :
BUILDER`, etc.) and `ViewerEvtHandler.C`. Fixed by adding a dedicated
`ecce-viewer` wrapper (`CMakeLists.txt`, alongside the main
`ECCE_GUI_APPS` wrapper loop — a one-off since there's no separate
`viewer` build target) that sets those two env vars and execs
`$ECCE_HOME/bin/builder`, and repointing both `ResourceDescriptor.xml` and
`ResourceDescriptorRxn.xml`'s `<InvokeArg>` from the legacy bare `viewer`
to `ecce-viewer` — matching this build's `ecce-<app>` naming convention
specifically *because* a bare `viewer` on `$PATH` is exactly the kind of
generic-name collision risk that convention exists to avoid (see the
existing comment on the main wrapper loop).

### Bug 2: the blocking pipe open turned "child fails" into "gateway hangs forever"
`AuthCache::pipeOut()` (`src/tdat/resources/AuthCache.C`), used to share
cached login credentials from `gateway` to the about-to-launch child via a
named pipe (`mkfifo` + a plain `open(pipeName, O_WRONLY)`). Opening the
write end of a FIFO **blocks until a reader opens the read end** — with
bug 1 in place, nothing ever did, so `gateway` blocked forever. **The
original author already diagnosed this exact failure mode and left a
comment describing the needed fix, never implemented**: *"Blocking here
is suboptimal because the caller (e.g. gateway) will go out to lunch if
the child app doesn't start properly. Some kind of timeout after a few
seconds would be much preferred."*

Implemented exactly that: `open()` with `O_NONBLOCK` (returns immediately
with `ENXIO` instead of blocking when no reader exists yet) in a
poll-with-timeout loop, up to 10 seconds — matching the *already-existing*
timeout convention in the paired `pipeIn()` function a few lines below
(which waits up to 10 seconds, 1 second at a time, for the pipe to even
exist). On timeout, `fd` stays `-1` and the function falls through to its
existing (already-correct) "give up quietly" path — no new failure mode,
just bounded instead of unbounded blocking. Kept even after fixing bug 1:
it's real, independently-justified robustness (any future tool whose
child genuinely fails to start, for any reason, no longer hangs the
launching app forever), not just a workaround for bug 1's specific
trigger.

Not yet re-verified live (found and fixed together, same package rebuild,
not yet re-tested against a real "click Viewer" repro).

## Help CGI backend (context-sensitive help links) — ported (2026-08-28)

Previously documented as deliberately deferred (a `/cgi-bin/EcceHelp/
toolhelp?...` link 404ing "gracefully"). Turned out to be genuinely
tractable once actually read, unlike the data server's account-creation
CGI (which needs `CGI.pm`, dropped from Perl core in 5.22+): `toolhelp`
and `cshelp` (`data/admin/dataserver/help/eccehelp.tar`'s `cgi-bin/`) only
depend on `getopts.pl`, a Perl-4-era library still bundled with modern
Perl (confirmed working on Debian 13's Perl 5.40) — no missing-module
blocker at all.

Three real, independent compatibility breaks, all in decades-old assumed
environment details rather than the scripts' own logic:
1. **Shebang**: `#!/msrc/apps/bin/perl` (PNNL's own path) → `#!/usr/bin/perl`.
2. **Hardcoded server path**: `global.pl`'s `$help_dir =
   '/msrc/proj/ecce/www/help'` → this install's real path,
   `/opt/ecce/data/client/WebHelp/EcceHelp`.
3. **`require 'global.pl'` with no path** — relies on `.` (current
   directory) being in Perl's `@INC`, true when this was written, **false
   since Perl 5.26 removed it by default for security** — silent
   `Can't locate global.pl in @INC` failure on any current Perl. Fixed to
   an absolute path (`require '/opt/ecce/.../cgi-bin/global.pl'`) rather
   than fighting `@INC`/cwd — this is fixed, package-installed content,
   an absolute path is simplest and correct, same reasoning as fix #2.

All three patched at build time by a new standalone script,
`packaging/dataserver/patch-help-cgi.sh`, invoked from a new
`ecce_help_cgi_patch` CMake custom target right after the existing help-
tarball extraction (`CMakeLists.txt`). **Kept as a standalone script
rather than inlined `sed` commands directly in `CMakeLists.txt`**: a first
attempt inlining them hit real, hard-to-read breakage — CMake's own
command-argument escaping doesn't survive a literal `$help_dir = ...`
sed pattern through nested CMake→shell→sed quoting layers (silently
mangled the output command in ways that only showed up at build time, not
at configure time) — not worth fighting further versus just shelling out
to one `.sh` file with normal, unmangled quoting.

`ecce-dataserver-start` also now seeds `$STATEDIR/cgi-bin/EcceHelp/` from
the patched, installed copy on first run (same idempotent "only if
missing" pattern as the existing DocumentRoot seed) — this is what makes
`ScriptAlias /cgi-bin/ "##HTTPDROOT##/cgi-bin/"` (pre-existing, originally
added for the still-unimplemented account-creation CGI) actually resolve
`/cgi-bin/EcceHelp/toolhelp` to a real, executable script.

**A fourth, separate bug found immediately after the CGI scripts
themselves started working**: `toolhelp` correctly generates a new
frameset pointing its "main" frame at a real `.shtml` content page (e.g.
`/EcceHelp/gateway/overview.shtml`), but that page's actual body text and
every link on it come from `<!--#include file="..."-->` Server Side
Include directives — and Apache was serving those back as literal,
unprocessed text (confirmed via `curl`: the raw `<!--#include-->` comment
syntax visible in the response, not the included content). `mod_include`
was never loaded, and neither `Includes` (an `Options` value) nor
`AddOutputFilter INCLUDES .shtml` were ever set on the new `/EcceHelp`
`<Directory>` block (`##DATAROOT##`'s block already had `Includes` — for
the DAV tree, not help content). Fixed by adding `LoadModule
include_module`, `Includes` in `Options`, and `AddType text/html
.shtml` + `AddOutputFilter INCLUDES .shtml`, all in
`packaging/dataserver/httpd.conf.ecce`'s `/EcceHelp` block.

Not yet re-verified live end-to-end (found and fixed together with the
CGI scripts themselves, same package rebuild) — needs a real click-through
from a context-sensitive help link, past a data-server restart (config
template changes only take effect on a fresh `apache2` start, not an
already-running instance — see the earlier `/EcceHelp` Alias section for
the same caveat).

## Help CGI backend, round 2: 3 more scripts + SSI + LogLevel — FIXED (2026-08-28)

Follow-up to the section above, once real click-through testing (not just
`curl`) started surfacing more of the same class of gap:
- **Only `toolhelp`/`cshelp` were patched; 3 more navigation scripts
  needed the identical fix**: `key_topics` and `examples` (the "How To"
  and "Examples" tab content, invoked from `js_eccehelp.js`'s
  `startPage()` — `top.list.location.href =
  '/cgi-bin/EcceHelp/key_topics?...'`) and `openindex` (the master
  index). Same 3-part patch (shebang, `$help_dir`, absolute
  `require`) — `patch-help-cgi.sh` generalized to a `nav_scripts` list
  covering all 5. **Deliberately still not patched**:
  `EcceQSubmissionForm.pl`/`SubmitToEcceQueue.pl` — even fixed, they'd
  still fail trying to reach PNNL's own long-gone mail/queue servers, a
  real external-network problem, not an environment-path one like the
  rest of this tarball.
- **SSI (`<!--#include-->`) directives were being served back as literal
  text**, not processed — confirmed via `curl` showing the raw comment
  syntax in the response instead of the included fragment's content, so
  every `.shtml` page's actual body and links were simply missing.
  `mod_include` was never loaded, and neither `Includes` (in `Options`)
  nor `AddOutputFilter INCLUDES .shtml` were set on the `/EcceHelp`
  `<Directory>` block. Fixed in `httpd.conf.ecce`.
- **`LogLevel crit`** (inherited unchanged from the legacy template) was
  hiding the actual Perl error behind these bugs entirely — a CGI script
  failure normally logs at `error`, well below `crit`. Lowered to
  `LogLevel error` so future help-CGI (or any Apache-side) issues are
  actually diagnosable from `logs/error_log` instead of needing a `curl`
  reproduction every time.

Confirmed via the user's own live click-through: "Clicking now leads to
new pages" (round 1 alone got the top-level nav working) → "same issue as
before... How To tab gives internal server error on the left panel"
(round 2's actual trigger) → traced end-to-end via `js_eccehelp.js` before
touching anything further, rather than guessing at more CGI scripts to
patch blind.

## `scripts/parsers` packaging gap — real fix, but NOT the basis-set error's cause (2026-08-28)

Genuinely fixed a real packaging gap (below) while investigating the
basis-set/theory validation errors — but this turned out to be the wrong
tree entirely for *that* bug. **The actual root cause and real fix are in
the next section** ("Basis set library metadata — FIXED"); left this
section as-is rather than deleting it, since the packaging gap it
describes is real and worth having fixed regardless.

Reported while testing a real NWChem calculation setup (draw CH4 in
Builder, set SCF/RHF theory, open the basis set editor): spurious
"selected configuration doesn't cover all elements" and "group not
defined" errors for definitely-covered elements (C, H — any basis set
covers these), regardless of which theory/runtype was selected, and
"Theory Details" doing nothing when clicked.

User's own hunch ("I think these are managed by perl/python programs")
was exactly right, but not `scripts/gbs*` (the `gbsDAVConverter`/
`gbsDescriber`/`gbsNWChemConverter` trio already flagged in this repo's
own history as unported/diverged) — those turned out to be unrelated to
this specific bug, still genuinely unported (confirmed `gbsDescriber` is
real, un-migrated Python 2 — `chr(004)`-style octal literals are an
outright `SyntaxError` on Python 3 — left alone, not currently reachable
from any wired-up UI path, so out of scope for this fix). The actual
dependency, traced via `TGBSConfig.C`'s `system()` call →
`JCode::getScript("GBSExport")` → `get_string("BasisTranslationScript")`
→ `NWChem.edml`'s `<BasisTranslationScript>std2NWChem</BasisTranslationScript>`,
is `scripts/parsers/std2NWChem` (+ two required modules,
`rdStandardGBS.pm`/`wrNWChemGBS.pm`) — genuinely modern, already-working
Perl (`use strict`, `Getopt::Std`, no deprecated `getopts.pl` this time),
just never installed/packaged at all. **A pure packaging gap, not a
porting job** — confirmed via `perl -c` on all three files (clean) and a
real end-to-end smoke test (`echo "" | perl std2NWChem` → valid NWChem
basis-block stub output, no crash, no missing-module errors). Fixed by
adding `install(DIRECTORY scripts/parsers DESTINATION scripts)` to
`CMakeLists.txt` — `std2NWChem` itself already does
`push(@INC, "$ENV{ECCE_HOME}/scripts/parsers")`, and `ECCE_HOME` is
already exported by every `ecce-<app>` wrapper, so no further wiring
needed.

Scoped deliberately narrow tonight: only `scripts/parsers/` (used by
`GBSExport` and presumably other codes' basis/translation needs via the
same `JCode::getScript()` mechanism, not just NWChem) was packaged —
*not* the rest of `scripts/` (the `ecce`/`ebuilder`/`eviewer`-class
launcher scripts, or the still-genuinely-unported `gbs*` trio), which
remain out of scope per this file's existing documented reasoning
(diverged forks, missing Python 3 port, launcher scripts superseded by
this build's own `ecce-<app>` wrappers).

Not yet re-verified live against the user's real CH4/NWChem calculation
(found, fixed, and packaged in the same round as the Help CGI round-2
fixes above — needs `dpkg -i` + a real basis-set-editor retest).

## Builder crash on thumbnail generation (very likely = "Ctrl+S closes Builder") — FIXED (2026-08-28)

User report was an actual SIGSEGV with a clean, readable pre-crash
message this time (`Throw Log: ... In VizRender.C line: 230 Unable to
render scene graph`) — clean and readable specifically *because* of the
`EcceException::what()` dangling-pointer fix earlier tonight; before that
fix this same message would likely have been garbage, like the builder
SSSR crash investigation's `Throw Log` output was.

Two independent, real bugs found reading `src/wxviz/viewer/VizRender.C`
end to end (three call sites: `VizRender::file()`, and two overloads of
`VizRender::thumbnail()`, called from `Builder::doSaveThumb()` — invoked
automatically right after every `Builder::doSave()`, which is exactly
what Ctrl+S triggers):
1. **Uninitialized pointers "declared here in case of exception thrown"**:
   `VizRender::file()`'s `FILE * myFile;` and `VizRender::thumbnail(SoNode*,
   ...)`'s `SFile *rgbFile;`/`*jpegFile;` were never initialized at
   declaration. Each function's own cleanup code (`if (myFile)
   fclose(myFile);` etc.) runs on the failure path specifically to handle
   this "in case of exception" scenario — but on that exact path, before
   my fix, the pointer being checked was uninitialized stack garbage, not
   actually null. `if (garbage_that_looks_truthy) fclose(garbage_pointer);`
   is undefined behavior — the actual segfault. Fixed by initializing all
   three to `0` at declaration, so the "in case of exception" path they
   were written for actually behaves as intended.
2. **`catch (string& error)` doesn't catch `EcceException`** (extends
   `std::runtime_error`, not `std::string`) — found in all three
   functions in this file. Every `throw EcceException(...)` inside these
   try blocks (including the "Unable to render scene graph" one that
   triggered this report) was going completely uncaught, propagating out
   of the function entirely and very plausibly reaching
   `std::terminate()`/`abort()` further up the call stack — independently
   enough to explain a hard crash even without bug 1. Fixed all three to
   `catch (EcceException& ex)`, matching the pattern used correctly
   elsewhere in this same codebase. Confirmed via grep this exact
   mismatched-catch pattern doesn't appear anywhere else in the tree —
   isolated to this one file.

**Very likely the same bug as "Ctrl+S closes Builder"** (reported
separately, still open as of this fix): the user later confirmed Ctrl+S
does *not* crash when re-saving an already-existing, previously-saved
structure, only account for a first save — consistent with thumbnail
generation (which fires from `doSave()` on every save) hitting a code
path exercised differently, or for the first time, on a brand-new
resource. Not yet confirmed live — needs the user to retest the exact
"draw new structure, Ctrl+S" sequence that originally triggered it.

**Not investigated**: *why* `SoOffscreenRenderer::render()` fails in the
first place ("Unable to render scene graph"), which is what triggers this
whole path. Both bugs above make that failure survivable (a message in
`p_msg`, not a crash) rather than fixing the underlying render failure --
thumbnails may still just not get generated. Worth a follow-up look if
thumbnails turn out to be silently missing rather than genuinely working
end-to-end.

## Offscreen thumbnail rendering ("Unable to render scene graph") — FIXED (2026-08-28)

Follow-up to the crash fix above: once the crash was gone, the underlying
render failure itself was still there (a clean warning instead of a
segfault, but Organizer's thumbnails still weren't actually being
generated). Root-caused with a minimal, isolated repro rather than
guessing inside the real app — a standalone ~40-line C program replicating
just `XOpenDisplay` → `glXChooseVisual` → `glXCreateContext` →
`XCreatePixmap` → `glXCreateGLXPixmap` → `glXMakeCurrent`, the exact
sequence `SoOffscreenRenderer::initPixmap()`
(`src/inv/dbso/SoOffscreenRenderer.inc`) uses. Reproduced the exact
failure immediately: `glXCreateContext` throws a real X protocol error
(`X_GLXCreateContext` / `BadValue`), not just a `NULL` return.

Root cause: `glXCreateContext(dpy, vi, NULL, FALSE)` — the 4th argument,
`FALSE`, requests **indirect** GLX rendering. This code is old enough
that indirect rendering was a reasonable default at the time; modern
proprietary GPU drivers (confirmed here: NVIDIA) don't support creating
indirect GLX contexts at all and fail outright rather than silently
falling back. Confirmed the fix in the standalone repro first (flip
`FALSE` → `TRUE`, same call sequence renders successfully end to end)
before touching the real source, then applied the one-line fix directly
to `initPixmap()`. Confirmed via grep this is the only `glXCreateContext`
call site in the tree — no sibling instances elsewhere.

Not yet re-verified live (found, fixed, rebuilt, and packaged in the same
round as the crash fix above — needs a real retest generating an actual
Organizer thumbnail).

## Basis set library metadata — CORRECTED (2026-08-29): earlier "FIXED" entry was a misdiagnosis

The section that used to be here (`populate-gbs-metadata.py`, PROPPATCH-based
per-file properties, "9 of 17 Quick Basis Menu entries") **has been deleted
and reverted**. It solved a real symptom the wrong way, and in doing so
introduced a serious regression. Corrected story below, from a fresh
investigation prompted by a *new* user report: "Selected [a Quick Basis
Menu pick] without problem... but when I clicked the save icon I got
ERROR: Input files could not be generated--failed writing basis set."

### What the earlier session got wrong

1. **The vendored library was never actually missing metadata.** Direct
   `tar tf`/extraction of `data/admin/dataserver/server_config/
   data_server_files.tar` shows the per-type alias index files
   (`other_generally_contracted`, `correlation_consistent`, `ECPOrbital`,
   `pople`, etc.) already ship complete — `correlation_consistent` alone
   has 106 real entries — and roughly a fifth of the library's `.BAS`/
   `.POT` files already ship a real `*.BAS.meta`/`*.POT.meta` sidecar
   (`gbs.README`'s own documented, intentional format: `basis.BAS`,
   `basis.BAS.meta`, generated by `gbsNWChemConverter`/`gbsDescriber`).
   Whatever was actually broken when that session investigated wasn't "no
   metadata exists" — it was never conclusively isolated before landing a
   fix.
2. **`populate-gbs-metadata.py` PROPPATCHed properties that Apache
   silently rejects.** `httpd.conf.ecce`'s `<Directory ".../Ecce">` block
   has `<Limit MKCOL PUT DELETE LOCK UNLOCK COPY MOVE PROPPATCH> Require
   all denied </Limit>` — matches the *legacy* `build/server_admin/
   httpd.conf.ecce` ACL exactly, not a config regression introduced by
   this build. Confirmed via direct `curl -X PROPPATCH`: every one of the
   script's writes was silently rejected (403), which the script never
   checked for (`proppatch()` printed the HTTP status but nothing
   inspected it). The properties it thought it was writing were never
   actually applied.
3. **`populate-gbs-metadata.py`'s per-type index file writer used `open(...,
   "w")` — a destructive overwrite, not an update.** Because the
   idempotency gate in `ecce-dataserver-start` checks for the presence of
   `other_generally_contracted` (which, per point 1, the vendored tar
   *always* ships), the script would never actually fire on a real fresh
   install — but it silently clobbered the real 106/51/43-entry index
   files down to 2–4 curated entries when manually invoked directly
   against this session's live data server while investigating. That
   would have broken basis-set lookup for the entire library, not just
   the 9 "Quick Basis Menu" picks, for anyone whose data server ever ran
   this script. **Restored** the 3 affected index files
   (`correlation_consistent`, `other_generally_contracted`, `ECPOrbital`)
   and the 11 `*.meta` sidecars it also overwrote, verbatim from the
   vendored tar, and confirmed entry counts match (106/51/43).

`populate-gbs-metadata.py` is deleted, its `ecce-dataserver-start` call
site and CMake `install()` rule removed. Nothing replaces it — the
vendored data plus the C++ fixes below are sufficient on their own.

### The actual root cause of "failed writing basis set"

Traced via `ESInputController::write_gbsconfig()`
(`src/apps/calced/ESInputController.C:147`) → `TGBSConfig::dump()`
(`src/dsm/chemistry/TGBSConfig.C:2070`). `dump()` builds the basis-set
text internally, then **shells out** to a per-code formatting script
(`code->getScript("GBSExport")`, e.g. `std2NWChem`) via a bare
`system("std2NWChem < tmpin > tmpout")` — no path, relying entirely on
`$PATH`. **None of the `ecce-<app>` wrapper scripts ever added
`$ECCE_HOME/scripts/parsers` to `$PATH`**, despite `std2NWChem` and its 16
siblings being correctly installed there (a real prior-session fix,
`scripts/parsers` packaging, documented above — that fix shipped the
files but never wired them onto `$PATH`, so it was necessary but not
sufficient). Confirmed directly: a standalone repro program (`Ecce::
initialize()` + `EDSIGaussianBasisSetLibrary::simpleLookup()` +
`TGBSConfig::dump()`, linked against the real build's static libs, no
GUI) reproduced `sh: 1: std2NWChem: not found` on stderr and an empty
`dump()` result for all 9 Quick Basis Menu picks — exactly matching
`write_gbsconfig()`'s `charData[0]=='\0'` check that produces the
reported error message.

**Fixed**: added `export PATH="$ECCE_HOME/scripts/parsers:$PATH"` to the
shared `ECCE_GUI_APPS`/`ECCE_CLI_APPS` wrapper template and the separate
`ecce-viewer` wrapper in `CMakeLists.txt`. Also hardened
`TGBSConfig::dump()`'s error detection: `if (system(command.c_str()) < 0)`
never actually detects a failed child command (`system()`'s return value
is a wait-status, not errno-style; it's only negative if `fork`/`exec`/
`waitpid` itself failed) — a `sh -c` that ran fine but whose target
command was missing (exit 127) was silently falling through to read back
an empty results file. Changed to `!= 0`.

**Verified**: standalone repro against the real, rebuilt libraries with
`$PATH` fixed — all 9 Quick Basis Menu picks now produce real,
non-trivial chemistry data (117–5182 bytes of actual NWChem-formatted
exponent/coefficient output, not empty).

### Found while verifying: the DAV property store is unreliable for ~97% of the library, and 3 more uninitialized-enum crashes

Pulled 40 random `.BAS` files and PROPFIND'd each for their `name`/`type`
custom properties directly: **35–39 out of 40 returned 404**, across
repeated random samples. The vendored `.DAV/*.pag` DBM property store
(legacy httpd 2.2/APR-util binary format) genuinely doesn't survive
intact under modern `mod_dav_fs`, confirming a risk this project's own
earlier data-server work had flagged but never empirically tested — some
entries happen to still parse (`def2-svp.BAS`, `6-31GS.BAS` both returned
correct data every time), most don't.

`EDSIGaussianBasisSetLibrary::lookup()` (per-file metadata fetch, distinct
from the alias/coverage-check path) now tries three sources in order,
each a real fallback rather than silently failing to the next:
1. **The real vendored `*.meta` sidecar file**, parsed with the exact
   same `getBigProperty()` helper `details()`/`comments()` already use
   for this identical file format (alternating `key`/`value…`/`key`
   marker lines) — previously an *invented* `key=value` one-line format
   that doesn't match the real data at all, silently never matching
   anything and falling straight through to PROPFIND. Fixed to reuse the
   proven parser instead of a new, wrong one.
2. **DAV PROPFIND** (the pre-existing path), for the fraction of the
   library where the DBM store happens to still work.
3. **New**: derive `name`/`type` from the `gbs_alias` this file was
   already resolved through via `getGbsAlias()` earlier in the same
   function, when neither of the above yields anything (roughly 4/5 of
   the library has no `name`/`type` in its `.meta` sidecar either — only
   `references`/`info`/`comments`). This is real data, not a guess: every
   file in an alias group is already filed under that exact name/category
   in the (real, working) per-type index file, just not duplicated onto
   each individual file. Without this tier, most of the library beyond
   the historically-tested quick-picks would still silently drop basis
   sets rather than resolving them.

This surfaced a second, unrelated bug class while testing the `.meta`-file
path against real data: real vendored `.meta` sidecars **legitimately
leave `type`/`spherical`/`contraction_type` blank** for plain orbital
basis sets (the type is implied by which category index file it's filed
under, not restated per file). Feeding an empty string into
`TGaussianBasisSet::strToType()`/`strToCoordSys()`/`strToContType()`
(`src/dsm/chemistry/TGaussianBasisSet.C`) hit the same uninitialized-local
bug class already fixed twice elsewhere this session
(`gbsType`/`coordSys`/`contType` declared with no initializer, left
untouched on the `else`/unrecognized branch, then returned and used) —
confirmed via the standalone repro: `cc-pVDZ` segfaulted immediately once
the `.meta` sidecar path started actually getting exercised with real
(blank-field) data. All three now default to their respective
`Unknown*` enum value instead of leaving the local uninitialized.

**Verified**: standalone repro against ~40 basis sets spanning quick
picks, common non-quick-pick names (`3-21G`, `STO-3G`, `6-311G**`, `ADZP`,
`cc-pVQZ`), and 25 random names pulled live from the real alias index
files (including ones with spaces/parens/`*`/`+` in the name, e.g. `"SVP +
Rydberg (Dunning-Hay)"`, `"aug-cc-pV(D+d)Z"`) — zero crashes, zero
"not found", every basis set that actually covers the requested elements
(C/H/O/N) produces real data; basis sets that legitimately don't cover
those elements (e.g. heavy-atom-only ECPs) correctly produce an empty
result rather than fabricating one.

**Same root cause, different symptom**: the user's separately-reported
"In editor, 'final edit' doesn't work either: WARNING: Input file not
found" is `CalcEd::OnButtonCalcedFinalEditClick()`
(`src/apps/calced/CalcEd.C:979`) calling `doSave()` first (which silently
fails to actually write/PUT the primary input file, for the exact reason
above), then finding nothing to open. Should be resolved by the same
`$PATH` fix — not independently re-verified yet, same "needs a package
reinstall + live GUI retest" caveat as below.

**Likely also explains** the user's separately-reported "selecting a new
basis set in the Basis Set Tool leads to the basis set tool dialogue
closing": `src/apps/basistool/WxBasisTool.C` calls
`EDSIGaussianBasisSetLibrary::lookup()`/`details()` directly (same
functions fixed above) on selection — an uncaught segfault there looks
exactly like "the window disappeared." Not independently confirmed (no
GUI access), but high confidence given it's the identical code path and
identical crash signature (`strToType`/`strToCoordSys`/`strToContType` on
real, blank-field vendored `.meta` data) as the standalone repro's
`cc-pVDZ` segfault, fixed above.

**Was wrongly marked "NOT fixed, out of scope" here — corrected below.**
The claim that `scripts/codereg/*.py` was "genuine Python 2 wxPython
2.8" was asserted from running `file` on the scripts, which doesn't
distinguish Python 2 from 3 — never actually verified. See "Theory
Details dialog / missing DFT functional" further down for the real
story: an earlier session had already ported these to Python 3, and the
actual fix needed was much smaller than claimed here.

**Not yet done**: reinstalling the rebuilt `.deb` — this session has no
passwordless `sudo`, so `dpkg -i build-cmake/ecce_8.0.0_amd64.deb` needs
to be run by Andy directly once back. The live per-user data server tree
(`~/.ECCE/dataserver/htdocs/...`) has already been repaired in place
(index files + `.meta` sidecars restored) so no separate data-server
reseed is needed, just the package reinstall for the `$PATH` fix to reach
the actual `ecce-calced` wrapper. Also not yet done: an actual interactive
retest of "Quick Basis Menu → pick → save" in the real GUI — everything
above is verified via a standalone repro linked against the real build's
libraries, not the real Builder/CalcEd UI, since there's no GUI automation
tool available in this session.

### Correction (2026-08-29): the 6 "permanently unresolvable" quick picks aren't

In conversation (not previously written into this file, so nothing above
needed fixing, but worth recording here so it isn't asserted again): 3 of
the 17 Quick Basis Menu picks (`6-31++G`, `6-31+G*`, `6-31++G**`) were
claimed to have "no real diffuse-function component file anywhere in the
library," and 3 more (`DZVP`, `DZVP2`, `TZVP`, all "(DFT Orbital)") were
claimed to have "no matching `.BAS` file... under any name." **Both
claims were wrong**, caught when Andy asked directly whether these were
ever fixed. Checked properly:

- The diffuse-function trio's alias entries (`pople` index file) list 3-4
  files each, e.g. `6-31++G` → `6-31PPG-AGG.BAS 6-31G.BAS POPLDIFF.BAS`.
  The `-AGG` file genuinely is a 0-byte placeholder (confirmed against the
  original vendored tar, unchanged since 2015) — but it's *supposed* to
  be, `lookup()`'s own dummy-aggregate-skip logic deliberately ignores it
  and reads the real data from the sibling files instead.
  `POPLDIFF.BAS` (1729 bytes) has real exponent/coefficient data for the
  diffuse functions. The claim likely came from seeing the 0-byte `-AGG`
  file and not checking the rest of the alias's file list.
- The DFT-orbital trio resolves to real files under a *different* name
  than the quick-pick label: `DZVP (DFT Orbital)` → `DGAUSS_DZVP.BAS`
  (+ two Coulomb/Exchange fitting files), not a file literally named
  `DZVP.BAS`. The claim likely came from searching for the display name
  as a filename and not checking the alias mapping.

**Verified**: standalone repro against all 6 — every one now produces
real, non-empty `dump()` output (100-170+ bytes of correctly-attributed
per-element library references or exponent data, matching the pattern of
every other working quick pick). This means, as far as this session can
tell, **all 17 of 17** Quick Basis Menu picks resolve correctly, not 9 or
11 — not independently confirmed live yet (same reinstall+GUI-retest
caveat as everywhere else in this section), but the standalone repro
exercises the identical `TGBSConfig::dump()` code path `write_gbsconfig()`
calls, so there's no reason to expect the real GUI to behave differently.

## "Regenerate thumbnail on demand, independent of Save" — already exists, no new code needed (2026-08-28)

User wanted `Ctrl+S` to regenerate the thumbnail every time, even when
the structure hasn't changed (`updateSave()` correctly disables Save,
and its `Ctrl+s` accelerator with it, whenever nothing is dirty — see the
Builder crash section above; confirmed this is intentional, standard
"nothing to save" behavior, not a bug). Rather than change core Save
semantics, asked whether to always-enable Save vs. add a separate,
dirty-independent thumbnail action — chose the latter.

Turned out to already exist: `Builder`'s File menu has **"Create
Thumbnail"** (`ID_SAVE_THUMB` → `OnSaveThumbClick()` → `doSaveThumb()`,
`src/apps/builder/BuilderGUI.C`/`Builder.C`), enabled purely by "is there
a valid calculation context" (`updatePropertyMenus()`'s
`dynamic_cast<Resource*>(p_calculation)` check) — **already completely
independent of the dirty flag**, and `doSaveThumb()` itself has no dirty
check either. No new menu item, no new code — just needed the crash fix
above (this calls the exact same `VizRender::thumbnail()` that was
crashing and failing to render) for this pre-existing feature to actually
work. Not yet confirmed live that the menu item is visible/clickable in
practice — worth a quick look next session.

## Basis Set Tool editor "looks weird" — periodic table FIXED, column widths still open (2026-08-28)

Investigated via the same no-interaction screenshot technique used for
the periodic-table and Help bugs earlier (`basistool` opens to a usable
default state standalone, no GUI clicking needed to see the bug). Two
separate issues found, one fixed, one still open.

### Fixed: mini periodic table rendered as an unreadable hatched strip
The "Chemical Formula" element-picker (`WxBasisTool.C` embeds
`PerTabPanel` in `isMini` mode, same class as the standalone Periodic
Table app fixed earlier, and `Builder`'s own `MiniPerTab` — see that
section's "worth checking" note, now checked) rendered as a thin strip of
tiny black-and-white hatched rectangles, not distinguishable element
buttons. **Not the same root cause as the standalone Periodic Table
bug** — that fix (drop `wxALIGN_CENTER` from the per-button sizer flags)
was already in place here and had zero effect on this symptom, confirmed
via screenshot before looking further.

Actual cause, in the *designer-generated* `WxBasisToolGUI.C`:
`itemBoxSizer64->Add(itemPanel68, 1, wxGROW|wxFIXED_MINSIZE, 0);` —
`wxFIXED_MINSIZE` locks a sizer item to whatever (near-zero) best-size it
happens to have **at the moment it's added to the sizer**, permanently
ignoring any size hint set afterward. `itemPanel68` is the *empty*
placeholder panel added to the sizer *before* the real `PerTabPanel`
child gets created and put inside it — so the panel was permanently
frozen at its pre-content, near-zero size. Confirmed two prior,
independent attempts at fixing this were already sitting commented-out
right next to the panel's construction in `WxBasisTool.C` (both explicit
`SetMinSize()` hints) — neither could have worked regardless of the
value chosen, since `wxFIXED_MINSIZE` on the *parent* sizer item ignores
child size hints entirely. Fixed by removing `wxFIXED_MINSIZE`, keeping
`wxGROW`. Confirmed via screenshot: full 118-element, correctly colored
and labeled periodic table, matching the already-fixed standalone tool's
appearance. As a side effect, the basis-set list panel above it also
now shows properly (more rows, category headers visible) — apparently
the same layout pass was also constraining that sibling panel.

### Still open: grid column headers truncated ("Polarizatio", "T Charg", "T Exchan")
`WxBasisTool::setGridColumnVisible()` hardcodes `SetColSize(col, 30)` for
the ECP/Fit-Charge/Fit-Exchange columns specifically — 30px is too narrow
for their header text even with the `AutoSize()` call immediately after,
which apparently doesn't widen based on header-label length (possibly
computing width from cell *content* only, and cells are empty in this
no-molecule-loaded state — not confirmed). Tried raising the hardcoded
value to 85px; **no visible change** in a follow-up screenshot, so
`AutoSize()` (or something else downstream) is winning over the explicit
width, same failure signature as the periodic-table fix's own
`SetMinSize()` dead end before `wxFIXED_MINSIZE` was found — some other
constraint here hasn't been identified yet. Also: the "Polarization"/
"Diffuse"/"Rydberg" columns showing the *same* truncation aren't even
routed through `setGridColumnVisible()` at all (grepped — no call site
for those columns), so whatever's actually setting their initial width
is a still-undiscovered separate code path. Left the 85px change in
place (harmless, may help in states not visible in this screenshot) but
this needs real investigation before claiming it's fixed. Purely
cosmetic — doesn't block using the grid, unlike the periodic table issue
above which did.

## Machine Registration: "Unable to save changes to machine registration!" — FIXED (2026-08-29)

Same bug *class* as the `std2NWChem`/basis-set save failure above (a
`scripts/` helper never installed), found from a fresh user report, not a
regression from that fix. `WxMachineRegister::machineChangeButtonClickedCB()`
(`src/apps/machregister/WxMachineRegister.C:473`) shells out to
`Ecce::ecceHome() + "/scripts/processmachine"` (a **full path**, not
relying on `$PATH` like `std2NWChem` did) via a CGI-POST-style
`system()` call (`CONTENT_LENGTH` env var + form-encoded stdin) and
correctly checks the exit status (`status >> 8`, `!= 0`) — the error
message itself was accurate, just pointing at a script that was never
packaged. `find /opt/ecce -iname processmachine` came back empty;
`scripts/processmachine` was never added to `CMakeLists.txt`'s install
rules at all (only `scripts/parsers` was, from the earlier basis-set fix).

Confirmed genuinely portable before fixing, same diligence as
`scripts/parsers`: `perl -c` clean, no `CGI.pm`/module dependency, no
hardcoded EMSL paths in its active code (its one CVS/svn revision-control
integration branch is already commented out in the script itself, not
something this session had to remove). It also **already** does the
correct shared-vs-per-user split without any changes needed: a
`siteconfig` form field (from `p_adminFlag`, itself only ever `true` when
`machregister` is launched with a literal `-admin` argv — Gateway's own
"Machine Registration" tool invokes it with no such flag) branches
between `$ECCE_HOME/siteconfig` (the read-only, root-owned package tree —
correctly admin-only, matching this build's established pattern for the
JMS broker and data server) and `$ECCE_REALUSERHOME/.ECCE` (always
writable) — so ordinary, non-admin use was always going to resolve to the
writable path once the script existed at all.

**Fixed**: added an `install(PROGRAMS ...)` rule for `scripts/
processmachine` to `CMakeLists.txt`, mirroring the `scripts/parsers`
block immediately above it.

**Verified**: ran the real script directly (not through the GUI — no
interactive access) with a realistic form-encoded payload matching what
`WxMachineRegister::collectSettings()` builds (`siteconfig=false` +
all of `@addgottahaves`'s required fields: `machine`, `name`, `qmgr`,
`type`, `nodes`, `vendor`, `model`, `processor`), `ECCE_REALUSERHOME`
pointed at a scratch directory — exit status 0, and a correctly
tab-delimited `MyMachines` entry written (`testmachine	
testhost.example.com	Unspecified	Unspecified	Unspecified	4:1	ssh	...`).
Confirmed present, executable, in the rebuilt `.deb`
(`dpkg-deb -c ecce_8.0.0_amd64.deb | grep processmachine`). **Not yet
verified against the real GUI** — needs the package reinstalled (still
blocked on `sudo`, same as the basis-set fix) and a live "Machine
Registration → fill in fields → Save" retest.

## Machine Browser: GTK-CRITICAL spam at launch — FIXED (2026-08-29)

User report: "lots of error messages in the terminal" from `machbrowser`,
immediately at launch, e.g. `gtk_text_buffer_get_start_iter: assertion
'GTK_IS_TEXT_BUFFER (buffer)' failed` (×2, for the status text ctrl) and
`gtk_editable_get_chars: assertion 'GTK_IS_EDITABLE (editable)' failed`
(×3, for single-line entries elsewhere in the same window). Not fatal —
the app kept running — but real, reproducible on every launch, not just
under a debugger.

Root-caused directly via `G_DEBUG=fatal-criticals` (turns a GTK
"critical" log into a real `SIGTRAP`, so `gdb -batch -ex run -ex bt`
catches an exact, shallow backtrace right at the offending call instead
of just a log line with no context — much faster than guessing from 5
scattered warnings):
```
wxTextCtrl::DoGetValue -> wxTextEntry::DoSetValue -> wxTextCtrlBase::SetValue
  -> ewxTextCtrl::SetValidator (ewxTextCtrl.C:159) -> wxWindowBase::CreateBase
  -> wxTextCtrl::Create -> ewxTextCtrl::Create -> ewxTextCtrl ctor
  -> MachineBrowserGUI::CreateControls
```
`wxWindowBase::CreateBase()` (part of the *base* `wxWindow::Create()`
sequence) installs any validator passed to the constructor by calling
`SetValidator()` — and `ewxTextCtrl`'s override of that function
unconditionally calls `SetValue(gv->getValue())` to self-seed from an
`ewxGenericValidator`. That's fine when `SetValidator()` is called
explicitly *after* construction (the intended use case, per its own
comment: "the text field's default value is set from the validator's
current value") — but `CreateBase()` calls it *during* `Create()`,
before the native `GtkWidget`/`GtkTextBuffer` exists yet, so `SetValue()`
tries to touch a buffer that isn't there. Confirmed this is genuinely
redundant in the premature case: `Create()`'s own `value` parameter
already seeds the control's initial text through the normal creation
path, so skipping the self-seed here loses nothing.

**Fixed**: guarded the self-seed with `GetHandle()` (returns the native
`GtkWidget*`, null until the native peer actually exists) in
`ewxTextCtrl::SetValidator()` (`src/wxgui/ewxClasses/ewxTextCtrl.C`) — one
line, no behavior change for the real (post-construction) use case.

**Verified**: same `G_DEBUG=fatal-criticals` + `machbrowser` launch,
before and after — zero GTK-CRITICALs after the fix (previously 2+3 on
every single launch, every time). This is a generic `ewxTextCtrl` fix,
not `machbrowser`-specific — every app using `ewxTextCtrl` with a
validator (most of them, via the wx-designer-generated `*GUI.C` files)
was hitting the same premature-`SetValue()` path at startup; this should
quiet the same class of warning wherever else it was firing unnoticed
(nothing else was specifically reported, but this matches the pattern
already noted for `WxJMSMessageDispatch`'s auth dialog in the "Post-login
usability bugs" section above — `gtk_editable_get_chars` — likely the
exact same root cause, not independently re-verified there).

## `gensub`: job submission fails with "Command not found" — FIXED (2026-08-29)

Found while chasing a *different*, initially more alarming report:
launching a Gaussian 16 job appeared to freeze the whole app ("mouse
cursor spinning, then ECCE is not responding"). That turned out to be a
red herring, self-diagnosed live by the user: the machine had been
registered under the literal name `127.0.0.1` rather than its real
hostname, and the remote-shell connection attempt hung on that (not
independently root-caused — renaming the registration to the machine's
actual hostname, `niobium`, immediately fixed the hang and let the
connection open). Worth revisiting if a *real* remote host (not a
loopback naming quirk) is ever reported hanging the same way — `RCommand`
opening a first-time SSH connection with no `BatchMode`/
`StrictHostKeyChecking` handling would be a classic way to hang forever
on an unattended host-key prompt, but not confirmed as the cause here.

Once past that, the *real*, reproducible bug: "Verifying remote
login... Validating local directory... Validating job... Generating job
submission script... **ERROR: Could not find command gensub -v  -p
subParams** / Submit script generation failed: gensub: Command not
found." Traced to `Launch.C:2372`
(`src/comm/commxt/Launch.C`): `p_localconn->execout("gensub -v  -p
subParams", output)` — a **bare command name**, run locally (not on the
remote machine), with **no full-path fallback at all** (unlike
`std2NWChem`, which at least had `scripts/parsers` on `$PATH` once that
earlier fix landed). `scripts/gensub` was never added to `CMakeLists.txt`
either — same packaging gap as `std2NWChem`/`processmachine` before this
session, just never reached until a real job launch was attempted.

Confirmed portable before packaging, same diligence as the other
`scripts/*` fixes this session: `perl -c` clean, **zero** `use`/`require`
statements (fully self-contained core Perl, no module dependencies at
all), no hardcoded EMSL paths — only reads already-installed,
already-readable package content (`$ECCE_HOME/siteconfig`,
`$ECCE_HOME/data/client/config/Version`).

**Fixed**: added an `install(PROGRAMS ...)` rule for `scripts/gensub`
(mirroring `scripts/processmachine`'s). Since `gensub` lives directly
under `scripts/`, not `scripts/parsers/`, and is invoked with **no**
full-path fallback, also widened the wrapper template's `$PATH` from
just `$ECCE_HOME/scripts/parsers` to `$ECCE_HOME/scripts:$ECCE_HOME/
scripts/parsers` (both `CMakeLists.txt` wrapper blocks — the shared
`ECCE_GUI_APPS`/`ECCE_CLI_APPS` loop and the separate `ecce-viewer`
wrapper).

**Verified**: confirmed `gensub` present, executable, in the rebuilt
`.deb` (`dpkg-deb -c ecce_8.0.0_amd64.deb | grep gensub`). Ran the real
script directly (no GUI/job context available) with a deliberately
missing params file — failed fast and cleanly ("Param file specified but
not found") rather than hanging, confirming it won't introduce a new
freeze once wired up. **Not yet verified against a real job launch** —
needs the package reinstalled (still blocked on `sudo`, same caveat as
every other fix this session) and a live Gaussian 16 (or any code)
launch retest against a properly-named machine.

## Audit: every "$ECCE_HOME/scripts/*" reference in the codebase — one more gap found and fixed (2026-08-29)

After hitting the same "helper script never packaged" bug three times in
a row (`std2NWChem`, `processmachine`, `gensub`), Andy flagged the
pattern directly. Rather than keep fixing these one at a time as they're
hit live, audited every reference systematically: `grep -rn '"/scripts/'
src/` (the literal path fragment every genuine shell-out to a `scripts/`
helper should contain, whether built via `Ecce::ecceHome() + "/scripts/
..."` or via `Ecce::ecceDataControllersPath()`), then checked each hit.

**One more real gap found**: `scripts/eccejobmonitor`
(`src/comm/commxt/Launch.C:1566,1642`) — SCP'd to the remote machine as
part of job submission, then run there (`perl eccejobmonitor
-configFile eccejobmonitor.conf`, per `src/comm/commxt/JobStore.C:1608`)
to report job progress/completion back to ECCE. Would have failed the
exact same way `gensub` did, just one step later in the launch sequence
(after a submission script actually got generated). Confirmed portable
before packaging, same diligence as the other `scripts/*` fixes: `perl
-c` clean, only core modules (`POSIX`, `Socket`, no CPAN dependencies),
no hardcoded EMSL paths, despite being a large (4800+ line) legacy
script. **Fixed**: added an `install(PROGRAMS ...)` rule, mirroring
`gensub`'s — no wrapper `$PATH` change needed this time, since it's
referenced via a full path, not a bare command.

**Everything else in `scripts/` came back clean** — confirmed each has
*zero* references from any compiled C++ code, not just "wasn't grepped
for a false-positive-prone short name": `ecce`, `ebuilder`, `eviewer`,
`epmf`, `eprp`, `ecce_checks`, `ecce_env`, `runtime_setup`/
`runtime_setup.sh`, `sysdir`, `gbsDAVConverter`, `gbsNWChemConverter`,
`gbsDescriber`, `pmf_gui.py`, `prp_gui.py`. These are either top-level
launcher scripts this build's own `ecce-<app>` wrappers already
supersede (see the wrapper comment in `CMakeLists.txt`), or manual/
admin-only tooling meant to be run by a human once (e.g. `load_tgbs.C`'s
own header comment: "BEFORE running this program, you need to run
`gbsDAVConverter`" — a prep step for a rarely-used dev tool, not
something ECCE itself ever shells out to). `scripts/codereg/`'s status
at this point in the audit was wrong — see "Theory Details dialog /
missing DFT functional" further down for the correction.

**Not yet verified against a real job launch** — same "needs the package
reinstalled" caveat as `gensub`, plus this one needs an actual
successful submission to reach the point where `eccejobmonitor` gets
copied over at all.

### VERIFIED end-to-end (2026-08-29): first successful real job

Andy reinstalled the package and ran a real Gaussian 16 RHF/6-31G
calculation on CH4 against the `niobium` machine registration —
**completed successfully**. This is full, live confirmation of the
entire chain fixed this session, not just the individual standalone
repros documented above: Gateway login → Quick Basis Menu save
(`std2NWChem` `$PATH` fix) → Machine Registration (`processmachine`
packaging fix) → job launch (`gensub` packaging fix) → job monitoring
(`eccejobmonitor` packaging fix, found via the audit above) all had to
work, in sequence, for this run to finish. First real calculation to run
to completion since this modernization effort's wx3.2/GTK3 port began.

## Theory Details dialog / missing DFT functional — FIXED, all 14 codereg scripts verified (2026-08-29)

**Correcting an earlier claim in this file**: two prior sections above
("Basis set library metadata", the `scripts/*` packaging audit) asserted
`scripts/codereg/*.py` (the Theory/Runtype "Details" dialogs, e.g. the
button that lets you pick a DFT exchange-correlation functional) was
unported Python 2 + wxPython 2.8, needing a real port, and left it
alone. **That was asserted from running `file` on the scripts, which
doesn't distinguish Python 2 from 3 — never actually verified.** Andy
caught the inconsistency directly ("I thought you had already fixed
that?") and it was worth checking properly rather than defending the
original claim.

Checked properly: all 21 `scripts/*.py` files, including every file
under `codereg/`, already compile clean under `python3` and import
`wx` (Phoenix-style), not the Classic `from wxPython.wx import *`. Git
blame traces this to an **earlier session's own commit**, `32d2806`
("Fix JMS fd leak (#5)... port Python scripts to Python 3", 2026-08-26)
— its own message already said exactly this: "All 21 repo .py files now
compile clean under python3. wxPython Classic -> Phoenix API
compatibility for the wx-using scripts not yet verified (no wxPython/
display in this sandbox)." That caveat was still true and unresolved,
but the Python-2-ness this file claimed was already false at the time
it was written.

**What was actually still broken**, once checked for real:
1. `scripts/codereg` was never added to `CMakeLists.txt`'s install
   rules — same packaging-gap shape as every other fix in the audit
   above.
2. `JCode::getTheoryRunTypeEditorNames()` (`src/dsm/xml/JCode.C:603`)
   hardcoded a bare `python` (not `python3`) to invoke these scripts —
   Debian 13 has no bare `python` on `PATH` by default, so this would
   have failed regardless of packaging. Also prepended a legacy
   `LD_LIBRARY_PATH=../3rdparty/wxwidgets/lib:...` from the old
   vendored-3rdparty multi-platform layout — relative paths that never
   resolved to anything real in this build's flat layout, now dead
   weight now that a real Debian package resolves its own linking.
3. **`python3-wxgtk4.0` (the Phoenix wxPython bindings) isn't installed
   on this system at all** (`python3 -c "import wx"` →
   `ModuleNotFoundError`) — needed either way, packaging fix or not.
   Available in Debian 13's repos (`python3-wxgtk4.0`, `4.2.3+dfsg-2`),
   just never added as a dependency.

**Fixed**: `python3` + dropped the legacy `LD_LIBRARY_PATH` prefix in
`JCode.C`; added `install(DIRECTORY scripts/codereg ...)` to
`CMakeLists.txt`; added `python3-wxgtk4.0` to
`CPACK_DEBIAN_PACKAGE_DEPENDS` (dpkg-shlibdeps only catches linked
`.so`s, not a spawned `python3` subprocess's own module needs, same
reasoning as the existing `apache2`/`apache2-utils` entries).

**Why this matters more than it looked like at first**: traced how a
selected theory's DFT functional actually reaches the generated input
file. The Details dialog reports back to `CalcEd` over a UDP socket
(`CalcEd::OnTheoryIPC()` → `processTheoryInput()` →
`p_GUIValues->append(databuf)`), and `write_setup()`
(`ESInputController.C`) dumps `p_GUIValues` straight into the `.param`
file the Perl parser (`scripts/parsers/ai.gauss16`, etc.) reads
`XCFunctionals`/`ExchangeFunctionals`/`CorrelationFunctionals` from to
build the theory keyword line. With the dialog never launching, that
key was simply never set — this is confirmed to be the root cause of a
*separately*-reported, real failure: Andy's first real Gaussian 16 job
used RDFT with no functional selected, producing a keyword line reading
`#P r/6-31g` (a bare `r`, no functional, before the `/`) — an "ambiguous
keyword" error from Gaussian itself, not from ECCE. Andy edited the
generated input by hand to confirm: adding the functional (e.g.
`rB3LYP`) fixed it. **This is very likely the same bug**, not
independently confirmed live yet.

### Update (2026-08-29, same day): reinstalled, real Classic→Phoenix bugs found and fixed for Gaussian-16's theory dialog

Andy reinstalled (`sudo apt-get install -f` after a plain `dpkg -i`
correctly reported the new `python3-wxgtk4.0` dependency as unmet, exactly
as anticipated above) and retested live. Confirmed still broken —
terminal output showed a real `TypeError` from `wx.Font()`. **With
`python3-wxgtk4.0` now actually installed on niobium, this became
directly testable and fixable for real** — no more guessing from source
reading alone.

Root-caused and fixed four genuine Classic→Phoenix API breaks, all in
the two files every `codereg/*.py` script shares (`globals.py`,
`templates.py`), found by iterating: run the real script standalone with
a real bound UDP socket standing in for `CalcEd`'s side of the IPC
protocol (`ged16theory.py <outfile> <port> GUIValues Writable DebugOff
DFT RDFT Geometry TestCalc 0 11 12 13 14 15 16 17`), fix whatever it
throws, repeat:

1. `wx.Font(pointSize=.., family=.., style=.., weight=.., face=..)` —
   Classic's keyword names don't match any Phoenix overload (`face` is
   `faceName` in Phoenix; mixing legacy keyword names confuses Phoenix's
   overload resolution entirely, confirmed via a `TypeError` listing all
   7 rejected candidate overloads). Rewrote using `wx.FontInfo`, the
   modern, unambiguous, non-deprecated way to build a `wx.Font` — e.g.
   `wx.Font(wx.FontInfo(8).FaceName("Helvetica").Bold())`.
2. `wx.RESIZE_BOX` doesn't exist in Phoenix at all (a legacy, effectively
   Windows-only decorative style bit; there's no separate "resize box"
   concept from `wx.RESIZE_BORDER` on GTK regardless) — dropped from the
   `~(wx.RESIZE_BORDER|wx.RESIZE_BOX|wx.MAXIMIZE_BOX)` frame-style
   expression in `EcceFrame.__init__`.
3. `Sizer.Add(item=(0,0), ...)` — Phoenix has no `item` keyword on
   `Sizer.Add()` at all; the two spacer-adding call sites needed the
   size tuple passed positionally instead for SIP's `wx.Size` conversion
   to apply.
4. `wx.ShowEvent.GetShow()` was renamed `IsShown()` in Phoenix
   (`AttributeError`, only reached once the DFT-specific code path in
   `Ged16TheoryFrame` bound `wx.EVT_SHOW`).

**Also found, and this one is the actual reason `32d2806`'s "port"
wasn't sufficient on its own**: `EcceGlobals.Socket.send(str)` —
`socket.send()` requires `bytes` in Python 3, not `str`. This is a real
Python-2-ism `python3 -m py_compile` can never catch (it's syntactically
valid Python 3, just wrong at runtime) — six call sites across
`globals.py`/`templates.py`, all fixed with `.encode()`.

**Verified working end-to-end**, not just "doesn't crash": ran
`ged16theory.py` with `RestoreFlag=GUIValues` (the real interactive
path `CalcEd`'s "Theory Details" button uses, not the `NO_GUIValues`
defaults-only path) for both an SCF theory and a DFT theory
(`category=DFT theory=RDFT`), screenshotted the live X11 window both
times (`xwininfo` + `import -window`). Both render completely correctly
— a real, working "ECCE Gaussian-16 Editor: Theory Details" dialog with
all its sections (SCF Convergence, DFT Convergence, Solvation), and
critically, the DFT one shows a working **"Exchange-Correlation
Functionals: Combined XC: PBE0 (hybrid)"** dropdown — this is the exact
control the original bug report needed. The `NO_GUIValues` path (used
for CalcEd's own startup default-population, not the interactive
button) also verified separately: clean `#STARTED`/full settings
dump/`#INITIALIZED`/`#CLOSING` UDP sequence, no exceptions.

Also removed two stale, tracked Python 2 bytecode files
(`scripts/codereg/{globals,templates}.pyc`) left over from before the
32d2806 port — harmless (Python 3 never looks for a bare `.pyc` next to
a `.py` file, only versioned ones under `__pycache__/`) but pure
clutter.

### Follow-up (same day): the other 13 scripts, all verified clean

Went through the remaining `amicatheory.py`, `ged03{runtype,theory}.py`,
`ged09{runtype,theory}.py`, `ged16runtype.py`, `ged98{runtype,theory}.py`,
`guk{runtype,theory}.py`, `meta{rtyp,thry}.py`, `ned{runtype,theory}.py`
the same way, via a small reusable driver
(`test_codereg.py` — real bound UDP socket standing in for `CalcEd`'s
side of the IPC, both `NO_GUIValues` and `GUIValues` restore modes,
screenshot the live window when one appears). Ran all 14 scripts across
every `Category` each one actually branches on (`SCF`, `DFT`, `MP`,
`CC`, `CI`, `SE`, `NWPW` — whichever a given script checks for).

**One more real bug found**, same class as before: `wx.StaticLine_
GetDefaultSize()` (Classic's module-level static-method naming) doesn't
exist in Phoenix — it's `wx.StaticLine.GetDefaultSize()` (a real class
method) there. One call site in `templates.py`
(`EcceLineLabelSeparator.__init__`), hit by `guktheory.py` specifically
(the first script whose theory options include this separator widget)
but shared by all of them. Fixed the same way as yesterday's four:
confirmed the failing call, confirmed the Phoenix-correct replacement
interactively first, then patched the shared file.

**Verified working end-to-end for every one of the 14 scripts**: clean
run in both restore modes, no exceptions, and a real rendered screenshot
in `GUIValues` mode for each — spot-checked several visually (not just
"no exception raised"): `guktheory.py` renders its full DFT
Exchange-Correlation Functionals section (confirming the `StaticLine`
fix), `amicatheory.py`/`metathry.py`/`nedruntype.py`/`gukruntype.py`/
`ged09runtype.py` all render correctly with real widgets, correct
layout, no garbled/missing sections. This closes out the "codereg"
Classic→Phoenix porting work — five real API-rename bugs total across
`globals.py`/`templates.py` (the shared base every script imports), all
fixed, all 14 scripts confirmed rendering live.

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

**Second watch-list item, same shape**: `wxFIXED_MINSIZE` (the sizer-item
flag behind the Basis Set Tool's mini periodic table fix above) appears
73 times across the tree — but the overwhelming majority are in `.pjd`
wx-designer *project* files, not the compiled `.C` sources those files
generate (this build doesn't regenerate `.C` from `.pjd` at build time,
so `.pjd` content is inert for runtime behavior either way). The actual
compiled call sites, not yet individually checked: `Gateway.H`,
`src/apps/machbrowser/MachineBrowserGUI.C`,
`src/apps/builder/StructLibGUI.C`,
`src/apps/machbrowser/WxMachineStatusGUI.C`,
`src/apps/organizer/CalcMgr.C`,
`src/apps/organizer/NoAccessContextPanelGUI.C`,
`src/wxgui/wxtools/WxFeedbackGUI.C`. `wxFIXED_MINSIZE` is only a bug when
the wrapped item's content changes *after* it's added to the sizer (an
empty placeholder later filled with a real child, exactly what happened
here) — it's harmless, even useful, when the item already has its final
size at insertion time. No evidence any of these other 7 have the same
"empty placeholder, filled later" pattern; not preemptively touching them
without a reported symptom, same reasoning as the item above.
