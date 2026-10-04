# ECCE environment variables

Every `ECCE_*` variable that ECCE's code, scripts and generated launchers
read, taken from the source (`getenv`, `$ENV{}`, `os.environ`, shell
`${VAR}`), not from older documentation. Where a value's meaning could not
be settled from the code it says so.

Set a variable before starting ECCE, for one run or in your shell profile:

    ECCE_EDITOR=geany ecce
    export ECCE_EDITOR=geany        # in ~/.profile, for good

Most "on/off" variables only test whether the variable is **set**, so
`ECCE_NO_VIZIMAGES=0` still turns the feature off. Where a variable is
read for a value (`1`, a number, a path) the entry says so.

**About `siteconfig/site_runtime`.** That file lists many of these with
comments, and says its entries are "put into the user environment at
runtime". Only the old csh launcher scripts (`scripts/ecce_env`) do that.
The `ecce` and `ecce-<app>` launchers of this build do not read the file,
so on a packaged install those entries are **not** applied; export the
variable instead. The exception is `ECCE_AUTO_ACCOUNTS` and
`ECCE_STORE_TRAJECTORIES`, which the C++ code reads from the file itself
(see Deployment).

## User choices

Set the editor, terminal and browser in the Organizer under
**Edit > Preferences > External programs**. The variables below win over
those settings.

