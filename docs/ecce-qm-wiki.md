# ECCE-QM: developer notes

ECCE-QM is the Hartree-Fock and density functional engine that ships with ECCE
(program `ecce-qm`, source `src/qm`, registered in ECCE as the code ECCE-QM).
It is ours: unlike the other codes ECCE drives, there is no external manual, so
this page and `docs/qm/README.md` in the repository are the documentation of
how it works. The user's view is the help page "ECCE-QM". The engine is a
library (`eccems`, no wx and no ECCE dependencies) plus a command line driver
(`main.cpp`); ECCE talks to it only through a text input file and a text output
file.

## Layout of `src/qm`

| File | Content |
|---|---|
| `common.hpp` | Eigen matrix types, unit constants, the `Error` exception |
| `molecule.*` | atoms (atomic number, position in bohr), charge, multiplicity, element tables |
| `basislib.*` | reads ECCE's basis set files (`data/admin/basissets/*.BAS`); a basis name may join files (`6-31G*` is `6-31G` plus `6-31GS`); explicit shells from the input replace the library |
| `basis.*` | the basis set of one molecule in the layout libcint wants (`atm`, `bas`, `env` arrays), AO order and normalisation |
| `cint_wrap.hpp` | the only place libcint's header is included (its lower-case macros clash with ordinary names) |
| `integrals.*` | one-electron integrals (overlap, kinetic, nuclear attraction, dipole) and the in-core, 8-fold packed two-electron store with the J and K builds |
| `ao.*` | AO values and derivatives on points, for the DFT code and orbital plotting |
| `grid.*`, `lebedev_data.inc` | the molecular grid: Treutler-Ahlrichs radial points, Lebedev angular points, Becke weights |
| `xc.*` | functionals through Libxc, the integration of the exchange-correlation energy and potential, and its gradient |
| `scf.*` | RHF, UHF, ROHF and Kohn-Sham: atomic-density guess, DIIS, the converged result (`ScfResult`) with Mulliken charges, dipole and S^2 |
| `gradient.*` | analytic nuclear gradient |
| `optimizer.*` | Cartesian BFGS with a model Hessian, trust radius, ORCA's convergence thresholds |
| `vibration.*` | finite-difference Hessian, projection, frequencies, IR intensities, thermochemistry |
| `charges.*` | Loewdin, CHELPG and Merz-Kollman charges |
| `input.*` | the input parser (`Input`, `RunType`) |
| `output.*` | the output writer and the Molden writer |
| `main.cpp` | the driver: parse, build the basis, run the SCF or the optimisation, write the output |

The data flow of an energy calculation is: `parse_input_file` gives an `Input`;
`Basis` is built for the molecule; `run_scf` returns an `ScfResult` (energies,
orbitals, density matrices, Fock matrices, overlap, Mulliken charges, dipole);
`write_text_output` prints it. A gradient adds `scf_gradient`, an optimisation
wraps `evaluate` (basis, SCF, gradient) in `optimize_geometry`, and a frequency
run calls `finite_difference_derivatives` with the same evaluation at
displaced geometries.

The basis function order matters outside the engine because the molecular
orbital coefficients are exported: atoms in input order, shells sorted by
angular momentum within an atom, real solid harmonics within a shell in
libcint's order. `data/client/cap/ECCE-QM.edml` describes the same order to
ECCE (`MOOrdering`). Change one and the orbital pictures are silently wrong.

## libcint and Libxc

**libcint** (6.1.3) is fetched at configure time and built statically; Debian
does not package it. All integrals go through its C interface with the arrays
in `Basis`. One-electron integrals are looped over shell pairs and copied into
matrices (see `one_e_deriv` in `gradient.cpp` for the pattern with derivative
integrals). An integral over a nucleus or a point charge uses `int1e_rinv`
with the operator centre set in `env[PTR_RINV_ORIG ..+2]`; that is how the
nuclear attraction of each nucleus in the gradient and the electrostatic
potential at the fit points in `charges.cpp` are computed. `env` is modified
in place, so code that runs in parallel over centres must give each thread its
own copy of `env`. The spherical and Cartesian variants of each function
(`_sph`, `_cart`) are chosen by `Basis::spherical()`.

**Libxc** (system package `libxc-dev` if it is 5.1 or newer, else fetched) is
used only in `xc.cpp`. `functional_by_name` maps a name to a list of Libxc
functional identifiers and the fraction of exact exchange; the integrator
evaluates them on the grid in blocks and builds the exchange-correlation matrix
and, for the gradient, the contribution from the basis function derivatives.
Only LDA and GGA functionals are implemented; a meta-GGA needs the kinetic
energy density in the integrator and in the gradient.

Eigen provides the linear algebra and OpenMP (when the compiler has it; Apple's
compiler does not, so the macOS build runs on one core) the parallelism.

## Adding a functional

1. In `xc.cpp`, add the name to `functional_by_name` with its Libxc
   identifiers, the exact exchange fraction `exx` and `gga`. A range-separated
   hybrid needs more than that (the exchange build has no range separation).
