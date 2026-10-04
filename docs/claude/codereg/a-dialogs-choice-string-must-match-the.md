---
type: pitfall
title: "A dialog's choice string must match the generator's expected string EXACTLY, or selecting it silently emits nothing"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["ged{03,09,16}theory.py", "scripts/codereg/*theory.py"]
---
**A dialog's choice string must match the generator's expected string
EXACTLY, or selecting it silently emits nothing.** The
`scripts/codereg/*theory.py` combo lists and the `ai.<code>` subs that
translate them (`$xcFun eq "..."`, `$exchFun eq "..."`) are two
hand-maintained copies of the same list, and every `ai.<code>`
translation sub ends in an `else { $result = ""; }` — so a mismatch
produces an empty keyword, the `##token##` line is deleted, and the
job runs *without* the functional the user picked. No error anywhere.
Found **nine times in one afternoon** (2026-09-21) across
`ged{03,09,16}theory.py`: letter `O` for zero (`MO6HF` vs `M06HF`), a
missing closing paren (`"MPW1PBE (hybrid"`), wrong case (`MPW3PBE` vs
`MPw3PBE`), a stray space (`"Becke 89"` vs `Becke89`), a dialog
offering a functional with no generator case at all (`M11`), and
ged03 offering `BP86 (hybrid)`/`M06L (nonlocal)` where its generator
wants `BP86 (GGA)`/`M06L (meta-GGA)`.
**Also check for missing commas in the Python list**: adjacent string
literals are implicitly concatenated, so
`"N12SX (range)" "MN12SX (range)"` silently became one nonsense
dropdown entry in all three dialogs while both real functionals
became unselectable.
Detect mechanically rather than by eye — extract the dialog's list and
the generator's `eq "..."` cases and diff the two sets, checking both
directions (offered-but-unmapped is a dead control; mapped-but-unoffered
is a capability the UI can't reach, which is how Gaussian's three
double hybrids stayed hidden). Watch out that `xcFuncDefault` is an
**index** into the list, so append rather than insert, and verify the
defaults still resolve to the same entry afterwards.
