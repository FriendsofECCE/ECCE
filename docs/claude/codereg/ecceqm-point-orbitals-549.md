---
type: pitfall
title: "On an ECCE-QM scan or IRC, getProperty(\"MO\") returns the selected point's orbitals, not the stored final ones"
area: codereg
paths: ["src/dsm/edsiimpl/PropertyTask.C", "include/dsm/IPropCalculation.H", "scripts/parsers/ecceqm.parse", "scripts/parsers/ecceqm.desc", "src/qm/output.cpp", "src/qm/main.cpp", "src/apps/builder/PointNote.C", "tests/parsers/ecceqm_point_orbitals_test.py"]
issues: [549]
---
- ECCE-QM writes every scan/IRC point's converged orbitals
  (`begin point_orbitals` / `begin point_mo_coefficients`, numbered as the
  `scan_geometry` / `irc_geometry` blocks). They are stored as PTORB/PTORBBETA
  (per point: energy, occupation per MO) and PTMO/PTMOBETA (per point: the
  MO table), one PropTSVecTable table per point, indexed like SCANGEOM/IRCGEOM.
- `IPropCalculation::orbitalPoint(k)` (set by the Relaxed Scan / IRC Path
  panels) makes PropertyTask answer MO, MOBETA, ORBENG, ORBOCC, ORBENGBETA and
  ORBOCCBETA from table k. Every reader follows without knowing:
  MoPanel, MoCoeffs, the density panels, MoDiagramPanel, ComputeMoCmd. A
  reader that needs the calculation's own final set must call
  `orbitalPoint(-1)` first (and restore it).
- LOCMO* (localised orbitals) exist for the final set only: at a point whose
  orbital energies differ from the stored ORBENG, getProperty returns 0 for
  them, so no localised orbital is drawn on another point's structure.
- The built properties are cached per key and point until
  `flushPropertyCache()`; panels keep the pointers they got, so the cache must
  not be cleared on a point change.
- ORCA, Gaussian and ECCE-QM runs older than 9.4.1 have no PTMO:
  `hasPointOrbitals()` is false and nothing changes for them.
- Size: about nbf^2 x 15.5 bytes per point in the output (and again in the
  stored props); the 84-function NMA scan example (25 points) would grow by
  about 2.8 MB.