2. In `scripts/codereg/ecceqmtheory.py` add it to `XC_FUNCTIONALS`; in
   `scripts/parsers/ai.ecceqm` add the display name to `%XCKeyword`. The two
   lists must agree exactly; `tests/dialogs` compares them.
3. Add cases to `tests/qm/gen_oracle.py` (`CASES`, and the ORCA keyword in
   `ORCA_METHOD`), regenerate `oracle.json` with ORCA, and check the
   gradient against finite differences (`tests/qm/gradient_test.py`).
4. Update the method list in the user help and `docs/qm/README.md`.

Energies from different programs differ by the integration grid and by the
form of the functional (B3LYP exists with VWN3 and VWN5). Decide which program
is the reference, and write down the tolerance as `run_tests.py` does.

## Adding a property

A property flows through four stages, and a mismatch between any two is
silent: the job runs and the property is just absent in ECCE.

1. **Compute** it from the `ScfResult` (or in a new file, as `charges.cpp`
   does) and add the input keyword in `input.cpp` if it is optional.
2. **Write** it in `output.cpp` as a `begin <name>` ... `end <name>` block, or
   a `key value` line.
3. **Parse** it: add an entry to `scripts/parsers/ecceqm.desc` whose `Begin`
   matches the block's first line and no other line, and the branch in
   `scripts/parsers/ecceqm.parse` that prints ECCE's property format. The key
   names (`LOEWDIN`, `MKFIT`, ...) are ECCE's.
4. **Show** it: a line in `data/client/config/properties` and, for a per-atom
   charge set, one entry in `data/client/config/PropertyPanelDescriptor.xml`
   (class `MullikenPanel`, which reads the key from the descriptor).
   If the user chooses the property, add a tick box in
   `scripts/codereg/ecceqmtheory.py` and the keyword line in `ai.ecceqm`.

Add a fixture to `tests/parsers` (a real output, `cases.py`, then
`run_tests.py --update`), a test for the number itself against an independent
program, and a line in the help. The Loewdin, CHELPG and Merz-Kollman charges
are a complete small example of all of the above.

## Adding a run type

1. `input.hpp`: add the value to `RunType` and, if it does geometry changes or
   frequencies, to the helper predicates; `input.cpp`: parse it.
2. `main.cpp`: the driver is a few lambdas (`evaluate`, `frequencies`) and two
   branches (optimisation or single evaluation); add the new path there.
3. `output.cpp`: write whatever is new, including `progress` lines if the run
   is long (the job monitor shows them).
4. `scripts/parsers/ai.ecceqm`, `RunType()` maps ECCE's run type name to the
   keyword and refuses the rest; `data/client/cap/ECCE-QM.edml` lists the run
   types of each theory; `ecceqm.desc` and `ecceqm.parse` read the new output.
5. Tests as for a property, and `tests/launch/ecceqm_test.py` for the path
   through Launch.

## Tests

Everything is in `tests/qm` and in the ctest names `qm_*` unless noted. The
engine can be built and tested on its own:

```
cmake -S src/qm -B build-qm -G Ninja
cmake --build build-qm
ctest --test-dir build-qm -R 'qm_(unit|scf_quick|gradient_quick|opt_quick|freq_quick)'
```

| Test | What it checks | Needs |
|---|---|---|
| `qm_unit` | Lebedev rules against analytic integrals, AO values against libcint's overlap by quadrature, model Hessians, symmetry numbers | nothing |
| `qm_scf_quick`, `qm_scf` | energies and orbital energies of 67 cases against the numbers ORCA 6.1.1 printed (`oracle.json`, made by `gen_oracle.py`), NWChem for a subset | nothing: the oracle is stored |
| `qm_gradient[_quick]` | analytic gradient against central differences of the energy | nothing |
| `qm_opt[_quick]` | optimised energy, distances and angles against ORCA (`opt_oracle.json`) | nothing |
| `qm_freq[_quick]` | frequencies, IR intensities, thermochemistry against ORCA (`freq_oracle.json`) | nothing |
| `qm_diatomic_order` | valence orbital order of the lab diatomics | nothing |
| `qm_charges` | Loewdin, CHELPG and Merz-Kollman charges against ORCA and Gaussian 16, run live | ORCA or g16, else skipped (exit 77) |
| `qm_charges_deck` | the Theory Details tick boxes become the `charges` line | nothing |
| `tests/parsers` (`ecceqm-*` cases) | the output of the engine through ECCE's parse descriptor | nothing |
| `launch_ecceqm`, `apps_ecceqm_*` | the whole chain: input deck, run, parse, Builder panels, size warning | a built tree, Xvfb |

The oracle files record ORCA's version and keywords for every number, so a
disagreement can be traced to a setting rather than a program. When a number
differs, check what it depends on before changing a tolerance: the grid for
density functionals, the form of the functional (VWN3 or VWN5), spherical or
Cartesian functions, and whether the other program uses its density fitting
by default (ORCA's RI approximation is switched off in the oracle runs).

The `quick` variants cover a smaller set and finish in minutes; the full lists
take hours and are run before a release. Tests that need a built ECCE use
`--build` or `ECCE_TEST_HOME` to find the binaries, as the other `tests/apps`
tests do.
