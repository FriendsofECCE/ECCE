# A GROMACS calculation from your own files

A GROMACS study (see [Your first GROMACS calculation](gromacs-first-calculation.md))
builds its inputs for you. If you already have a run set up, with your own
`.mdp`, structure and topology, make it as one GROMACS calculation instead.
It is a calculation like an ORCA or xTB one: it can be rerun, continued from
its final frame, and its structure can go to and come from other codes.

## Make the calculation

1. Select a project in the Organizer.
2. Choose **File > New > GROMACS Calculation from Files...**.
3. Choose the run input: a `.mdp` file, or a `.tpr` file that is ready to run.
4. For a `.mdp`, ECCE then asks for the starting structure (`.gro`) and the
   topology (`.top`). Each dialog opens in the folder of the file before it.

ECCE stores the files with the calculation. The `.mdp` is kept exactly as you
wrote it. The files your topology includes (`#include` lines, including a
force-field folder such as `min.ff/`) are attached when they lie beside the
topology, and so is an index file (`.ndx`) in the topology's folder. The
message line lists any include it could not find; GROMACS's own library
files are not stored, and grompp reads them on the machine that runs the job.

The calculation is named after the `.mdp` (or `.tpr`) file, with a number
added if the name is taken, and it is Ready to launch.

A structure in `.pdb` format is not accepted. Convert it first:

```
gmx editconf -f structure.pdb -o structure.gro
```

## Run it

Launch it as any calculation. The job runs `gmx grompp` and then `gmx mdrun`;
with a `.tpr` there is no grompp step, and the `.mdp` (if you gave one) is
only stored for reference. The energies, the trajectory and the final
structure are stored and shown as for a study.

There is no editor for this calculation: your `.mdp` is the setup. Change the
run by making a new calculation from the changed file.

## Make more calculations from it

Right-click the calculation and use **New from This**:

- **Duplicate**: a copy with the same files, Ready to launch.
- **Continue from Last Geometry** (also **Edit > Duplicate Setup with Last
  Geometry**): a copy whose starting `.gro` is the final frame of the run,
  velocities included. A calculation made from a `.tpr` has no `.gro`, so its
  copy runs the same `.tpr` again.
- **Continue in Another Code...** (in the calculation table): a calculation in another
  code, for example xTB or ORCA, that starts from the final frame.

The other way, from an xTB, ORCA or other calculation to GROMACS: select the
calculation, choose **New from This > Continue in Another Code...** and pick
**GROMACS (from files)**. ECCE asks for your `.mdp`, `.gro` and `.top` as
above, and the coordinates in the `.gro` are replaced by the calculation's
final structure. The atoms must be the same and in the same order as in the
`.gro`; if they are not, the calculation is not made and the message line
says which atom differs. The `.gro`'s box line is kept and its velocities are
dropped.
