# tests/mocomp -- MO composition against Multiwfn (#161)

```
build-cmake/mocomp   -> installed as ecce-mocomp
tests/mocomp/run_tests.py [build-dir]     (also: ctest -R mocomp)
```

`ecce-mocomp CALC --mo N` prints which basis functions, shells, shell types
and atoms an orbital is made of. The Builder's MOs panel shows the same
numbers (`MoAoBasis`, `include/tdat/MoAoBasis.H`).

Default method: **Mulliken**, `c_i (S c)_i / c.S.c`, so the parts sum to 100%.
It is what the MO diagram's fragment shares use, and what ORCA and Multiwfn
call the composition. Individual Mulliken parts are not bounded (a part can be
negative, or above 100%). `--method c2` gives `c_i^2 / sum c^2`, which is
bounded but depends on how diffuse the basis is.

## What the test compares

For each fixture, `ecce-mocomp --tsv` for a set of orbitals against
`reference/<name>.tsv`, the numbers Multiwfn's orbital composition analysis
printed (main function 8 > 1, Mulliken; `<name>.c2.tsv` is 8 > 3, the
Ros-Schuit partition, which is plain c^2). Every basis function, contracted
shell, (atom, shell type) and atom must agree within 0.1 percentage point; the
actual worst difference is printed (about 0.001). Multiwfn reads a molden file
(`orca_2mkl NAME -molden`) or a Gaussian formatted checkpoint file, never
anything ECCE wrote.

| fixture | code and basis | covers |
|---|---|---|
| water | ORCA RHF def2-SVP | all 24 MOs |
| benzene | ORCA RHF def2-SVP | 18 MOs, sigma and pi |
| ch3 | ORCA UHF def2-SVP | alpha and beta |
| h2co631 | Gaussian 16 RHF 6-31G* | Cartesian d functions, all 34 MOs |
| crco6 | ORCA RHF def2-SVP Cr(CO)6 | transition metal, f functions, 16 MOs including virtuals with large negative shares |

`inputs/` has the ORCA and Gaussian inputs; `molden/` the files Multiwfn read
(crco6's is 1 MB and left out; its reference came from `orca_2mkl` on
`crco6.gbw`).

With Multiwfn installed (`MULTIWFN=/path/to/Multiwfn_noGUI`, or on PATH, and
executable) the reference files are also re-derived from `molden/` and compared
with the checked-in ones; that part is skipped otherwise.

## Making a fixture

A fixture is a calculation folder in local data mode (`.ecce-meta` sidecars,
`Parameters/`, `Props/`):

1. `importmeta OUT CODE` (built with the tests) imports the output into a local
   data folder, which makes `Parameters/` and the sidecars.
2. `make_fixture.py OUT FOLDER` (ORCA) or `make_fixture.py --fort7 fort.7
   FOLDER` (Gaussian 16, `Punch=MO`) fills `Props/` from the real
   `scripts/parsers/orca.mo` / `gaussian-16.mo`.
3. `multiwfn_ref.py MOLDEN MOS [--beta MOS] [--scpa]` writes the reference.

Geometry for the Gaussian run was put in Gaussian's standard orientation first,
so the stored frame is the one the coefficients are in.
