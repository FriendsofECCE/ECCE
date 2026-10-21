---
type: pitfall
title: "GROMACS COM pull: grompp wants a pbc atom for chain-sized groups, the distance cannot pass half the box, and 0.1 nm/ps drags a chain nowhere"
area: codereg
paths: [scripts/parsers/ai.gromacs, scripts/parsers/md.gmxtask, scripts/parsers/gromacs.pull, scripts/parsers/gromacs.desc, scripts/gensub, src/wxgui/mdtools/GromacsRunPanel.C, src/apps/builder/PullPanel.C, tests/gromacs_md/pull_test.py]
issues: [410]
---
- Settings live in `GromacsRun` (`GmxPull*` tags) -> `md.gmxtask`
  (`ES.Runtype.MD.Pull*`) -> `GMXPull` in `ai.gromacs`; dynamics tasks only.
  The two group names are index groups from the "Groups by residue name" box
  (`Chain_A = mol:Protein_chain_A`). A reference chain held in place is the
  existing Restrain positions with `POSRES_A`.
- grompp 2025.2 refuses a group larger than a quarter of the box without a
  hand-picked `pull-group1-pbcatom`. ECCE writes
  `pull-pbc-ref-prev-step-com = yes` instead (no atom number to choose;
  cosine weighting is no use, it allows one pull dimension only).
- With `geometry = distance` the distance is a minimum-image one: a pull that
  starts at d and runs at rate r for t ps stalls at half the shortest box
  vector, with the force still rising. Leave room in the box.
- 0.1 nm/ps is 100 m/s: Stokes drag on a 20-residue chain is then several
  hundred kJ/mol/nm and a 1000 kJ/mol/nm^2 spring lags by most of the
  reference's travel. The tutorial's 0.01 nm/ps is not a typo. The test uses
  0.03 nm/ps with k = 5000 and fits the slope over the second half only
  (the first 2 ps are the spring settling).
- pullx.xvg / pullf.xvg are joined by gensub into `ecce-parse-pull.txt`
  (a File= parse like the trajectory) and become PULLTIME, PULLDIST,
  PULLFORCE for the Pull Force panel.
