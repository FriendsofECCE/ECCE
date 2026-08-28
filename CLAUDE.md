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

## New, separate bug found: `GatewayPrefs::Show()` reentrancy crash — UNRESOLVED

Found 2026-08-28, same evening as the fix above, while trying to visually
verify the fix on a real display. **This is a different bug from the one
just fixed** — same root cause *class* (wx3.2/GTK3 `DoSetSize`/`Layout`
reentrancy), different trigger, not yet covered by any guard. Not a
regression from the fix above; it's a pre-existing latent bug nobody had
reached before, because `gateway` always died at the `DataServers` check
(previous section) before a user could ever click Preferences.

### How it was found
A standalone test harness was written (not committed — lived at
`/tmp/claude-1000/.../scratchpad/GatewayPrefsVisualTest.C` in that session's
scratchpad, gone once that session ends) that constructs `GatewayPrefs`
directly and calls `Show(true)` on it, bypassing `GatewayApp::OnInit()`'s
server checks entirely (per `GatewayPrefs.H`'s own header comment: "this
dialog can become a standalone app"). Compiled and linked manually against
the already-built `gateway` object files (`GatewayPrefs.C.o`,
`GatewayPrefsGUI.C.o`, `StateButton.C.o`) and static libs — see that
session's transcript for the exact compile/link commands if this needs
reconstructing; it's a ~15-line wxApp subclass, trivial to rewrite from
scratch too.

**Safety note for whoever runs this again**: the first run had no memory
cap and grew to 40GB+ RSS, freezing the machine — this exact failure mode
was already documented above ("multiple GB of RSS... OOM-killed after
~20s") and should have been guarded against from the start. Every run after
that used a hard, kernel-enforced cap that actually contains it:
```
systemd-run --user --scope -p MemoryMax=512M -p MemoryHigh=384M -p TasksMax=64 --collect \
  timeout -s KILL 10 bash -c "ulimit -s 8192; ulimit -v 1000000; '<binary>'"
```
`ulimit` alone (as used for the construction-time crash above) is
insufficient for a fresh, unverified code path — it caps the *stack*, but an
unbounded heap-growth variant of this same bug class would sail past it.
Use the `systemd-run` wrapper for **any** further live testing of this
specific bug, not just `ulimit`.

### Root cause (confirmed via gdb + a diagnostic trace, fix not yet found)
`Gateway.C:477` (`case wxID_PREFERENCE:`) does a plain
`p_prefsDlg->Show(true); p_prefsDlg->Raise();` to open the Preferences
dialog — this is the real, normal interactive code path, not a test
artifact. Calling `Show(true)` on this previously-hidden frame triggers the
same `DoSetSize -> wxEVT_SIZE -> InternalOnSize -> Layout() ->
RepositionChildren -> DoSetSize` cycle as the construction-time bug,
confirmed via gdb backtrace (identical shape to the original crash).

Critically, **the storm happens after `Show()` already returned**, not
synchronously inside it — confirmed by instrumenting the test harness with
`fprintf` markers before/after the `Show(true)` call: `OnInit()` completes
and returns cleanly every time, and the crash happens later, inside the wx
main event loop (processing ~12,500 `FilterEvent` calls before segfaulting).
This means GTK's realize/configure/size-allocate negotiation for this
widget tree spans **multiple event-loop turns**, not one synchronous call —
unlike `Fit()` at construction time, which is a single synchronous call
suppression can cleanly bracket.

Two fix attempts were tried and both failed (tried live, safely capped,
then reverted — the tree is back to the clean `94ed704` commit state, no
half-finished attempt left in place):
1. **Override `GatewayPrefs::Show()`** to arm the existing
   `g_suppressSizeEventsDuringFit` guard, call the base `Show()`, then
   disarm synchronously — same pattern as the construction-time fix. Failed:
   confirmed via trace that `OnInit()` (and therefore this synchronous
   wrapper) completes and returns *before* the actual storm starts, so
   nothing was suppressed by the time it happened.
2. **Same override, but disarm via `CallAfter()`** (runs once the event
   loop has drained everything currently queued) instead of disarming
   immediately. Failed: the trace showed the flow working as designed —
   `finishShowSuppression()` fires, calls one manual `Layout()` while still
   suppressed, disarms — but a *separate*, later storm still starts right
   after, with suppression already off (~12,500 `FilterEvent` calls, all
   `suppress=0`, right after the "suppress now false" log line). One
   `CallAfter` round evidently isn't enough — GTK's negotiation for this
   widget tree apparently needs more than one event-loop turn to settle,
   and the single manual `Layout()` call doesn't converge it either.
3. **Bind `wxEVT_SHOW` as the release signal, plus a 2s safety-net
   `wxTimer`**, based on reading wxWidgets' actual GTK3 backend source
   (`src/gtk/toplevel.cpp`, `wxTopLevelWindowGTK::Show()`/
   `GTKDoAfterShow()`/`GTKUpdateClientSizeIfNecessary()` — see "Is this a
   wx version issue?" below). Theory: `Show(true)` on wxGTK3 can defer the
   actual `gtk_widget_show()` while it round-trips with the window manager
   for `_NET_FRAME_EXTENTS`, and only once that completes does it apply a
   pending client-size fit and send `wxEVT_SHOW` — so binding release to
   that event should bracket the real trigger regardless of how many
   event-loop turns the WM round-trip takes. Failed anyway, still crashed
   the same way. Not fully diagnosed why — see "diagnostic dead end" below;
   this specific theory was not conclusively ruled in or out.

### Diagnostic dead end: gdb backtraces can't reach the real trigger here
After attempt 3 also failed, tried to find the true (non-recursed) entry
point into the storm directly, rather than keep guessing mechanisms:
- Broke on `wxWindowBase::InternalOnSize` and captured the very first hit
  of the whole program: it's an ordinary, harmless, expected size event
  fired while constructing a `wxStaticBox` in `CreateControls()` — routine
  construction-time noise, not the storm. This approach doesn't
  distinguish "routine call" from "start of runaway recursion" and isn't
  useful as-is (would need a real reentrancy check — e.g. is
  `InternalOnSize` already on the stack N frames up — not just a hit
  counter, to be useful).
- Tried instead to capture the *bottom* of the stack at the moment of the
  crash (`bt -40` — oldest frames, closest to program entry — as opposed
  to `bt 40` which only ever shows the same repeating ~11-frame cycle near
  the top). This is the right idea but hit a practical wall: gdb itself
  needs real memory and CPU time to unwind and symbolize 100,000+ frames,
  and that need is directly in tension with the memory/time caps this bug
  requires for safety (see the `systemd-run` note above) — a 15s timeout
  cut gdb off mid-unwind; a 120s timeout let gdb run out of virtual memory
  instead (`./gdb/utils.c:666: internal-error: virtual memory exhausted`)
  and dumped raw unsymbolized addresses. Whoever picks this up needs a
  setup where gdb itself has more room than the crashing process does —
  e.g. run gdb unconfined, only cap the *inferior* process (may need
  `set exec-wrapper` or a `catch signal` + immediate `bt -60` before any
  further recursion, rather than letting it run all the way to the actual
  stack-limit SIGSEGV) — not attempted yet.

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

### Next steps for whoever picks this up
- Get a real backtrace to the true trigger first, before another fix
  attempt — see "diagnostic dead end" above for why the last three
  attempts were all somewhat shots in the dark, and what a working
  diagnostic setup might need (gdb unconfined, only the inferior capped).
- Once the real trigger is known, the fix likely needs suppression held
  across as many event-loop turns as it actually takes to settle, not a
  fixed one or two — e.g. a debounced approach: on each `wxEVT_SIZE` that
  would otherwise be let through, re-arm suppression and re-schedule
  another `CallAfter`, only doing the final un-suppressed `Layout()` once a
  round genuinely produces no further churn.
- Alternative angle worth investigating first: *why* does this widget tree
  need multiple rounds to settle on first realize, when a single manual
  `Layout()` pass was sufficient at construction time? If that's
  answerable, a targeted fix (e.g. pre-sizing something before `Show()`)
  might avoid needing the debounce complexity at all.
- Rebuild the test harness from scratch (see "How it was found" above) —
  it wasn't committed and won't persist across sessions. Always run it
  under the `systemd-run` memory cap, never bare.
- Test repeatedly once a fix seems to work — timing here has already proven
  inconsistent enough (this bug went unnoticed through 8+ clean runs of the
  full `gateway` binary, because those never reached the `Show()` call) that
  one clean run should not be trusted alone.
- Once actually fixed: manually resize both the main toolbar and the
  Preferences dialog interactively, per the original investigation's
  checklist — still not done, still blocked on either this bug or a working
  `DataServers` setup (see above).

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
