# Organometallic complexes: metal-π bonds

This page shows how to build a complex in the Builder when a ligand binds
through a π system (cyclopentadienyl, benzene, allyl, an alkene) as well as
when it binds through a lone pair.

## Nubs

A nub is an open bonding site, drawn as a short stub on an atom. Each atom
has as many nubs as its geometry leaves free: a tetrahedral Fe has four, a
linear Fe two. **Add H** fills every free nub with a hydrogen, including the
nubs on a metal, where it gives an M-H bond. Before you add hydrogens, bond
the ligands you want, or delete the nubs you do not want (see
[Spare nubs](#spare-nubs)).

## σ ligands

The ligands in the **Ligands** folder of the Structure Library have a nub on
each donor atom. A monodentate ligand such as PPh3, CO or NH3 has one. A
chelate such as bipyridine has one on each donor. Cyclopentadienyl has
none: it is a π ligand, bonded as described below.

1. Add the metal with the Atom tool and a geometry (for example Fe,
   tetrahedral).
2. Choose **Mode > Add Structure**, select the ligand in the Structure
   Library, select a nub on the metal and then the ligand's nub, and click.

You can also bond two nubs in Bond mode with the ordinary bond orders. A
ligand that binds through one atom (η1) is an ordinary σ bond, drawn with the
**Single** order.

## π ligands: the Metal order

Choose the **Metal** bond order and Bond mode. Click the metal (the atom or
one of its nubs) and then any atom of the ligand. The order of the two
clicks does not matter.

ECCE bonds the whole π system that contains the ligand atom you clicked:

| Ligand | Bonded atoms |
|---|---|
| Cyclopentadienyl | five ring carbons (η5) |
| Benzene and other flat rings of three to seven atoms | the ring (η6 for benzene) |
| An alkene or alkyne | the two carbons (η2) |

The ligand is moved over the metal's free nub that lies nearest to it, with
the ring plane (or the C=C axis) at right angles to that direction and at a
typical distance: about 1.7 Å from the metal to a ring's centre and 2.0 Å
to the middle of a C=C bond. That nub, and the ligand's own nubs on the
bonded atoms, are removed, so **Add H** does not put hydrogens there. If the
metal has no free nub, the atoms are bonded where they are and the ligand
does not move.

For a partial hapticity, or for a chain such as allyl (η3), select the two or
more ligand atoms first (for example the three carbons of an allyl). Only the selected atoms are bonded. A single
selected atom counts as no selection: for an η1 contact use the Single order
instead.

## Examples

**Piano stool.** Add a tetrahedral Fe. Attach three PPh3 from the Ligands
folder to three of its nubs. Choose the Metal order and click the Fe and a
carbon of a benzene: the benzene goes onto the fourth nub. The neutral
Fe(η6-C6H6)(PPh3)3 has a 20-electron count and is a geometry exercise;
(η6-C6H6)Cr(CO)3 is the 18-electron classic.

**Ferrocene.** Add a linear Fe, which has two nubs. Choose the Metal order
and click the Fe and a carbon of a cyclopentadienyl from the Ligands folder,
then do the same with a second one. Each ring is placed over one nub.

**Fe(CO)4(η2-ethene).** Add a trigonal-bipyramidal Fe, attach four CO and
click the Fe and a carbon of ethene with the Metal order. The ethene goes on
the remaining equatorial nub.

## Spare nubs

A nub that is left over is an open site. To remove one, select it and use
**Edit > Delete Selection** or the Delete key. If you leave it, **Add H**
turns it into an M-H bond, and the Calculation Editor does not generate an
input while nubs remain.

## Clean

**Clean** tidies the starting geometry and keeps a π complex intact: it does
not bend the ring towards the metal. The real geometry comes from an
optimisation, for example with ECCE-QM or ORCA.

## Structure Library mode

After you insert a structure from the Structure Library, the Builder stays in
Structure Library mode, so you can add a second ligand straight away. The
message line names the mode. A click in empty space puts the structure
beside the atoms already there. Press Esc to return to Select mode.
