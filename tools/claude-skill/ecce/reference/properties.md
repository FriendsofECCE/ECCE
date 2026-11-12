# Common ECCE property keys (`Props/<KEY>`)

Read `Props/` first: it is already parsed, and the units are in the XML.

- A scalar looks like `<value name="TE" units="Hartree">-79.227…</value>`.
- A vector looks like `<vector name="VIBFREQ" rows="30" units="cm-1">…</vector>`.
- Orbitals and geometry traces are `<table …>` or time-series tables.

| Group | Keys |
|---|---|
| Energies | TE (final energy, Hartree), TEVEC (per optimisation step), DELTAE, EGIBBS (Gibbs energy where the code prints thermochemistry), ETIME |
| Geometry | GEOMTRACE (structure per step); SCANENERGY, SCANGEOM, SCANCOORD (relaxed scans); IRCCOORD, IRCENERGY, IRCGEOM (IRC path) |
| Gradient | EGRAD (final), EGRADVEC (per step) |
| Vibrations | VIBFREQ (cm⁻¹; leading near-zeros are translations and rotations), VIBIR (km/mol), VIBRAM (Raman activity), VIBSYM (irreps); normal-mode vectors in the vibration table |
| Orbitals | MO, ORBENG, ORBOCC, MOAOORDER; LOCMO, LOCMOENG, LOCMOOCC, LOCMOLABEL (localised orbitals, ECCE-QM) |
| Charges | MULLIKEN, LOEWDIN, HIRSHFELD, CHELPG, MK, APT where computed |
| Other | DIPOLE, S2 (⟨S²⟩) |
| MD | TIMEVEC, TEMPVEC, PRESSVEC, MDPEVEC, MDKEVEC |

Not every code fills every key, so when a key is missing, read the output itself.

`ecce-results` converts orbitals and Hessians from Molden, fchk and ORCA files into one HDF5 file (help page "The results file format"). The full list of keys with their units is `data/client/config/properties` in the ECCE source.
