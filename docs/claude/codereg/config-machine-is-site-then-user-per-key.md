---
type: rule
title: "`CONFIG.<machine>` is the site file, then the user file, merged per key (C++ and gensub alike)"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["src/tdat/resources/RefMachine.C", "include/tdat/RefMachine.H", "scripts/gensub", "scripts/processmachine", "tests/queues/config_test.py", "tests/queues/configdump.C", "siteconfig/submit.site", "siteconfig/CONFIG.dummy"]
issues: []
---
**One rule, two readers.** `RefMachine::config(refname)` (C++: `shell`,
`sourceFile`, `perlPath`/`qmgrPath`/`xappsPath`, `libPath`, `frontendMachine`,
`frontendBypass`, `singleConnect`, `checkScratch`, `noRemoteAccess`,
`userSubmit`, `exePath`) and `readConfig` in `scripts/gensub` (job-script keys)
both read `$ECCE_HOME/siteconfig/CONFIG.<m>` first, then
`~/.ECCE/CONFIG.<m>` (`Ecce::realUserPrefPath()`), and per key:

- keys are case-insensitive (`Shell:` = `shell:`);
- within one file the last duplicate wins;
- the last **non-empty** value wins, so `key:` with nothing after it does
  not override and cannot clear;
- the value `-` (`key: -`, `key - `, `key { - }`) **clears** the key: it
  removes what an earlier file set and is never a value. It never reaches a
  job script, because gensub deletes the key (the built-in default then
  applies).

gensub additionally layers `submit.site` and `CONFIG.<VENDOR>[.<MODEL>[.<PROC>]]`
below the machine files; C++ reads only the machine files (and the vendor
files for `exePath`'s fallback).

**Line grammar** (same in both): `key: value`, `key value`, `key { value }`
and multi-line `key {` ... `}` (closing brace in column 0). A line is colon
form only when the first word is followed by `:`; otherwise `key value:with:colons`
is space form. No trailing `# comment` stripping any more (the C++ reader
used to strip it, gensub never did). Blocks are replaced whole, never appended.

**Layers (#192):** the readers do not build `$ECCE_HOME/siteconfig/<name>`
themselves but ask `Ecce::siteConfigLayers(machine)` (`siteConfigFile`,
`userRegistrationDir`, `siteConfigDirs`): `[user, server site, install]` when
`-remote` has a cache under `~/.ECCE/server/<host>_<port>/` (`site/`,
`user-<login>/`), else `[~/.ECCE, install]`. A machine comes whole from one
site layer (the highest whose `Machines` lists it; its `CONFIG.<m>` is never
merged with a lower layer's), the user's `CONFIG.<m>` merges per key on top.
`localhost` is always the client's: server layers are dropped for it.
gensub gets the same list as `ECCE_SITECONFIG_DIRS` (set by `Launch.C`).
Tests: `queues_config` (three layers, C++ against gensub), `queues_layers`.
Before a cache exists (stage 3 of #192 writes it) nothing differs from local mode.

**The invariant is tested**: `tests/queues/config_test.py` runs
`build/configdump` (C++) and `GENSUB_DUMP_CONFIG=1 gensub` on one site+user
pair and requires identical effective values. Change the grammar in one reader
and the test fails until the other follows.

**Writer side** (`scripts/processmachine`): the GUI owns only the code paths,
`perlPath` and `qmgrPath` in CONFIG.<m> (removed case-insensitively, outside
blocks) and the five queue fields in `<m>.Q`; everything else is kept, and the
file is deleted only when nothing but comments is left. Queues' `<m>|queueMgrName`
is rewritten when the manager changes.
