---
type: pitfall
title: "Verify a code's keyword list by running the code, not by reading its manual"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["nedtheory.py"]
---
**Verify a code's keyword list by running the code, not by reading its
manual.** Sweeping all 55 functionals `nedtheory.py` offers through
NWChem 7.2.3 found three that abort the job every time (CAM-B3LYP and
LC-wPBE put `cam` on the `xc` line, where it is a directive of its
own; plain `hcth147` is deprecated and fatal, it wants
`hcth147@tz2p`). The same sweep showed the documented spellings for
dispersion (`disp grimme3`, a trailing `bj`) are rejected outright —
only `disp vdw <1..4>` works — and that D3/D3BJ with a functional
lacking parameters is **fatal rather than ignored**, which is why
`ai.nwchem` validates the pairing in the main flow. Where a support
table like that is needed, keep it in the generator alone and let it
report; putting a copy in the dialog recreates the
two-hand-maintained-lists bug this file already warns about twice.
