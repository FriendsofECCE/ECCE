# ECCE-QM

ECCE-QM is the quantum chemistry program that comes with ECCE. It does
Hartree-Fock and density functional calculations on small molecules, on the
computer you are sitting at, with nothing to install and nothing to register:
choose **File > New > ECCE-QM Calculation...** in the Organizer, build or import a
molecule, and launch. It is meant for teaching and for quick looks at molecules
of up to a few tens of atoms. For larger systems, other basis sets, effective
core potentials, correlated methods or anything that must run for hours, use
one of the other codes ECCE sets up (ORCA, Gaussian, NWChem, MOPAC).

ECCE-QM is part of ECCE and is maintained with it, so a problem with it is a
problem to report to ECCE (see the end of this page). The text of its input
and output files is described in [ECCE-QM input and output
files](ecce-qm-files.md).

## Methods

| Choice | Offered |
|---|---|
| Hartree-Fock | restricted (RHF), unrestricted (UHF), restricted open shell (ROHF) |
| Density functional | SVWN (Slater exchange with VWN5 correlation, the local density approximation), PBE, B3LYP, PBE0, each restricted, unrestricted (UKS) or restricted open shell (ROKS) |

In the Calculation Editor, **Theory** selects HF or DFT, and **Theory Details**
selects the functional and, for a molecule that is not a closed shell, whether
the orbitals are restricted open shell (the default) or unrestricted. A closed
shell molecule always uses restricted orbitals. A new calculation starts as
B3LYP.

B3LYP here uses the VWN5 form of the local correlation, as ORCA does. Gaussian
uses a different form (VWN3) and gets slightly different energies from the same
functional name; the two should not be compared to better than about a
millihartree. The density functionals are evaluated on a numerical integration
grid of 99 radial and 590 angular points per atom. The functionals themselves come from the Libxc library.

## Basis sets

STO-3G, 3-21G, 6-31G, 6-31G*, 6-31+G*, 6-311G*, cc-pVDZ, aug-cc-pVDZ,
cc-pVTZ and def2-SVP, chosen from the **Basis Set**
menu of the Calculation Editor. A new calculation starts with 6-31G*. The
basis sets are the ones in ECCE's library; the Basis Set Tool is not used for
this code. Spherical d and f functions are used throughout. An element that a basis set does not cover, or that would
need an effective core potential, is named before the job is launched.

## Run types

| Run type | What it does |
|---|---|
| Energy | the energy, orbitals and charges at the given geometry |
| Gradient | the same, plus the analytic force on each atom |
| Geometry | optimises the geometry to a minimum; the steps appear in the geometry trace and the energy plot as for any other code |
| Vibration | harmonic frequencies, IR intensities, normal modes and thermochemistry at the given geometry |
| GeoVib | a geometry optimisation followed by a vibration calculation at the result |

The optimiser stops under the same thresholds as ORCA's default optimisation.
The vibration calculation takes the second derivatives from finite differences
of the analytic forces, so it is only meaningful at a stationary point of the
same method and basis set: run **GeoVib**, or optimise first. Thermochemistry
is for an ideal gas at 298.15 K and 1 atm in the rigid-rotor, harmonic
oscillator approximation; the symmetry number is found from the geometry.

For Vibration and GeoVib, **Runtype Details** has a **Raman activities** tick
box. It adds the Raman activity and the depolarization ratio of every mode,
shown in the vibration panel and the Raman pane of the spectrum viewer, and the
static polarizability of the molecule, drawn as an ellipsoid in the Viewer.
The polarizability is worked out with a small electric field applied in the
three directions, at the given geometry and at every geometry the frequency
calculation displaces, so the calculation takes about seven times as long as
the frequencies alone. **Open in window** in the vibration panel shows the
spectrum in a window of its own, so the IR and Raman spectra can be seen side
by side.

## What you get

- the total energy, and its parts (nuclear repulsion, one-electron, Coulomb,
  exchange, exchange-correlation);
- orbital energies and occupations, the HOMO and LUMO, and the orbital
  coefficients, so the molecular orbitals can be drawn and the orbital
  diagram shown (the orbital diagram is off by default for this code, and is
  switched on under Preferences);
- atomic charges: Mulliken always, and on request Loewdin, CHELPG and
  Merz-Kollman charges, which are ticked under **Theory Details** and listed
  under Charges in the Viewer next to the Mulliken charges. CHELPG and
  Merz-Kollman charges are fitted to the electrostatic potential and are
  less dependent on the basis set than Mulliken charges; the electrostatic
  potential surface uses the CHELPG charges when they were requested;
- the dipole moment and, for open shells, the expectation value of S^2;
- for Vibration and GeoVib, the frequencies, IR spectrum, normal modes,
  zero-point energy, thermal energy, enthalpy,
  entropy and Gibbs energy, and with **Raman activities** ticked, the Raman
  spectrum and the polarizability.

