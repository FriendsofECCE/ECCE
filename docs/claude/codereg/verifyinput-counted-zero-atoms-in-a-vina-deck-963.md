---
type: pitfall
title: "verifyinput judged a Vina deck by the generic atom count, which is always 0 (#963)"
area: codereg
paths:
  - scripts/parsers/verifyinput
  - tests/dock/vina_autoligand_test.py
issues: [963]
---

The `$code ne "Vina"` guard only covered the "no geometry found" branch; the
Calculation Editor also passes `-a <atoms>`, and that branch compared the 0
atoms found in the deck with the calculation's count, so every Vina deck was
BAD. For `Vina` the generic geometry check is now skipped (the structure is
the auxiliary input `vina.vinafrag`), and `vina()` checks that the
`#| LIGAND_ATOMS=` and `#| RECEPTOR_ATOMS=` lists together cover atoms
1..N exactly once (overlaps, gaps, numbers beyond N). Other codes are
unchanged.
