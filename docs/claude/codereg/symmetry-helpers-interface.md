---
type: map
title: "The Fortran symmetry helpers: how ECCE calls them, and which parts of their output are contract (#422)"
area: codereg
paths: ["src/apps/symmetry/", "src/tdat/chemistry/SymmetryOps.C", "src/tdat/chemistry/Fragment.C", "src/tdat/chemistry/MoFragments.C", "tests/symmetry/reference/"]
issues: [422]
---
**Any port of `autosym`, `cleansym`, `symops`, `genmol`, `getfrag`,
`genmollat` must keep this interface; `tests/symmetry/reference/run_tests.py
--impl <dir-or-prefix>` checks it** (corpus `corpus/`, Fortran goldens
`expected/`; `make_corpus.py` and `generate_goldens.py` rebuild them;
`generate_goldens.py --check-determinism` shows the Fortran is deterministic).
It lives in `reference/` because `tests/symmetry/run_tests.py` is the MO-diagram
suite. All programs are separate processes resolved by `Ecce::ecceBinCommand`.

## Calls (stdin and stdout; no file arguments)
All but `symops` read `N`, the threshold (C `%G`), then atoms, from stdin and
write to stdout; the caller drops ghost atoms `X` first and puts the
non-zero exit status in the exception text.

| program | stdin after `N` / threshold | stdout | called from |
|---|---|---|---|
| autosym | per atom: tag line (`%-16s`), then `Z x y z` | group name (a8, upper case), then N lines of 3 x `e24.14` | `SymmetryOps::find` (threshold: Symmetry panel field, default 0.01; MO diagram and `VibAnalysis` use 0.01) |
| cleansym | group (`%-8s`, upper case), then N lines `x y z` | N, then N lines `x y z` | `SymmetryOps::clean`; reads only the N coordinate lines |
| getfrag | group, then tag + `Z x y z` per atom | K, then K 1-based atom numbers to KEEP | `findIrreducible`, `Fragment::generateIrreducibleFragment` (0.01) |
| genmol | as getfrag | N', blank line, then N' lines `a16 tag` + 3 x `e24.14` | `generateFragment`, `Fragment::generateFullMolecule` (0.01) |
| genmollat | group, then `a b c alpha beta gamma`, then atoms with FRACTIONAL coordinates | as genmol, fractional | `generateLatticeFragment` (0.1, Builder Generate and structure read) |
| symops | none; group name is `argv[1]` (stdin only if absent), 2>/dev/null | count, then per operation 3 lines `r1 r2 r3 t` | `MoFragments::symmetryOperations` |

Exit status: 0 ok; 1 reported as "failed to execute"; 2 more than 2000
atoms (symops: unknown group); 3 group not one of the 46 names (cleansym,
genmol, getfrag: `C*V`/`D*H` land here); 255 genmollat cannot match the
group name (message on stdout, not stderr); 127/signal values are reported
as a crash. A non-zero status must stay non-zero.

## What the consumers depend on (compared exactly) 
- **autosym**: group name; the coordinates come back in **the same atom
  order as the input**, centred on the centre of mass and **in the group's
  standard frame**, which is the frame `symops`/gensym matrices are written
  in. The MO diagram, `alignFrames` and genmol rely on the frame, not just
  the geometry, so coordinates are compared atom by atom (1e-6 A); a second
  line says if only the frame differs. The name is lower-cased and
  capitalised by `readSymmetry`.
- **symops**: count equals the group order, the **identity is first**, the
  other operations are used as a set (no consumer indexes into them), so the
  runner compares them order-free (1e-9). Translation column is 0.
- **genmol / genmollat**: atom order is the original atoms first, then the
  generated ones in generation order, tags verbatim; compared in order
  (1e-6 A; fractional 1e-8). An already complete molecule comes back
  unchanged ([genmol-is-handed-the-whole-molecule](genmol-is-handed-the-whole-molecule.md)).
- **getfrag**: the exact list of kept atoms, in file order.
- **cleansym**: coordinates per atom, input order.

## Behaviours the goldens pin (all current, several are bugs; a port must
decide deliberately, not by accident)
- **Threshold works backwards**: a looser threshold finds LOWER symmetry
  (0.001 H2O C2v, 0.05 Cs). See the corpus `*.t0.001/0.05/0.1` cases.
- **Distortion around 0.003-0.02 A per axis finds odd groups**: C4, C5, C4H,
  C6H, even `D1D`, not only a lower real subgroup. Several are absent from
  the 46 names, so a following `cleansym`/`genmol` exits 3.
- **Linear molecules**: C4V (HCN, CO, HCl) and D4H (N2, CO2, C2H2).
- **Icosahedral B12H12 comes back TH, not IH**.
- **genmollat normalises the name it is given like its tables** (spaces and
  underscores removed), so the Builder's `P2_1/c` (from
  `data/client/config/SpaceGroups`) and `P21/c` give the same structure. A
  port must keep that; the corpus has both spellings and their goldens are
  identical.
- genmollat compares group names case-sensitively; genmol/cleansym upper-case.

Nothing about operation ORDER of symops, the numbers' printed width, or
trailing blanks is depended on.
