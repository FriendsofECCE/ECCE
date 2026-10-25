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
Absent = vacuum, input and output unchanged. **Energy runs only**: with a
solvent, runtype gradient/optimize/frequency/ts/irc, scans, `polarizability`
and `raman` are refused at parse time with a one-line error naming the
solvent (no analytic solvent gradient exists yet). The Theory Details dialog
warns (never blocks) in the message area when a solvent is chosen for another
run type; the Calculation Editor's notes line and the Launcher's pre-launch
question (Launch anyway is the default) say the same, from
`include/tdat/QmSolventWarning.H`; combo `ES.Theory.ECCEQM.Solvent` (None | names | Custom) and float
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

Gradients would need: dq/dx of the cavity (switching function, point
positions, area-dependent zeta), the derivative of M, the solute-potential
derivatives at the points (`int1e_iprinv` per point, as `gradient.cpp` does
for nuclei) and the reaction-field Fock term derivative; the SCF response of
the charges drops out at convergence.
