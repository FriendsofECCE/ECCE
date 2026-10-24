# Common ECCE property keys (Props/<KEY>)

Energies: TE (total/final energy, Hartree), TEVEC (energy per optimisation step),
DELTAE, EGIBBS (Gibbs free energy where the code prints thermochemistry), ETIME.
Geometry: GEOMTRACE (structure per optimisation step), SCANENERGY / SCANGEOM / SCANCOORD
(relaxed scans), IRCCOORD / IRCENERGY / IRCGEOM (IRC path).
Gradient: EGRAD (final), EGRADVEC (per step).
Vibrations: VIBFREQ (cm-1; leading zeros are translations/rotations), VIBIR (km/mol),
VIBRAM (Raman activity), VIBSYM (irreps); normal-mode vectors in the vibration table.
Orbitals: MO (coefficients), ORBENG (energies), ORBOCC (occupations), MOAOORDER;
LOCMO, LOCMOENG, LOCMOOCC, LOCMOLABEL (localised orbitals, ECCE-QM).
Charges: MULLIKEN, LOEWDIN, HIRSHFELD, CHELPG/MK/APT where computed.
Other: DIPOLE, S2 (spin contamination), MD: TIMEVEC, TEMPVEC, PRESSVEC, MDPEVEC, MDKEVEC.
The full list with units: `data/client/config/properties` in the ECCE source.
