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
