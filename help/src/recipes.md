# Recipes

A recipe makes the boxes of a [workflow](workflows.md) for a kind of study,
already linked, with the run types and settings the study needs. ECCE ships
seven: **TS search with IRC**, **Conformers**, **Protonation sites**,
**Tautomers**, **Solvation shell**, **xTB first, then conformers** and
**Fragment analysis**. A site can add its own, and any workflow can be saved
as a recipe.

- **Conformers**: an xTB optimisation (CREST needs an optimised structure, in every recipe that uses it), a CREST conformer search, then xTB, then a DFT optimisation with frequencies.
- **Protonation sites**: CREST finds the protonation sites, then xTB and DFT refine the lowest protomer, with the charge raised by one.
- **Tautomers**: CREST finds the tautomers, then xTB and DFT refine the lowest one.
- **Solvation shell**: CREST grows a shell of solvent molecules (water, ten, by default) around the solute, then xTB and DFT refine the whole cluster with the same implicit solvent.
- **xTB first, then conformers**: an xTB optimisation of a rough structure, then a CREST conformer search from it, then DFT.
- **TS search with IRC**: reactant and product, a TS search with frequencies, then the IRC.
- **Fragment analysis**: a single point of a complex and one of each of its
  fragments, all at the complex's geometry with the same method and basis set,
  for the [Fragment orbitals](#fragment-orbitals) box. ORCA only for now.

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

**3 Fragments** appears for the Fragment analysis recipe; see
[Fragment analysis](#fragment-analysis). The next section is then numbered 4.

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
  when you open their editor after the source is Complete, or when you choose
  **Launch Ready Steps**.
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

## Using a workflow made from a recipe

1. After **Apply Recipe** and **Create boxes**, select the first box marked
   **needs structure**. Build or import its molecule in the Builder and save
   it. A recipe continued after an existing box takes its starting structure
   from that box.
2. Open the first step's Calculation Editor. Check its charge, multiplicity,
   theory and run type, then save. Open the Launcher, choose the machine and
   launch settings, and launch the step.
3. Wait until the source step is **Complete**. Open the next step's editor;
   an empty step now receives its source's final geometry, charge and
   multiplicity. Check the recipe's settings, save, and launch this step.
4. Repeat for the remaining steps. For branches, **Launch Ready Steps**
   launches every step whose sources are Complete using its saved launch
   settings. Use **Open in Launcher** for steps that still need launch
   settings. Nothing launches automatically when a step finishes.

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

## Rate

A workflow with a reactant step and a TS step (and a product step, when it
has one) also gets a **Rate** box, below the Key data box or in its place. The
steps are those marked reactant, ts and product: the TS search recipe marks
them, and so does the SN2 workflow in the Workflows examples. The box waits
until those steps are Complete and says which one it is waiting for.

Once they are, the box shows, at the temperature in its **T / K** field
(298.15 K at first):

- the free energy of activation, ΔG‡, in kJ/mol;
- the rate constant k without tunnelling, and with the Wigner and with the
  Eckart correction, each with its factor κ, in the unit of the reaction
  (s⁻¹ for one reactant, M⁻¹ s⁻¹ for two);
- with a product step, the reverse rate constant and the equilibrium constant K.

The line under the title gives the treatment of the low frequencies and the
standard state; the box uses Grimme's quasi-RRHO with ω₀ = 100 cm⁻¹ and
1 M. The numbers are those of `ecce-rate`. Press Enter in the T field to
recompute. A TS that does not have exactly one imaginary frequency is not a
transition state, and the box shows the reason instead of numbers. The box
needs calculations stored on this machine.

**Details...** opens a window with the same quantities at a range of
temperatures (250 to 400 K in steps of 25 by default; change the three
fields and press **Update**), the Arrhenius and Eyring fits over the range,
and **Export CSV...** for the table.

## Fragment analysis

The recipe makes three or more boxes: **complex**, a single point (run type
Energy) of the whole structure, and **fragment-1**, **fragment-2**, ..., a
single point of each fragment, linked from the complex by its geometry. All
use the one method and basis set of the panel's level-of-theory row, since
each fragment's orbitals are expanded in the complex's basis set.

Apply it after the box of a finished calculation of the complex (select the
box, or right-click it and choose **Apply Recipe from here...**): the complex
box starts from that calculation's final geometry, and each fragment box
holds its own atoms of that geometry. For a new workflow, build the complex
in the **complex** box; the fragment boxes take their atoms when they are
launched.

**Fragments.** One row per fragment, with:

- **Atoms**: the complex's atom numbers as the Builder shows them, for
  example `1` or `2-13` or `1,3,5-8`. `rest` takes every atom that no other
  fragment has.
- **Charge** and **Multiplicity** of the fragment on its own.
- **Formula**, shown when the structure is known.

**Add fragment** adds a row; **Remove last** removes the last one (two
remain at least). Fragments must not share atoms and together must cover the
complex. The panel says what is wrong under the controls, and **Create
boxes** makes nothing until it is right. It also warns, without refusing,
when the fragment charges do not add up to the charge of the complex.

When the recipe is applied after a box, the panel proposes a split of its
structure:

- with a metal atom: the metal, and the rest. A ligand (a molecule bonded to
  the metal) with an odd number of electrons is counted as an anion, for
  example Cl⁻; one with an even number as neutral, for example CO or NH₃. The
  metal takes the remaining charge: for Co(NH₃)₄Cl₂⁺ the proposal is Co³⁺ and
  one fragment (NH₃)₄Cl₂²⁻. A closed-shell anion with an even
  number of electrons, such as sulfate, is counted as neutral: change its
  charge by hand.
- without a metal: one fragment per molecule, or atom 1 and the rest.

Each fragment gets the lowest multiplicity its number of electrons allows
(singlet or doublet). A fragment with a multiplicity above 1 is calculated
unrestricted (UHF or UKS).

## Fragment orbitals

A workflow with a complex box and its fragment boxes gets a **Fragment
orbitals** box, below the Rate or Key data box or in their place. It waits
until the complex and every fragment are Complete and says which ones it is
waiting for. The numbers are those of `ecce-fragorb analyse`, from each
calculation's `results.h5`; the box needs calculations stored on this
machine.

The line under the title names the fragments with their charges, for
example Cr | C₆O₆. Each following line is one orbital level of the complex
near the frontier, highest first: two levels above the LUMO, the LUMO, the
HOMO and two below it. A level is one orbital, or several degenerate ones
(MO 34-36). The line gives its energy in eV and the fragment orbitals (FOs)
it is made of, with the Mulliken share of each, largest first: the first
always, the next two when they are 10 % or more. An FO is named by its
fragment and its place there (C₆O₆ LUMO+1); degenerate FOs of one fragment
are taken together (Cr HOMO (×3)). For degenerate complex orbitals the share
is the mean over the level. For an open-shell complex the α and β orbitals
are listed separately.

The complex orbital c is written in the fragment orbitals as
a = S_FO⁻¹ C_FOᵀ S c, with S the complex's overlap matrix; the Mulliken share
of FO i is aᵢ (C_FOᵀ S c)ᵢ. The shares of one orbital add up to 1 minus the
part outside the span of the FOs, which is listed in the Details window and
is near zero when the fragments together have the complex's basis set.
A Mulliken share can be above 100 % or below 0 when fragment orbitals
overlap strongly; the shares of one orbital still add up as above.

**Details...** opens a window with:

- every complex orbital with its energy, occupation, the part outside the
  FOs and each FO level with a share of 1 % or more;
- the FO levels of each fragment with their energies and occupations;
- the partial density of states (PDOS) of each fragment and the total, and
  the overlap population density of states (OPDOS) between the fragments,
  from `ecce-fragorb dos`: Gaussian broadening with a full width at half
  maximum of 0.3 eV, in states per eV (eV⁻¹), with a line at the midpoint
  between HOMO and LUMO. A positive OPDOS is bonding between the fragments,
  a negative one antibonding. The line under the plots gives the occupied
  overlap population, the OPDOS summed over the occupied orbitals;
- **Export CSV...**, the share of every FO in every complex orbital.

The analysis refuses calculations that are not at the same geometry
(within 0.001 Å), fragments that share atoms or leave atoms out, and a
fragment whose basis set differs from the complex's on its atoms; the box
then shows the reason.

## Save Workflow as Recipe

Click **Save as Recipe...** beside **Apply Recipe...** and give the recipe a
title. The workflow's steps, run types, settings and links
are kept, its structures and results are not. The recipe is written to the
`Recipes` folder of your ECCE configuration and appears in the list the next
time the panel opens.

## Your own and your site's recipes

Recipes are text files, read from your own Recipes folder, then the site's,
then the installed ones. See [Writing a recipe](writing-a-recipe.md).
