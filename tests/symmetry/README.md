# tests/symmetry — are the character tables right?

```
tests/symmetry/run_tests.py       check every table
tests/symmetry/run_tests.py -v    show each check
```

Exit 0 if every table is sound, 1 otherwise. No build, no display, no
chemistry codes.

## Why

`data/client/config/CharacterTables` is a hand-entered grid of small
integers — exactly the kind of data a transcription slip survives in. A
wrong sign in one cell gives a table that still *looks* like a character
table and silently misassigns one irrep, which would then propagate into
every MO symmetry label built on it (#132).

It does not have to be checked by eye. The great orthogonality theorem
makes the data self-verifying.

## What is checked

| | |
|---|---|
| class counts sum to the group order | catches a missing or duplicated class |
| squared dimensions sum to the group order | catches a wrong dimension |
| every irrep's own norm equals the order | catches most single-cell errors |
| distinct irreps are orthogonal | catches the rest |
| distinct classes are orthogonal, normalised | independent of the row checks |
| as many irreps as classes | catches a whole row left out |
| **(x, y, z) reduces to the recorded irreps** | catches a row **swap** |

## The one orthogonality cannot catch

Swapping two whole rows leaves **every** orthogonality relation intact —
it is a relabelling, so the table stays perfectly self-consistent while
two irreps wear each other's names. That was confirmed, not assumed: the
Td T₁/T₂ swap passed the orthogonality checks cleanly.

The fix is to tie a label to something physical. The character of the
3-D Cartesian representation follows from the class *names* alone —
1+2cos θ for a proper rotation, −1+2cos θ for an improper one, 1 for a
mirror, −3 for inversion — so reducing it and comparing against the
`translations:` line is an independent constraint. In Td it must come
out T₂, in Oh T₁ᵤ, in D3h E′+A₂″.

Every check here was self-tested by introducing the error and watching
it fail: a flipped sign, a row swap, a renamed irrep, a g/u mislabel.

## The C++ loader

`CharacterTable` reads the same file and provides the reduction formula,
`n_i = (1/h) Σ_c n_c χ_i(c) χ(c)`. It is checked separately, because the
data being sound says nothing about the parser: one that drops a row, or
goes off by one between `classes` and `counts`, leaves a table that is
still internally consistent and simply describes a different group.

It is checked against the same oracle — what (x, y, z) spans, computed
from the class names — so the check does not come from the numbers being
read.

The parsing deliberately has no ECCE runtime dependency (`loadFile()`
takes a path; only the convenience `lookup()` consults `$ECCE_HOME`), so
the test compiles with plain g++ and needs no build tree.

A reduction that comes out negative or non-integral is **refused, not
rounded**. That case means the characters handed in are not a
representation of the group, and the likeliest cause is a wrong frame —
rounding would turn that into a plausible answer.

## Scope

Thirteen groups: C1, Cs, Ci, C2, C2v, C3v, C4v, C2h, D2h, D3h, D4h, Td,
Oh — enough for AH_n and ML_n, which is the scope #132 sets for the
first MO diagram.

Only **real** characters. The cyclic groups Cn with n > 2 have complex
characters for their E irreps; none of the above is one, and a group
that needs them must not be added without extending the format rather
than rounding them.
