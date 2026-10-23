---
type: pitfall
title: "TS Search warnings (#468): end points at another level, and a straight path that cannot bend"
area: codereg
paths: ["src/apps/calced/CalcEd.C", "src/apps/calced/QstStructures.C", "include/tdat/TsPath.H", "src/tdat/chemistry/TsPath.C", "scripts/parsers/ai.orca", "scripts/codereg/orcaruntype.py", "tests/tspath/tspath_test.C", "tests/apps/calced_ts_warnings_test.py"]
issues: [468, 419, 464]
---
Two warnings under the TS Search pickers, never a block (Launch stays on).

- **Level.** The picked reactant/product calculations' theory, DFT functional
  and basis set (`tsLevelOfCalculation`) are compared with the editor's live
  values (`CalcEd::tsOwnLevel`). A component missing on either side is not
  compared, so an end point with no stored level never warns. The other
  calculations' levels are read when a pick changes; the own level is read on
  every `refreshTsBox`, so changing theory or basis updates the text.
- **Path.** `TsPath::check` interpolates reactant -> product (through the guess
  when one is picked) on the mapped, aligned structures that `nebMatch` /
  `qstMatch` already compute. All structures linear, or any pair closer than
  0.5 A on the way, warns. Fewer than 3 atoms are never "linear". The fixture
  IRC end points in `calced_qst_test.py` are not linear; the test builds its
  own linear HCN/HNC (`makeProject(linear=True)`).

ORCA `%neb`: `Preopt_Ends TRUE` re-optimises reactant and product at the NEB
level (ORCA 6.1.1 reads it: "Before the actual NEB run reactant and product are
preoptimized."). The Runtype Details choice `ES.Runtype.NEB.PreoptEnds` is
Automatic/Yes/No. Automatic reads `ES.Runtype.NEB.LevelsDiffer`, which
`syncTsKeys()` writes on every Save from the same comparison; a plain checkbox
could not follow the levels once the dialog had stored its default.
