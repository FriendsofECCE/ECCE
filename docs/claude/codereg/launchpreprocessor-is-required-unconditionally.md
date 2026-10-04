---
type: checklist
title: "`<LaunchPreprocessor>` is required, unconditionally"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
---
**`<LaunchPreprocessor>` is required, unconditionally** —
`Launch::postProcessInput()` shells out to `JCode::launchPPScript()`
with no empty-string check, so a code without one fails launch with
a bare `-p postParams` (empty command name) "command not found",
which reads like a shell/PATH problem but isn't. Even a trivial
script is needed if there's nothing to post-process. This script,
not the input generator, is also the *correct* place for genuinely
launch-time-only info the input generator can't know at edit time
(processor count, scratch dir path) — it receives a `postParams`
dictionary with the real values (see `nwchem.launchpp` for the
pattern: read `-p <paramfile>`, rewrite the already-generated input
file in place, idempotently).
