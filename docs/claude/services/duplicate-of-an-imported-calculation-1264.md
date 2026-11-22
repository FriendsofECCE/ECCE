---
type: pitfall
title: Duplicate of an imported calculation (a stored example) has no deck; readiness comes from DeckWriter::setupComplete
area: services
paths: [src/apps/organizer/CalcMgr.C, src/dsm/edsiimpl/TaskJob.C, src/dsm/edsiimpl/DeckWriter.C, src/apps/calced/CalcEd.C, tests/apps/organizer_duplicate_test.py]
issues: [1264]
---
`TaskJob::resetForRerun()` (no target state) maps Loaded (imported) and
Created to Created and everything else to Ready, because only a calculation
that went through the editor has an input deck. The stored examples are
imported runs with no `Inputs/` folder at all, so their copies came out
Created although their settings were complete.

`CalcMgr::duplicateForRerun` now calls `DeckWriter::makeReadyIfComplete` on a
copy left Created: it applies `DeckWriter::setupComplete` (the editor's own
`isReady` rule, which `CalcEd::isReady` also calls) to the stored structure,
basis and theory, creates `Inputs/` when missing and writes the deck. An
incomplete copy stays Created. Change the readiness rule in
`setupComplete`, not in one of its callers.
