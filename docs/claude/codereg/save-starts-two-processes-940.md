---
type: rule
title: "A Save starts at most two shell commands: do file work in process (#940)"
area: codereg
paths: [src/dsm/edsiimpl/DeckWriter.C, src/dsm/chemistry/TGBSConfig.C, src/apps/calced/InputVerifier.C, src/apps/calced/CalcEd.C, tests/apps/deck_regen_test.py]
issues: [940]
---
On Windows every process start goes through MSYS `sh.exe` and the virus
scanner, and costs a large fraction of a second; a Save that started 21
processes (11 RCommand commands in `DeckWriter::generate`, 2 for the input
checker, plus their children) took 7-9 s there while it took 0.35 s on Linux.

`DeckWriter::generate` now checks and copies files in process (`SFile`,
`copyTemplate`, `prettyCopy` for `scripts/parsers/prettyInput`, a directory
scan for the auxiliary inputs) and has no `localconn.cd`: the generator
inherits the working directory set by `changeWD`. The only RCommand commands
left are `ai.<code>` and `verifyinput`. `std2<code>` (not an RCommand) is
spawned directly with `posix_spawnp` in `TGBSConfig.C`, without `sh -c`;
on Windows it still goes through `cmd.exe` + `perl`.

- `prettyCopy` reads and writes in text mode, as Strawberry Perl did on
  Windows, so a Windows deck keeps CRLF line ends.
- `apps_deck_regen` counts the `Command (` lines of `ECCE_RCOM_LOGMODE`
  during each Save and fails above `SAVE_COMMANDS` (2), besides comparing
  the decks byte for byte.
- A Save still slower than 0.5 s shows a "Saving" progress dialog at its
  next step (`DeckWriter::Options::stage`, `CalcEd::p_saveStage`).
- The controllers directory is put on PATH with `;` on Windows; a `:` glued
  it onto the first PATH entry there.
- On Windows the narrow paths in these helpers are UTF-8 only because the
  executables carry the UTF-8 `activeCodePage` manifest
  (`packaging/windows/ecce-utf8*.manifest`).