| Variable | What it does | Values / default |
|---|---|---|
| `ECCE_EDITOR` | Editor for input decks and output files. Overrides Preferences. Order: this, Preferences, `VISUAL`, `EDITOR`, `vi`. | A command, arguments allowed (`geany -i`, `emacs -nw`). Default `vi`. |
| `ECCE_BROWSER` | Web browser for Help. Overrides Preferences. Order: this, Preferences, then the first of `xdg-open`, `x-www-browser`, `sensible-browser`, `firefox`, `firefox-esr` on `PATH`. | A command, arguments allowed. |
| `ECCE_XTERM_FONT` | Font passed as `-fn` to the xterm that opens for a terminal editor and for a shell in a run directory. Not passed when unset. | An X font name, e.g. `fixed`. Default unset. |
| `ECCE_GATEWAY_WINDOW` | Show the Gateway launcher window, which is hidden by default. Also adds a "Gateway window" page to Preferences. | Set and not `0` to enable. Default off. |
| `ECCE_CODE_ORDER` | Codes listed first, in this order, in the Editors menu and code-switch buttons; the rest follow in no fixed order. | Space-separated code names, e.g. `NWChem ORCA`. Default unset (the old csh launcher set `NWChem`). |
| `ECCE_FILESIZE_WARNING` | Organizer asks before showing a file larger than this. | Kilobytes. Default 10240. |
| `ECCE_WIREFRAME_LIMIT` | Above this many solute atoms the trajectory panel offers to switch to wireframe. | Atom count. Default 5000. |
| `ECCE_MAX_CACHED_MO` | Number of computed MO, density and spin-density grids the viewer keeps. | Integer. Default 10. |
| `ECCE_NO_VIZIMAGES` | Turn off image-file and thumbnail generation, which uses OpenGL offscreen rendering (POV-Ray output is unaffected). For platforms where offscreen rendering crashes. | Set to disable. Default off. |
| `ECCE_SURFACE_COLOR1` | Colours of the positive and negative isosurface in the "light/dark" colour scheme, as 0.0 to 1.0 values. | Six numbers separated by space, `,`, `:` or `/`: positive R G B then negative R G B. Default 0.95 0.95 0.95 / 0.2 0.2 0.2. |
| `ECCE_SURFACE_COLOR255` | As `ECCE_SURFACE_COLOR1` with 0 to 255 values; wins over it if both are set. | Six integers, same separators. |
| `ECCE_ESP_RANGE` | Fixes the colour ramp of an electrostatic-potential surface to plus and minus this value instead of scaling to the data. | Positive number, Hartree per e. Unset or invalid: scaled automatically (an invalid value is reported on stderr). |
| `ECCE_NWCHEM_DFT_USE_B3LYP` | NWChem DFT dialog starts with B3LYP instead of VWN5 with Slater exchange. The user can still change it. | The exact text `true`. Default off. |
| `ECCE_JOB_CHECK` | The Organizer, when a project is opened, checks whether calculations shown as submitted or running still have a running `eccejobstore`, and repairs a stuck state. | Set to enable. Default off. |
| `ECCE_TRANSPORT` | Obsolete. There is one transport: a local machine runs commands directly, an ssh machine uses libssh or the `ssh` command (see `ECCE_SSH_BACKEND`). The value is ignored; `pty`, which an 8.x job master exports, is noted once on stderr. | |
| `ECCE_SSH_BACKEND` | Which client serves an ssh machine. `libssh` is ECCE's own connection. `openssh` runs the `ssh` command as a plain subprocess (BatchMode, no tty, real exit status; files go through `cat`; a front end becomes `-J`) and so reuses a connection the user's `~/.ssh/config` shares (`ControlMaster`/`ControlPath`), which libssh cannot: a login with a second factor is done once, by hand, and ECCE rides on it. It never prompts itself: when the host shares connections and none is open, it opens one with `ssh -f -N` and the questions (password, second factor, an unknown host key) go to ECCE dialogs through `SSH_ASKPASS` (see `ECCE_ASKPASS`; needs OpenSSH 8.4). Without a display, or when the dialog is cancelled, the error says to run `ssh <host>` once; after a refused or cancelled attempt ECCE does not ask again for that host for two minutes. `auto` picks `openssh` for a host whose `ssh -G <host>` shows a `controlmaster` other than `no` or a `controlpath`, or a `proxyjump`/`proxycommand` (libssh cannot answer the proxy's own login prompt), and libssh otherwise (a build without libssh always uses `openssh`). For a proxied host with no sharing of its own, ECCE adds `ControlMaster=auto`, a `ControlPath` under `~/.ECCE/cm` and `ControlPersist=10m`, so the login is asked once; a `ControlMaster` or `ControlPath` of the user's is never overridden. | `auto`, `libssh` or `openssh`. Default `auto`. |
| `ECCE_ASKPASS` | The program ssh runs to ask for a login when ECCE opens a shared connection: it gets the prompt as its argument, prints the answer, and exits non-zero to cancel; it finds the host and user in `ECCE_ASKPASS_HOST` and `ECCE_ASKPASS_USER`. Default: `$ECCE_HOME/scripts/ecce-askpass` (ECCE's own dialogs) when there is a display, none otherwise. | Path to a program. |
| `ECCE_SSH_FRONTEND_POOL` | A machine behind a front end (login node) reached by a forwarded connection over libssh: all the sessions of one process through the same front end (the commands, each job-monitor stream, copies) share one login to it, so a front end that asks for a second factor asks once. `0` gives every session a login of its own, as before. A nested ssh (the fallback where the front end refuses forwarding) still logs in per session. The OpenSSH backend gets the same from ssh itself when `ControlMaster` is configured. | `0` to turn off. Default on. |
| `ECCE_SSH_FRONTEND_IDLE` | Seconds the shared front-end login is kept after its last session closed, so the next one does not log in again. | Seconds. Default `120`. |
| `ECCE_SSH_KEEPALIVE` | Seconds of silence before the libssh transport probes an ssh connection. On the job-monitor stream a probe goes out after that many quiet seconds and a host that has sent nothing for three times as long is declared dead: the stream ends and the job master restarts the monitor, instead of waiting for the three-minute heartbeat. The same value sets TCP keepalive on every libssh connection, which ends a blocked command on a link whose host has vanished (the per-command idle timeout stays the main guard). Needs Linux for the stream check; elsewhere only TCP keepalive applies. | Seconds; `0` turns it off. Default `30`. |

## Deployment and site

| Variable | What it does | Values / default |
|---|---|---|
| `ECCE_HOME` | Root of the installation. Required by every ECCE program; the launchers default it. | Path. Default `/opt/ecce` (the `ECCE_HOME_DIR` given at configure time). |
| `ECCE_REALUSERHOME` | Home directory whose `.ECCE` holds this user's settings, machine registrations and per-user service state. Required. Lets a second "user" be simulated on one machine. | Path. The launchers default it to `$HOME`. |
| `ECCE_REALUSER` | The user name ECCE runs as; names the temporary directory `ecce_<user>`. Required. | User name. The launchers default it to `id -un`. |
| `ECCE_DATA` | The data directory. | Path. Default `$ECCE_HOME/data`. |
| `ECCE_SYSDIR` | Sub-directory of `ECCE_HOME` that holds `bin`, for a multi-platform install. Used to find helper programs such as `genmol` and to detect the reaction-rate tools. | Directory name with trailing `/`. Default empty (`$ECCE_HOME/bin`). |
| `ECCE_TMPDIR` | Where job-monitoring temporary files go, in `ecce_<user>` under it. | Path. Default `/tmp`. |
| `ECCE_REMOTE_SERVER` | Central-server mode: read `siteconfig/RemoteServer/{DataServers,site_runtime}`, start no local broker or data server. `ecce -remote` sets it. | Set to enable. Default off. |
| `ECCE_LOCAL_DATA` | Local data mode (#216): keep projects and calculations in this folder instead of a data server; no data server is started. Wins over Edit > Preferences > Data folder. The launchers export it for the whole session, set to the preference's folder or to empty (data server). | Path. Unset: the preference (off by default; folder `~/.ECCE-local`). Empty: a data server. |
| `ECCE_NO_MESSAGING` | Run without the JMS broker. Apps do not publish or subscribe. The launchers set it for `ecce -admin` and `-machine`. | Set to enable. Default off. |
| `ECCE_NO_DATASERVER` | Launchers do not start the per-user data server. | Set to enable. Default off. |
| `ECCE_DATASERVER_PORT` | Port of the per-user data server, as used by `ecce-dataserver-start` and `-status`. The test suite uses it for private servers. | Port. Default 8096. |
| `ECCE_BROKER_PORT` | Port of the central broker under `ecce -remote`, as used by `ecce-gateway-start`. The per-user broker has no port. | Port. Default 8088. A server account's broker listens on it too. |
| `ECCE_DATASERVER_LISTEN` | Which addresses the data server and its broker listen on. Also read from `~/.ECCE/dataserver/listen`; the variable wins. | `loopback` (default; `localhost` is the same), `all` (also `*`, `0.0.0.0`, `::`), or a list of addresses added to loopback. |
| `ECCE_HELP` | Base URL of the help pages. Required by the help code. | URL. The launchers default it to `http://localhost:8096/`. |
| `ECCE_NWCHEM_DATA` | Directory holding NWChem's force-field data (`amber_s/amber.par`), referred to as `$ECCE_NWCHEM_DATA` in `siteconfig/DataServers`. A setting that does not hold that file is ignored with a note. | Path. The launchers fill it in with `ecce-nwchem-datadir`, which searches the usual locations. |
| `ECCE_HOST` | Host name that keys the per-session state files. | Default `$HOST`, else `hostname`. The launchers pass `$HOST` to the dispatcher. |
| `ECCE_AUTO_ACCOUNTS` | Data server creates web accounts automatically. **A key in `site_runtime`, not an environment variable.** | `yes`/`true` (any case) to enable. Users cannot override it. |
| `ECCE_STORE_TRAJECTORIES` | Users may store molecular-dynamics trajectories on the server. **A key in `site_runtime`, not an environment variable.** | `yes`/`true` (any case). `no` forbids it for everyone. |

### Job monitoring tuning

| Variable | What it does | Values / default |
|---|---|---|
| `ECCE_JOB_COMMS` | Obsolete. Job monitoring always runs over stdio; `socket` and `socketlocal` are accepted and treated as `stdio`, with a note in the job log. | `stdio` |
| `ECCE_JOB_MAXCONNECTS` | Total attempts to reconnect to a compute machine before job monitoring gives up and sets a failed state. | Integer. Default 25. |
| `ECCE_JOB_MAXQUICKCONNECTS` | Attempts within `ECCE_JOB_MAXQUICKTIME` before giving up early. | Integer. Default 5. |
| `ECCE_JOB_MAXQUICKTIME` | The time limit for those quick attempts. | Seconds. Default 60. |
| `ECCE_JOB_RESTARTRESET` | A job-monitoring run that lasted at least this long before failing resets the `ECCE_JOB_MAXCONNECTS` count, so only rapid failures use up the budget. Matters on login nodes that kill long-running processes. | Seconds. Default 600. |
| `ECCE_SUBMIT_TIMEOUT` | How long a job submission command may run. | Seconds. Default 120. |
| `ECCE_JOB_PARSE_TIMEOUT` | How long one perl parse script may run on the client. | Minutes. Default 5. |
| `ECCE_JOB_PARSE_IGNORE` | Parse descriptor blocks whose `Script` name occurs in this text are dropped when the descriptor is staged for a job. | Text containing script names. Default unset. |
| `ECCE_JOB_MESSAGE_BODY` | Size above which a property message body is treated as large. Exact effect not traced. | Bytes. Default 2048. |

## Troubleshooting and debugging

### Filing a bug: `ecce --bug`

`ecce --bug` (or `ECCE_BUG=1 ecce`; any value but empty or `0`) is one
switch for a bug report, so nobody has to set the variables below by hand.
It makes `~/ecce-bug-<time>/` (under `ECCE_REALUSERHOME`), exports it as
`ECCE_BUG_DIR`, and sets, for that session only:

- `ECCE_RCOM_LOGMODE`, `ECCE_JOB_LOGMODE=yes,yes` (keep the job logs),
  `ECCE_JOB_LOGALL`, `ECCE_AI_DEBUG=1`, `ECCE_DEBUG_PANELS`,
  `ECCE_DEBUG_GL_VISUAL`, `ECCE_DEBUG_MACHNOTICE`;
- `ECCE_DEBUG_MOSYM_LOG` and `ECCE_DEBUG_AUTHPARENT`, pointed at files in
  `ECCE_BUG_DIR`.

Left out on purpose: `ECCE_DAV_DEBUG` (always writes to `/tmp/davtraffic`
and includes the `Authorization` header, which holds the password),
`ECCE_LOG_ALL_EVENTS` and `ECCE_DEBUG_ATOM_COLOR` (they flood), and everything that changes behaviour (`ECCE_DEVELOPER`,
`ECCE_NO_MESSAGING`, `ECCE_EXIT_AFTER_DUMP`, `ECCE_DISABLE_RENDER_CACHE`,
`ECCE_OLD_RECEIVE`).

The session's own output goes to `session.log` in that folder and is
still shown in the terminal. When `ecce` returns, or on Ctrl-C, bug mode
adds what the broker, session relay and per-user data server logged
during the session (for a central server or a shared broker it notes that
those logs are on the server), the job logs of jobs touched during the
session, and the output of `ecce-diagnose`; then packs the folder as
`~/ecce-bug-<time>.zip` (`.tar.gz` if `zip` is missing) and prints its
name. The archive has no passwords but does hold host names, user names
and paths.

Output goes to stderr unless a file is named. Apps started from the
Organizer are detached, so their stderr does not reach the terminal that
started `ecce`; use the variables that take a file where there is one.

| Variable | What it does | Values |
|---|---|---|
| `ECCE_RCOM_LOGMODE` | Print what each connection and remote command is about to do to the console. | Set to enable. |
| `ECCE_JOB_LOGMODE` | Keep or delete job-monitoring log files. Two comma-separated modes, for `eccejobstore` and then `eccejobmonitor`. | `yes`, `rmifok` (keep unless the job was clean), `no`. `true` and `false` are accepted for the second. Default `rmifok`. |
| `ECCE_JOB_LOGALL` | Log every job-monitoring event in the data server's run log (Run Mgmt, View Run Log). | Set to enable. |
| `ECCE_LOG_ALL_EVENTS` | The same, for the property interpreter. | Set to enable. |
| `ECCE_DAV_DEBUG` | Log every WebDAV request and response header, appended to `/tmp/davtraffic` (a fixed name, shared by all users). The requests include the `Authorization` header, so the file holds your password in encoded form: delete it, and do not attach it to a report. | Set to enable. (`ECCE_DAV_DEBUG_BODY` appears only in commented-out code and does nothing.) |
| `ECCE_OLD_RECEIVE` | Receive from the data server with `MSG_WAITALL`, the older behaviour. The newer default fixed very slow transfers over NFS. | Set to enable. |
| `ECCE_DEVELOPER` | Developer mode: shows ECCE's internal files (`linkbase.xml`, `Parameters`, `Props`), the Organizer's Developer menu, the Builder's command line and Dump menu item, and logs XML parse errors. | Set to enable. |
| `ECCE_AI_DEBUG` | Input generators (`ai.<code>` in `scripts/parsers`) print one line per template tag on stderr saying what it resolved. | `1`. |
| `ECCE_DEBUG_GEOMTRACE` | Print every geometry-trace step and each Open Inventor render callback (issue #99). | Set to enable. |
| `ECCE_DEBUG_ATOM_COLOR` | Print every colour handed to the scene. | Set to enable. |
| `ECCE_DEBUG_MOSYM` | Print where the MO diagram's symmetry labelling gave up. | Set to enable. |
| `ECCE_DEBUG_MOSYM_LOG` | Write the same, plus the `[MOLOC]` lines, to this file; setting it alone turns the output on. | File path. |
| `ECCE_DEBUG_HALVES` | Trace the half-molecule analysis used for the MO diagram. | Set to enable. |
| `ECCE_DEBUG_PANELS` | Report a Builder property panel dropped because `isRelevant()` was false. | Set to enable. |
| `ECCE_DEBUG_PANEL_SIZE` | Print what each Builder panel asked for and got. | Set to enable. |
| `ECCE_UNIFORM_PANEL_HEIGHT` | Give every Builder panel the same height, the old behaviour. | Set and not `0`. |
| `ECCE_DEBUG_GL_VISUAL` | Report once which OpenGL visual the canvas received. | Set to enable. |
| `ECCE_GL_DEFAULT_VISUAL` | Take whatever OpenGL visual the platform offers instead of ECCE's requested one. | Set and not `0`. |
| `ECCE_DISABLE_RENDER_CACHE` | Turn off Open Inventor render caching. | Set to enable. |
| `ECCE_DEBUG_MATERIAL` | Log OpenGL material state to this file (issue #83). | File path. |
| `ECCE_DEBUG_MATERIAL_HEAVY` | With `ECCE_DEBUG_MATERIAL`, reopen the file on every call, for A/B tests. | Set to enable. |
| `ECCE_DEBUG_MACHREGISTER_SIZE` | Print the Machine Registration window's size calculation. | Set to enable. |
| `ECCE_DEBUG_MACHNOTICE` | Print whether the Organizer's "no machines registered" notice is shown. | Set to enable. |
| `ECCE_DEBUG_AUTHPARENT` | Log each authentication prompt and its parent window to this file. | File path. |
| `ECCE_NWDIRDY_DEBUG` | The reaction-rate editor prints its general theory data. | Set to enable. |
| `ECCE_BUG` | Same as `ecce --bug`: see above. | Set and not `0`. |
| `ECCE_BUG_DIR` | Set by bug mode to the folder collecting the report. | Do not set. |
| `ECCE_NO_REAP` | `ecce-gateway-reap` does nothing; whoever sets it stops the services itself. Used by the test suite. | Set to enable. |
| `ECCE_REAP_QUIET` | `ecce-gateway-reap` prints nothing. The launchers set it on exit. | Set to enable. |
| `ECCE_GATEWAY_LOCKED` | Internal: `ecce-gateway-start` tells the reaper it already holds the lock. | Do not set. |
| `ECCE_INVOKE_VIEWER`, `ECCE_INVOKE_FROMECCE` | Internal: `ecce-viewer` sets both so the Builder starts as the Viewer, and `ECCE_INVOKE_FROMECCE` marks a start from inside ECCE. | Do not set. |

### For automated capture and tests

| Variable | Used by |
|---|---|
| `ECCE_OPEN_PANEL=<name>` | Builder opens the named property panel once its calculation loads. |
| `ECCE_MODIAGRAM_DUMP=<path>` | Builder writes a text record of the finished MO diagram model. |
| `ECCE_EXIT_AFTER_DUMP`, `ECCE_EXIT_AFTER_DUMP_DELAY_MS` | Builder closes after that dump, optionally after a delay in milliseconds. |
| `ECCE_TEST_HOME`, `ECCE_TEST_BUILD`, `ECCE_TEST_SYMOPS`, `ECCE_TEST_STATE`, `ECCE_TEST_WRAPPERS`, `ECCE_TEST_SHELL_DIRS`, `ECCE_TEST_SHARED_BROKER_PORT` | Where the suites under `tests/` find the install, build tree, `symops`, state directory, launchers, shell fixtures and a test broker port. |
| `ECCE_XVFB`, `ECCE_APPS_BUDGET`, `ECCE_APPS_TRACE`, `ECCE_DIALOG_TESTS_USE_DISPLAY`, `ECCE_E2E_REQUIRE`, `ECCE_HARNESS_SCRIPT`, `ECCE_HARNESS_OUT`, `ECCE_GATEWAY_START`, `ECCE_DATASERVER_START` | Options of the GUI, dialog and end-to-end suites; see the README beside each. |

## Build-time settings that look like variables

These are CMake options, not environment variables: `ECCE_HOME_DIR`
(install root, default `/opt/ecce`), `ECCE_WRAPPER_DESTINATION` (where the
`ecce-<app>` launchers go, default `/usr/bin`), `ECCE_SPLIT_PACKAGES`
(separate client and server packages), `ECCE_MONOLITHIC_BREAK_VERSION`.

## Variables ECCE reads that are not named `ECCE_*`

| Variable | Effect |
|---|---|
| `VISUAL`, `EDITOR` | Editor when `ECCE_EDITOR` and the Preferences setting are empty. |
| `HOST`, `DISPLAY` | The launchers default them (`hostname`, `:0`); per-session state files are keyed by both. |
| `GDK_BACKEND` | The launchers set it to `x11` under a Wayland session unless already set, to avoid mis-sized windows. |

## Keys in `site_runtime` that nothing in this build reads

`ECCE_MESA_OPENGL` and `ECCE_MESA_EXCEPT` are read only by the old csh
launcher (`scripts/ecce_env`). `ECCE_SUPPORT` appears only in code that is
compiled out.
