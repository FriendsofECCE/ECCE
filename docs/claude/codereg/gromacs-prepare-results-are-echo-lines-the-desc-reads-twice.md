---
type: pitfall
title: "GROMACS Prepare results: ECCE-PREPARE lines in the output file feed both the Progress box and the properties; the script is chosen by the first key and must print every key"
area: codereg
paths: [scripts/gensub, scripts/parsers/gromacs.desc, scripts/parsers/gromacs.prepare, src/util/genutil/JobProgress.C, src/comm/commxt/JobParser.C, data/client/config/properties, tests/launch/gromacs_prepare_test.py, tests/progress/progress_test.C]
issues: [930]
---
`gensub` (gromacsPrepare) writes `ECCE-PREPARE step k/4 <tool>`, then after
the last step two result lines (`system atoms= residues= waters= pos= neg=
posname= negname=`, `box charge0= charge1= box= a= b= c= alpha= beta= gamma=`)
and `ECCE-PREPARE done` into the output file. `gromacs.desc` reads them twice:

- **Progress.** `[PROGRESS]` entries make stage `prepare` (`point=k/4 tool=`,
  then the result fields, `done=yes`). A `[PROGRESS]` entry has only `$1..$9`;
  the box line uses all nine, so the system line is a second entry. The tool
  names are gmx's (`editconf`, `genion`); `JobProgress` shows them as *box*
  and *ions*. The bar counts finished steps, so it is full only at `done=yes`.
  `eta` is ignored for this stage (it would be the time left of one step).
- **Properties.** `[NATOMS][NRESIDUES]...[BOXGAMMA]` with `Script=
  gromacs.prepare`, `Lines=2`. **`JobParser::storeProperty` finds the script
  through the first key only and runs it once; the key it passes is not
  the one being stored.** A script that prints just `$key` stores one
  property and silently drops the other twelve (the first version did, and
  `props` showed none of them). Print every key on the one call, as
  `gromacs.energy` does. The properties are class `Process`, so the Viewer
  lists them under Run Statistics (`KeyValuePropertyPanel`, `type="value"`);
  a string (`PropString`) would not be listed there, hence the box type is
  only in the Progress summary.
- **The numbers are gmx's.** Atoms from the final `.gro`, residues from
  pdb2gmx's "Now there are N residues", waters/ions from the topology's
  `[ molecules ]`, charge from grompp's "non-zero total charge" note (none =
  0; a second grompp on the final system gives the charge after), box from
  editconf's `new box vectors/angles` (it prints lengths and angles, not the
  triclinic matrix).
- **The job log is appended to the output file**, so every `ECCE-PREPARE`
  line is in the file twice. Harmless for the stored snapshot and the
  properties (`Frequency=last`), but a test that counts steps must use a set.
