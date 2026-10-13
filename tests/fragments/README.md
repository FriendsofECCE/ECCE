# tests/fragments -- fragment-orbital analysis against Multiwfn CDA (#201)

```
build-cmake/fragments   -> installed as ecce-fragments
tests/fragments/run_tests.py [build-dir]     (also: ctest -R fragments)
```

`ecce-fragments PARENT --fragment CALC [--label NAME] ...` expands each MO of
the parent in the orbitals of the fragment calculations (same geometry, same
basis) and lists the fragment orbitals contributing at least `--threshold`
percent (default 10). The maths is in `include/tdat/FragmentAnalysis.H`
(no I/O, no wx); `FragmentLoader.H` reads calculations through `MoAoBasis`.

## What the test compares

Cr(CO)6, ORCA 6.1.1 RHF/def2-SVP, as Cr + (CO)6 (both closed-shell singlet
neutrals). `oracle.json` holds, for parent MOs 30-90, every fragment orbital
Multiwfn's CDA (main function 16, option 2) gave at least 0.05 percent, with
Mulliken and with SCPA. Each must agree within 0.5 percentage point (the
printed precision of Multiwfn is 0.01). The test also asserts the t2g and eg*
character, that the library's own checks pass, and that wrong fragment sets
are refused.

## Remaking the data

`inputs/` has the three ORCA inputs (run `orca NAME.inp` for `parent`, `cr`,
`co6`; the parent input is `tests/mocomp/inputs/crco6.inp`). A fixture is
made as in `tests/mocomp/README.md`: `importmeta NAME.out ORCA`, then
`tests/mocomp/make_fixture.py NAME.out fixtures/NAME --meta IMPORTED_FOLDER`.
`gen_oracle.py ORCADIR` runs `orca_2mkl` and Multiwfn on the three `.gbw`
files and rewrites `oracle.json`. Degenerate orbitals are arbitrary mixtures
that differ from run to run, so the fixtures and the oracle must come from
the same three ORCA runs.
