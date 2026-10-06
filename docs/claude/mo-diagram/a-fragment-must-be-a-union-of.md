---
type: map
title: "A fragment must be a union of orbits"
area: mo-diagram
section: "The MO correlation diagram (#132)"
paths: ["MoFragments.C"]
issues: [132]
---
**A fragment must be a union of orbits** — otherwise the group maps
it outside itself and it has no symmetry orbitals at all. This is
why the panel's chooser groups *orbits* rather than atoms, and why
ethene-as-two-CH2 needed a different mechanism entirely: a CH2
crosses D2h's orbits ("both carbons", "all four hydrogens"). The way
through is to analyse each half in the **subgroup** that preserves
it, then combine in and out of phase — `buildHalves()` /
`inducedIrreps()` in `MoFragments.C`. Which irreps a combination
spans is the induced representation (computed, and it declines
rather than rounding); which combination is in phase comes from the
sign of the character on a class that swaps the halves.
