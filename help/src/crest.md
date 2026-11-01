# CREST: conformer search

CREST is the code button for the `crest` program of the Grimme group. It
searches the conformations of a molecule (or a cluster of molecules) with
tight-binding methods and returns the ensemble of low-energy structures.
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
  choice for a first look at a large or floppy system.
- **GFN2//GFN-FF**: the conformers are searched with GFN-FF and then
  re-optimised and ranked with GFN2-xTB. CREST 3.0.2 offers this only in
  its older (`--legacy`) search driver, which the input line then names; that
  driver takes longer than the default one.

The charge and the multiplicity are those of the calculation. There is no
basis set.

**Theory Details** has **Implicit Solvent**: None (the default), ALPB or GBSA,
and the solvent. The list is that of ALPB; GBSA has parameters for fewer
solvents, and a deck written with a solvent GBSA lacks is refused with a
message when the input is generated.

## Run type: Conformer Search

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

## Results

The job script adds the ensemble (`crest_conformers.xyz`, lowest first) and
the best structure (`crest_best.xyz`) to the end of the output file, between
lines beginning with `ECCE-CREST-FRAME` and `ECCE-CREST-BEST`.

In the Builder, the **Geometry Trace** shows each conformer as a frame, with
the plot **Relative Conformer Energy** in kcal/mol: the first frame is the
lowest and has 0. These energies compare conformers within the one method of
this calculation only. They cannot be compared with the energy from another
method, or from another run with a different window or solvent.

The Progress box counts the metadynamics runs of the search.