## Practical limits

ECCE-QM keeps all two-electron integrals in memory. This makes each SCF cycle
fast, and it sets the size limit: the memory needed grows as the fourth power
of the number of basis functions, about 170 MB for 114 functions and about 4 GB
for 250. The program refuses a calculation whose integrals would need more
than 12 GB, and ECCE warns before launching one with more than 25 atoms or 150
basis functions. The warning is only a warning; you can launch anyway.
The larger basis sets matter here: water has 13 basis functions in 6-31G but
41 in aug-cc-pVDZ and 58 in cc-pVTZ, so a molecule that is quick with 6-31G*
can need much more memory and time with aug-cc-pVDZ or cc-pVTZ.

A frequency calculation needs six energy and force calculations per atom, so
its time grows roughly as the square of the number of atoms times the number
of basis functions. As a guide, measured on one computer (an Intel Core
i7-7700, four cores with two threads each, that was also busy with other work)
with B3LYP and 6-31G*: ethanol takes about two minutes for its frequencies,
and about ten minutes when Raman activities are included, a little over five
times as long. Benzene takes 10 minutes with Hartree-Fock and 23 minutes with
B3LYP on an 8-core desktop computer; a laptop with four cores takes about
twice as long. ECCE shows an estimate and asks before launching a frequency
run that is expected to take long, Raman activities included.

The displaced geometries of a frequency calculation are independent, so
ECCE-QM runs several of them at the same time, as many as the cores and the
free memory allow (each one holds its own integrals). On the computer above
this shortened ethanol from 134 to 111 seconds, and ethanol with Raman from 13
to 10 minutes; the gain is modest because the integrals and the integration
grid already use every core. `parallel_points false` in the input, or the
environment variable `ECCE_QM_SERIAL`, runs the points one after another.

Where the compiler supports it (the Linux version), the integrals and the
integration grid use all the cores of the computer. The macOS version of
ECCE-QM runs on one core, which makes it slower there.

## How it was checked

The results were compared with ORCA 6.1.1 (and, for a subset, NWChem 7.2.3)
on small molecules: water, ammonia, methane, carbon monoxide,
nitrogen, hydrogen fluoride, benzene and open-shell oxygen, with the STO-3G,
6-31G*, def2-SVP, 3-21G and cc-pVDZ basis sets; water and hydrogen chloride
were also checked with 6-31G, 6-31+G*, 6-311G*, aug-cc-pVDZ and cc-pVTZ.

- Energies: Hartree-Fock agrees to about 1e-8 hartree. B3LYP, PBE and PBE0
  agree to about 1e-5 hartree; the remaining difference is the integration
  grid, which differs between programs.
- Orbital energies agree to about 1e-5 hartree for occupied orbitals.
- Forces agree with finite differences of the energy to 1e-8 hartree/bohr
  (Hartree-Fock) and 1e-6 (density functionals).
- Optimised geometries agree with ORCA's to 1.4e-4 angstrom in bond lengths
  and 0.007 degrees in angles.
- Frequencies agree with ORCA's analytic frequencies to 0.8 cm-1, IR
  intensities to 0.07 km/mol, and the zero-point energy and Gibbs energy to a
  few microhartree. ORCA cannot compute restricted open shell frequencies, so
  those were checked against the curvature of ECCE-QM's own energies.
- The static polarizability agrees with ORCA's analytic (coupled-perturbed)
  value to 1e-3 bohr^3 in every tensor component at Hartree-Fock and B3LYP,
  and with NWChem's to 1e-4 bohr^3, for water, ammonia, carbon monoxide and
  hydrogen fluoride. Raman activities agree with ORCA's to 0.1 % and
  depolarization ratios to 3e-4 for water, ammonia and methane; the
  frequencies of those calculations agree to 1.5 cm-1.
- Loewdin charges agree with ORCA's to 1e-5 e. CHELPG charges agree with
  ORCA's within 0.005 e; the small differences come from the points where the
  potential is sampled, which each program chooses in its own way.
- The orbitals were checked in the Molden format against the program Multiwfn.

## Reporting a problem, and citing

Report a problem with ECCE-QM, with the input file (`ecceqm.qmin`) and the
output file (`ecceqm.qmout`) if you can, on the ECCE issue tracker on GitHub.
A calculation that gives a different energy from another program is worth
reporting too, with the version of that program and its settings.

When you publish results from ECCE-QM, cite ECCE as described in [How to cite
ECCE](citing-ecce.md), and name the functional and basis set you used, as for
any calculation. The functionals are evaluated with Libxc and the integrals
with Libcint; their authors ask to be cited as well (S. Lehtola et al.,
SoftwareX 7, 1 (2018); Q. Sun, J. Comput. Chem. 36, 1664 (2015)).
