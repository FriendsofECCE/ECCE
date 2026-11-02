---
type: map
title: "ELF, LOL, s and sign(lambda2)rho come from DensityPlane::Evaluator::fieldsAt (analytic AO first and second derivatives) in wx-free DensityFields, written as cubes by ecce-density-field and checked against Multiwfn"
area: wx-viewer
section: "wxWidgets 3.2/GTK3, the 3D viewer, and C++ pitfalls"
paths: ["include/tdat/DensityFields.H", "src/tdat/chemistry/DensityFields.C", "include/tdat/EspField.H", "src/tdat/chemistry/EspField.C", "include/tdat/DensityPlane.H", "src/tdat/chemistry/DensityPlane.C", "src/apps/densityfield/ecce-density-field.C", "tests/tdat/densityfields_test.C", "tests/tdat/densityfields_cli_test.py", "tests/tdat/densityfields_oracle.py", "tests/tdat/make_densityfields_fixtures.py"]
issues: [798, 799, 800]
---
- **Derivatives.** `EspBasisFunction::derivatives` gives value, gradient and
  Hessian of one AO with the same angular terms, contraction coefficients and
  `ar < 70` cut as `value()`, so the AO order and normalisation are the stored
  ones. `Evaluator::fieldsAt(x, y, z, FieldPoint&)` (Angstrom in, bohr units out)
  sums them over the MOs: rho, grad rho, Hessian (xx yy zz xy xz yz), its sorted
  eigenvalues, and per spin rho, grad rho, tau = 1/2 sum n |grad psi|^2. One
  evaluation of the basis per point; cost is nbf x (nocc + 10).
- **Spin.** Unrestricted calculations use the alpha and beta MO sets; a
  restricted one puts min(n,1) of an orbital's n electrons in alpha and the rest
  in beta (ROHF occupations 2, 1, 0 work).
- **Open-shell ELF/LOL** follow Multiwfn: D = sum over spin of
  (tau_s - |grad rho_s|^2/(8 rho_s)), D0 = (3/10)(6 pi^2)^(2/3) sum rho_s^(5/3);
  for a closed shell that is the textbook (3/10)(3 pi^2)^(2/3) rho^(5/3) of the
  total. LOL = t/(1+t), t = D0/tau.
- **Guards**: total rho below 1e-12 gives ELF = LOL = 0 and s = 100.
- **Pitfall: Multiwfn defaults are not the definitions.** `settings.ini` has
  `RDG_maxrho = 0.05` (s forced to 100 above that density) and
  `ELF_addminimal = 1` (+1e-5 added to D). Set both to 0 to compare. Its point
  menu (main menu 1) asks for the unit after every point (`2` = Angstrom).
- **Pitfall: a finite-difference check near a core needs a small step.** At 0.4 A
  from O the 1s primitives (exponent 5.5e3) gave 2e-5 relative error in the
  gradient with a 2e-3 A step; the analytic value matched Multiwfn to 3e-7.
- **CLI** `ecce-density-field CALC --field elf|lol|rdg|signl2rho|density
  [--spacing A] [--margin A] [--threads N] -o out.cube`, and `--at X,Y,Z` (A,
  repeatable) prints every field at points, which the oracle scripts use. Cube
  lengths in bohr. Listed in `ECCE_CLI_APPS` like `ecce-thermo`.
- **Oracle** (Multiwfn 2026.9.20, same wavefunction as molden from
  `ecce-qm --molden`; 6-31G*, aug- sets are refused): 60+ random points per
  molecule, `densityfields_oracle.py`. Water, N2, S22 water dimer (RHF) and the O2
  triplet (UHF and B3LYP/UKS): ELF and LOL within 5e-7, rho and grad rho within
  1e-6 relative, s within 2e-6, lambda2 within 5e-7.
- **Tests**: `densityfields` (AO and density derivatives and tau against finite
  differences, guards) and `densityfields_cli` (CLI against Multiwfn numbers
  stored in the script; Multiwfn is not needed). Fixtures:
  `tests/tdat/densityfields-fixtures` (`make_densityfields_fixtures.py`).
