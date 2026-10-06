---
type: map
title: "`ConfigFile` reads and writes `CONFIG.<m>`; `MachineConfigDraft` is what Register Machines edits"
area: codereg
section: "Machine configuration (CONFIG files)"
paths: ["include/tdat/ConfigFile.H", "src/tdat/resources/ConfigFile.C", "include/tdat/MachineConfigDraft.H", "src/tdat/resources/MachineConfigDraft.C", "include/util/MiniJson.H", "tests/queues/configedit.C", "tests/queues/configedit_test.py"]
issues: [212]
---
**`ConfigFile`** is the one C++ implementation of the CONFIG line grammar:
`RefMachine::config()` merges files through `ConfigFile::mergeFile`, and the
writer edits through the same parse, so reader and writer cannot drift (gensub
stays the Perl twin; `config_test.py` and `configedit_test.py` hold them
together). It keeps every line, so `load` + `save` is byte-identical, and
`set`/`remove`/`clear` touch only the key's own lines.

Things that are easy to get wrong:

- `set()` refuses `""` and `-` (use `remove()`/`clear()`), values with leading
  or trailing blanks (the readers trim them, so the value would not round-trip)
  and block bodies with `}` in column 0. A single-line value containing `{` is
  written as a block, because the readers split `key: a{b}` as a one-line block.
- `save()` writes a temporary file beside the target, `fsync`s and renames
  (through a symlink), leaves the owner able to write, except that a site
  file (`setSiteFile(true)`, admin mode) that was read-only when loaded is
  locked again, as `processmachine` does; user files stay writable. It
  **deletes the file when only comments and blank
  lines remain after an edit**, as `processmachine` does.
- Warnings (unclosed block, `}` not in column 0, a key with no value) are
  reported, never fatal: the readers skip them silently.

**`MachineConfigDraft`** holds, per lower-case key, the layers below the edited
file (from `GENSUB_EXPLAIN` JSON, parsed by `MiniJson`) plus this session's
edit (`Inherit`/`Set`/`Clear`). `effective()`, `tag()` and `isDirty()` are pure
functions of that; `applyTo(ConfigFile&)` writes only the keys that changed
since load. `effective(key, cppOnly)` drops `submit.site`/vendor layers, which
only gensub reads, so it can be compared with `RefMachine::config()` for the
twelve C++ keys. In admin mode layers from the user file are dropped.
`explain` reports a cleared key by its file only (`source: "cleared"`); the
layer's source is recovered from the draft's site/user paths.

`tests/queues/configedit` is the command-line front end and
`configedit_test.py` (ctest `queues_configedit`) the three-way check: the
draft's prediction made before a write, gensub's explain after it, and
`configdump` (C++) must agree.
