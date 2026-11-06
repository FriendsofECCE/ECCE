# CREST: conformer search and related run types

CREST is the code button for the `crest` program of the Grimme group. It
searches the conformations of a molecule (or a cluster of molecules) with
tight-binding methods and returns the ensemble of low-energy structures. It
also finds the protonation sites and the tautomers of a molecule, and grows a
shell of solvent molecules around it.
Typical uses are the starting structures for a DFT calculation, and the
arrangements of a flexible molecule or an ion with its water molecules.

CREST is not in the Debian archive; its makers publish a ready-built Linux
program on the CREST releases page on GitHub (`crest-lab/crest`). Unpack it
and copy `crest` to `/usr/local/bin`; the steps are in
[Running codes on Linux](running-codes-on-linux.md#crest).

CREST runs `xtb` itself, so both programs are needed. On the **Codes** tab of
the machine in **Register Machines**, register **xTB** (see [xTB](xtb.md)) and
**CREST**. For CREST set **Program** to the full path of the program, such as
`/usr/local/bin/crest`; **Find** looks for it. A CREST job uses the xTB path
registered for the same machine. The CREST button in the code bar, next to
xTB, starts a calculation with this code.

CREST must start from an optimised structure; from an unoptimised one the job
fails. Optimise with xTB first (**New step > xTB**, run type Geometry) and start
CREST from that result. The recipes and **New step > CREST** on a box that is
not an optimisation add this xTB step for you, and Launch warns when a CREST
calculation does not follow an optimisation (**Launch anyway** goes on).

## Theory

The calculation's **Theory** is the method used for the search:

- **GFN2-xTB**, the default.
- **GFN-FF**, a force field of the same family; much faster, and a good
  choice for a first look at a large or floppy system. It is not offered for
  Protonation Sites or Tautomers, which CREST places from localised orbitals
  that a force field does not have.
- **GFN2//GFN-FF**: the conformers are searched with GFN-FF and then
  re-optimised and ranked with GFN2-xTB. CREST 3.0.2 offers this only in
  its older (`--legacy`) search driver, which the input line then names; that
  driver takes longer than the default one. It is offered for the conformer
  search only.

The charge and the multiplicity are those of the calculation. There is no
basis set.

**Theory Details** has **Implicit Solvent**: None (the default), ALPB or GBSA,
and the solvent. The list is that of ALPB; GBSA has parameters for fewer
solvents, and a deck written with a solvent GBSA lacks is refused with a
message when the input is generated.

## Run types

- **Conformer Search**: described below.
- **Protonation Sites**: CREST's `--protonate`. It adds a proton at each
  basic site of the molecule and returns the protonated structures, lowest
  first. The charge of the calculation is the charge of the molecule before
  protonation. GFN2-xTB only. A molecule with a single basic site gives an
  ensemble of one structure.
- **Tautomers**: CREST's `--tautomerize`. It moves protons between the sites
  of the molecule and returns the tautomers, lowest first. GFN2-xTB only.
  Neither of these two run types has settings of its own.
- **Solvation Shell**: CREST's quantum cluster growth (`--qcg`). It adds
  solvent molecules one at a time around the molecule of the calculation.
  **Runtype Details** has the **Solvent molecule** (water, methanol or
  acetonitrile), the **Number of solvent molecules** (10 by default) and
  **Ensemble of clusters**. The solvent is chosen once: Theory Details shows
  the same solvent, greyed, and its **Model** is ALPB by default (None gives
  a cluster without implicit solvent), so the deck has `--alpb <solvent>`.
  The solvent has to have ALPB parameters, which is why ammonia is not on the
  list. **Ensemble of clusters** (off by default) adds `--ensemble`, which
  samples the arrangements of the finished cluster. CREST 3.0.2 crashes in
  that step, so it is off; without it the result is the grown cluster, one
  structure with its energy. CREST
  docks each solvent molecule with `xtb dock`, which aborts in xtb 6.7.1
  (Debian 13's package and the 6.7.1 release alike); the job then ends without
  a result, and the output file shows CREST's own error. A newer xtb (the
  developers' current build) docks; register its path as the machine's xTB,
  which CREST uses. The same holds for xTB's own Docking run type.

## Conformer Search

**Runtype Details** has:

- **Energy window** (kcal/mol, 6 by default): conformers higher than this
  above the lowest are not kept. A larger window keeps more conformers.
- **Non-covalent complex (NCI mode)**: CREST's `--nci`. For a complex held
  together by non-covalent contacts, such as a host and a guest, it keeps the
  parts together during the search.
- **Skip the reference-topology check**: CREST's `--noreftopo`. CREST
  normally discards a structure whose bonding differs from the input, and it
  reads contacts such as Na-O in an ion cluster as bonds, so most structures of
  such a cluster can be thrown away. Tick this to keep them.

The number of cores is the processor count chosen in the Launcher. A search
takes from seconds for a small molecule with GFN-FF to hours for a large one.

## The input file

The input is a single file. Its first line is the command line for CREST,
beginning with `crest`, and the rest is the geometry in xyz form:

    crest --gfnff --chrg 0 --uhf 0 --ewin 6
    4
    methane
    C ...

`--chrg` is the charge and `--uhf` the number of unpaired electrons, the
multiplicity minus one. Anything else CREST accepts can be added to the first
line by hand; the input check looks only at the shape of the file and at the
charge and multiplicity.

For the other run types the first line has `--protonate` or `--tautomerize`
instead of the energy window, or `--qcg solvent.xyz --nsolv <n>` (with `--ensemble` if chosen).
A solvation-shell deck also has the solvent molecule below the first line,
between `$solvent` and `$end`; the job script writes it to `solvent.xyz`.

## Results

The job script adds the ensemble to the end of the output file, between lines
beginning with `ECCE-CREST-FRAME` (and, for a conformer search, the best
structure between `ECCE-CREST-BEST` lines). The ensemble is
`crest_conformers.xyz` for a conformer search, `protonated.xyz` for the
protonation sites, `tautomers.xyz` for the tautomers and CREST's final
ensemble for the solvation shell; each is lowest first.

In the Builder, the ensemble is a Properties panel named for the run type:
**Conformers** (conformer search), **Protonation Sites**, **Tautomers** or
**Clusters** (solvation shell). It lists one row per structure, lowest first:
the rank, the energy relative to the lowest in kcal/mol, the Boltzmann
population in percent at 298.15 K, and, for an ensemble around an ion, the
coordination pattern (the donor atoms in the first shell). Click a column
heading to sort, and a row to show that structure in the 3-D view. Below the
table, a plot of the energy against the rank (lowest first) marks the chosen
structure; clicking a point selects its row. **Export .csv** saves the table.

Select several rows with Ctrl or Shift and choose **New Calculation from
Selected...** to set up one calculation per structure: it asks once for the
code and creates each with that structure, the Geometry Optimization run type
and the charge and multiplicity of this calculation. If this calculation is a
step of a workflow, they become steps linked from it; otherwise they are
calculations beside it, named `<calculation>_conf<rank>`.

### Superposing selected members

Ctrl-click or Shift-click several rows to compare their structures in the
3-D view. Each member is fitted onto the **first selected member**, using a
proper rotation (Kabsch) over all heavy atoms, with equal weights. Atoms are
matched by their stored indices: equivalent atoms and solvent molecules are
not permuted. An ensemble containing only hydrogen is fitted over all atoms.
The key below the table lists each rank, its colour and its heavy-atom RMSD
to the reference in Å. The first six colours are the shared plot palette;
larger selections use lightness variants. Sorting preserves the selection
and its reference. Selecting one row restores the normal structure view;
closing the panel or unticking **Show in viewer** removes the overlay.
**Export .csv** also includes the selected members' reference rank, RMSD and
colour while a multiple selection is active.

### Structure thumbnails and table pictures

The **Structure** column contains off-screen rendered ball-and-line pictures,
with atoms coloured by element and inferred covalent bonds. Pictures use the
viewer's orientation when thumbnail generation starts, fitted to each member.
They are generated in small batches and cached in memory for this ensemble,
so scrolling does not render them again. A property refresh regenerates the
pictures; no additional files are stored with the calculation.
**Export .png...** writes the whole table, including every thumbnail and rows
outside the visible scroll area.

The Builder scene-script hooks are `gtenselect <rank>...` (selection order
sets the reference), `gtenfit` (members, RMSDs, colours and overlay visibility),
`gtenthumbs` (cached ranks), `gtenpng <file>` and `gtencsv <file>`.
`gtenviewer on|off` operates the **Show in viewer** checkbox.
`gtensort <column> up|down` sorts rank (0), energy (1), population (2) or
pattern (3). These run through the existing `ECCE_VIEWER_SCENE` interface.

The energies and populations compare the structures within the one method of
this calculation only. They cannot be compared with the energy from another
method, or from another run with a different window or solvent, and the
population is a weight of the energies shown, not a measured one.

The same patterns, grouped, with how many structures have each and the lowest
relative energy and rank of each, are printed by
`ecce-contacts crest_conformers.xyz --group`.

While a search runs, the Progress box shows the stage (metadynamics
sampling, the multilevel optimisation and its crude and tight passes), how
many metadynamics runs are done of the total, how many structures were
optimised and how many remain within the energy window, the lowest energy so
far in hartree and, once CREST has timed a trial run, the estimated time of a
batch of metadynamics runs. The metadynamics runs go in parallel and finish in
any order, so the count is of the runs finished, not the number of the last
one. In a workflow, the CREST box says how many conformers (protonation sites,
tautomers or clusters) the finished search kept, within the energy window
of a Conformer Search.
