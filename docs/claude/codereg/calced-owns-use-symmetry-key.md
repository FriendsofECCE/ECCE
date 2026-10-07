---
type: pitfall
title: "CalcEd's own ES.Theory.UseSymmetry key is lost whenever p_GUIValues is replaced"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["src/apps/calced/CalcEd.C", "scripts/parsers/ai.gauss16", "scripts/parsers/ai.gauss09", "scripts/parsers/ai.nwchem", "scripts/parsers/ai.orca"]
issues: [145]
---
**The "Use symmetry" tick lives on the Calculation Editor's page, but its
value lives in `p_GUIValues` beside the details dialogs' keys.** Every
place that replaces `p_GUIValues` with a fresh `GUIValues` (a theory
change via `resetTheoryDetails()`, the no-molecule branch of
`refreshChemSysFields()`) drops it, while the box stays ticked. The
generators read an absent key as "off": Gaussian writes `NoSymm`, NWChem
`noautosym`, ORCA no `UseSym`. So a user who ticked the box and chose a
theory got NoSymm (9.0.0-alpha.6 report).

The rule now: the key is put back from the box after every reset, and
`doSave()` stores the box's value just before the input is generated.
Do not "fix" this by making the generators default to on: MetaDyn and
other editors drive `ai.nwchem` without the key and rely on `noautosym`.

The same key also made `p_GUIValues->size() == 0` (the test for "the
details dialogs have not supplied their defaults yet") never true for a
calculation with a molecule; use `hasDetailsValues()`, which ignores it.

Tests: `tests/symmetry/run_tests.py` (`checkUseSymmetryRule`, every
generator against the rule) and `tests/apps/session_end.py local-usesym`
(the real CalcEd, driven through `ECCE_TEST_CALCED`, checking the .param
and the route line after theory changes, Theory Details, reopening).
