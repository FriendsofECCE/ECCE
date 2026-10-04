---
type: pitfall
title: "`ai.<code>`'s template engine silently swallows `die()`"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["mopac.tpl"]
issues: [92]
---
**`ai.<code>`'s template engine silently swallows `die()`** (#92).
`modifyInputFile` resolves every `##tag##` through `eval "&$subname"`,
so a `die` inside a resolver — or anything it calls — is caught, the
tag's line vanishes, and the script **exits 0 with a quietly
incomplete input deck**. All 10 `ai.*` scripts share this engine and
nine contain `die`s. Confirmed reachable in `ai.mopac`:
`mopac.tpl`'s `##MOPACSecondJob##` → `MOPACSecondJob` →
`HamiltonianKeyword`'s "unsupported theory" `die`, so MOPAC's
reduced-scope guards cannot report anything and emit a deck missing
its keyword line instead. Put validation in the **main flow** before
generation starts, never inside a resolver — `ai.qe`'s
`&verifyPeriodic` is the pattern. **FIXED for all ten generators in
`a6d0512`**: each `eval` site now re-raises a real error while leaving
"Undefined subroutine" silent, so a resolver's `die` reaches the user.
Expect it to surface errors that were previously invisible.
