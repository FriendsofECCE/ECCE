---
type: map
title: "ECCE-QM localised orbitals: one set per spin, *BETA keys, a note when none"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["src/qm/localise.cpp", "src/qm/output.cpp", "scripts/parsers/ecceqm.parse", "scripts/parsers/ecceqm.desc", "src/apps/builder/MoPanel.C", "src/viz/propsgcommands/ComputeMoCmd.C"]
issues: [533]
---
The engine localises (Pipek-Mezey) the occupied alpha and the occupied beta
orbitals separately for UHF and ROHF (ROHF has one canonical set but two
localised ones) and writes `begin localised_orbitals alpha|beta` and
`localised_mo_coefficients alpha|beta`; closed shell keeps the `restricted`
block. The parser stores beta under `LOCMOBETA`, `LOCMOENGBETA`,
`LOCMOOCCBETA`, `LOCMOLABELBETA`, like `MOBETA`/`ORBENGBETA`. When nothing can
be localised (SCF not converged, no occupied orbitals) the engine writes
`localisation_skipped <reason>`, stored as `LOCMONOTE`, and the MOs panel shows it.
The Theory Details box `ES.Theory.ECCEQM.Localise` writes `localise true`; the
engine's own default is already true, so unticked input is unchanged and the
box only makes the request explicit.
The MOs table lists the highest orbital first (lowest at the bottom, as in an
MO diagram); the MO number column still says which orbital a row is.
