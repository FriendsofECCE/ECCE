---
type: pitfall
title: "NWChem aborts on three offered functionals and rejects its documented dispersion spellings"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/codereg/nedtheory.py", "scripts/parsers/ai.nwchem"]
issues: []
---
Found by running every functional `nedtheory.py` offers through NWChem
7.2.3 ([verify by running](verify-by-running-the-code.md)). Three abort the
job: CAM-B3LYP and LC-wPBE put `cam` on the `xc` line, where it is a
directive of its own; plain `hcth147` is deprecated and fatal, it wants
`hcth147@tz2p`. For dispersion, `disp grimme3` and a trailing `bj` are
rejected; only `disp vdw <1..4>` works, and D3/D3BJ with a functional
lacking parameters is **fatal rather than ignored**, which is why
`ai.nwchem` validates the pairing in the main flow. Keep such a support
table in the generator alone and let it report; a copy in the dialog
recreates the [two-hand-maintained-lists bug](a-dialogs-choice-string-must-match-the.md).
