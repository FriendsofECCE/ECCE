# Re-Parse Outputs

**Run Mgmt > Re-Parse Outputs** in the Organizer runs the parsers of the
calculation's code again over the output files stored with the calculation.
It needs no login to the compute machine, and nothing is copied from it. Use
it after a parser has been corrected, or when a calculation shows fewer
properties than its output contains.

## Re-parsing

1. Select one or more finished calculations: in a project's table (with the
   Ctrl key held), or in the tree.
2. Choose **Run Mgmt > Re-Parse Outputs**. The same item is in the right-click
   menu of a calculation. It is available for a calculation that is
   **Completed**, **Failed**, **Killed** or otherwise ended.
3. The Organizer starts the parsing in the background. The properties appear
   as each calculation finishes. The message area at the bottom of the
   Organizer reads "Re-parsing 3 calculations..." while it runs and
   "Re-parse finished: 3 done" when the last one has ended; a calculation
   that could not be parsed is named there, and its run log says why. When
   several calculations are selected and copying their outputs takes a
   moment, a progress window is shown.

## What changes

- The properties (energies, geometries, orbitals, vibrations and so on) are
  replaced by the ones the parsers find now. A property that the parsers no
  longer find is removed.
- The Progress box record is made again from the stored output.
- The run state, the start and completion dates, the reason a run was
  incomplete and the run log stay as they were.

## When it cannot be used

- A calculation that is **Submitted**, **Running** or **Waiting for Login**
  is skipped. Use **Run Mgmt > Reconnect** for it.
- GROMACS and NWChem MD calculations are skipped: their results come from
  trajectory and topology files as well as the output. Use **Run Mgmt >
  Reconnect** for them.
- A calculation without a stored output file is skipped. Only the output files
  stored with the calculation are read. Files that were never brought back from
  the compute machine cannot be parsed.
