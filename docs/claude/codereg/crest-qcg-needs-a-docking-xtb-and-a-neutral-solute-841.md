---
type: pitfall
title: "CREST QCG: only an xtb whose `dock` works, and a charged solute is grown as the neutral species (#841)"
area: codereg
paths: [scripts/parsers/ai.crest, scripts/gensub, data/client/recipes/solvation-shell.recipe, data/client/recipes/protonation-sites.recipe]
issues: [841, 758, 809]
---
Found while building the workflow examples (CREST 3.0.2):

- QCG calls `xtb dock`, which dies in its final optimisation (SIGSEGV in
  `axis_trafo.f90`; Debian's build: stack smashing) in both Debian's xtb
  6.7.1 and the 6.7.1 release static build, for any pair of molecules
  (water + water too). CREST then stops with "error while reading input
  coordinates" in `rdxmol` from `qcg_grow`. The upstream bleeding-edge
  build (6.7.1 e20416b, 2026-11-02) docks.
- With `--chrg 1` the cluster energies in `grow/qcg_energy.dat` are those of
  the neutral cluster without ALPB (checked: the Na+ + water cluster's
  energy equals an xtb single point with charge 0, gas phase). An ion is
  therefore no QCG solute; the example uses urea.
- `--protonate` keeps protomers within 30 kcal/mol (GFN2, gas phase). For
  4-aminopyridine that is only the ring N (the amine N is at 38.8, ring C at
  36.4), so Protonation Sites and Tautomers have an Energy window
  (`ES.Runtype.Sites.EnergyWindow`, `--ewin`) and the protonation-sites
  recipe sets 50.
