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
  solvent molecules one at a time around the molecule of the calculation and
  then samples the arrangements of the cluster (`--ensemble`).
  **Runtype Details** has the **Solvent molecule** (water, ammonia, methanol or
  acetonitrile) and the **Number of solvent molecules** (10 by default). The
  implicit solvent of Theory Details is applied on top, if chosen. CREST
  docks each solvent molecule with `xtb dock`, which aborts in the xtb 6.7.1 of
  Debian 13 on the machine this was written on; where it does, the job ends
  without a result, and the output file shows CREST's own error.

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
instead of the energy window, or `--qcg solvent.xyz --nsolv <n> --ensemble`.
A solvation-shell deck also has the solvent molecule below the first line,
between `$solvent` and `$end`; the job script writes it to `solvent.xyz`.

## Results

The job script adds the ensemble to the end of the output file, between lines
beginning with `ECCE-CREST-FRAME` (and, for a conformer search, the best
structure between `ECCE-CREST-BEST` lines). The ensemble is
`crest_conformers.xyz` for a conformer search, `protonated.xyz` for the
protonation sites, `tautomers.xyz` for the tautomers and CREST's final
ensemble for the solvation shell; each is lowest first.

In the Builder, the **Geometry Trace** shows each structure as a frame, with
the plot **Relative Conformer Energy** in kcal/mol, against the number of the
conformer: the first frame is the lowest and has 0. These energies compare conformers within the one method of
this calculation only. They cannot be compared with the energy from another
method, or from another run with a different window or solvent.

The Progress box counts the metadynamics runs of the search.
