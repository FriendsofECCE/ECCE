# Writing a recipe

A recipe is a plain text file ending in `.recipe`. You can write one in any
text editor. ECCE reads recipes from three folders, in this order: your own
**Recipes** folder in your ECCE settings folder (made the first time you click
**Apply Recipe...**), the **Recipes** folder of the site configuration (the
central server's), and the recipes installed with ECCE. When two recipes have
the same `recipe:` name, the first one read wins, so yours replaces the site's
and the site's replaces the installed one. **Save as Recipe...** writes into
your folder.

A recipe that cannot be read is named in red under the recipe list, with its
file and line number; the other recipes still work.

## Syntax

One `key: value` per line. A value with several fields separates them with
`|`. A line starting with `#` is a comment, and empty lines are ignored.

| Line | Fields | Meaning |
|---|---|---|
| `recipe:` | name | identifies the recipe; no spaces |
| `title:` | text | the name in the list |
| `field:` | text | the area, for example Reactions |
| `summary:` | text | one line in the list |
| `description:` | text | the paragraph under the list |
| `role:` | id, label, method, basis, why | a level of theory chosen as one row; `why` is shown in green |
| `variant:` | id, label, code | a way to do the recipe; code is the label on the code bar (G16, ORCA, xtb, CREST, ECCE-QM, CP2K, QE, NWChem, MOPAC) |
| `step:` | variant, id, code, role, run type, caption, flags | a box; role is a `role:` id or empty |
| `set:` | variant, step, key, value | an extra setting (an ES key) of the step |
| `link:` | variant, from, to, name | an arrow; name is `geometry`, `reactant`, `product` or `tsguess`, optionally followed by `.last`, `.max` or `.N` |
| `rules:` | names | any of `anion-diffuse`, `heavy-ecp`, `size`, `registered`; none given means all |

Flags of a step, separated by commas: `structure` (the box needs a structure),
`thermo` (the key data gives G for it), and `key=reactant`, `key=product`,
`key=ts`, `key=min` or `key=final` (its place in the key data).
`charge=+1` (or `charge=-1`, `charge=+2`...) adds that number to the charge the
box takes from its source. Use it on the first box after a CREST
**Protonation Sites** step: CREST's structures are cations, one charge above
the structure that went in. Later boxes copy the charge of the box before them,
so only the first box needs the flag.
The run type is the name the Calculation Editor uses, for example `Energy`,
`Geometry`, `GeoVib` (optimisation and frequencies), `IRC`,
`Transition State (QST) + Frequencies` or `Conformer Search`. List the steps
in the order they depend on each other: a step comes after the steps that
link to it.

## A complete example

```
# Optimise with xtb, then refine with ORCA and take the energy.
recipe: xtb-orca
title: xtb then ORCA
field: Structures
summary: xtb optimisation, then an ORCA optimisation with frequencies
description: A cheap optimisation first, then DFT from its geometry.
role: dft | ORCA optimisation | PBE | def2-SVP | a cheap functional for a first look
rules: anion-diffuse, heavy-ecp, size

variant: main | xtb, then ORCA | ORCA
step: main | first | xtb | | GeoVib | xtb Opt + Freq | structure
step: main | second | ORCA | dft | GeoVib | ORCA Opt + Freq | key=final, thermo
link: main | first | second | geometry
```

Save it as `xtb-orca.recipe` in your Recipes folder and click **Apply
Recipe...** to see it in the list.
