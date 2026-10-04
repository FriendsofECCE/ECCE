---
type: pitfall
title: "Retired codes are not maintained, and the suite no longer checks them"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: [".edml", "QuantumESPRESSO.edml", "tests/dialogs/cases.py"]
---
**Retired codes are not maintained, and the suite no longer checks
them.** `tests/dialogs/cases.py`'s `RETIRED` (Gaussian-03,
Gaussian-98, GAMESS-UK, Amica) is skipped unless named with `--code`.
It is *not* the same as `NOT_IN_MENU`, which only means "absent from
the New Calculation menu" — Polyrate and GROMACS are in that one and
are maintained. **MetaDyn is not retired**: its `.edml` declares
`codeName="NWChem"`, so it is NWChem's plane-wave metadynamics front
end, and it is what `QuantumESPRESSO.edml` was modelled on.
