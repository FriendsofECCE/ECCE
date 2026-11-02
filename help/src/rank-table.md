# Rank calculations

Rank lists the calculations under a folder (isomers, ion placements,
substitution sites) from the lowest energy up, with each energy relative to
the lowest, in kJ/mol and eV.

1. In the Organizer, select a folder (or some calculations).
2. Right-click and choose **Rank...**.
3. Click a column heading to sort by it. **Export CSV...** and
   **Export LaTeX...** write the table to a file.

dE is the electronic energy and dG the stored Gibbs energy, each relative to
the lowest in the group. Only calculations with the same code, method, basis
and solvent are compared: one that differs is left out of dE and dG, and the
Flags column says why. Excluded calculations and calculations with no stored
energy are also left out. A different composition or a different cell is
flagged, but the difference is still shown; check that it is what you intend.

## From a terminal

`ecce-si-export --rank csv <folder>` prints the table (`latex` for LaTeX),
and writes each warning to the error output. Options:

- `--ref NAME` takes dE and dG relative to that calculation instead of the
  lowest; its method is then the one the others are compared with.
- `--per-fu N` (or `auto`, the greatest common divisor of the atom counts)
  and `--per-site N` add columns divided by the number of formula units or
  substituted atoms in the cell.
- `--substitution Si>Al` says that Al replaces Si in the series, so the
  composition check does not flag it.
- `--treatment rrho|qrrho-grimme:100|truhlar|head-gordon` takes G from
  `ecce-thermo` with that low-frequency treatment instead of the stored G.
- `--sort G` sorts on dG instead of dE.
- `--descriptor "name=EXPR"` (repeatable) adds a column from `ecce-contacts`,
  which looks at metal-donor shells only: `pattern` (the shell pattern),
  `cn:Ti` (donor atoms around the Ti atoms) or `dist:Ti-O` (the shortest
  Ti-O distance in angstrom).
