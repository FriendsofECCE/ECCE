# Recipes

A recipe makes the boxes of a [workflow](workflows.md) for a kind of study,
already linked, with the run types and settings the study needs. ECCE ships
six: **TS search with IRC**, **Conformers**, **Protonation sites**,
**Tautomers**, **Solvation shell** and **xTB first, then conformers**. A site can add its own, and
any workflow can be saved as a recipe.

- **Conformers**: a CREST conformer search, then xTB, then a DFT optimisation with frequencies.
- **Protonation sites**: CREST finds the protonation sites, then xTB and DFT refine the lowest protomer, with the charge raised by one.
- **Tautomers**: CREST finds the tautomers, then xTB and DFT refine the lowest one.
- **Solvation shell**: CREST grows a shell of solvent molecules (water, ten, by default) around the solute, then xTB and DFT refine the whole cluster with the same implicit solvent.
- **xTB first, then conformers**: an xTB optimisation of a rough structure, then a CREST conformer search from it, then DFT.
- **TS search with IRC**: reactant and product, a TS search with frequencies, then the IRC.

## Apply Recipe

1. Open a workflow. An empty workflow shows **Apply Recipe...** and
   **New Step...** in the middle of the view. A workflow that has boxes shows
   **Apply Recipe...** beside **Launch Ready Steps**.
2. To add the recipe after a box, select that box first. Otherwise the recipe
   starts a new chain.
3. Click **Apply Recipe...**. The set-up panel opens on the left of the view
   and a preview of the boxes is drawn on the right. The preview changes as
   you choose.
4. Choose the settings below, then click **Create boxes**. The panel closes
   and the boxes keep the places they had in the preview.

**Cancel** closes the panel and makes nothing. **Help** opens this page.

## The panel

**1 Recipe.** The list has one row per recipe with a one-line summary; the
text under it describes the selected one. A recipe from a site directory is
marked (site).

**Code.** A recipe can come in variants, for example Gaussian 16 (QST3) or
ORCA (NEB-TS) for the TS search. Choose one with its button. A code that this
installation does not have is greyed.

**2 Level of theory.** One row per role, for example "Optimisations, TS
search, frequencies, IRC". Each row has a **Method** and a **Basis set** box, both
editable, and a green line **Why** giving the reason for the recommendation.
The full role label wraps over several lines when needed. **Applies to:** lists
which boxes that row sets, for the chosen code variant.
The recommendation is the recipe's default, changed by its rules:

- an anion gets a basis with diffuse functions;
- an element beyond Kr gets a def2 basis, which carries an effective core
  potential;
- a structure of more than 60 atoms gets a note to consider the RI
  approximation or a smaller basis.

The rules read the selected box when the recipe is applied after it; for a new
chain there is no structure yet and the recipe's defaults are shown. The
rules only warn: any method and basis you type is accepted.

**3 Where the new steps go** offers **Start a new workflow** or
**Continue after the selected box "&lt;name&gt;"**. This section is hidden when no
box is selected, since only a new workflow is possible then. Right-click a
box and choose
**Apply Recipe from here...** to open the panel with that box already
selected as the box to apply after. The first boxes of a new chain that
need a structure are drawn in amber and marked "needs structure". After a
box, the first boxes start from that box's geometry, charge and
multiplicity.

## Solvent

Every recipe has one **Solvent** row, including recipes supplied by a site or
written by hand. It applies to every step. Choose **none** for gas phase, or
choose a solvent once for the whole chain. The list offers shared solvents
accepted by the supported codes in the chosen variant. Changing the variant
keeps the choice if available; otherwise it returns to **none**.

The row names the model used by each code: CREST and xTB use ALPB, ORCA uses
CPCM, and Gaussian 16 uses PCM. ECCE-QM uses its own CPCM support. A code
without a recipe solvent mapping is noted in the row; set its solvent in
that box. Every box remains editable after creation.

## After Create boxes

The boxes are ordinary steps and can be edited like any calculation.

- A box marked **needs structure** has no structure yet. Click it and build
  or import the structure in the Builder.
- The other boxes take the geometry, charge and multiplicity of their source
  when you choose **Launch Ready Steps** after the source is Complete.
- The method and basis chosen in the panel are kept with each box. For
  Gaussian 16, ORCA and ECCE-QM the method is set as the box's functional (HF as its
  SCF theory) when one of the editor's functionals has that name; other
  codes keep their own default. A box gets its basis set when it gets its
  structure, as a basis set is chosen per element.
- When a box's structure is saved in the Builder, or taken from its source,
  the recipe's rules run again with that structure: an anion gets diffuse
  functions, an element beyond Kr a def2 basis. A level you typed in the
  panel is kept; only the recommended one changes.
- The Calculation Editor shows the other settings of the recipe's run type.

## Key data

A workflow made from a recipe ends with a **Key data** box to the right of its
last column. It lists the energy of each step that is Complete (and G where
the recipe asks for it), the reaction energy E(product) - E(reactant), the
barrier E(TS) - E(reactant), and the free-energy values when the steps have
frequencies. The last line says how many steps are complete; a step that is
not shows "waiting", and the box updates when the step completes.

The box shows numbers only when every Complete step uses one code and one
level of theory (method, basis and solvent). Otherwise it says so and shows
none: energies of different methods are never compared.

## Save Workflow as Recipe

Click **Save as Recipe...** beside **Apply Recipe...** and give the recipe a
title. The workflow's steps, run types, settings and links
are kept, its structures and results are not. The recipe is written to the
`Recipes` folder of your ECCE configuration and appears in the list the next
time the panel opens.

## Your own and your site's recipes

Recipes are text files, read from your own Recipes folder, then the site's,
then the installed ones. See [Writing a recipe](writing-a-recipe.md).
