# GUI application smoke suite

The third tier. `tests/parsers` stops at a parser script's stdout,
`tests/dialogs` at the codereg dialogs, and neither one ever loads a line of
this project's C++. The bugs that have cost this port the most time live
exactly there:

* **#78** — `wxGrid::CreateGrid()` fires `wxEVT_GRID_SELECT_CELL`
  *synchronously* on wx3.2/GTK3, during construction, into a handler whose
  dependencies did not exist yet. It segfaulted `builder` on opening any job
  with vibrational data.
* **the layout-reentrancy class** documented in CLAUDE.md —
  `DoSetSize` → `wxEVT_SIZE` → `Layout()` → another `DoSetSize`, sometimes
  non-convergent, sometimes crashing *well after* `Show()` has returned.

Both reduce to one automatable question: *does the app start, put up a
window, and stay up?* That is all this suite asks. It is a narrow question,
but it is the one these bugs answer "no" to, and nothing else in the tree
notices.

## The reaper, and why this suite turns it off

Every ECCE app's wrapper runs `ecce-gateway-reap --if-idle` on exit, so
the broker and dispatcher are stopped once the last app on a display
closes (#102 — a SIGABRT once stranded them for ten days).

That rule is right for a user's session and wrong here, where apps are
run **one at a time** and every app is therefore the last one out. The
gateway was being torn down after the first app exited, and every app
after it met a dead gateway:

```
ecce-gateway-reap: no ECCE app left on :70 -- stopping JMSDispatcher
ecce-gateway-reap: no ECCE session left -- stopping ActiveMQ broker
```

which surfaced as ten apps "opening no window" and gateway showing
`ECCE Server Failure`.

So the suite sets `ECCE_NO_REAP=1`. It already stops the services itself
afterwards, so taking that job over costs nothing. The variable is a
general escape hatch for anything that manages the services itself — a
debugging session, for instance.

It is unset again for the shutdown, and that matters more than it looks:
`ecce-gateway-stop` stops the dispatcher itself but hands the **broker**
to `ecce-gateway-reap`, which is the only thing that knows whether another
display still needs it. With `ECCE_NO_REAP` still set, the reaper exited
immediately and the broker was never stopped — so every run leaked a
512MB JVM, which is #102 again, caused this time by the fix for it being
switched off and left off.

## A window is not proof an app started

ECCE reports a dead service by putting up a dialog — `ECCE Server
Failure` — and this suite counted any window as success. So one stopped
dataserver presented as **eleven failures**: the ten apps that need the
server timed out with "opened no window within 40s", and the app that
actually named the cause sat in the passing column.

Two changes, both about making one fault report as one fault:

- a window whose title announces a failure (`failure`, `error`, `fatal`,
  `cannot`, `unable`, matched on word boundaries) fails the app
- the services are checked after starting and before the sweep, so a
  dataserver that did not come up is reported once, by name, instead of
  as a dozen apps that cannot open windows

## What it does

For each installed wx-linked app:

1. snapshot the X server's top-level windows;
2. launch `/usr/bin/ecce-<name>`;
3. wait for a new window to appear (default 40s);
4. **keep watching for another 8s** — the asynchronous variant of the
   reentrancy bug does not crash until after the window is up, so a bare
   "did it start?" check would miss it;
5. terminate it and check it did not die on a signal, and that its output
   carries no crash or assertion marker.

Apps are launched through the `ecce-*` wrapper, never the binary in
`$ECCE_HOME/bin`: the wrapper is what sets `ECCE_HOME`, `ECCE_REALUSER` and
the Wayland/GDK_BACKEND workaround, and what starts the background services.
Running the binaries directly is explicitly unsupported.

## Why Xvfb and not broadway

`tests/dialogs` runs its wxPython dialogs under `broadwayd`, GTK3's
displayless backend, and so needs no X server at all. That does not work
here: every one of these apps links GLX (`libGLX`, `libwx_gtk3u_gl`) because
the molecular viewer is Open Inventor, and broadway provides no GLX. So this
suite needs a real X server, with Mesa's llvmpipe supplying software
OpenGL — confirmed working: `direct rendering: Yes`, renderer
`llvmpipe (LLVM 19.1.7)`. The suite prints the GL status on startup, because
a run without it would silently only be testing half of what matters.

Xvfb is not installed by a Debian desktop metapackage, so:

* `apt install xvfb`, or
* `ECCE_XVFB=/path/to/Xvfb`, which also covers not having root:

      apt-get download xvfb && dpkg -x xvfb_*.deb ./root
      ECCE_XVFB=$PWD/root/usr/bin/Xvfb tests/apps/run_tests.py

* with neither, the suite **skips** rather than fails.

It deliberately does not fall back to `$DISPLAY`. These apps open full-size
windows and start background services; doing that to somebody's live desktop
by accident is not acceptable behaviour for a test.

## Running it

    tests/apps/run_tests.py                  every installed GUI app
    tests/apps/run_tests.py --app builder
    tests/apps/run_tests.py -v               window titles and startup times
    tests/apps/run_tests.py --list
    tests/apps/run_tests.py --keep-services
    tests/apps/run_tests.py --budget 600
    tests/apps/run_tests.py --use-real-state   # shares your live session

Also wired into CTest:

    cd build-cmake && ctest -R apps --output-on-failure

## Testing an install that needed no root

`/opt/ecce` is root-owned, so testing a C++ change used to mean asking
somebody to run `sudo dpkg -i`. It does not have to: the install location is
a build option.

    cmake -B build-user -GNinja \
          -DCMAKE_INSTALL_PREFIX=$HOME/.local/ecce \
          -DECCE_HOME_DIR=$HOME/.local/ecce \
          -DECCE_WRAPPER_DESTINATION=$HOME/.local/bin
    cmake --build build-user --target install

    ECCE_TEST_HOME=$HOME/.local/ecce \
    ECCE_TEST_WRAPPERS=$HOME/.local/bin \
      tests/apps/run_tests.py

Both default to `/opt/ecce` and `/usr/bin`, so packaging and an ordinary run
are unaffected, and the whole suite — including the fixture's data-server
account — follows `ECCE_TEST_HOME`. It did not: `fixture.py` had a
hardcoded `/opt/ecce` and a hardcoded port 8096 of its own, so a run
against a private install created its account with the *installed*
package's script and served it from the *real* data server.

The two installs no longer share `~/.ECCE` either, because no run uses
`~/.ECCE` at all — see the isolation section below.

## Every run is isolated from your real ECCE session

All per-user state — preferences, the data server's whole document root,
the ActiveMQ data directory, the JMS port files — lives in
`$ECCE_REALUSERHOME/.ECCE`, and both the C++ (`Ecce::realUserHome`) and the
shell scripts honour that variable. The services are also per-user and
listen on fixed ports. So a run that shares them with a live ECCE session
does not collide tidily: two brokers contend for one `~/.ECCE/activemq`
data directory, `ecce-dataserver-start` early-exits because "something is
already listening" and the apps then read somebody else's document root,
and the result is a page of failures that read exactly like application
bugs. A suite that can do that to you is not one you will trust.

So **isolation is the default, not an option** (`isolate.py`). Every run
gets:

* its own state directory — `$XDG_CACHE_HOME/ecce-apps-suite`, or
  wherever `ECCE_TEST_STATE` says — exported as `ECCE_REALUSERHOME`,
  which is the variable everything else actually reads. It is kept
  between runs rather than thrown away, so the seeded document root and
  the synced basis-set library are paid for once;
* its own ports, 8296 and 8288 by default rather than the real 8096/8088,
  or the next free ones; `ECCE_DATASERVER_PORT` / `ECCE_BROKER_PORT`
  still pin them explicitly, and a pinned port that is busy is an error
  rather than a silent move;
* its own `$ECCE_HOME`: a directory of symlinks to the installed tree
  with one real `siteconfig/` of its own, with `DataServers` and
  `jndi.properties` repointed at those ports. This is the piece that was
  missing before, and its absence made moving the ports actively harmful
  — `siteconfig/DataServers` is where the *apps* learn the data server's
  URL, it is written at package time with `http://localhost:8096/Ecce`,
  and it lives in a root-owned install a test cannot edit. Moving the
  service but not the apps pointed the apps straight back at the real
  session. `ECCE_HELP` moves with it for the same reason.

The rewrite of those two files is **checked**, not assumed: a regex that
quietly matched nothing would leave a run looking isolated while talking
to port 8096.

`--use-real-state` opts out, for the rare case where you want the run to
see your own calculations. It is not recommended and says so.

### The account the server needs

`EDSIServerCentral::checkServerSetup()` reads the server's `users`
collection and throws "A failure was detected in the ECCE server setup"
when it cannot — and a data server nobody has ever added an account to is
in exactly that state, which is every fresh state directory and every CI
run. A person does this once by hand from `GETTING_STARTED.md`; the suite
now does the same thing for its own state directory before the sweep.
Without it the gateway reported `ECCE Server Failure` and `BuilderApp`
quits outright on the same check.

## The run is bounded

The suite held a CI job for two days once (#127), and the thing that
eventually stopped it was GitHub's own six-hour ceiling. Every individual
step is bounded now — the window wait, the settle, the output drain, the
`xwininfo`/`xdpyinfo` calls, `proc.wait()` — but "every step I thought of
is bounded" is a weaker claim than "the run terminates", and only the
second one is worth relying on. So the whole run has a wall-clock budget
(`--budget`, `ECCE_APPS_BUDGET`, 20 minutes by default against a healthy
run of about six), enforced with `SIGALRM` so that it fires *into*
whatever is stuck rather than waiting to be polled between apps. The
services are still stopped on the way out, under a second, shorter alarm
in case the shutdown is what hung.

## One wedged display is one failure, not thirteen

The other half of #127. Once the X server stops answering, every
remaining app "opens no window within 40s" and is killed — identical
failures that have nothing to do with the apps they name, with the one
interesting fact (that it started right after a particular app) left for
the reader to infer from the ordering. The display is now checked after
every app, and a run that finds it gone says so once, names the app it
happened after, lists what is still connected to the server, and stops.

## It tests the INSTALLED build

Unlike the other two suites, this one is not build-independent — it runs what
is in `/opt/ecce`, not what is in the build tree. That is deliberate: it is
the only tier that can catch the "correct in the repo, broken in the `.deb`"
class of problem this project has hit repeatedly (a `scripts/*` file with no
`install()` rule is absent from the package and produces "command not found"
only when installed). If nothing is installed, the suite skips.

It also means a green run says nothing about uninstalled changes. Package and
install first, or the result is stale.

## Services

The gateway and data server are brought up once before the loop rather than
being left to the first app's own wrapper, so broker startup does not eat
into that app's window timeout and look like slowness.

The suite records whether they were already running and, if it started them,
stops them again afterwards — these are somebody's per-user services and a
test run is not entitled to leave them behind. `--keep-services` opts out.

## Coverage is discovered, not listed

`apps.guiBinaries()` finds every executable in `$ECCE_HOME/bin` that links
`libwx_gtk3` and has a matching `/usr/bin/ecce-*` wrapper, so a newly
packaged app is covered the moment it ships — the same principle as
`tests/dialogs` reading the `.edml` files rather than keeping its own table.

`cases.HELPERS` marks the few binaries that are not top-level applications
(`msgdialog`, `passdialog`, `vizthumbnail`, `launcher`): other apps exec
them with arguments, so running them bare is expected to exit rather than
open a window. They are still launched, and still fail the run if they exit
*on a signal*.

## What it deliberately does not do

* **It does not click anything beyond opening a calculation.** No synthetic
  input, no menu walking.
* **It does not click anything.** No synthetic input, no menu walking. That
  is the expensive, brittle tier (Squish/dogtail) and is the right place to
  stop for now.
* **It does not check what the window contains** — only that one exists and
  the process survives. A blank or misdrawn window passes.

## Bugs found on the first run

Two, both of which had been live for weeks:

**Every app aborted on SIGABRT.** `JMSMessage.C:497`,
`Assertion (infile.good()) failed Could not open ~/.ECCE/<host>_<display>`.
The JMSDispatcher is per *session*: `ecce-gateway-start` launches it with a
`-DDISPLAY` and it writes its port to a per-display file that `JMSMessage.C`
reads back and `EE_FATAL`s on when missing. But the pidfile was a single
display-agnostic `jmsdispatcher.pid`, so with a dispatcher already running
for one display, starting ECCE on a **second** display — a VNC session, an
X-forwarded session, a second seat, or this suite — reported "JMSDispatcher
already running", never wrote that display's port file, and killed every app.
Fixed by making the pidfile per-display across
`ecce-gateway-{start,stop,status}` and requiring the port file to exist too.
Not a test artefact: any user with two sessions hits it.

**`metadyn` SIGSEGV'd on every launch**, shortly *after* its window appeared
— which is precisely why this suite keeps watching after the window is up.
`MetaEd.C:167` called `ResourceDescriptor::getTool(METADYNAMICS)->getIcon()`
unchecked, and that `<Tool>` registration was archived when MetaDyn's menu
entries were removed on 2026-08-30. Unnoticed for three weeks because nobody
launches it. Fixed; see `cases.XFAIL` for the sibling audit it implies —
about a dozen apps share the unchecked `getTool(...)->getIcon()` pattern.

### A note on the first run after a version bump

`Builder::restoreSettings()` (`Builder.C:1938`) shows a modal "Window layout
preferences are being reset due to the version upgrade" notice once whenever
the stored perspective version differs from the running one. So immediately
after an upgrade, the first window this suite sees for `builder` is that
modal rather than the main frame. That is correct behaviour and the check
still holds -- a window appeared and the process survived -- but it is worth
recognising in the `-v` output rather than mistaking for a mislabelled
window.

## Running against an uninstalled service fix

`ECCE_GATEWAY_START` / `ECCE_DATASERVER_START` point at an alternative copy
of those scripts. This exists because `/opt/ecce` is root-owned: without it,
a fix to a service script cannot be tried at all without a packaging
round-trip — and the first bug above lives in exactly those scripts. It was needed while
the per-display fix existed only in the tree:

    ECCE_GATEWAY_START=$PWD/packaging/gateway/ecce-gateway-start \
      tests/apps/run_tests.py

It is **not** needed now -- 8.11.0 carries the fix and the suite runs against
the installed build with no overrides at all. Keep the escape hatch for the
next time a service script needs changing.

## Opening a real calculation

Starting an app empty proves it constructs; it cannot reach the bug that hurt
most (#78, `builder` segfaulting on any job with vibrational data), because
that path only runs when there is data to show. So one check opens a real
calculation.

`fixtures/calc-water-vib/` is a Gaussian-16 water job (STO-3G, opt+freq)
carrying VIB, VIBFREQ, VIBIR, VIBRAM, MO, ORBENG, GEOMTRACE and the usual
scalars — enough to reach the vibration, MO and geometry-trace panels, three
of the four places viewer crashes have been found here. Its `Outputs/`
directory was dropped: nothing in the property pipeline reads it, it was the
bulk of the size, and it was the only part carrying absolute paths and a real
username.

`fixture.py` installs it into a throwaway `eccetest` data-server account and
returns a context URL, which the suite passes as `builder -context <url>`.

**"It stayed alive" is deliberately not the whole check.** If the context
were ignored the app would also stay alive, and the check would pass while
testing nothing — so it additionally requires evidence in the data server's
access log that the calculation was actually read, including
`Parameters/chemsys.mvm`. That guard earned itself immediately: it caught a
typo in `BuilderApp.C` (`strcmp(argv[1], "-pipe")` instead of `argv[i]`)
which made `-pipe` match on every later iteration and swallow the `-context`
URL, so the calculation was silently never opened.

### Why the fixture needs no password

`httpd.conf.ecce` allows anonymous reads at the `Ecce` level, but a PROPFIND
on a calculation still 401s for every account, because
`ecce-dataserver-adduser` writes a per-account `.htaccess` ending in
`require user <name>` and `Ecce/users` is `AllowOverride AuthConfig Limit`.

Passing credentials instead is a rabbit hole: the auth-cache key is
`EcceURL::getRef() + realm`, and the realm is not the top-level
`AuthName "Ecce"` but a per-account one synthesised from the account's name
(`AuthName Ecce-Andy.Test`). Several plausible spellings all still 401'd.

Since the fixture owns its account, it simply grants anonymous reads to
itself — `fixture.py` overwrites the generated `.htaccess` with one allowing
read methods and still denying writes. No credentials, no `-pipe`, no
guessing, and the permission change is confined to an account holding
nothing but the fixture.

## Adding an app

Nothing to do: it is discovered. If it needs an argument to be useful, add
it to `cases.HELPERS` (with the reason) or give it a `cases.TIMEOUTS` entry
if it is merely slow. A reproduced crash that is not being fixed yet goes in
`cases.XFAIL` with enough detail to act on. All three lists are checked in
both directions, so a stale entry fails the run.
