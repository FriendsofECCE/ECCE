---
type: map
title: "ECCE-QM implicit solvent (CPCM): model, keywords, what is and is not supported, how it was validated"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["src/qm/solvent.cpp", "src/qm/solvent.hpp", "src/qm/scf.cpp", "src/qm/input.cpp", "src/qm/output.cpp", "scripts/parsers/ai.ecceqm", "scripts/parsers/ecceqm.parse", "scripts/parsers/ecceqm.desc", "scripts/codereg/ecceqmtheory.py", "scripts/codereg/ecceqmtheory.dlg", "tests/qm/solvent_test.py", "tests/qm/unit_test.cpp"]
issues: []
---
Keywords: `solvent <water|methanol|ethanol|acetonitrile|dmso|chloroform|toluene|hexane|none>`,
`epsilon <x>` (custom dielectric constant), `cavity_scale <x>` (default 1.2).
Absent = vacuum, input and output unchanged. Every run type works with a
solvent (energy, gradient, optimize, frequency, ts, irc, scans) except
`polarizability` and `raman`, refused at parse time (no solvent response to a
field). Frequencies and the TS Hessian use the finite-difference Hessian of
solvated gradients: `analytic_hessian_available` (hessian.cpp) is false with a
solvent, which also makes `ts_hessian_every` default to 0. The Calculation
Editor's notes line and the Launcher's pre-launch question (Launch anyway is the
default) warn only for Raman, from `include/tdat/QmSolventWarning.H` (the
Raman tick is on the run-type panel, so the Theory dialog cannot see it); combo `ES.Theory.ECCEQM.Solvent` (None | names | Custom) and float
`ES.Theory.ECCEQM.Epsilon`; .py and .dlg kept in step.

Model: conductor-like PCM, K q = -f V with f = (eps-1)/eps (Barone and
Cossi, JPCA 102, 1995 (1998)); cavity = union of atomic spheres of Bondi radii
(H 1.10, ORCA's table, the values ORCA 6.1.1 prints divided by its 1.2) times
1.2; 302 Lebedev points per sphere; Gaussian surface charges whose diagonal is
divided by the point's exposed fraction (York and Karplus, JPCA 103, 11060
(1999); Scalmani and Frisch, JCP 132, 114110 (2010)), exposure = product of
Gaussian-smoothed steps (erf, width = the charge's own) over the other
spheres, so the energy is continuous. Exponent zeta = 4.84/sqrt(area); the
table value for 302 points was taken from memory. The solute potential at a
point is the point-charge potential (not Gaussian-smeared), nuclei plus
`int1e_rinv` electrons; q is rebuilt from the current density every SCF cycle
(Fock term -sum q_k <mu|1/r_k|nu>, energy + q.V/2). Elements without a radius
(d block, beyond Kr) are an error. `Cpcm::charges` solves
M p = -f V with M_kl = S_kl F_l and q = F p (well conditioned as F -> 0).

Output: `solvation_energy` (Eh) and `solvation_energy_kjmol` are **q.V/2, the
polarisation energy of the medium, already inside `energy_total`**. It equals
ORCA's "CPCM Dielectric". It is not E(solvent) - E(vacuum) (that needs a
second run), so it is stored as `EPCM`, never as `ESOLV`. `solvent_model` ->
`SOLVMODEL` ("CPCM, Water").

Dielectric constants are Gaussian/SMD table values written from memory, not
checked; ORCA's own are 80.151 (water), 33.428, 25.451, 36.763, 47.381,
4.803, 2.387, 1.889 in the order above after water.

Validation (ORCA 6.1.1, `! CPCM(water)`, 6-31G*, epsilon 80.151 in both):
E(solv) - E(vac) agrees within 0.1-0.7 percent and 0.9 kJ/mol at most for
H2O, NH3, NH4+, Cl-, CH3OH, HF and B3LYP; UHF/ROHF NH3+ within 0.8 kJ/mol;
orbital energies within 0.2 mEh. Born ion on one sphere: ratio to
-(1-1/eps)/(2R) is 1.0009. `tests/qm/solvent_test.py` keeps the ORCA numbers.

Gradient (#317): the SCF energy is variational, the converged Fock matrix
carries the reaction-field term, so the energy-weighted density in
`scf_gradient` is already right and only the explicit nuclear dependence of
E_pol is added: with A q = -f V (A symmetric, A_kk = zeta sqrt(2/pi)/F_k,
q = F p, E_pol = q.V/2),
dE_pol/dx = q.(dV/dx) + q.(dA/dx).q / (2f).
Points ride with their sphere's atom, zeta is fixed (the area does not change).
dV: nuclear potential (`Cpcm::add_gradient`) plus `int1e_iprinv` per point at the
point as operator centre, the point's own derivative being minus the sum over
basis centres (`CpcmField::add_electronic_gradient`, no per-point matrix cache).
dA: erf term through r_kl and the diagonal through F_k, which depends on the
own atom and every other atom. Points dropped for F < 1e-8 are a negligible
discontinuity (finite differences agree to 1e-8). Cost: one `iprinv` pass per
point, i.e. about (points/atoms) nuclear-attraction gradients.

Validation of the gradient: `tests/qm/gradient_test.py` (solvated cases against
central differences, 1e-4 bohr, pushed geometries; net force zero) agrees to
5e-9 Eh/bohr for HF H2O, NH4+, CH3OH and UHF NH3+; B3LYP H2O 3e-7. ORCA 6.1.1
`HF 6-31G* TightOpt Freq CPCM(water)` (2026-10-26): optimised bonds within
0.0001 A, angles within 0.03 degrees, the solvated-minus-gas geometry change
reproduced to 0.005 degrees in angles, frequencies within 4 cm-1 (the water bend
and methanol torsion are the largest), `tests/qm/solvent_test.py`.
