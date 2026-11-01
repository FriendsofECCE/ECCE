---
type: map
title: "CREST is registered like xtb: a two-part deck split by gensub, the ensemble appended to the output, relative energies as CONFREL (#682)"
area: codereg
section: "Getting a calculation set up (the \"code registration\" system)"
paths: ["data/client/cap/CREST.edml", "scripts/parsers/ai.crest", "scripts/parsers/crest.desc", "scripts/parsers/crest.conformers", "scripts/gensub", "scripts/codereg/cresttheory.py", "tests/launch/crest_test.py"]
issues: [682]
---
Application type `CREST` (resource `crest_es`, CONFIG key `crest`, gensub sub
`crest`), Semi-empirical group next to xTB. One run type, `Conformer Search`;
theories GFN2-xTB, GFN-FF, GFN2//GFN-FF.

- **Deck = `crest.crestin`**: line 1 `crest <options>`, then an xyz file, split
  by gensub exactly as for xtb. Options: method, `--chrg`, `--uhf`, ALPB/GBSA,
  `--ewin`, `--nci`, `--noreftopo`; `-T` is the Launcher's processor count.
- **xtb path.** CREST calls xtb, so gensub passes `--xnam` from the `xtb` key
  of the same machine's CONFIG and refuses to write the script without it.
- **GFN2//GFN-FF needs `--legacy`.** CREST 3.0.2 answers "option not yet
  available with new calculator" otherwise (checked by running it).
- **The ensemble is a file, not output.** gensub appends `crest_conformers.xyz`
  between `ECCE-CREST-FRAME` markers, the comment line rewritten to
  `energy: <Eh> relative: <kcal/mol>` (relative to the lowest, computed in the
  awk because the parser sees one frame at a time), then `crest_best.xyz`.
  `crest.conformers` emits GEOMTRACE and the new `CONFREL` (class NA, kcal/mol,
  `PropTSVector<Geometry Step>`) from the same frame. No TEVEC: the geometry
  trace panel opens on TEVEC when it exists, and a total energy is not what
  an ensemble is read by.
- A conformer search with one structure is a valid result (rigid molecule).
- Register Machines: `CodePrograms::names("CREST")`, `SchedulerQuery` signature
  `crest`, the built-in command text, and `crest` in the `Machines` lines.
