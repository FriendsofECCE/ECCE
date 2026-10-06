# ECCE — context for Claude Code sessions in this repo

`FriendsofECCE/ECCE`, single active branch `main`. This is a
**modernization**, not a rewrite: porting a ~1200-file legacy scientific
C++/wxWidgets desktop app (wx 2.8 → 3.2, C++14 → 17, `build_ecce`/
recursive-make → CMake/CPack, Python 2 → 3) to build and run on current
Debian, keeping old behavior as the correctness anchor. This file is a
lean map of where things live and what to watch out for — it is
intentionally *not* a running log of past sessions. For that, see
"Where the history lives" at the bottom.

Once a design decision is settled, delegate the implementation to the
sonnet-implementer subagent rather than writing the files yourself, unless
the edit is trivial (a few lines in one file, no investigation needed):
make those directly, since a fresh agent re-reads the context and costs
several times the tokens of the change itself (Andy, 2026-09-29). Use
agents for investigations, multi-file work and anything that runs in
parallel. Keep doing the design work, review, and any decision the
subagent flags, directly.

## Standing preferences
- **Comments: a few lines, saying WHY — never the history.** How a bug
  was found, what was tried first and which versions failed go in the
  commit message, not the source. Long essay comments went stale and
  wrong in this tree (a "the OP is on no fixed scale" comment outlived
  the fix that disproved it). Agreed with Andy 2026-09-27.
- Andy runs Debian ("trixie") on his machines — **beryllium** and
  **niobium**. Default to Debian conventions, not Ubuntu, for anything
  environment/package related, unless told otherwise.
- Releases are tagged on `main`, which is the **9.x line** (9.0.0-dev
  from 2026-10-01; the breaking changes — split packages, sh job
  scripts, no site-defined shells, "killed" only on a user cancel —
  made it 9.0.0). **8.x gets bug fixes only** (Andy, 2026-10-01): each
  8.x patch release goes on its own `release/X.Y.Z` branch off the
  previous tag, with the fix cherry-picked from `main` (`-x`), as
  8.18.1 and 8.18.2 were. Before tagging, run `tests/teaching` on the
  release branch; after tagging, fast-forward `stable-8` to the new
  tag and push it, since it is what 8.x users build from. The `v9` branch was merged into `main` and retired
  (`archive/v9`); work that needs isolation goes on short-lived `wip/*`
  branches. Old branches (`develop`, `modernize-build`, `stable`,
  `master`, `make`) were consolidated into `main` and renamed to
  `archive/*` — no reason to branch from or compare against them.
- Build directory is `build-cmake` (not `cmake-build`) — `ninja` or
  `cmake --build .` from inside it.
- **The Gateway window no longer appears** (#93): `ecce` opens the
  Organizer directly; the gateway process still owns the session.
  `ECCE_GATEWAY_WINDOW=1 ecce` brings the window back. Session-end and
  broker-lifetime rules per deployment mode: `docs/claude/services/`.
- `GETTING_STARTED.md` (repo root) has the full build/package/install/
  first-login walkthrough. Don't reproduce it here.
- **The central-server deployment is unconditional** (stated 2026-09-25).
  ECCE must keep supporting a data server + broker on one machine with
  students connected to it as clients — this is the teaching use case
  and it is how the original PNNL deployment worked. Much of this fork's
  new work localises services *per user* (per-user Apache, per-user
  Mosquitto, loopback-only `Listen`, per-session state files keyed by `ECCE_SESSION_ID` (#233)), and
  that direction is fine only for as long as the two-machine path keeps
  working. Treat `ECCE_REMOTE_SERVER`/`-remote` and
  `siteconfig/RemoteServer/` as first-class rather than a fallback, and
  before changing the data server, the broker, `Listen` or service
  startup, ask explicitly what it does to a central install. #138 is
  the live instance. There are three deployment modes (GETTING_STARTED,
  "Deployment modes"): local, central server (`-remote`), and a shared
  system broker on an app server (`siteconfig/SharedBroker`, #191).
  `tests/apps/session_end.py` covers all three, but on one machine as
  one Unix user (a second "user" is a second `ECCE_REALUSERHOME`). None
  has been tested with real separate accounts or two machines, and
  `ecce-broker.service` has never run under the system manager.
- **Memory settings should be entered/labeled in GB everywhere, for
  every code** — Andy's explicit UX preference (2026-09-07), not each
  code's native convention. The wire format still has to match what
  each code's input file actually expects (words for GAMESS-UK, MB for
  ORCA's `%maxcore`, a `gb`/`GB` suffix NWChem and Gaussian both accept
  natively) — convert at the point closest to the generated input
  (`ai.<code>`/the `.tpl`), not by changing what the wire format itself
  accepts. See `scripts/codereg/{ged*,nedtheory,orcatheory,guktheory,
  metathry}.py` for the pattern. (#77 history: `docs/claude/codereg/`.)

## Knowledge bundle — read what applies before changing code
Detailed maps and every pitfall found so far live in `docs/claude/`, an
[Open Knowledge Format](https://github.com/GoogleCloudPlatform/knowledge-catalog/tree/main/okf)
(v0.1) bundle: one Markdown file per fact, YAML frontmatter (`type`:
map/pitfall/checklist/rule, `title`, `area`, `paths`, `issues`), an
`index.md` per area. Most entries describe silent failures that are not
discoverable from the code, so **before changing a file, run
`grep -rl '<file or directory name>' docs/claude` and read the hits**;
browse `docs/claude/<area>/index.md` for an overview.
- `codereg/` — code registration (EDML, `scripts/codereg`,
  `scripts/parsers` `ai.*`/`*.expt`/`*.desc`, basis writers, gensub,
  eccejobmonitor, how properties reach the Properties menu), the
  new-code checklist (ORCA, MOPAC), the input checker (#148).
- `services/` — gateway, broker, per-user Apache data server, sessions
  and session end, session state keyed by `ECCE_SESSION_ID` (#233; `$DISPLAY` only draws windows).
- `mo-diagram/` — the MO correlation diagram (#132).
- `wx-viewer/` — wx3.2/GTK3 pitfalls, the Open Inventor viewer's
  redraw, C++ pitfalls (iterator invalidation, format strings).
New findings go in as new entry files (frontmatter as above), not as
paragraphs here.

## GUI application layer
- `src/apps/*` — one directory per top-level app (`builder`, `organizer`,
  `calced`, `gateway`, `machregister`, `machbrowser`, `basistool`, ...).
- `src/wxgui/` — shared wx widget classes/toolkit used across apps.
- `src/wxviz/`, `src/inv/` — the 3D molecular viewer, Open Inventor API
  (`SoWxRenderArea`/`SoWxExaminerViewer`/scene graph). Since 9.0.0-alpha.4
  it is built against the distribution's Coin3D by default; `src/inv/moiv`
  (chemistry nodes), `wxinv` (our wx binding) and `flclient` (fonts) are
  ours either way, the vendored SGI core only with `-DECCE_USE_COIN=OFF`
  (#166). The broker is Mosquitto (#213), not ActiveMQ.
- `src/tdat/` — calculation/resource data model. Notably
  `src/tdat/properties/Prop*.C` — a family of copy-pasted-from-template
  classes (`PropTable`, `PropVector`, `PropTSVecTable`, ...) holding
  parsed results (MOs, energies, geometry traces). They share the same
  bounds-checked `value()` accessor shape closely enough that a bug found
  in one is worth grepping the siblings for.
- `src/dsm/`, `src/comm/`, `src/util/` — chemistry/XML data model, remote
  job/comm layer, general utilities. No wx dependency.

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
