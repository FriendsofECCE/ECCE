---
type: pitfall
title: "Retired codes are not maintained, and the suite no longer checks them"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: [".edml", "QuantumESPRESSO.edml", "tests/dialogs/cases.py"]
issues: []
---
**Retired codes are not maintained, and the suite no longer checks
them.** `tests/dialogs/cases.py`'s `RETIRED` (Gaussian-03,
Gaussian-98, GAMESS-UK) is skipped unless named with `--code`. DirDyVTST,
Polyrate and Amica are not in it: they were removed with their `.edml`
(#873), and stored calculations of them list read-only in the Organizer.
It is *not* the same as `NOT_IN_MENU`, which only means "absent from
the New Calculation menu" — GROMACS is in that one and
is maintained. **MetaDyn is not retired**: its `.edml` declares
`codeName="NWChem"`, so it is NWChem's plane-wave metadynamics front
end, and it is what `QuantumESPRESSO.edml` was modelled on.
