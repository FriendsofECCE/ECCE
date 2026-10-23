# GROMACS MD: run settings

The task editors of a GROMACS MD study (Optimize, Equilibrate, Dynamics)
write the run-control file, the `.mdp`, from their pages. The **Run settings**
page holds what the other pages have no field for. Every setting starts off,
and a task that leaves them alone gets the same `.mdp` as before.

## Defines and position restraints

- **Defines:** flags for the topology's preprocessor, for example
  `-DFLEXIBLE -DOPC3_IOD_LM`. They are written as the `define` line.
- **Restrain positions:** adds `-D<name>` to the defines, unless they already
  hold it. The name is `POSRES` until you change it (a topology may call its
  restraints `POSRES_noH`, for instance); the topology has to use that name
  around its `[ position_restraints ]`. When constant pressure is on as well,
  `refcoord-scaling = com` is written, which GROMACS asks for with restraints
  and a barostat.

Every task of the study has its own settings, so the restraints of a
minimisation are not carried into a later run unless you set them there too.

## Groups by residue name

Centre-of-mass removal and the thermostat can act on groups of the system
instead of on all of it. The page is for Equilibrate and Dynamics tasks.

1. Write one group per line as `name = residue names`, for example

   ```
   MMT = MMT
   Water_and_ions = SOL Cu
   ```

   or press **One group per residue name**, which makes a group of each
   residue name in the starting structure of the task, or **One group per
   molecule (chains)**, which makes a group of each molecule.
2. Choose **System** or **Defined groups** for the centre-of-mass removal
   groups and for the thermostat groups. Each is chosen on its own.

With the thermostat on the defined groups, the relaxation time and the
temperature of the Temperature page are used for every group. The groups of a
thermostat have to cover all atoms of the system, or GROMACS refuses the run.

ECCE writes the index file for the groups from the starting structure when
the task is launched, and sends it with the job. If you attach an index file
(`.ndx`) on the Inputs page, it is used as it is: the names of its groups are
the ones you write in the choices, and ECCE writes no index file. A residue
name that is not in the structure stops the launch, and the message lists
the names that are.

## Ensemble

At the top of the Dynamics page, **Ensemble** chooses what the run keeps
constant:

- **NVE:** number of atoms, volume and energy. No thermostat and no barostat.
- **NVT:** number of atoms, volume and temperature. The thermostat holds the
  temperature; the Temperature tab gives the value and the relaxation time.
- **NPT:** number of atoms, pressure and temperature. The thermostat and the
  barostat hold them and the box changes size; the Pressure tab gives the
  pressure and the compressibility.

NPT keeps the thermostat on, since pressure coupling without temperature
coupling is not a useful ensemble. A calculation made earlier opens with the
ensemble its settings amount to.

The usual sequence for a solvated system is: minimise; NVT with position
restraints on the solute, to bring it to the temperature; NPT with position
restraints, to bring the density to equilibrium; then production, usually
NPT without restraints.

## Pressure coupling

On an Equilibrate or Dynamics task with constant pressure on (Pressure tab),
choose:

- **Barostat:** C-rescale (the default), Berendsen or Parrinello-Rahman.
- **Coupling type:** isotropic (the default), semiisotropic or anisotropic.
  The pressure and the compressibility of the Pressure tab are repeated for
  each direction the type has: `P P` for semiisotropic, and `P P P 0 0 0` for
  anisotropic, where the last three are the off-diagonal terms.
- **Coupling interval (steps):** how often the pressure is coupled. Leave it
  blank for GROMACS's own choice.

GROMACS 2025 does not take C-rescale with anisotropic coupling; the page says
so and leaves the choice to you. It takes Berendsen, but warns that the
barostat does not give a correct ensemble; ECCE lets that warning pass, so
use Berendsen for equilibration only.

## Extra .mdp lines

Anything else goes in the last box, one `key = value` per line, for example
`DispCorr = EnerPres`, `fourierspacing = 0.16`, `verlet-buffer-tolerance =
0.005`, `rlist = 1.0` or `nstxout = 0`. The lines are written at the end of
the `.mdp`. A key you write replaces the line ECCE would write for it, with
`-` and `_` and capital letters taken as the same, so there is never a second
line for one key.
