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

**`-remote`:** the site files are the copies `ecce-remote-setup` fetched into
the client's `siteconfig/`; the user file merges over them exactly as in local
mode. `ecce-dataserver-start` publishes `submit.site` and `QueueManagers` too;
a client of an older server simply keeps its own.

**The invariant is tested**: `tests/queues/config_test.py` runs
`build/configdump` (C++) and `GENSUB_DUMP_CONFIG=1 gensub` on one site+user
pair and requires identical effective values. Change the grammar in one reader
and the test fails until the other follows.

**Writer side** (`scripts/processmachine`): the GUI owns only the code paths,
`perlPath` and `qmgrPath` in CONFIG.<m> (removed case-insensitively, outside
blocks) and the five queue fields in `<m>.Q`; everything else is kept, and the
file is deleted only when nothing but comments is left. Queues' `<m>|queueMgrName`
is rewritten when the manager changes.
