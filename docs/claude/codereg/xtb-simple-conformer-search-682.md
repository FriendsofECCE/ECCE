---
type: map
title: "xTB run type 'Conformer search (simple)': ecce-confsearch is written into the job, prints the ensemble in CREST's frame format (#682)"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["scripts/ecce-confsearch", "scripts/gensub", "scripts/parsers/ai.xtb", "scripts/parsers/xtb.desc", "scripts/codereg/xtbruntype.py", "data/client/cap/xTB.edml", "tests/launch/confsearch_test.py"]
issues: [682]
---
- **Deck**: line 1 is `xtb <method> --chrg --uhf --opt normal --ecce-confsearch
  samples=N,seed=S,ewin=E,rmsd=R`. The last option is not xtb's: gensub strips
  it from `xtbopts` and, when it was there, runs `python3 ecce-confsearch.py`
  (cat'ed from `scripts/ecce-confsearch` into the run directory, as for
  ecce-mlp) instead of xtb. It keeps line 1 of the deck an ordinary xtb line,
  so the input check needs no change (an `$md`-style block below it would have
  broken the check's line-2 atom count).
- **Output**: the script prints `ECCE-XTB-FRAME` ... `ECCE-XTB-ENDFRAME`
  frames, comment `energy: <Eh> relative: <kcal/mol>`; `xtb.geomtrace` turns a
  comment with `relative:` into GEOMTRACE + CONFREL (no TEVEC), the same
  properties as CREST, so the Viewer, the pattern table and the best-N steps
  need nothing new. A second `[GEOMTRACE]` block in `xtb.desc` using
  `crest.conformers` does NOT work: the parser keys on the property, the later
  duplicate wins and the ensemble came out as xtb.geomtrace's TEVEC without
  CONFREL. gensub points the usual XTB-FRAME append at /dev/null in this mode
  (else the input geometry would follow the ensemble) and adds no `--molden`.
  The script ends with `normal termination of xtb`, gensub's success check.
- Needs `python3` (stdlib only) on the machine that runs the job; xtb samples
  run in parallel, one OMP thread each, up to the job's processor count.
- Mirror images (the two gauche butanes) are separate conformers: the RMSD is
  over heavy atoms, proper rotation only, no atom permutation. Samples that
  change the bond graph or fail to converge are dropped.
- GFN2 n-butane: gauche is 0.596 kcal/mol above anti.
