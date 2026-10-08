---
type: pitfall
title: "A resolver that returns \"\" without setting `$_` leaves its `##tag##` in the deck"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/parsers/ai.qe", "scripts/parsers/qe.tpl", "tests/apps/qe_walkthrough_test.py"]
issues: []
---
**A `##tag##` resolver that wants to emit nothing must set `$_ = ""` itself.**
`modifyInputFile` only overwrites the line when the resolver returns a
non-empty string; on `""` the line still holds `##QEIons##` and is written
to the deck. `ai.qe`'s `QEIons` and `QECell` returned `""` for an scf run
without clearing `$_`, so every Quantum ESPRESSO energy deck had two literal
`##QEIons##`/`##QECell##` lines (pw.x ignores unknown cards with a warning,
which is why nobody noticed). Found by reading the deck the Calculation
Editor stored in `tests/apps/qe_walkthrough_test.py`; set `ECCE_AI_DEBUG=1`
for the "returned EMPTY and nothing replaced the tag" line.

Same walkthrough: pw.x's own default `ecutrho = 4 * ecutwfc` is too low for the
ultrasoft/PAW files of the SSSP set (they want 8x), and the energy depends on
it (0.5 mRy for the 8-atom silicon cell: -91.35634 Ry at 120 Ry, -91.35686 Ry at 240 Ry). `ai.qe` writes `ecutrho = 8 * ecutwfc` when the
"Charge Density Cutoff" box is unticked and one of the chosen files is
ultrasoft or PAW.
