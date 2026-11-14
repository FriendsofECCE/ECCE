# A GROMACS run from your own files

A GROMACS study (see [Your first GROMACS calculation](gromacs-first-calculation.md))
builds its inputs for you. If you already have a run set up, with your own
`.mdp`, structure and topology, make it as one calculation instead.

## Make the run

1. Select a project in the Organizer.
2. Choose **File > New GROMACS Run from Files...**.
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

The Dynamics editor is not meant for these runs: it writes a new `.mdp` and
would replace yours. Change the run by making a new one from the changed file.
