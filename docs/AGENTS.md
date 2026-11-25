# ECCE for scripts and AI assistants

Much of what ECCE does can be driven from the command line, so scripts and AI
assistants can work with it too. This file says what ECCE is and can do, how
to do the usual tasks, how to set up compute machines and codes, how ECCE
stores its data, which commands there are, and how to check a result. It is read by people,
scripts and coding assistants alike. The scientist stays in control: the
assistant prepares, checks and explains, and acts on ECCE only when asked to.
Nothing here says that an assistant gets the chemistry right; the results are
the scientist's to judge.

Privacy: what an assistant reads (inputs, outputs, structures, this file) is
sent to the provider of that assistant. Do not point an assistant at data that
may not leave the machine.

## Contents

1. Safety rules
2. What ECCE is
3. What ECCE can do (codes, run types, help pages)
4. How to do things (task recipes)
5. Compute machines and HPC
6. Registering a new code (a developer task)
7. Data layout
8. Commands
9. Checking a result
10. Documentation shipped with ECCE
11. Keeping this file current

## Safety rules

1. Never edit a stored calculation by hand (`.ecce-meta`, `Inputs/`,
   `Outputs/`, `Props/`, `Parameters/`, `linkbase.xml`). ECCE keeps the files
   consistent with each other; a hand edit makes the calculation look
   finished, failed or different from what ran. To change a calculation, use
   ECCE, or make a new one.
2. Launch, terminate, reconnect or delete only what the user asked for, and
   only the calculations the user named.
3. Before submitting, say what will be submitted: which calculations, on which
   machine, with which queue, cores and wall time. Wait for the user's yes.
4. For experiments, use a separate state directory so the user's own data and
   settings are not touched (see "Data layout", "A separate state directory").
5. Say plainly what was checked and what was not. Do not raise optimisation
   cycle limits to make a calculation converge; find the cause.

## One Builder script per tab

`ECCE_BUILDER_SCRIPT` runs once per process: in one program only the first
Builder or Viewer tab runs it. With `ECCE_BUILDER_SCRIPT_EACH=1` every
Builder and Viewer runs the script when it opens, so a test can rewrite the
file between `start` commands and give each tab its own steps (the
Viewer-tab close test in `tests/apps/one_process_test.py --panels-only`).

## Select heteroatoms (#1244)

Builder `Edit > Select Heteroatoms (not protein)` selects every atom outside
standard amino-acid/nucleotide residues except the ligand `Select Ligand`
would pick (the largest non-water, non-ion, non-additive residue); with no
ligand it selects everything non-protein except water. Delete then removes
them. Script command in `ECCE_BUILDER_SCRIPT`: `select_hetero`.

## Add Hydrogen on a PDB structure (#1244)

`Edit > Select Ligand` is a submenu of the ligand candidates (non-polymer,
non-water residues, `BNZ 400 (6 atoms)`), the likely ligand first, additives
after it, single atoms and ions last; it is refilled when Edit opens. Script
commands: `ligands` (prints the entries), `ligand N` (select the Nth),
`select_ligand` (the first). The Vina box's Ligand atoms use the same rule.
Ligand choice skips crystallisation additives by residue name
(`SGFragment::residueClass`: GOL, EDO, HED, MPD, SO4, ...), so 181L's ligand is
BNZ and not the larger HED.

Builder `Build > Add Hydrogen` (and the toolbar button) only turned nubs into
hydrogens, so a structure imported from a PDB file (no nubs) got nothing. With
residues and no nubs it now runs the residue-template path (the Residue table's
Add Hydrogens) and the status line says `Added N hydrogens to M of K residues`,
naming the residues that got none (waters). Residues without a template
(ligands) get hydrogens from their geometry: bond orders from lengths
(aromatic ring C-C 1.32-1.46 A), then hydrogens in the ideal directions for C, N, O, S. A structure with neither nubs nor residues (heavy atoms from
XYZ or CIF) is filled from its geometry the same way, each bonded group on its
own (lone ions and water oxygens are left alone). Nubs on a drawn part of a
structure with residues are filled and every residue is filled too, in one
undo step (#1251). With nothing to fill it says `No hydrogens added`. Script commands in
`ECCE_BUILDER_SCRIPT`: `import FILE` (File > Import of FILE), `resatoms`
(atoms per residue name), `menu Build|Add Hydrogen`; see
`tests/apps/builder_import_pdb_test.py` (also the XYZ and the 181L plus
`add C Tetrahedral 40 40 40` cases). The script command `add ELEM SHAPE X Y Z`
on a structure with residues and no residue being edited first inserts a new
residue `UNK` at the end and edits it, as Insert in the GUI's residue prompt
does; an atom with no residue to go into is refused with a message.

A click in the 3-D view (selecting an atom, clearing the selection on empty
space, Edit > Select All) leaves the camera where it is, also with Auto
Center on; an import, an edit that puts atoms outside the view and View All
still fit the structure (#1260). Script commands in `ECCE_BUILDER_SCRIPT`:
`zoom F` (camera angle or height times F), `inview` (`BUILDER: inview
outside=0|1`), `camerastate` (position, focal distance, angle or height),
`pickatom N` and `emptyclick` (what a click on atom N, or on empty space in
Select mode, runs), `viewall`. Test: `tests/apps/builder_click_camera_test.py`.

Builder Edit menu: **Invert Selection** (Ctrl+R, formerly Reverse Selection).
After a structure opens and after every Edit > Select command the 3-D view
has the keyboard focus, so Delete and Ctrl+Z act at once. Script commands
`undo`, `redo`, `ctrlz` (the Ctrl+Z key hook) and `focus` (class of the
focused window); see `tests/apps/builder_delete_undo_test.py`. `menuenabled
<Menu>|<item>` logs `BUILDER: menuenabled ... top=0|1 item=0|1`, whether a
click could reach a menu-bar item (`menu` sends the item even when greyed).
`delkey` sends a Delete key press through the frame's key hook with the 3-D
view focused; `undolabel` logs `BUILDER: undolabel [<Edit > Undo label>]
enabled=0|1`; `drawnstyles` logs `BUILDER: drawn N of M`, the atoms whose display
style the 3-D view still has (an undone atom outside it is not drawn);
`drawn` logs the atoms handed to the display nodes by element,
`BUILDER: drawn <El>=n ... total=N of M`.

A PDB opened directly (`ecce-builder FILE.pdb`, as the file manager's Open
With does) is editable. If it holds records ECCE does not write, File > Save
goes to Save As instead of overwriting it; under `ECCE_BUILDER_SCRIPT` that
logs `BUILDER: save goes to Save As` and opens no dialog
(`tests/apps/builder_pdb_open_test.py`).

## Scripted total charge and multiplicity

`ECCE_CALCED_SCRIPT` and `ECCE_BUILDER_SCRIPT` accept `charge INTEGER` and
`mult INTEGER` for the total calculation state, separately from `fragment N
charge Q mult M`. Set charge first, then multiplicity: e.g. `charge -1`,
`mult 2` for an 11-electron water anion. Charge changes keep an allowed spin,
otherwise choose singlet/doublet by electron parity. Multiplicity must be
positive, match electron parity and be at most the electron count plus one.
Invalid syntax or unavailable/read-only fields log `FAIL`. CalcEd invokes
the Charge/Spin typed-field handlers; invalid spin logs `FAIL` and selects
the GUI default. Builder rejects invalid spin without changing it.

CalcEd: `button save` persists the fields and regenerates input. Builder:
spin is stored immediately, `save` persists the structure charge; then save
in CalcEd to regenerate input. The regression is in
`tests/apps/calced_script_solvation_test.py` (`apps_calced_script_solvation`).
See [Scripting and AI assistants](../help/src/scripting-and-ai.md).

## Titles in generated input

Input generators preserve spaces and punctuation in the structure's `.frag`
`title:` line. ECCE-QM writes this as `title <text>`; ORCA includes it as a
`#` comment (hashes in the title become spaces so they cannot close the comment).
NWChem's displayed `Title` comes from the `.param` `annotation`,
while its `Start` name comes from `.param` `title`. Keep those metadata fields
distinct when generating decks. CP2K project names and QE prefixes still use
their existing identifier filtering and 40-character limit. Regression test:
`ctest -R inputgen_titles`.

## Reading final periodic stress

In the Builder, **Properties > Geometry > Stress Tensor** displays the
final QE or MLP `STRESS` (nine Cartesian components stored in kbar).
The display negates and symmetrises this tensor and converts to GPa:
tension positive, compression negative. QE prints compression positive;
MLP stores minus ASE stress. Pressure is `-trace(display)/3` and must match
stored `PRESSURE` (1 GPa = 10 kbar), allowing printed rounding.
The panel gives principal values, pressure, von Mises and maximum shear,
and normal stresses along unit cell vectors as shrink/expand words.
Blue arrows point away from the cell for tension; vermillion arrows point in
towards the cell face for compression. They lie outside the cell, starting at
the face crossed by each principal axis, with one common magnitude scale.
Use **Show in viewer** to hide them. This uses final stress and geometry;
per-step vc-relax stress, CP2K ingestion and directional surfaces are not
implemented. See [Viewer panels](../help/src/viewer-panels.md).

With one atom selected the Builder/Viewer shows `El N  (x, y, z) Å  RES` at the
bottom left of the 3-D view; with two atoms the distance `El N - El M  d Å`
(#1236). Builder script `selectoverlay [N [M]]` (from 1) selects those atoms and
prints `SELOVERLAY: <count> [<overlay text>]`; test
`tests/apps/atom_numbers_test.py`.

Build > Cut Cluster (#1033) turns a periodic structure into a non-periodic
cluster around the selected atoms. Builder script:
`cutcluster R [center N] [whole none|molecules|units] [cap none|H|OH]` (R in
Angstrom, N from 1; without `center` the selection is the centre). It prints one
`BUILDER: clusteratom EL X Y Z` line per atom and `BUILDER: cluster atoms=N
caps=M`; undo restores the periodic structure. The geometry is in the wx-free
`tdat/ClusterCut.H`; test `tests/apps/builder_cut_cluster_test.py`.

Build > Solvate (#1118) fills the periodic cell, or a rectangular cell of
the structure's extent plus a margin (structure centred), with a shipped
solvent box (`data/client/solvents/*.xyz`), on the client, for any code.
Builder script: `solvate [solvent NAME] [margin A] [scale S]` (defaults
water216, 10 A, 1.0; scale times the sum of vdW radii is the solute-solvent
cut-off). It prints `BUILDER: solvate molecules=N added=M atoms=T cell=A B C
density=D`; undo removes the solvent. Waters are HOH residues (OW, HW1, HW2),
and GROMACS Prepare skips `gmx solvate` for a structure that has them
(`Solvent already present ...` in its output). The geometry is wx-free in
`tdat/Solvate.H`; `solvate_cli SOLVENT.xyz SOLUTE.xyz|- MARGIN|"a b c"
[SCALE]` (build tree) writes the solvated XYZ to stdout. Tests: ctest
`solvate`, `tests/apps/builder_solvate_test.py`. The old Solvate window
(Condensed Reaction Study) opens only with Preferences > Advanced
"Separate Solvate window" (`ECCE_SOLVATE_WINDOW=1`); otherwise a Solvate
task opens in the Builder and a new Condensed Reaction Study gets no
Solvate task.

For isolated GUI checks, `ECCE_BUILDER_SCRIPT` accepts `list Stress Tensor`,
`stressinfo` (panel summary plus one `label N anchor=x,y,z` line per principal axis; anchors lie outside the cell), `vector STRESS` (the six rendered half-arrows),
and `viewer Stress Tensor` (toggle). `tests/apps/stress_tensor_test.py`
checks a hand-computed tensor and the tensile QE Al cell example;
`--unit-only` runs just the wx-free arithmetic, `--png DIR` keeps pictures
and logs for visual review. Run with a private installed tree and the
test harness as described below.

`ECCE_BUILDER_SCRIPT` drives Geometry Constraints with `constraints show`,
`constraints select I J ..` (1-based atoms), `constraints press constrain|restrain|fix yes|no`,
`constraints scan <steps> <step> [yes]` (the selected 2-4 atoms, step in Angstrom for a
bond and degrees for an angle or torsion, steps 1-500, step not zero; the same code
as OK in Scan..., a refused line FAILs with the dialog's message; a trailing `yes` confirms a pair of
unbonded atoms, e.g. a hydrogen-bond O...O distance),
`constraints list` and `constraints stored`; `save` writes `Parameters/GeomConstraintModel.gcst`.

`ECCE_BUILDER_SCRIPT` also accepts the Conformers-panel hooks `gtenselect
<rank>...`, `gtenfit`, `gtenthumbs`, `gtenpng <file>`, `gtencsv <file>`,
`gtenviewer on|off`, `gtensort <column> up|down` and `gtendistinct <rmsd>
[n|0] [window]` (open the panel with `ECCE_OPEN_PANEL=Conformers`), so one
script can set `background 1 1 1` and select conformers. `gtendistinct` is
the panel's **Select Distinct...** button: it runs `ecce-ensemble pick
--json` on the calculation, selects the picks' `energy_rank` rows and prints
`GTSTRESS: endistinct ranks=<r>... note=<n distinct of N; m merged within X Å>`. Output goes to stderr as `GTSTRESS: ...` lines; the same
commands work under `ECCE_VIEWER_SCENE`. Test: `tests/apps/ensemble_patterns_test.py`.

`surfmesh <name>` (Builder scene script, after an MOs-panel surface is drawn)
writes `<name>.txt`: per isosurface its threshold, vertex and triangle counts,
boundary edges (used by one triangle; 0 means closed), edges used by more than
two triangles and edges whose two triangles wind the same way, plus the grid
dimensions and step (A). Test: `tests/apps/surface_closed_test.py` (`--res N`
sets the grid points per axis, `--shot DIR` saves PNGs).

`paneorder`, `dropat <panel name> <pos>`, `dockfloating` (Builder scene
script): `paneorder` writes `paneorder.txt`, the shown docked panes of the
Properties column top to bottom, and `paneall.txt` (every pane: floating, shown, dock direction, layer, row, position); `dropat` docks a panel into that column at
dock position `<pos>` as a mouse drop would; `dockfloating` is the Dock
button. The Properties list stays first in the column whatever the drop.
Test: `tests/apps/mo_explore_workflow_test.py` (e).

`panelpaints <name> <ms> <property>` (scene script, `ECCE_VIEWER_SCENE`,
#1263): clicks `<property>` in the Properties list and writes every paint of
a window inside the MOs/Surfaces panel for `<ms>` to `<name>.txt` (`paint
<class> <WxH then> now <WxH> shown 0|1`, then `panel <WxH> paints N`).
`tests/apps/props_switch_paint_test.py` checks that no hidden window or
superseded layout of that panel is painted.

`saveimage <path.png> <factor> [white]` (scene script, `ECCE_VIEWER_SCENE`)
is File > Save Image...: a PNG of the 3-D view at `factor` (1, 2, 4) times
its size, drawn with blended transparency (not the on-screen screen-door
mode), supersampled edges and line widths scaled with the factor; `white`
replaces the view's background; the corner texts are left out and atom
labels grow with the factor. It works under `ECCE_BUILDER_SCRIPT` too, where
`saveimagemenu <factor> <white|keep> <path>` runs File > Save Image... with
its dialogs answered. At factor 2 or 4 the shown orbital/density is recomputed
on a grid of 30 x factor points per axis (cap 160) for the render only, then
redrawn as before (log line `MOPANEL: fine grid N`). Use it for documentation figures. The image is drawn from the viewer the
scene script itself drives, so `viewall`, `zoom` and `rotate` before it show
in the PNG with or without `ECCE_VIEWER_SCENE_HOLD` (#1090). `depthcue on|off` runs the Depth cueing toggle's handler (#1214); `ortho on|off`
switches the camera between orthographic and perspective (then frames the
scene), so a `saveimage` after it is an orthographic figure; `zoom <f>` scales
the height of an orthographic camera, and `camera` prints `type orthographic|perspective`.
A fixed Å-per-pixel scale is not available yet (#1133). Test:
`tests/apps/saveimage_test.py`.

Layer Builder (`ECCE_BUILDER_SCRIPT`): `pbclayer <field> <value>` sets a field (layers, spacing, shifta, shiftb, ion1/ion2/ion3 with ion1n, ion1q, ion1min and likewise 2 and 3, water, watermin, seed); `pbcpress layers` presses Build Layers. Three ion rows. Test: `tests/apps/pbc_layers_test.py`.
Builder File > Export Structure... writes `.gro` or `.pdb` with residues and the cell (`.xyz` without); Builder script `savegro PATH` / `savepdb PATH` (prints `BUILDER: savegro: done`). Atoms without a residue are written as HOH (water, OW/HW1/HW2) or as a residue named by the element (a lone ion). Test: ctest `fragreaders`.
GROMACS Prepare, Mineral source, **Mineral from: the Builder** (#1140): the Builder's whole structure (mineral, water, ions; e.g. a Layer Builder stack) is typed as it is by `minff_topology.py system STRUCTURE.gro -o DIR [--variant|--mineral|--angle-k|--water|--ion-set]` (MIN, then ion molecules, SOL last; summary in `minff.json`: `waters`, `ions`, `total_charge`). Editor hook `set mstruct builder|cell` (then `set mbuild 1`); `summary` shows `mstruct=`. The job adds no water layer and runs genion only for a non-zero total charge. Test: ctest `minff_prepare` (case "system-na", "system-charged").

## Compare relaxed scans in the Builder

Open **Relaxed Scan** (`ECCE_OPEN_PANEL=Relaxed Scan`) and use
`ECCE_BUILDER_SCRIPT` commands:

- `scancompare URL_OR_PATH...` replaces the comparison selection with stored
  calculations anywhere in the project hierarchy, including data-server URLs.
  Sibling short names remain accepted. `scancompare none` clears it;
  bare `scancompare` lists sibling names as a convenience. The GUI's
  **Compare...** calculation browser adds one calculation at a time.
- `scanzero lowest|largest|first` chooses each curve's reference energy.
  Default `lowest`; `largest` is the energy at the largest sampled coordinate,
  not an extrapolation. `first` means stored first point, even after joining.
- `scanjoin on|off` merges matching first-point geometries (same atom order
  and elements, pair distances within 1e-4 Å), first coordinates (1e-5) and
  first energies (1e-6 hartree). Missing structures or unmatched starts stay
  separate. One common start is kept; other overlaps are retained. Verify the
  method, state, basis and scanned atoms before joining.
- `scanlayout overlay|multiples` chooses overlaid curves or a vertical panel
  per resulting curve with separate y scales and a common x range.
- `scancsv PATH [kcal]` exports every resulting curve as a labelled CSV block
  (source URLs, absolute hartree and relative kJ/mol, or kcal/mol).
- `scanpng PATH [WIDTH HEIGHT]` exports the combined layout on white. Minimum
  width 200 and height 160 per resulting panel; for multiples use e.g.
  `scanpng curves.png 1000 1200`. Morse-fit samples appear in PNG only.

`scanmorse on|off [window]` still fits only the active scan. Structure clicks,
table and playback refer to the active calculation. Test:
`tests/apps/scan_compare_test.py`, with isolated stored fixtures copied from
an existing scan example. Put `background 1 1 1` before screenshots.

### Playback Repeat (#1229)

The Relaxed Scan, IRC Path and Geometry Trace panels have **Repeat: off /
from the start / back and forth** next to the play buttons: what **Play**
does at the last point. Back and forth is the default for scans and IRC
paths, from the start for the Geometry Trace. Builder script:
`scanplayopt repeat off|start|backforth` and `ircplayopt repeat ...`
(`loop on|off` still works: on = backforth); `... order <ticks>` logs the
points that many ticks of play show, and the reply carries `repeat=<word>`.
`gtrepeat [off|start|backforth]` sets the Geometry Trace choice and logs
`GTSTRESS: repeat=<word> frames=<n> order: ...` (two passes). Test:
`tests/apps/irc_panel_test.py`.

## Checking the Basis Set Tool window

The window is retired (#1063): with tabs, `start Basistool URL`, the
calculation's Basis Set button and the editor's basis button open the
Calculation Editor at its basis panel (the start message carries
`actiontopic=basis`), and the Organizer's Tools menu has no Basis Set Tool
item. `ECCE_FULL_BASIS_TOOL=1` (Preferences > Advanced, key
`Advanced.FullBasisTool`) brings the window, the menu item and the panel's
**Full Basis Set Tool...** button back; tests of the old window set it.
Without tabs (`ECCE_ONE_PROCESS=0`, layout Wide) the old window still opens.

`tests/apps/basis_tool_size_test.py` (ctest `apps_basis_tool_size`) checks
content sizing, selection-row growth and saved geometry on a private
1366×768 display. `--png DIR` saves a screenshot. Use the installed-tree
`ECCE_TEST_HOME` and `ECCE_TEST_WRAPPERS` variables as for the app suite.
The isolated `ECCE_TEST_BASISTOOL=<file>` hook reads appended commands:
`info` reports frame, grid and periodic-table geometry; `resize x y w h`
sets frame geometry; `rows N` replaces the test grid's rows without changing
the calculation; `changed` prints `BASISTOOL: changed=0|1`, whether the
tool holds an unsaved change; `close` closes normally and saves the window settings.

## Fragment definitions and ORCA counterpoise export

Builder script (`ECCE_BUILDER_SCRIPT`) commands:

| Hook | Meaning |
|---|---|
| `fragments` | Print per-fragment atoms, charge and multiplicity; initialise connected-molecule defaults when unassigned |
| `fragments auto` | Reset to connected molecules, numbered from 1 by first atom; neutral defaults, multiplicity from electron parity |
| `fragment N atoms LIST` | Move listed 1-based atoms into fragment N; spaces, commas and ranges such as `1-3` accepted |
| `fragment N charge Q mult M` | Set fragment state; reject if fragment charges do not sum to total structure charge |
| `counterpoise orca FOLDER [MULT [ROUTE...]]` | Write `complex.inp` and `fragment-N.inp`, partner atoms as ORCA `Symbol:` ghosts; default route `HF def2-SVP` |

MULT is the complex multiplicity (default calculation multiplicity, else
electron parity). The monomer inputs use each fragment's state. Use an empty
destination. The exporter validates charge sums and electron/multiplicity
consistency. Automatic neutral states on a charged structure are pending:
for total-charge +1 Na⁺···H₂O, explicitly set the Na fragment to `charge 1
mult 1`. A rejected command leaves the existing state intact.
`save` persists fragment assignments/states in the MVM structure; XYZ loses
them. These fragment/export hooks only write inputs. The Counterpoise Energy
option and Relaxed Scan action below run calculations and collect energies. See [Scripting help](../help/src/scripting-and-ai.md#fragments-and-orca-counterpoise-inputs).

## Properties list click diagnostics (#965)

`ECCE_DEBUG_PANELS=1` makes the Properties list print, on each mouse-down,
`[PANELS] list click y=.. hit=<row> rowh=.. scrollpos=.. viewstart_y=..` on
stderr (event y, the row HitTest gives, row height, vertical scroll). One
real click that selects the wrong row gives the numbers to compare.

## Isolated GUI tests

Builder hook `key text cut|copy|paste|undo|selectall` fills an editable
single-line field with `abc`, focuses it, selects its text, and sends the
frame's shortcut command. It prefers the focused field, then the first
visible enabled field; if none exists, it opens the Symmetry panel and
uses its first text field. `key viewer selectall` focuses the 3-D view instead.
See [Scripting help](../help/src/scripting-and-ai.md#builder-shortcut-checks).

Run GUI checks through `tests/` with an isolated HOME and test data.
The shared Xvfb harness chooses a free display from `:90` through `:999`,
checking both sockets and lock files; `ECCE_TEST_XDISPLAYS=a-b` restricts
that range. Explicit `--display` numbers and ranges must start at `:10`
or higher. Keep the tests' existing `:0` safety guards.
Run the display-allocation regression with
`nice python3 tests/apps/xdisplay_test.py`; no ECCE install is
needed, and the live-server check skips when Xvfb is unavailable.

The `create` driver of `tests/launch` (`launchjob create ...`, `Session.create(..., extra=())`)
stores the Calculation Editor's setup when given `code=<JCode>`,
`theory=<category>:<name>` and `runtype=<name>` (for example
`theory=SE:GFN2-xTB runtype=Geometry`), with detail values from
`gui=<settings file>`. Without a stored theory `ecce-series` and
`ecce-ensemble pick --create` refuse the calculation as a template.
`tests/launch/series_setup_test.py` is the example.

## What ECCE is

ECCE (Extensible Computational Chemistry Environment) is a desktop
application for setting up, running and analysing computational chemistry
calculations. It builds the input for a code, submits the job to the
computer it runs on, follows it, parses the output, and stores everything
with the calculation. It does not contain most of the codes: ORCA, Gaussian,
NWChem, Quantum ESPRESSO and the others are installed on the machine where
they run. ECCE-QM, a small Hartree-Fock and DFT program, is built in.

Your user works in ECCE's windows. You work on the same stored data and
through the command-line tools in "Commands". You cannot click in the windows.

### The programs

| Program | What it is | How it starts |
|---|---|---|
| Organizer | The main window: the tree of projects and calculations, the project table, the Run Mgmt menu (launch, terminate, reconnect, re-parse) and the Tools menu. | `ecce` (it opens the Organizer directly) |
| Builder | Builds and edits a structure; the Viewer panels show results (orbitals, spectra, trajectories, properties). | From a calculation in the Organizer |
| Calculation Editor | Sets up the calculation: theory, basis set, run type, details; writes and checks the input. | From a calculation in the Organizer |
| Launcher | Chooses machine, queue, cores, wall time and allocation profile, and submits. | Opened from the Organizer for several calculations; for one calculation the Organizer's Launch button (hook `launchbutton URL`) opens its editor tab with the Launch panel (same as the editor's Launch... button), the Launcher tab only without tabs or without a set-up area |
| Register Machines | Registers compute machines: codes, job script, queues. | **Tools > Register Machines...**, or `ecce -machine` |
| Machine Browser | Runs a few status queries on a registered machine over ECCE's login. | From the Organizer or Launcher |
| Basis Set Tool (retired) | The old basis window; the editor's basis panel replaces it. | Only with `ECCE_FULL_BASIS_TOOL=1` (Preferences > Advanced, **Separate Basis Set Tool window**) |

One program is the default (#1067): unless `ECCE_ONE_PROCESS=0` is in the
environment of `ecce` (the old route, every window its own program), the Builder, the Calculation Editor, the
Basis Set Tool, the MD editors, the Periodic Table, the Machine Browser and
Register Machines open as windows of the Organizer's process instead (the
last three as plain windows, not tabs); job programs and the other
tools stay separate. Off by default. Quitting the Organizer (or the
session's `ecce_quit`) then closes those windows first (with `ECCE_ONE_PROCESS=0` the Organizer's Quit also ends the separate tools on Windows and macOS, where Quit ends the application; on Linux they stay open and keep the session), each asking about
its own unsaved changes; Cancel in one of them cancels the Quit. Test hooks
(`ECCE_TEST_ORGANIZER`): `inprocess` (pid, open Builder/Calculation Editor/MD Energy/Basis Set
Tool/MD Prepare windows),
`closetools`, `importfile FILE [PROJECT-URL]` (Import as Calculation on a local or `sftp://` file), `gatewayquit` (the session's `ecce_quit`), and
`answer cancel|no` for a tool's save question.

File text viewer (#1084): double-click a document in the Organizer's folder
list or tree, or right-click **View**. The EDSI reader probes the first 8 KiB
for NULs and loads the last 64 KiB; it never downloads the whole file for
viewing. **Load more** prepends 64 KiB, **Find next** searches loaded text,
and **Follow** polls the tail every three seconds (also handles truncation).
Binary files show size and extension. **Open in editor** downloads a separate
`.txt` copy and calls the system default application; changes are local to
that copy. Text viewing currently refuses sizes above the signed-int EDSI
range limit (2 GiB); the editor download is still available.

Organizer thumbnails (#1238): selecting a calculation with a structure requests
`Parameters/Thumbnail.jpeg` if it is missing or unreadable, once per URL per
session. The structure itself determines eligibility, including periodic
structures without formula metadata. VizThumbnail publishes `ecce_url_property`
with `name=thumbnail` after storing each image, so the selected button updates
without reselecting. Rendering and thumbnail styling are unchanged.
`ECCE_NO_VIZIMAGES` disables generation. stderr always records unreadable
images and render failures; `ECCE_TIMING=1` also records `Organizer: thumbnail
requested|displayed <URL>`, `Organizer: no structure for thumbnail <URL>` and
`VizThumbnailApp: thumbnail stored <URL>`. Open Editors ignore the
`name=thumbnail` message, so it never triggers their auto-save.

Organizer details pane timing (#1237): selecting a calculation fills the
pane and lays it out first; the progress backfill and the thumbnail follow
from the event loop. `ECCE_DIAG=1` prints the start-up marker `Organizer:
window shown` and the desk-panel repair line (`ORGANIZER: desk panel ...`);
`tests/apps/apps.py` sets it for every app it runs. `ECCE_TIMING=1` prints one stderr line per refresh,
`ORGANIZER: refresh <name> total=… props=… launch=… backfill=… thumb=…` (ms);
a refresh over 1 s prints it without the variable. The `ECCE_TEST_ORGANIZER`
hook `refreshtimes` answers the last such line plus `formula=[<pane text>]`
(`tests/apps/refresh_timing_test.py`).

Isolated `ECCE_TEST_ORGANIZER` hooks: `view <url>` opens a window;
`view-status`, `view-more`, `view-follow on|off`, `view-find <text>`,
`view-text <output path>` (displayed text in UTF-8), and `view-close` operate on that
window. Replies include `size`, `start`, `bytes`, cumulative `read`, `lines`,
`binary=0|1`, `follow=0|1` and `readonly=1`. Find adds `found` or `not found`.
Tests: `tests/apps/organizer_file_view_test.py` for the window and Follow;
`tests/filedsi/range_test.py` for actual local read byte counts and the exact
SSH-side range/stat scripts (no login needed). Both run through `srun`.

`treeimage <url>` (`ECCE_TEST_ORGANIZER`) answers `bg=0|1 ink=none|ssh|server`:
rows below an SSH-folder or ECCE-server root have no background colour, and
their folder icons are drawn in the root's ink (green or maroon);
`tests/apps/server_roots_test.py` checks the server case.

Server and SSH-folder roots (`ECCE_TEST_ORGANIZER`): `server rootpos NAME`
answers the screen position `x y` of that root's label, for a real
right-click with xdotool; `ECCE_TEST_POPUP_LOG=<file>` appends the labels of
every context menu the Organizer pops up (`;`-separated, `-` for a
separator).  A connected root offers `Disconnect;Remove from list;-;` and
then its folder's items (#1221).  `ECCE_SSH_TIMING=1` writes
`ecce-ssh-timing <ms> [tid] <step>` lines to stderr for each ssh process and
each step of a login, to find where a slow connection waits (#1220).
SSH folders marked to connect at start log in on a worker thread after the
window is up (#1272): `server roots` lists them as `NAME (SSH)
(connecting...)` until the login returns (their context menu shows a greyed
`Connecting...`), folders on one host share one login, and a refused or
cancelled login is not asked again for the host's next folder.  Connect from
the Servers menu or the root's menu waits behind a cancellable "Connecting
to HOST..." dialog while the window keeps answering.

Start feedback (#1071, `ECCE_TEST_ORGANIZER`): `start TOOL [URL]` shows a busy
cursor, "Opening the <tool> for <calculation>..." in the status line, a "Starting
the <tool>..." window after 1.5 s and a "has not started" note after 30 s; a
second `start` of the same tool and URL meanwhile is dropped. `toolstate` answers
`state=starting|up|timed out|none status="..." busy=0|1 window=shown|none
launches=N`. The tool sends `ecce_tool_up` when its invoke is handled;
`ECCE_TEST_TOOL_DELAY=<ms>` holds that signal back for a slow-start test.

Live objects (#1136, `ECCE_TEST_ORGANIZER`, one-program mode): `livecount`
answers `Builder=N CalcEd=N CommandManager=N PropertyPanel=N SGContainer=N
SGContainerManager=N SGViewer=N WxLauncher=N` (classes never made are left out).
After every tool tab is closed the counts return to their values before the
first tab; `tests/apps/monolith_campaign_test.py H` checks that and the RSS.

Paints (#1210, `ECCE_TEST_ORGANIZER`, one-program mode): `paintlog on` starts a
fresh log of every paint of a window in the main window; `paintlog` answers
`paints=N seq=Class@WxH*count,...` in order (repeats merged), or `off`. Use it
to see what reaches the screen while a tab opens or closes;
`tests/apps/one_process_test.py --1210-only` runs it for a Viewer tab.

Greyed main window (#1268, `ECCE_TEST_ORGANIZER`): `greywatch on` starts
counting the times the main window is found disabled (a wxSafeYield or a
modal loop greys it whole when tools are tabs); `greywatch` answers the
count, or `off`. `tests/apps/one_process_test.py --launcher-only` checks
that opening the editor leaves it at 0.

The tabbed part of `one_process_test.py` (`ECCE_TABS_ONLY=1`) runs as six
ctests, `apps_organizer_one_process_tabs1`..`6`, each under 30 min;
`ECCE_TABS_PART=N` runs group N alone and each step prints `time <step> N s`.

Desk (#1237, `ECCE_TEST_ORGANIZER`): `desk` answers `desk WxH shown=0|1
splitter=WxH sel=<tool-tab notebook selection or none> pages=N chain=<classes up
to the top level, "(hidden)" after a hidden one>`. After a tool tab closes it
must say shown=1, a selection and no hidden window;
`tests/apps/refresh_after_viewer_test.py` checks that.

Release round (#1279): `tests/releaseround/run_round.py [--only W1,...]
[--keep]` runs the release workflow scripts W1-W6 headlessly through the
hooks above (an isolated HOME and data folder, one fresh Organizer per
workflow) and prints one `W1.3 PASS|FAIL|MANUAL|SKIP <evidence>` line per
step, then a summary; exit 1 on a FAIL. ctest `releaseround` (label
`long`); offload: `run-offload.sh <sha> apptest
tests/releaseround/run_round.py`. Steps without a hook (a drag, the
pointer over a menu, a Slurm submit) are listed as MANUAL;
`tests/releaseround/README.md` maps each step to its hook.

**Second start and file types (#1069)**, one-program mode only: a second `ecce`
for the same user, data folder and server (`ECCE_REALUSERHOME`,
`ECCE_LOCAL_DATA`, `ECCE_SERVER_SESSION`/`ECCE_REMOTE_SERVER`) raises the
running window and exits with "ECCE is already running for this user".
`ecce FILE.xyz` (also .pdb .cif .mvm .car .gro; file managers on Windows and Linux, and Finder on macOS: Open with ECCE; .mvm opens in ECCE by default, the others only offer it; a double-click on ECCE.app while it runs raises its window)
opens the file in a Builder tab of the running copy, or of the one being
started. Files are also opened (one Builder each) when the tabbed mode is off. A Quit during the exit does not leave a listener behind: a start then opens a new window. In a tool's tab, File > Quit reads Quit ECCE and quits the whole program. The check needs a private run folder (`$XDG_RUNTIME_DIR/ecce-one`, `$TMPDIR/ecce-one` on macOS, else `/tmp/ecce-one-UID`); if it is not yours with mode 0700 the check is off. `ECCE_MULTIPLE_INSTANCES=1` switches the check off. `ecce` itself asks the running copy first (`$ECCE_HOME/libexec/ecce-one-instance [FILE ...]`, exit 0: handed over, 1: none running), so a second start starts no gateway and asks for no password, under `-remote` and in local mode (the user's own data server or a local data folder) alike; with ECCE_LOCAL_DATA unset, `ecce` gives the client the folder `ecce-localdata` prints, as the Organizer gets it. Test hook
`raised` (Organizer command file): how many second starts reached the window.

In one-program mode (the default) the Viewer (`CalculationViewer`) is an in-process
Builder tab as well; `start TOOL URL` twice for one calculation reuses the
tool's tab (`inprocess` reports `Viewer=N`); `start Builder URL` for a
run (read-only) calculation opens or selects its Viewer tab instead
(`InProcessTools::alias`, #1254); closing a tool's tab never
asks to quit the Organizer (`ToolTabs::closingTool`). Input is blocked
during tool start and job submit by `InputBlock` (swallows mouse and key
events, nothing disabled; the separate-window path keeps `wxWindowDisabler`).
Checked by `ECCE_1088_ONLY=1 tests/apps/one_process_test.py`.

In one-program mode (the default): **Edit > Preferences > Windows
and tabs** chooses which tools open as tabs of the Organizer's window (the
Organizer is the first tab and cannot be closed). Presets **Normal**,
**Small**, **Wide** (separate windows; default: Normal); **Customize...** sets
each tool's row (Builder / Viewer, Calculation Editor, Basis Set Tool, MD
editors, MD Prepare) to Tab or Window, which makes the layout Custom. Under
Normal the Calculation Editor's tab is the Set-up tab: a detail area right
of the editor shows the Theory or Runtype Details (codes with a `.dlg`
description: ECCE-QM, ORCA, xTB, Gaussian 16, MOPAC, Vina; others open the Python
dialog's window) or, with no detail chosen, the input file (after an
unsaved change, a dry run of the generator with the editor's settings,
500 ms after the last change; the calculation is not written). MOPAC now has
both `.dlg` descriptions for this area. In layouts with separate Details
windows, `ECCE_NATIVE_DIALOGS=1` selects native MOPAC dialogs and
`ECCE_NATIVE_DIALOGS=0` selects the Python fallback. They use the same
settings keys and units as the Python dialogs. The window
is widened to 1100 px within the work area. Hook `setup info|theory|runtype|basis|launch|launch info|launch go|launch get WHAT|launch notify machreg|launch notify state STATE|results|results viewer|results output|input|fold|unfold|save|preview|text|width N|set KEY VALUE|find TEXT` on the
active Set-up tab (`area=input|theory|runtype title=... folded=0|1
controls=N ligandpick=N marked=<button> chargespin=shown|hidden symmetry=shown|hidden overlaps=N more="none|+N hidden"`, where `folded=1` means the Set-up area's content is folded away to its ◀/▶ button (`fold`/`unfold`; the editor column never folds and keeps its width; opening any set-up view unfolds it), `chargespin` says whether the editor shows Charge, Spin Mult. and Open Shells (hidden for a code whose .edml has `<ChargeAndSpin>false</ChargeAndSpin>`, Vina), `symmetry` the same for Symmetry and Use symmetry (`<Symmetry>false</Symmetry>`, Vina), `overlaps` counts shown controls of the editor's column whose rectangles intersect (0 is right) and `more` is the code bar's hidden-codes button; `width N` resizes the Organizer's window to N px; `preview` prints `shown=<len:fnv> file=<len:fnv>`; `text` prints the note above the preview, ` || `, and the preview itself with newlines as `|`
(the standalone CalcEd's ECCE_TEST_CALCED and ECCE_CALCED_SCRIPT command files both take `setup WHAT` and `watch`; `watch start` samples the Theory/Runtype choices and Details buttons and the preview note every 10 ms, and `watch` reports `theory=offN/onM runtype=... theory-details=... runtype-details=... notes=K |note1 |note2 ...`, what a user saw change, e.g. while `details runtype` opens);
`results` reports `results=shown|hidden editlaunch=enabled|disabled` (the Viewer/Output row under the editor's buttons, shown once the calculation is submitted; the editor's Launch... is off from then on); `results viewer` presses Viewer (opens the calculation's Viewer tab), `results output` presses Output (the Set-up area shows the primary output read-only, `area=output lines=N following=0|1 editor=1`; it reloads every 5 s and stays at the end while the job is submitted or running; Open in editor uses the external editor); `launch get WHAT` reads the embedded Launcher panel (`machines`, `machine`, `state`, ...; `greyed` is how many times the editor's window was disabled during the last launch, 0 when the log lines go out without greying it); `launch notify machreg|state STATE` publishes the broker notice the panel follows (machine registration changed; the calculation's state, read from its stored metadata);
`set` edits a field of the embedded dialog as a user would, `find` says
whether the preview contains TEXT, `basis` shows the basis panel
(`area=basis list=N of M picked=<set> covered=H,O uncovered=<elements>
use=on|off assigned=All_atoms:_<set>`; `basis filter TEXT` narrows the library
list, `basis pick NAME` chooses a set, `basis all` is Use for all atoms:
assigns it to the whole system and saves, as the Basis Set Tool's Add to Table
plus save would; the panel uses the Basis Set Tool's library and `GBSAssign`;
the describe line also has `assign=on|off checked=<elements> ecpoffer=<ecp>:<elements>|-
msg=<last message>`; `basis assign NAME [@ O,H|H1] [; NAME2 [@ H]]...` assigns an orbital set to the
listed elements (default all; the library name is lower case, e.g. `def2-tzvp`),
replacing what they had, and saves (an element with no set yet takes the assigned one, `msg=<El>_took_<set>_for_now.`; if the set covers none, the call is refused with `msg=Also_choose_a_basis_set_for_...`); `basis ecp NAME
[@ I]` adds an ECP from the library's ECP list, `basis ecp` alone adds the
matching ECP the panel offered after an ECP orbital set (`no ECP offered`
otherwise), `basis fit NAME [@ O]` adds a charge or exchange fitting set
(names with spaces, e.g. `Ahlrichs Coulomb Fitting`; it needs an orbital set on
the element first); the stored assignment reads `O: def2-tzvp + ECP
<name> + charge fit <name>` per element in `assigned=`),
several `;`-separated sets are applied as one step (`basis assign def2-tzvp @ O ; def2-svp @ H`; an element left without a set takes the set last used with `basis all`, else the assigned one); a target may be an atom name such as `H1`, which makes the basis per-atom, the element's group kept for the other atoms (`assigned=` lists `H: ...|H1: ...`); `basis atoms H1,H2` fills the Atoms box the buttons use instead of the checked elements (empty clears it); `basis aug NAME [@ O]` adds an augmenting (polarization/diffuse/Rydberg) set; `basis contractions EL` prints `contractions EL: s=9/3 p=4/2 ...` (primitives/contracted per shell of EL's sets), `basis contractions EL ROW uncontract|delete|keep` first sets row ROW (0 = the first contracted function in the Edit contractions... dialog's list for EL) and saves, as the dialog does (`no contraction row N`, `row N has no contraction` if refused), `basis coefficients` prints `coefficients=<text>` (the Show exponents and coefficients text as the editor's code lists it, line ends shown as `|`; `no basis set` if none); `basis tight|diffuse EL SHELL N` appends N tight or diffuse functions to shell SHELL (0 = first) of EL's set (`not possible` if refused); picking an ECP (`basis pick def2-tzvp-ecp`) lists in `covered=` only the elements it has a core for, the rest (all-electron) in `uncovered=`, and `assign=off` when none has a core; `fit=shown|hidden` in the describe line says whether fitting sets are listed (only with an ORCA RI approximation or a DFT-fitting code such as NWChem; otherwise `basis fit` answers `no such basis set`); `basis ecp Def2-ECP` adds ORCA's ECP only to elements it has a core for,
`launch` (the Launch... button) shows the Launcher's panel (`area=launch machine=<name> launch=enabled|disabled label=Launch|Launch_anyway`; pre-launch warnings appear in the panel and the button becomes Launch anyway: warn, never block), `launch info` reads it, `launch go` clicks Launch (`Launch is off`, `no Launcher panel` otherwise; `launch info` adds `clipped=N`, controls reaching past the area's right edge), `width N` sets the window's width,
`info` adds `window= virtual= dry=0|1
theorysum= runtypesum=` (the editor's summary lines); `no set-up area` under Small, `not a set-up tab` when the active tab is
not an editor). Stored in `~/.ECCE/EcceGlobal` as `WindowsAndTabs.Layout`
(`normal|small|wide|custom`) and, for Custom, `WindowsAndTabs.<key>`
(`tab|window`, keys `builder calced basistool mdeditors mdprepare launcher`). A tool
opened after a change follows it; the tab strip itself is made when the
Organizer starts, so going from Wide to tabs needs a restart (a fresh install is Normal). The window's
menu bar is the active tab's. Right-click on a tab: **Open in Window** (this
time) and **Always open in a window** (also sets that tool's row to Window);
the tool's **Window > Move to Tabs** puts it back. With
`ECCE_ONE_PROCESS=0` every tool opens in a window whatever the setting. Test
hooks: `tabs` (tabs, `*` = active, tools in windows, the menu bar's menus;
`tabs dots` also appends each tab's painted dot colour as `dots=0:#3060B0 ...`),
`launcher machine|pick NAME|launch|get WHAT` (the Launcher script's commands on the active Launcher tab; `get machines` lists the machine choice), `builderinfo` (the active Builder or Viewer tab: `atoms=N title=...`, the atoms its scene draws; `not a Builder tab` otherwise), `tab N`, `towindow N`, `totabs <title part>`, `mainwindow` (the main window: `host=MainFrame organizer=hidden ...`, or `host=none`
when the Organizer is its own window), `menu exit|about|preferences|minimize|<id>` and `hostclose` (a menu command, or the window
manager's close, on the main window; queued, answer the questions with `answer`; Quit, Preferences and About always reach the Organizer, also from a tool tab; a logged error never opens a modal box: a tool tab's goes to its log panel, others to a non-modal box on the main window, and with `ECCE_TEST_ORGANIZER` every log message is also on stderr), `windowmenu [N]` (the **Window** menu every bar has while hosted, `0:Organizer* 1:Viewer`; N chooses that entry and switches the tab; the main window's title then reads "ECCE Organizer <ver> - <tab>"; on macOS this menu has its own Minimize (Cmd-M; hook `menu minimize`) but no Zoom item or window list, since wx's automatic Window menu is off in one-program mode; `ECCE_DEBUG_MENUS=1` logs every menu-bar swap on stderr), `tabtips` (each tab's tooltip, `0:|1:(HOME)/proj/calc – Viewer|`; the Organizer's is empty; tab labels are the tool's name only), `nexttab` / `prevtab` (Window > Next Tab / Previous Tab, wrapping; answers the selected index), `tabkey next|prev|pgdn|pgup` (Ctrl+Tab, Ctrl+Shift+Tab, Ctrl+PgDn, Ctrl+PgUp as the key hook gets them; answers the selected index; steps less than 400 ms after the previous one only move a pending target, selected 150 ms after the last, #1267), `tabsteps N` (N Next Tab steps in a row; answers `selected=<i> selects=<tabs drawn>`), `windowitems` (every Window menu item in order: `Next Tab[Ctrl+PgDn]|Previous Tab[Ctrl+PgUp]|-|` on Linux (Ctrl+Tab on Windows, Cmd+Shift+] on macOS) then `Organizer|Viewer – (HOME)/proj/calc*|`, `-` a separator, `*` the active tab; starting a Builder, Viewer, Calculation Editor or Launcher for a calculation that already has that tool's tab selects that tab instead of opening another), `orgwindow prefs|find|props URL` then `orgwindowinfo` (the Organizer's modeless windows sit on the main window: `transient=host inside=1 mapped=1`), `layout show|normal|small|wide|KEY=tab|window`
(prints `preset=... rows=builder:tab,...`), `tabmenu N` (the tab's
right-click items), `alwayswindow N`, and `prefs windows show|preset NAME|row KEY tab|window [PNG]|snap PNG`
(the Preferences page and its Customize table, as clicks). A tab's
dialogs open centred over the Organizer's window: `dialog` opens a test
message box over the active tool tab and closes it 1.5 s later;
`dialoginfo` then reports whether it was transient for the Organizer,
shown and inside its window, its and the window's rectangles, and whether
the focus came back (`focus=set`).
Popup menus and floating panes of a tab: `popup` pops a menu over the
active tool tab and closes it after 1 s; `popupinfo` reports `parent=page
shown mapped inside near`. `float` floats the first docked pane of
the active Builder tab; `floatinfo` reports whether its frame is shown and
transient for the Organizer's window.

The commands (`ecce-launch`, `ecce-series`, ...) are separate and are the
way an assistant acts on ECCE. A calculation can be given to them as its URL or as
a path inside the data folder (local data mode); the path is compared as a
path, so a Windows path (`C:\Users\...\ECCE-local\...`) works the same way
(#1285, `LocalData::toUrl`). The help pages (**Help > ECCE Help**, `help/src/*.md`)
describe every window; `help/src/index.md` lists them.
The Organizer's menus and Preferences, item by item, are on the help pages
`organizer-menubar` (menu bar, toolbar, Preferences) and `organizer-menus`
(right-click menus).

### Which codes are offered (Preferences > Codes, #1049)

Codes unticked in **Edit > Preferences > Codes** are hidden from making new
calculations (File > New, right-click New, Continue in Another Code, the
workflow new-step buttons, the Calculation Editor code bar); existing
calculations of them are unaffected. The setting is `Codes.Shown` in
`~/.ECCE/EcceGlobal` (a string list starting with the marker `chosen`; absent =
default: ECCE-QM plus codes registered on a machine, or `Shown:` in
`$ECCE_HOME/siteconfig/Codes`). Every hiding list ends with
`N more codes (hidden)...` (editor bar `+N hidden`). Test hooks (Organizer
`ECCE_TEST_ORGANIZER`): `prefs codes list|all|registered|page [hiddenfirst]|
tick NAME on|off`; `tests/apps/code_choice_test.py`. One filter:
`CodeChoice` (`include/dsm/CodeChoice.H`).

### The life of a calculation

A calculation has a run state (help page `run-states`):

1. **Created**: it exists but has no input file.
2. **Ready**: the input file exists and it can be launched. A failed launch
   leaves it Ready with the reason in the run log.
3. **Submitted**: the job is in the queue of the compute machine.
4. **Running**: the job runs and ECCE follows its output. **Running?** and
   **Waiting for Login** mean ECCE has lost the connection to the job, which
   may still be running; Run Mgmt > Reconnect Selected (or `ecce-reconnect`)
   takes it up again.
5. **Completed** (the code finished normally, not necessarily with a right
   result), **Imported** (read from an output made elsewhere), **Incomplete**
   (ended without a complete result; the reason names the cause),
   **Killed** (cancelled by a user), **Monitor Error**, **System Error**.

In `.ecce-meta` the state is `state`; the reason for an ended job is
`runStatusReason`.

### Projects, studies and workflows

- A **project** is a folder. Calculations live in a project, optionally in
  subfolders. File > New > Project... makes one.
- A **study** is a project-like folder whose members are tasks with
  dependencies: GROMACS and NWChem MD studies (Prepare, Optimize, Equilibrate,
  Dynamics); a task starts from the one before it and is launched after that
  one has completed.
- A **workflow** (steps may be MLP (MACE) too; the geometry link carries the
  cell of a periodic structure) draws calculations that start from the geometry of an earlier
  one as a tree, with **Launch Ready Steps** (help page `workflows`). A
  **recipe** is a workflow template for a kind of study (`recipes`,
  `writing-a-recipe`).

Read-only workflow folders can be opened in the Organizer to view their
stored steps and links. Opening does not rewrite step indices; use a writable
copy to change or run the workflow.

### Where the data is, and deployment modes

- **Local data**: the folder printed by `ecce-localdata`
  (`~/.ECCE-local`). Settings and the machine registry are in `~/.ECCE`.
- **Data server**: calculations stored on a server that several users reach
  (`ecce -remote`); see "Data server" below.

There are three deployment modes (GETTING_STARTED.md, "Deployment modes"),
decided by the site's configuration in this order: a shared system broker
(`siteconfig/SharedBroker`), then `ecce -remote`, otherwise local.

1. **Local**: one user, ECCE's own broker for each session, no network
   port. The session's broker file is `~/.ECCE/broker_<key>` (the key comes
   from `ECCE_SESSION_ID`). On Linux and macOS the broker is the program
   `$ECCE_HOME/libexec/eccebroker`, started by `ecce-gateway-start`; it
   listens on a Unix socket in `~/.ECCE` (file: `user=local`,
   `socket=<path>`; no login, the 0700 directory keeps other users out).
   On Windows it is built into the Organizer, on 127.0.0.1 at a free port
   (`user=`, `password=`, `host=`, `port=`). A command reads that file
   (the key's host part is `%COMPUTERNAME%` there, so a command run from any
   Command Prompt finds the session ECCE started). The
   broker stays up while any ECCE program (the gateway, Builder, a job
   store) is connected and ends 10 s after the last one leaves
   (`ECCE_BROKER_LINGER` on Linux and macOS), removing the file;
   `ecce-gateway-stop` removing the file also ends it. `ecce-gateway-status`
   names it. `ECCE_BROKER=mosquitto` set before `ecce` runs the per-user
   mosquitto instead (installed separately).
2. **Central server**: a data server and broker on one machine; users connect
   as clients with `ecce -remote` (the teaching set-up, also the original
   PNNL set-up).
3. **Shared broker**: one system service (`ecce-broker.service`) on an
   application server.

The mode changes who may connect, not how you use the commands.

## What ECCE can do

The codes ECCE knows are the `.edml` files in `data/client/cap/` of the
source (`data/client/config/ResourceDescriptor.xml` lists which calculation
types a project may hold). Run types come from `scripts/codereg/<code>runtype.py`; the Calculation
Editor's Run Type list is the authority for what one version offers. Codes
marked (experimental) are newer and less tested.

| Code | Run types (as offered) | Typical use | Help page |
|---|---|---|---|
| ECCE-QM (built in, `localhost`, no setup) | Energy, Gradient, Geometry, Vibration, GeoVib, Opt-TS, IRC; relaxed scans through Geometry Constraints | HF and DFT (SVWN, PBE, B3LYP, PBE0) on molecules of a few tens of atoms; localised orbitals, MO diagram | `ecce-qm`, `ecce-qm-files` |
| ORCA | Energy, Geometry (minimum or TS), Vibration, GeoVib, IRC | DFT, composite methods, post-HF | `orca-composite-methods`, `running-codes-on-linux` |
| Gaussian 16 | Energy, Gradient, Geometry (minimum or TS), Vibration, GeoVib | Molecular QM; licensed, installed by the site | `running-codes-on-linux` |
| NWChem | Energy, Geometry, GeoVib, Property; plane-wave PSPW and BAND | Molecular QM; MD through NWChemMD studies | `first-calculation` |
| MOPAC | Energy, Geometry, GeoVib, Vibration | Semi-empirical | `running-codes-on-linux` |
| xTB | Energy, Geometry, GeoVib, Molecular Dynamics, Conformer Search, Docking | GFN2-xTB, GFN1-xTB, GFN-FF | `xtb` |
| CREST | Conformer Search, Solvation Shell, Protonation Sites, Tautomers | Conformer ensembles (needs `xtb`) | `crest` |
| Quantum ESPRESSO (experimental) | Energy, Geometry, vc-relax, Dynamics | Periodic plane-wave DFT | `qe-first-calculation`, `qe-choosing-settings` |
| CP2K (experimental) | Energy, Geometry | Periodic and large DFT | `cp2k-scf`, `cp2k-basis-potential` |
| GROMACS (experimental) | Prepare, Minimisation, Equilibrate, Dynamics (a study); MD Analysis, umbrella sampling | Biomolecular and condensed-phase MD | `gromacs-first-calculation`, `gromacs-prepare`, `gromacs-md-settings`, `md-analysis`, `umbrella-sampling` |
| MLP, MACE (experimental) | Geometry, Vibration, Molecular Dynamics | Screening and relaxation of periodic cells | `mlp` |
| Vina | Receptor and Ligand docking | AutoDock Vina poses and scores | `vina` |

`data/client/cap/` also holds older codes (Gaussian 98, 03 and 09, GAMESS-UK,
MOLCAS, NWChemMD, MetaDyn); do not expect new features for them.

ECCE-QM ghost centres (counterpoise): in a `.qmin` geometry block a fifth
column `ghost` after the coordinates (` O 2.91 0.0 0.0 ghost`) makes that atom
a ghost: the element's basis functions, no nucleus, no electrons. Single-point
`runtype energy` only; gradients, optimisation, frequencies, solvent, `charges`
and polarizability are refused with a message. The output's `begin atoms` block
ends such rows with `ghost`.

Counterpoise calculation (ECCE-QM and ORCA, Energy runs, structure with 2 or
more fragments): SetupParams keys `ES.Runtype.Counterpoise` (1 = on) and
`ES.Runtype.CounterpoiseOwn` (1 = also each monomer in its own basis). Fragments
come from the Builder hooks (`fragments auto`, `fragment N atoms|charge|mult`)
and are stored in the `.frag` file (`fragment_states`, `atom_fragment_list`).
`ai.ecceqm`/`ai.orca` append the monomer decks to the one input file after
`# ecce-run fragment-N[-own]` lines; `gensub` splits them off, runs them after
the complex in the same folder and appends a `begin counterpoise` energy block
to the output. Properties (Hartree, Energies panel group Counterpoise):
`EINTCP` = E_AB(AB) - sum E_X(AB), `EINTRAW` = E_AB(AB) - sum E_X(X),
`EBSSE` = EINTCP - EINTRAW (positive), and the string `CPTABLE` with kJ/mol and
kcal/mol. The raw energy and BSSE exist only with the own-basis runs.
The Relaxed Scan panel supports ECCE-QM and ORCA stored scans:
`scan cpsingle [launch]` creates one sibling Energy calculation for all stored
geometries; `launch` requests submission through the Organizer with the copied
scan machine settings. The GUI action includes the launch request. A second
call refuses and names the existing calculation. `scan cpcolumn` refreshes and
prints `point coordinate raw_hartree cp_hartree`, or `pending` until complete.
The snapshot is stored in `ES.Runtype.CPScanGeometry` (semicolon-separated
frames of Cartesian Angstrom triples). `SCANCP`, `SCANRAW`, `SCANBSSE` are
Hartree vectors, one per stored geometry; SCANCP equals per-point EINTCP.
`ecce:scanCounterpoise` links the source to the sibling; `ecce:scanSource`
links back. The original scan properties are retained. Table columns ΔE (raw)
and ΔE (CP), the two active plot curves and CSV/PNG exports use interaction
energies referenced to the fragments at each geometry, without a further
lowest/first/largest shift; ordinary comparison scans keep their own zeros.
Test: `tests/launch/counterpoise_scan_test.py` (real ECCE-QM launch).

ECCE-QM scans and IRCs store the orbitals of every point (PTORB/PTMO, about
17 MB for an IRC of 20 atoms). For a lean output set `ES.Runtype.PointOrbitals`
to 0 (checkbox "Store orbitals at every point", Geometry and IRC run types):
in a CalcEd script `details runtype`, then `guiw ES.Runtype.PointOrbitals 0`.
The deck then has `point_orbitals no`; the tables, geometries and final
orbitals are still written, but the orbital panels have nothing per point.
Test: `tests/qm/point_orbitals_test.py`, `tests/qm/localise_deck_test.py`.
Gaussian 16 scan CP and CP-corrected optimisation are not implemented.
Gaussian 16 Energy runs use `ES.Runtype.Counterpoise=1`: `ai.gauss16` writes
native `Counterpoise=N`, consecutive `(Fragment=n)` atom tags and the complex
then fragment charge/multiplicity pairs. Stored fragment IDs are mapped in
first-atom order. `ES.Runtype.CounterpoiseOwn` is unused for Gaussian 16;
`EINTCP`, `EINTRAW`, `EBSSE` and `CPTABLE` are always read from its native
counterpoise summary in the one job. `EINTCP` is the corrected interaction
energy, not the corrected total energy. See
`docs/claude/codereg/counterpoise-job-and-properties-1091.md`.

Property views (which panel shows what) depend on what is stored in `Props/`;
see "Properties". Analysis views: the MO diagram (experimental; check it
against a textbook diagram), Density Map, ELF/LOL/NCI, localised orbitals,
density of states and OPDOS, dipole moment, Compare Energies, Energy Calculator,
thermochemistry, spectra and band matching, MD speeds and diffusion, MD
Analysis. Each has a help page named in `help/src/index.md`.

The Density Profile panel shows only its mode and atoms, the **Options**
button, the orbital list and the plot; quantity, orbital set, spin, axes and
**Show all** are under **Options**. Builder scene hooks (`dp*`, as in
`tests/apps/densityprofile_panel_test.py`) set these controls whether or not
Options is open; `dpmore 1` opens it (for a screenshot) and `dplayout W` prints
which controls are visible, with rectangles, at panel width W (`visible mode,from,to,options,list,plot`).

The Density of States (OPDOS) panel shows the A and B rows (**Use selection**
under each), **Options**, **Open in window...** and the plots; Width, Shape,
Sticks, Integrated OPDOS, Occupied only, From/To, Reset and the exports are under
**Options**. An atom given to one group is removed from the other (the group
just set wins; the status line names the atoms moved, and an empty group is
reported there, not refused). Builder scene hooks (`opdos*`, test
`tests/apps/opdos_panel_test.py`): `opdosatoms "A" "B" [A|B]` (the named group
keeps an atom in both, default A), `opdosmore 0|1` (Options), `opdoslayout W`
(`visible ...` at panel width W, fails if clipped), `opdoswindow` (opens both
plots in the resizable window; `opdosstate` then prints `window_open`,
`window_plots`, `window_curves`, `window_intcurves`, `window_int_shown`),
`opdoswinshot NAME` (picture of that window) and `opdoswinclose`.

The Density Map has a **Fill** tick under **Options** (default off, remembered
in `wxbuilder.ini`): it colours the plane between the contour lines on a log
scale from the outermost level, two signs blue/orange for an orbital, viridis
for a density, with a colour bar in the key. Scene hook `dmfill on|off` sets it
(`dmstate` prints `fill 0|1`); `dmpng` and **Export .png...** draw the picture
as the panel shows it (test: `tests/apps/densitymap_fill_test.py`).
The panel has one **Reset** button (next to **Options**): whole plane, offset 0,
greyed out at the defaults; scene hook `dmreset` presses it (`dmzoomreset` resets
the zoom only, as the double-click does; `dmstate` prints `resetbutton 0|1`).
The Offset row sits under **Options**; `dmlayout W` also reports `optionsrow`,
`resetrow` (`mo` when on the MO line) and `offsetrow shown|hidden`.

## How to do things

For each task: what the user does in the GUI, and what you run.

**Build or import a structure.** User: the Builder, a structure file or the Structure Library (`importing-a-structure`); a calculation
from a finished output: **File > Import Calculation from Output File...**
(`importing-a-calculation`), or right-click a recognised output file in a
listed folder (local or SSH): **Import as Calculation...** (it records the
host:path in the annotation). You: write `.xyz` files with charge and
multiplicity in the comment line as `charge=-1 mult=2`.

**Set up one calculation.** User: select a project, File > New, choose the
code, then Builder (structure) and Calculation Editor (theory, basis, run
type, Theory Details); the first calculation is `first-calculation`. You
cannot do this step; ask the user to make a template calculation. The input
is generated when the calculation reaches Ready; check it in `Inputs/`.

**Launch.** User: Run Mgmt > Launch Selected (`launch-selected`) or the
Launcher. You: after the user's yes, `ecce-launch [on=MACHINE] [profile=NAME]
[queue=QUEUE] [wall=H:MM] <calculation>...`.

**A series or a batch.** User: select calculations in the table, Tools >
New Calculations in Another Code... (or the table's right-click New from
This > Continue in Another Code...; `new-series`): the first new
calculation opens in the Calculation Editor, then "Make the Others" copies
its setup to the rest, each with its source's final geometry, charge and
multiplicity (the same code as `ecce-series`; test hooks `anothercode`,
`anothercodefinish`). For a list of
structures or xyz files: Tools > Batch Add from SMILES (`batch-from-smiles`). You:
`ecce-series <template> <folder> <file.xyz>...` makes one Ready calculation
per structure with the template's setup; `ecce-batch-smiles` does the
SMILES table; then `ecce-launch` the Ready calculations. A different run
type needs a different template.

**Check a result.** See "Checking a result". Read `.ecce-meta`, the run log,
the output, `Props/`.

**Fetch files** the run directory holds and ECCE did not copy back (ORCA
`.hess`, cube files, PDOS): `ecce-fetch`.

**Rerun.** User: a calculation's right-click menu, **New from This > Duplicate** (a copy), or **Run > Reset for Rerun** (the calculation itself), then Launch.
You: `ecce-rerun <calc>` (an identical copy) or `ecce-rerun --last-geometry`
(continue from the last geometry).

**Terminate.** User: Run Mgmt > Terminate Calculation... or Terminate
Selected.... You: `ecce-terminate <calc>`. A user cancel gives Killed.

**Reconnect** (state Waiting for Login, Running?, or after a restart): Run
Mgmt > Reconnect Selected..., or `ecce-reconnect <calc>`.

**Re-parse outputs.** After a parser fix, or when fewer properties show
than the output contains: **Run Mgmt > Re-Parse Outputs** (`reparse-outputs`),
which needs no login, or `ecce-reparse <calc>...` (see below).

**Workflows and recipes.** User: build a workflow from calculations
(`workflows`), or Apply Recipe (`recipes`). Launch Ready Steps starts what
can start. `ecce-workflow` is the command.

**QSAR.** Tools > QSAR... (help `qsar`): one window per folder with Table,
PCA and PLS tabs; commands `ecce-qsar table`, `pca` and `pls`. PCA and PLS
headings are words (Component 1, Variance explained (%), Cumulative ..., Importance in the model (VIP)).
The Table tab's header tooltips and **Key...** explain each current column;
**Copy All** copies the key. `ecce-qsar table --describe` prints all built-in
column definitions, or the current columns when given calculation paths or
`--from-csv FILE`. Add `--json` for `columns` objects (`name`, `description`,
`unit`, `group`, `reference`), or `--csv FILE` for a definition table.
`--key-csv FILE` writes names and formatted key text alongside a normal table.
Measured/user columns identify their input filename (or pasted text);
older stored values without provenance say `unknown source (not recorded)`.
Definitions and the checked help key come from `scripts/ecce_qsar_chem.py`.
QSAR Analysis defaults to unit variance (autoscaling); full scaling names and
transformation tooltips explain the choices. Its report gives variance and
prediction percentages, measured-unit errors and warnings for weak validation
or small samples, with omissions grouped by reason. The Components grid holds
all computed component statistics; Ctrl+C copies selected cells as TSV and
Copy table copies the whole grid with headers. Molecule points have hover
numbers; clicking one selects its molecule in the QSAR window's Table tab. The
Variance plot (second page beside each result plot) shows component and
cumulative explained variance. PLS adds a sorted VIP grid
with descriptor descriptions (VIP > 1: more important than average).
Save Results writes the displayed `analysis_components.csv`,
`analysis_summary.txt` and PLS `analysis_vip.csv` alongside the existing numeric
files and saved model. CLI text uses the same presentation; JSON is unchanged.
In the QSAR window every results grid has a Copy table button and Ctrl+C.
The `qsarpca` and `qsarpls` test hooks accept `[png [width height]]` after the
usual model arguments, save screenshots of the report, grids and variance
plot, and verify real clipboard, hover and click paths (`QsarWindow::verifyPresentation`). Their output directory
contains `presentation.tsv`, `selection.tsv`, `hover.txt` and
`presentation-check.txt` for script assertions. The check file also records
painted axis labels as `plot-x`, `plot-y` and `scree-y`; all use enclosing
1–2–5 tick ranges, with equal ranges on the observed/predicted axes.

**Vina ligand.** A script sets the ligand by sending the detail
`ES.Runtype.Docking.LigandAtoms` (1-based atom range such as `10-15`); in
Runtype Details the *Ligand molecule* list fills that key from the structure's
residues (the editor writes them to `<dialog out file>.molecules`, one
`label<TAB>range` per line, likely ligand first) and an empty key defaults to
the first. The key can still be typed. The same ranking and range formatter
are `LigandSelect::ligandCandidates`/`atomRangeString` (`include/tdat/LigandSelect.H`).
In the Organizer's editor tab Runtype Details opens in the Set-up area (`setup runtype`; `setup info` reports `ligandpick=N`, the candidates listed, and `setup set ES.Runtype.Docking.LigandAtoms 10-15` sets the key); the standalone editor runs the Python dialog. `ECCE_VINA_TRACE=1` makes the Python dialog print how it chose the default (`VINADLG:` lines on the editor's stderr).

**Local file chooser (#342).** Builder File > Open... and Add Structure from
File... use the platform's file dialog (`ecceLocalFile`,
`include/wxgui/LocalFileDialog.H`, last folder per use in
`LocalFileDialog.ini`); "... from Remote Machine..." keeps ECCE's dialog for
registered machines and data servers. A native dialog cannot be driven, so
`ECCE_TEST_LOCAL_FILE=<path>` makes the chooser return that path without
showing (stderr `LOCALFILE: <key> -> <path>`); Builder script
`menu File|Add Structure from File...` then reads it. `ECCE_TEST_OPEN` drives
the remote item. Test: `tests/apps/local_file_test.py`.

**Vina scoring (#1278).** Theory Details key `ES.Theory.Vina.Scoring`,
`vina` (default) or `vinardo` (`setup set ES.Theory.Vina.Scoring vinardo`).
ai.vina writes `scoring = vinardo` only for vinardo; on a Vina without
`--scoring` (before 1.2) the job script drops that line and writes an
`ECCE warning:` line to the output, so the run scores with vina.

**Docking.** Vina calculations (`vina`); `ecce-pdbqt` prepares PDBQT files;
CREST/xTB docking is a run type of xTB.
The Builder Poses panel shows cached ligand-only thumbnails for docking.
For Vina, the line above the table reports the retained count, energy range
and maximum modes from the stored input deck (settings fallback).
Builder script `poses note` returns that text; `poses thumb 1 <file.png>`
saves the cached thumbnail of pose 1 for image checks.

**Diffraction.** Properties > Diffraction and scattering > Diffraction opens a docked Builder
pane (`fibre-diffraction`); Tools > Diffraction opens a standalone window.
View and Source stay above the pattern; Options is a modeless window with
Geometry, Sample and Display groups. Pattern toggles the same view into a
separate window and back, sharing all settings and data. The first pattern
calculates on open; physics edits debounce for about 300 ms and sliders
calculate on release. A newer change cancels the previous external process.
The pointer line shows Calculating while running. After a run exceeds about
two seconds, that structure switches to Update on request. Display settings
redraw without recalculation. No Calculate button is needed.

Existing `diffraction`, `calcdiffraction`, `calcpowder`, `calclayer`, `calcframe`,
`calcmeasure` and `debye` hooks still calculate and **wait for completion**
before exporting. To use the property pane through `ECCE_BUILDER_SCRIPT`:

- `list Diffraction` opens the Properties entry.
- `xrd views` lists the View entries in order with 1 if they can be chosen
  (`debye:1 fibre:1 powder:0 single:0 detector:0` without a cell, #1230).
  `xrd set view N` and `xrd state` use pattern modes, not list positions:
  0 Fibre, 1 Powder, 2 Single crystal, 3 Detector frame, 4 Debye. A
  structure without a cell opens on Debye, a crystal on Powder, and a view
  chosen once is reopened for that calculation in the same session.
- `xrd wait` waits for the current run (and pending debounce); it reports
  `Update required` if a slow structure has unapplied edits. `xrd update`
  explicitly starts those edits; then use `xrd wait`.
- `xrd state` reports view, generation, completed generation, running, dirty,
  manual, docked, source, wavelength and pointer-repeat override.
- `xrd set KEY VALUE` changes a real control (additional values simulate a
  typing burst through the same text events). Text keys: wavelength, min, max,
  step, width, B, distance, film, length, misalignment, repeat, gamma, layer,
  resolution, osc. Choice keys use zero-based indices: view (fibre, powder,
  single crystal, frame, Debye), source (Cu, Mo, Co, Other), scaling (log, sqrt,
  linear), scheme (Film, Dark), transform (Auto, Helical, Finite model), axis,
  zone. `phi` sets the released start-angle slider.
- `xrd breakout 1|0` opens/closes Pattern; opening calculates only if the
  pattern is missing or out of date. Choosing a view (`xrd set view N`)
  always calculates at once, also for a slow structure.
- `xrd canvas-shot FILE` saves the shown pattern canvas as on screen.
  `xrd options`, `xrd close-options`, `xrd defaults` operate Options.
- `xrd hover TWO_THETA` / `xrd click TWO_THETA` send pointer events to the
  powder or Debye plot. For layer/frame use `xrd hover H K L` or
  `xrd click H K L`; they move the pointer to that reflection's position.
  The return text is the tooltip at the pointer. Click pins it and selects
  the powder reflection's row in the Options peak list; `xrd selection` reports
  the pinned state and selected hkl.
- `xrd export OUT.png OUT.csv` exports the current result; use `xrd wait` first.
  `xrd size W H` sizes the Builder for captures, `xrd screen OUT.png` captures
  the display and `xrd options-shot OUT.png` captures Options.

The fibre repeat field is labelled **Layer-line repeat for the pointer (Å),
0 = from the model**. It changes pointer/tooltip layer numbers only. Nonzero
values are retained; zero uses `fibre-geometry.txt` from ecce-xrd. Detector
CSV appends `F,absent` after the old columns. F is the structure-factor
amplitude, before Lorentz/polarisation corrections; extinct reflections are
retained and flagged before the relative cutoff. Tooltips say "extinct in
this model (|F| ≈ 0)", without claiming a space-group classification.

`tests/apps/diffraction_panel_test.py --png DIR` checks the pane, automatic
updates, shared breakout, hover and display-only edits on a 1366×768 Xvfb
display and saves pane and Options screenshots.

`ecce-xrd` also works directly from the command line.
Image views (detector frame, precession layer and fibre film): wheel zooms
about the cursor, left drag zooms a rectangle, middle/right or Shift+drag
pans, Home or double-click empty space fits the whole image. Clicks still
measure spots in the zoomed view. PNG exports the current view; Home first
exports the whole image. CSV keeps the full data. The diffraction powder
panel currently has no zoom controls.
Builder script checks: append `viewtest` to
`calcmeasure layer0|frame OUT.png OUT.csv EXPOSURE SCHEME`, or to
`calcdiffraction OUT.png OUT.csv XYZ EXPOSURE SCHEME` for a fibre pattern.
These send mouse/key events, check cursor anchoring and zoomed hkl picking,
and save `OUT.png.full.png`, `.zoom.png`, `.pan.png`, `.reset.png` and
`.rectangle.png`; `BUILDER: image view: PASS` confirms the checks passed.

**Molecule (Debye scattering)** is available for every structure, including
the contents of a periodic cell, with the Powder sources and 2θ curve exports.

**Other analysis from the command line** (thermochemistry, RMSD, bands,
MD analysis, orbital composition, Morse fits): see "Other command-line tools".

## Compute machines and HPC

A calculation runs on a **machine** registered in ECCE. `localhost` is
registered for the user. A cluster is one more entry. **A machine
registered in ECCE is the way to reach an HPC cluster: ECCE logs in with its
own ssh library, creates the run directory, copies the files, submits to
Slurm (or PBS, LSF, Grid Engine, HTCondor), follows the job and copies the
output back.** If a user needs a cluster, first check whether it is
registered (Machine Browser, or `~/.ECCE/MyMachines` and the site machine
list) before looking for another way in.

### Registering a machine (user, in the window)

Help pages: `machines`, `register-machines`, `allocations`,
`running-codes-on-macos`. **Tools > Register Machines...** in the Organizer
(or **Job > Register Machines...** in the Launcher, or `ecce -machine`).
Tabs:

- **Machine:** host (fully qualified), name, cores and nodes, user name,
  account, run directory, scratch directory, **Allocation profiles** (one per
  allocation; each holds account, run directory, scratch directory and
  default queue; #541).
- **Connection:** shell, login script, **Allocation command**, directory of
  `sbatch`/`squeue`, **How jobs reach this machine** (Advanced).
- **Codes:** for each code, **Modules**, **Program** (full path), **Environment
  variables**, **Command line**; **Find** and **Check** confirm the path.
- **Job script:** the scheduler request lines and commands before and after
  the program; **Preview job script...**, **Advanced: edit file...**.
- **Queues:** **Queue manager**, **Allocation accounts used**, queues with
  limits, **Discover queues...** (Slurm: `sinfo`), **Test submission...**
  (Slurm: `sbatch --test-only`, which submits nothing).

Steps for a cluster: New Machine; host, name, processors, nodes; Codes tab
program paths; Queues tab; Save. Before the first real job run **Preview job
script...** and **Test submission...**. Allocation accounts and per-allocation
run and scratch directories are profiles; once a machine has more than one,
the Launcher shows a **Profile:** choice, and a calculation records
`launch_profile` and `launch_account` in `.ecce-meta`. **Tools >
Allocations...** shows what is left. Opening it never logs in; its
**Refresh** logs in to a machine with no open connection (as Launch does).
Hook (`ECCE_TEST_ORGANIZER`): `allocations` opens the window, waits for the
answers and prints `ALLOC: machine|account|used|checked` per row;
`allocations refresh` presses Refresh first. Test:
`tests/launch/allocations_ssh_test.sh --build <tree>`.

### How ECCE logs in

ECCE uses its own ssh library and keeps a master connection. It asks for the
password, one-time code or unknown host key in dialogs. **You cannot type into
those dialogs; ask the user to** (and to tick **Remember on this computer**
if a keyring is available; a remembered password is never offered for jump
hosts or one-time codes). A machine reached through a port-forward must not be
registered as `localhost`: give it an alias in `~/.ssh/config` and register
the alias (#144).

A password the host refused is never sent again without a dialog: the next
dialog says "The password was refused. Please try again.", and after Cancel
ECCE ends that ssh instead of letting it send empty passwords. Only a connect
(Servers menu, the root's menu, start-up) asks for an SSH folder's login;
listing, refreshing or opening a folder whose login has gone fails quietly
and asks nothing (#1274). Test hooks: `ECCE_TEST_AUTH_ANSWER` answers
passdialog without showing it (`cancel` cancels), `ECCE_TEST_AUTH_LOG=FILE`
then appends `Type<TAB>host<TAB>user<TAB>note=<text>` per dialog;
`tests/apps/fakessh/ssh` with `FAKE_SSH_PASSWORD` plays a password-only
host (`FAKE_SSH_MASTERS`, `FAKE_SSH_AUTH_LOG`: every prompt and answer).

Machines ECCE cannot log in to (two-factor): `noRemoteAccess: true` in
`CONFIG.<machine>` or the `dummy` machine: ECCE writes the input and job script
locally and stops; the user copies them over, submits, and imports the output
with **File > Import Calculation from Output File...**. `userSubmit: true`
still transfers files over ssh and leaves only the submit command to the user.

### What an agent can do on the cluster

Through ECCE's commands, only: `ecce-launch` (submit), `ecce-terminate`,
`ecce-reconnect`, `ecce-fetch` (bring files from the run directory, `job_path`
in `.ecce-meta`, into `Outputs/`), plus `ecce-diagnose` for a problem report.
**There is currently no documented command to put a file on a machine or to
run a command there (issue #1002).** The Machine Browser buttons (Machine,
Disk Usage, Process Status, Allocation) run fixed queries, started by the
user. So when something has to be installed or placed on the cluster (a data
directory, a pseudopotential set, a basis library), tell the user exactly
what and where, and ask them to do it, or to allow you to use their own `ssh`.
Do not use credentials you were not given and do not look for another way in.

### Setting up a code on a machine

In the machine's **Codes** tab (stored in `~/.ECCE/CONFIG.<name>`):

- **Program:** the full path of the executable on that machine. A job without
  one stops with "Path for <code> not found". ORCA needs the full path
  (it finds its helper programs next to itself, and needs the Open MPI named
  in its archive name to run in parallel); on Debian the package `orca` is the
  GNOME screen reader, not ORCA.
- **Modules:** `module load` names for that code only; machine-wide ones go
  on the Job script tab.
- **Pseudopotential directory** (Quantum ESPRESSO only; a script writes
  `QuantumESPRESSOPseudoDir: /cluster/pseudo` in `CONFIG.<name>`): at launch,
  Launcher and `launchjob` alike, the deck's `pseudo_dir` becomes this folder.
  The UPF file names still come from the local library when the deck is
  written. Launch then lists the files the deck names that the folder lacks
  on that machine as a launch warning (`LAUNCHJOB_WARNING=cancel` to stop);
  an unreachable machine gives "Could not check ..." and no warning. Unset:
  the deck is not changed.
- **Environment variables:** `NAME value` per line; names containing `PATH`
  are appended to, others replace. Stored as `<Code>Environment { }`.
- Gaussian 16: `g16root` and a scratch directory (`GAUSS_SCRDIR`); ECCE sets
  `GAUSS_EXEDIR` and `LD_LIBRARY_PATH` itself.
- **Quantum ESPRESSO: the pseudopotential files (UPF) must exist on the
  cluster.** ECCE picks one file per element from the pseudopotential
  directory, which is a field in the calculation's **Theory Details**
  (`ES.Theory.PW.PseudoDir`, default `/usr/share/espresso/pseudo`) and is
  scanned on the computer where ECCE runs, so the same set must be there for
  the input to be generated. On a cluster that directory is elsewhere: set the
  variable **`ESPRESSO_PSEUDO`** in the Quantum ESPRESSO **Environment
  variables** box (`QuantumESPRESSOEnvironment` block) to the cluster's
  directory. At job time it replaces `pseudo_dir` in the input. If that
  directory does not hold the files for the elements used, `pw.x` fails.
  Fill it, or have the user fill it, before the first job (for example with
  the SSSP set; Debian ships it as `quantum-espresso-data-sssp`).
- **ORCA:** program path as above; for NBO add `NBOEXE <path>` to its
  environment.
- **GROMACS:** `gmx` must be on the path of the shell that runs the job
  (module or **Program**). A Prepare task also runs `pdb2gmx` and needs its
  force-field data from the same installation.
- **CP2K:** the version on the cluster must contain the basis sets and
  potentials the input names; an older CP2K can lack ones a newer local CP2K
  has. Check the data files of the version that will run. Locally, the
  data directory is `CP2K_DATA_DIR`.
- **MLP:** the Program is a Python from an environment with PyTorch, ASE and
  MACE; **Set up...** next to Program builds one (`ecce-mlp-setup`).
- **xTB, CREST, MOPAC, NWChem:** program path; CREST runs `xtb` itself, so
  both must be installed.

### Checking a machine before launching

In Register Machines: **Find** and **Check** on each code, **Preview job
script...**, **Test submission...**. In the Machine Browser: the Machine
button (Slurm `sinfo`), Disk Usage, Process Status (the queue), Allocation.
From a terminal: `ecce-diagnose [-o directory] [run-directory]` collects ECCE's
version, logs and configuration into one text file; it changes nothing and
collects no passwords, but names hosts and users, so the user reads it first.

### Lessons from real use

- **A `Slurm { }` block in `CONFIG.<name>` replaces ECCE's whole header.** A
  block with one line gave jobs with no account, partition or time, and sbatch
  refused them ("You must supply an account"). Write the complete header.
  **Copy site setting** on the Job script tab keeps the site's lines.
- **No time-limit capability (`TL`)** means `#SBATCH --time` is dropped and the
  job runs with no limit. **No memory line** means Slurm's default; on a shared
  workstation that can give a 1-core job the whole node's memory, and on a big
  node a per-core default can exceed the node so the job sits as
  `BadConstraints`. `squeue` shows `BadConstraints` for harmless reasons too:
  check `sacct -j <id> -o ReqMem` first.
- **`ecce-launch on=<machine>`** on a calculation with no stored queue uses the
  machine's default queue; make sure that is not a short debug queue.
- **Kebnekaise (HPC2N):** only the `batch` partition accepts jobs and the node
  type comes from `--constraint` (`zen4`: 2 x 128 cores; `skylake`: 28 cores).
  Use one ECCE machine entry per node type (queue **Request lines** can hold
  `#SBATCH --constraint=zen4`). `--mem-per-cpu=2500M` works on Zen4. Software is
  listed at https://docs.hpc2n.umu.se/software/apps/ and by `module spider`.
  Allocations are a rolling 30-day window.
- **Local Slurm on a Linux workstation** (single node, `select/cons_tres` with
  `CR_Core_Memory`, a `DefMemPerCPU`, cgroup enforcement) makes ECCE queue and
  limit jobs properly. Slurm does not support macOS: use `localhost` with no
  queue manager.

### The settings files, for reference (user layer, `~/.ECCE`)

A user setting **replaces** the site setting for that key; it is not added.

| File | Holds |
|---|---|
| `MyMachines` | one tab-separated line per machine: name, host, three `Unspecified` fields, `procs:nodes`, `ssh`, `:Code1:Code2:...`, and capabilities (`MN:RD:SD:UN:PW` plus `AA` allocation account, `Q` queues, `TL` time limit, `MM` memory limit) |
| `CONFIG.<name>` | `<Code>: /full/path`, `<Code>Modules: mod1 mod2`, `QuantumESPRESSOPseudoDir: /dir`, `<Code>Environment { NAME value }`, `<Code>Command { ... }`, and the scheduler header block (`Slurm { #SBATCH ... }`) |
| `Queues` | `Queues:` list of machines with queues, and per machine `<name>\|queueMgrName: Slurm`, `<name>\|queueMgrVersion: 2.0~`, `<name>\|prefFile: <name>.Q` |
| `<name>.Q` | `Queues: q1 q2`, and per queue `minProcessors`, `maxProcessors`, `runLimit` (minutes), `memLimit` (MB), `scratchLimit`; 0 means no limit |
| `MachPrefs`, `MachProfiles` | the user's launch defaults and allocation profiles; written by the window, do not hand-edit |

The safest order: the user creates the machine in the window (New Machine, host,
name, cores, user, account, run directory, Save); then you fill in
`CONFIG.<name>` and the queues, with ECCE closed or the window reopened
afterwards. **Job script > Advanced: edit file...** opens `CONFIG.<name>` as
text and its **Check** names keys ECCE does not read. Copy a file to
`<file>.bak-<date>-<reason>` before editing it. A machine named in `Queues`
that is not registered stops ECCE from starting.

Placeholders in request lines and commands: `$nodes`, `$totalprocs`, `$ppn`,
`$wallTime`, `$memory`/`$memoryM`, `$account`, `$queue`, `$scratchDir`. A request
line whose placeholder is empty is left out. Placeholders do not work in
environment variables.

## Registering a new code (a developer task)

Adding a code to ECCE is a change to the source tree, not a setting. Treat it
as development: work in a clone of the repository, read the checklist first,
and test with an isolated state directory. The pieces:

1. **`data/client/cap/<Code>.edml`**: the code's capabilities (theories, run
   types, basis, properties, which tools handle it) and its importer.
2. **`data/client/config/ResourceDescriptor.xml`**: a `<code>_es` resource type
   and a `ContainsResource` entry under `project`.
3. **`scripts/codereg/`**: the Calculation Editor's dialogs, for example
   `<code>theory.py` and `<code>runtype.py`, written in the dialog grammar.
4. **`scripts/parsers/ai.<code>`**: the input generator, filling the deck
   (and `<code>.launchpp` for launch-time changes; `scripts/gensub` writes the
   job script section for the code).
5. **`scripts/parsers/<Code>.desc`** and the parser scripts it names: what to
   extract from the output into `Props/`; **`<Code>.expt`** imports an output
   made elsewhere.
6. The monitor (`scripts/eccejobmonitor`), property panels
   (`data/client/config/PropertyPanelDescriptor.xml`, `properties`), the
   input checker (`scripts/parsers/verifyinput`), help page and this file.

Read `docs/claude/codereg/index.md`, in particular "New-code checklist"
(found integrating ORCA, #38) and the MOPAC additions (#86), and follow the
entries that apply to each file you change (`grep -rl '<file>' docs/claude`).
The wiki page "Where to find things in the code"
(github.com/FriendsofECCE/ECCE/wiki) is the longer orientation. Memory is
entered in GB for every code and converted in `ai.<code>` or its template.

## Data layout

### Local data

With local data (the default of a stand-alone install), projects and
calculations are ordinary folders. `ecce-localdata` prints the data folder in
use (its default is `~/.ECCE-local`). Below it:

    <data folder>/users/local/<project>/<calculation>/

A project is a folder and may hold projects. A calculation is a folder that
contains `.ecce-meta`.

### Data server

With a central data server (`ecce -remote`, `ECCE_REMOTE_SERVER`), the server
keeps the same layout under its own root and ECCE reaches it over HTTP
(WebDAV). A calculation is then named by a URL, not a path. Give the URL to
the commands; a folder path is only turned into an address for local data.
The URL is shown in the Organizer and has the form
`http://<server>:<port>/<root>/users/<user>/<project>/<calculation>`.

### A calculation

| Item | Contents |
|---|---|
| `.ecce-meta` | metadata, below |
| `Inputs/` | the input as launched: `orca.inp`, `g16.g16in`, `ecceqm.qmin`, `gromacs.mdp`, ... |
| `Outputs/` | the program's output (`*.out`, `g16.g16out`, `ecceqm.qmout`, `gromacs.gmxlog`, ...) and `eccejobstorelog.ecce_run_log`, the submit and monitor events |
| `Props/` | parsed results, one XML file per property, see "Properties" |
| `Parameters/` | the setup: `chemsys.mvm` (the structure), basis set, code parameters |

`Inputs/` and `Outputs/` are what the job read and wrote. `Props/` is what
ECCE parsed from `Outputs/`. `Parameters/` is what the GUI edited; the input
is generated from it when the calculation becomes Ready.

### .ecce-meta

One property per line pair, `ecce:<key>`, the value after two tabs. Keys that
matter when checking or launching:

| Key | Meaning |
|---|---|
| `state` | Created, Ready, Submitted, Running, Running?, Waiting for Login, Completed, Imported, Killed, Incomplete, Monitor Error, System Error (help page "Run states") |
| `runStatusReason` | why a calculation failed, was killed or is waiting |
| `application` | the code (ORCA, Gaussian 16, NWChem, MOPAC, GROMACS, ECCE-QM, ...) |
| `theory`, `runtype` | method and run type |
| `spinmultiplicity` | spin multiplicity |
| `annotation` | the user's note (set with `ecce-annotate`) |
| `launch_machine`, `launch_queue`, `launch_totalprocs`, `launch_maxwall` | the stored launch settings |
| `job_path` | the run directory on the compute machine |
| `progress` | fraction done while running |

Read it; do not write it. To set the `annotation`, use `ecce-annotate`.

**Is it safe to write `ecce:annotation` into `.ecce-meta` while the Organizer
or a job monitor has the calculation open?** Only if the write does what ECCE's
own does, and then it is no better than `ecce-annotate`. One `.ecce-meta`
holds the properties of every calculation in its folder (one line per
property: resource name, property name, type, value, tab-separated, with
backslash escapes for tab and newline). ECCE changes it by taking an exclusive
`flock` on `.ecce-meta.lock` in that folder, reading the whole file, changing
one property and replacing the file by renaming a temporary file over it, all
under the lock. A reader never sees half a file, because of the rename. A
writer that skips the lock can lose an update: a job monitor storing progress
or state in the same folder between your read and your rename overwrites your
change, or you overwrite its, and the state of a sibling calculation goes
wrong. A running Organizer also keeps the values it has read, so it shows a
hand-written annotation only when it next re-reads the calculation, and can
write its old copy back. Use `ecce-annotate`; it goes through the Organizer
when ECCE is running.

### Studies

An MD study (GROMACS, NWChem MD) is a project-like folder whose tasks
(Prepare, Optimize, Equilibrate, Dynamics, ...) are calculations. `linkbase.xml`
in the study holds the chain: a task takes its starting structure from the one
before it, so it can only be launched after that one has completed.

A GROMACS calculation from the user's own files is its own type,
`gromacs_md_run` (content type `ecceMdRun`, no MD editor; `launchjob apptype
URL` prints `GROMACS ecceMdRun gromacs_md_run`). Make it with File > New >
GROMACS Calculation from Files... in the Organizer, or the test hook
`gmxrun PARENT-URL MDP|- STRUCTURE|- TOPOLOGY|- [TPR]` (`ECCE_TEST_ORGANIZER`,
prints `ok <url>`). The .mdp is stored verbatim as `gromacs.mdp`, the topology
as `<name>.gmxtop`, the structure as `<name>.gro`, includes beside the
topology and a `.ndx` with them; a ready `.tpr` is stored as `run.gmxtpr` and
the job skips grompp. A `.pdb` structure is refused (convert with `gmx
editconf`). The calculation is left Ready; launch it as any calculation
(`launchjob mdsetup`, `launch`).

Hand-offs: New from This > Duplicate and Continue from Last Geometry (hook
`duplicate URL [last]`; with `last` the copy's starting .gro is the run's
final frame, Ready), Continue in Another Code from it to another code (hook
`anothercode TYPE SUFFIX FOLDER|- URL`: the final frame, charge 0,
multiplicity from the electron count), and to GROMACS from any calculation:
Continue in Another Code > GROMACS (from files), hook `anothercodegmx MDP GRO TOP
SUFFIX|- FOLDER|- SOURCE...` (one calculation per source, named source +
suffix, Ready; the source's final structure replaces the .gro's coordinates,
refused with the differing atom when the atoms or their order differ; box
kept, velocities dropped). Not a workflow step yet.

### The run directory

The run directory on the compute machine (`job_path`) holds everything the
job wrote, including files ECCE does not copy back (for example ORCA `.hess`).
`ecce-fetch` brings such files into `Outputs/`.

### A separate state directory

ECCE's settings, machine registrations and per-user service state live in
`$ECCE_REALUSERHOME/.ECCE`, which the launchers set to `$HOME`. For an
experiment, point both the home and the data folder somewhere disposable
before starting ECCE and the commands:

    export ECCE_REALUSERHOME=$(mktemp -d)
    export ECCE_LOCAL_DATA=$ECCE_REALUSERHOME/data
    ecce &

All commands below must run with the same two variables as the ECCE they talk
to. Other variables: the `ENVIRONMENT.md` file next to this one.

## Commands

The `ecce-launch`, `ecce-terminate`, `ecce-reconnect`, `ecce-fetch`,
`ecce-annotate`, `ecce-rerun`, `ecce-move`, `ecce-copy`, `ecce-mkdir`, `ecce-rename` and `ecce-workflow` commands do not start ECCE; they ask the running Organizer to act, so ECCE has
to be running with the same data mode. Without it they print `ECCE is not
running: start ECCE first` and do nothing. A calculation is given as a folder
(local data) or a URL (data server). One line is printed per calculation.

Exit codes of these four: 0 everything done; 1 something was not done; 2 no
running ECCE session; 3 the Organizer is busy with a Launch Selected batch
(try again later); 4 no answer within the time limit (it may have been
carried out: check before repeating); 64 usage error.

### ecce-launch

    ecce-launch [--accept-large-qm] [on=MACHINE] [profile=NAME] [queue=QUEUE] [wall=H:MM] <calculation>...

Submits Ready calculations with the rules of **Run Mgmt > Launch Selected**,
using the settings stored in each calculation unless overridden. `on=` and
`profile=` choose a registered machine and allocation profile; `queue=` and
`wall=` replace the stored values. Output per calculation: `name: job id N` or
`name: not launched: reason`; `name: note: ...` lines say which setting was
filled in or dropped (for example no stored queue).

    ecce-launch ~/.ECCE-local/users/local/Examples/water-opt
    ecce-launch on=cluster queue=short wall=2:00 proj/calc-1 proj/calc-2

### ecce-rerun

    ecce-rerun [--last-geometry] [cores=N] [wall="D H:M"] [on=MACHINE] [launch] <calculation>...

Uses the Organizer's **New from This > Duplicate**: copies the full setup
(structure, code, theory, basis, run type and launch settings) into the same
folder, removes the copy's results and makes a finished, killed or failed
calculation Ready. A copy of an imported or Created calculation (a stored
example, say) gets its input deck written and is Ready when its settings are
complete as the Calculation Editor judges them (structure without open bonds,
a cell where the code needs one, a basis covering every element); otherwise
it is Created. The original stays untouched. Names are `<name>_r2`,
`_r3`, and so on, taking the first free name. Submitted, Running and Waiting
for Login calculations and non-calculations are refused with their reason.

`--last-geometry` uses **New from This > Continue from Last Geometry**
instead: the copy starts from the last geometry in the calculation's stored
geometry trace, and its annotation begins `continued from <name> (last
geometry)`. A Submitted, Running or Waiting calculation is ended first, as
`ecce-terminate` does; the result is the same copy as for a killed run. With no
stored geometry yet the command says so (`not rerun: no stored geometry yet`),
makes no copy and leaves a running job alone. `cores=N` and `wall=` (`D H:M`,
`H:M` or minutes) are stored as the copy's launch settings, with or without
`--last-geometry`; on a machine other than the stored one, `on=` still applies
that machine's limits at launch.

With `launch`, submits the new copy as `ecce-launch` does; `on=MACHINE`
overrides its launch machine. Output: `name: rerun as new`, optionally
`, job id N`, or `name: not rerun: reason`. A launch failure identifies the
copy that was made and leaves it available. Other calculations still proceed.
Exit codes, as ecce-move: 0 all done, 1 any refusal, 2 no running
session, 4 no answer in time (may have been carried out; check before repeating), 64 usage. Local data paths and data-server URLs only.

    ecce-rerun project/killed-job
    ecce-rerun on=localhost launch project/finished-job
    ecce-rerun --last-geometry cores=16 wall="2 0:00" launch project/slow-opt

### ecce-terminate

    ecce-terminate [--reason "text"] <calculation>...

Cancels submitted, running or waiting calculations without a confirmation
question. Calculations in other states are refused with their state. The reason
is stored as `runStatusReason`. Output: `name: terminated, job id N` or
`name: not terminated: reason`.

    ecce-terminate --reason "wrong basis set" proj/calc-1

### ecce-annotate

    ecce-annotate <calculation>... "text"

Sets the Annotation field (the note shown in the Organizer) of calculations
and of the tasks of an MD study; the last argument is the text, `""` clears
it. With ECCE running the Organizer stores it as its Annotation editor does
and shows it at once. Without a running session, a calculation in the local
data folder is written through the same data layer (with its locking); with a
data server it needs the running Organizer and says so (exit 2). Output:
`name: annotated` or `name: not annotated: reason`.

    ecce-annotate proj/calc-1 "ORCA B3LYP/def2-SVP, tests the new basis"

### ecce-move, ecce-copy and ecce-rename

    ecce-move <calculation or folder>... <folder>
    ecce-copy <calculation or folder>... <folder>
    ecce-rename <calculation or folder> <new name>

Move calculations (or folders) into an existing folder of the same data store,
or give one a new name, with the Organizer's own move, so its tree is updated
at once. Never move them with `mv` while ECCE is running: the Organizer keeps
stale nodes. Refused, with the reason: a calculation whose job is Submitted,
Running or Waiting for Login, or a folder holding one (the others still move,
exit 1); a name that already exists in the target (nothing is overwritten); a
system folder. The folder may be in another tree (Local data, `sftp://` SSH
folder, ECCE server; give its URL): the move is then copy, verify file count
and sizes, remove the source, and a failure removes the partial copy and
leaves the source. `ecce-copy` copies (a running calculation may be copied).
Output: `name: copied to folder`, `name: moved to folder`, `name: renamed to new name`, or
`name: not moved: reason` / `name: not renamed: reason`. Without a running
ECCE nothing is done (exit 2).

    ecce-move proj/calc-1 proj/calc-2 archive
    ecce-rename proj/calc-1 calc-1-b3lyp

### ecce-mkdir

    ecce-mkdir [-p] [--json] <project or folder>...

Makes a project or folder with the Organizer's own New Project, so its tree
shows it at once; then `ecce-move` can put calculations into it. The parent
must exist unless `-p` is given, which makes the missing parents too and is
not an error when the folder already exists. Output: `path: created`,
`path: exists` (with `-p`), or `path: not created: reason` (exists without
`-p`, missing parent, invalid name, outside the data folder, inside a
calculation); with `--json` an array of `{"path", "ok", "created", "url"}` or
`{"path", "ok": false, "reason"}`. Exit codes as ecce-move: 0 all done, 1 any
refusal, 2 no running session, 4 no answer in time, 64 usage.

    ecce-mkdir -p proj/series/step-1
    ecce-move proj/calc-1 proj/series/step-1

### ecce-reconnect

    ecce-reconnect <calculation>...

Reconnects calculations that were left Waiting for Login, or Submitted or
Running with nothing following the job (ECCE restarted, connection lost).
Reconnecting reads the job's output again from the start. A machine that needs
a password asks for it in the Organizer window. Output: `name: reconnected,
job id N` or `name: not reconnected: reason`.

    ecce-reconnect proj/calc-1

### ecce-reparse

    ecce-reparse [--json] <calculation>...

Runs the Organizer's Run Mgmt > Re-Parse Outputs: the current parsers read the
outputs stored with each finished calculation again, with no login to the
machine. This is how an existing calculation picks up a parser fix. The
properties are replaced; state, dates and the run log stay. The command waits
until the last calculation has been parsed. Output per calculation: `name:
done`, `name: failed: reason`, or `name: refused: reason` (a Submitted,
Running or Waiting job, GROMACS and NWChemMD, no stored output, or not a
calculation). `--json` prints an array of `{"name","status","reason"}`. Exit 0
only if all are done; 2 without a running ECCE.

    ecce-reparse proj/calc-1 proj/calc-2

### ecce-fetch

    ecce-fetch [--list] <calculation> [pattern ...]

Copies files from the run directory into the calculation's `Outputs/`, for a
job whose automatic copy-back missed them. Works on finished calculations
too. Patterns are file-name patterns relative to the run directory; the
default is `*.cube *.cub *.pdos`. `--list` lists the run directory and stores
nothing. Output per file: `name: fetched (N bytes)`, `pattern: no file
matched`, or `name: not fetched: reason`. Exit 0 needs at least one file
fetched (or listed) and nothing refused.

    ecce-fetch --list proj/calc-1
    ecce-fetch proj/calc-1 '*.hess'

### ecce-contacts

    ecce-contacts [--format text|csv|json] [--hbonds] [--group] [--cutoff-extra A] <file>

Reads a structure or an ensemble file and prints coordination shells,
hydrogen bonds and, for an ensemble, a pattern key per frame (`--group` keeps
one representative per pattern with its count). It reads files and talks to no
ECCE session.

    ecce-contacts --hbonds --format json solvated.xyz
    ecce-contacts --group --hbonds trajectory.xyz

Exit code 0 on success; anything else is an error message on stderr.

### ecce-rate

    ecce-rate REACTANT[,REACTANT2] TS [PRODUCT[,PRODUCT2]] [--T 298.15 | --T LOW:HIGH:STEP] [--standard-state 1M|1atm] [--treatment SPEC] [--sigma N] [--tunnelling none|wigner|eckart|all] [--json]

Conventional TST rate constant k = kappa sigma (kB T/h) c0^(1-m) exp(-dG‡/RT)
from stored calculations; dG‡, dH‡, dS‡ and the ZPE-corrected barriers come from
`ecce-thermo` (same `--treatment`, default `qrrho-grimme:100`). The TS must have
exactly one imaginary mode (otherwise exit 1 with the reason); reactants and
products with imaginary modes give a warning. kappa is none, Wigner or Eckart
(unsymmetric, from the imaginary frequency and the ZPE-corrected forward and
reverse barriers; symmetric with a note when no product is given). With a
product: reverse k and K_eq consistent with dG_rxn. Over a `--T` range:
Arrhenius (A, Ea) and Eyring (dH‡, dS‡) fits. Units are in the output
(`unit` in `--json`): s^-1, M^-1 s^-1 or cm^3 molecule^-1 s^-1. Half-life is
ln2/k, pseudo-first order at the standard concentration for two reactants.
Formulas in `ecce-rate --help`. Exit 0 done, 1 refused or failed. In the GUI, Compare Energies > Reaction overview shows the same per reaction; **Copy Details** copies its Details text as plain text.

    ecce-rate proj/complex proj/ts proj/product --T 250:400:25 --json

### ecce-ensemble

    ecce-ensemble pick <crest calc> [n=N] [window=E] [--rmsd A] [--skip-failed] [--create TEMPLATE FOLDER] [--json]
    ecce-ensemble minimum <calculation|folder|pattern>... [--treatment SPEC] [--json]
    ecce-ensemble benchmark <spec file> [--treatment SPEC] [--T K] [--save NAME] [--dir DIR] [--json]

Reads stored calculations; talks to no ECCE session. `pick` lists the N
(default 5) lowest distinct members of a CREST ensemble: members with the same
protonation and a heavy-atom RMSD below `--rmsd` (default 0.50 A, after a
fit, atoms in stored order) are merged. `window=E` (kcal/mol; `window=3kcal`,
`window=12.5kJ`) keeps only members within E of the lowest member of the
ensemble (stored energies); `n=` then caps the count, and a window without
`n=` is not capped. Order: merge, window, n. JSON has `window_kcal_mol`, and
each pick's `energy_rank` is its row in the Builder's ensemble table
(**Select Distinct...** selects exactly these rows).
Each pick is checked against the
input structure: protonation unchanged, every ion that is bound in the input
still bound, metal-N/O/S contacts listed with distances (cutoffs in `--help`).
`--create` makes one calculation per pick through `ecce-series` from the
setup of a template calculation, without launching. `minimum` gives the
lowest-G calculation without imaginary modes; calculations with imaginary
modes, without frequencies or without G are listed and excluded, and
calculations of different methods are refused. `benchmark` reads reactions
over species (several calculations per species: the lowest-G minimum is used),
and gives dG with the 1 M standard state (as `ecce-thermo`), log K,
experimental log K with its source and calc - exp; `--save NAME` stores
`NAME.benchmark.json` next to the spec file. The spec file format is in
`ecce-ensemble --help`. Exit 0 all done, 1 something not done.

    ecce-ensemble pick proj/crest-run n=5 --json
    ecce-ensemble minimum 'proj/conformers/*' --treatment qrrho-grimme:100
    ecce-ensemble benchmark proj/logk.txt --save logk-set1

Thermochemistry contributions and reaction H/G at another temperature
(#1044). `ecce-thermo CALC --json` carries, for each treatment, the
translational, rotational, vibrational and electronic contributions to U
(kJ/mol, without ZPE; `ZPE` separately), S and C_V, plus `Cv` and
`Cp` = Cv + R (J/mol/K); a single atom has no rotation. The sigma ECCE-QM
prints (`symmetry_number`) is stored as THERMOSIGMA and used, as for ORCA and
Gaussian. The Thermochemistry panel shows the same numbers in a second table;
the Builder script command `thermo <treatment> [T] [1atm|1M]` also prints
`BUILDER: thermocontrib <row>|U|U+ZPE|S|Cv|Cp` for the five rows. In the
Energy Calculator, the `ECCE_TEST_ORGANIZER` hook
`compareenergies <A> <B> <treatment|stored> <T>` recomputes the H and G rows at
T (with `stored`, Grimme qRRHO 100 cm-1), and `comparedialog <A> <B> <png|-> <T>`
sets the dialog's T / K field; the table then has an `S (T K)` row,
(dH - dG)/T in J/(K mol).

    ecce-thermo proj/N2 --treatment rrho --T 723 --json
    ecce-thermo proj/N2 --treatment rrho --contributions

### ecce-batch-smiles

    ecce-batch-smiles [--template CALC] [--name COL] [--smiles COL] [--response COL]... [--no-responses] [--header yes|no] [--replace] [--dry-run] [--launch] [--json] <folder> <table.csv|.tsv|->

Makes one calculation per row of a table of names, SMILES and measured values
in `<folder>`, as **Tools > Batch Add from SMILES** does: the structure comes
from the SMILES (formal charges give the charge) and is cleaned, and the setup
is that of `--template` (default: xTB GFN2-xTB optimisation). `-` reads the
table from standard input. Columns are given by header or 1-based number;
without `--name`, `--smiles` and `--response` they are guessed, and `--header`
says whether the first line is a header. The measured values are stored on each
calculation (`ecce:qsar.measured`, JSON keyed by the column header). `--replace`
makes again the calculations of the same name that were never launched;
`--dry-run` only reads the table and reports the rows; `--launch` runs
`ecce-launch` on the new calculations and needs a running ECCE. Output per
row: `name: made <url> (charge N)` or `name: not made: reason`. Exit 0 all
made, 1 some not, 2 no folder or table, 64 usage error.

    ecce-batch-smiles --dry-run proj/series logk.csv
    ecce-batch-smiles --template proj/xtb-template proj/series logk.csv

### ecce-qsar

    ecce-qsar table <folder|calc>... [--groups G,...] [--no-qm] [--csv FILE] [--json]
    ecce-qsar table --from-csv FILE [--smiles-col N] [--name-col N] ...
    ecce-qsar pca TABLE.csv [--y COL]... [--exclude COL]... [--scale uv|pareto|ctr|none] [--components N] [--cv G] [--outdir DIR] [--json]
    ecce-qsar pls TABLE.csv --y COL [--exclude COL]... [--scale uv|pareto|ctr|none] [--components N] [--cv G] [--test-names A,B | --test-fraction F --seed S] [--permutations N] [--save-model FILE] [--outdir DIR] [--json]
    ecce-qsar predict MODEL.json TABLE.csv [--csv FILE]
    ecce-qsar columns TABLE.csv
    ecce-qsar groups

Reads stored calculations and table files; talks to no ECCE session and
changes no calculation. `table` makes one row per calculation of a folder made
by Batch Add from SMILES: name, state, measured values, descriptors computed
from the SMILES, and the calculation's own results (HOMO, LUMO, dipole,
charges); calculations whose optimisation did not converge are flagged in the
columns `opt_converged` and `problems`. `pca` and `pls` fit a model to such a
table, `predict` applies a saved model, `columns` lists a table's columns and
`groups` the descriptor groups. `ecce-qsar --help` has the rest.

### ecce-umbrella

    ecce-umbrella windows <pull calc> [--spacing 0.1] [--from D] [--to D]
    ecce-umbrella extract <pull calc> --out DIR [same choices]
    ecce-umbrella wham <window calc>... [--bins 200] [--temp 300] [--zero end|min|none] [--jacobian]

Umbrella sampling from a stored GROMACS pull run; talks to no ECCE session.
`windows` lists the frames of the pull trajectory at the chosen spacing of the
pull coordinate (frame, time, distance); `extract` writes `window_NN.gro` (the
whole system, box included) into `DIR`; `wham` runs `gmx wham` on the windows'
`topol.tpr` and `pullf.xvg` and prints the PMF (kJ/mol) and the histograms,
with overlap warnings. Files are looked for in a calculation's `Outputs`
folder, then in the folder itself. `gmx` is `--gmx`, else `$ECCE_GROMACS`,
else `gmx` or `gmx_mpi` on PATH. `--json` is available for the Builder panel's
use.

### ecce-workflow

    ecce-workflow make <calculation> [name]
    ecce-workflow move <calculation> <workflow>
    ecce-workflow create <workflow-or-new-name> <calculation>... [--link A:B[=role]]... [--auto-links]

**Make Workflow...** and **Move into Workflow >** of the Organizer: `make`
moves a calculation into a new workflow of its folder as a step, `move` into an
existing one, and `create` does this for several calculations of one folder
with the links given (A and B are calculation names) or, with `--auto-links`,
the links the dialog would propose. Like `ecce-launch` it asks the running
Organizer, so ECCE has to be running. Output: `ok: <workflow URL>` or
`error: reason`. Exit 0 done, 1 refused or no answer, 2 ECCE is not running,
64 usage error.

### ecce-xrd

    ecce-xrd fibre STRUCTURE [--select LIST] [--axis auto|selection|x,y,z] [--transform auto|helical|finite] [--map film|rz] [--out FILE]... [--json] ...
    ecce-xrd powder STRUCTURE [--source cu|mo|co] [--tt-min D] [--tt-max D] [--fwhm D] [--measured FILE] [--table FILE.csv] [--out FILE]... [--json] ...
    ecce-xrd debye STRUCTURE [--select LIST] [--source cu|mo|co] [--wavelength A] [--no-doublet] [--tt-min D] [--tt-max D] [--step D] [--B A2] [--measured FILE] [--out FILE]... [--json]
    ecce-xrd layer STRUCTURE (--zone h=N|k=N|l=N | --normal U V W [--offset N]) [--resolution D] [--out FILE]... [--json] ...
    ecce-xrd frame STRUCTURE [--source cu|mo|co] [--phi D] [--osc D] [--distance MM] [--label N] [--out FILE]... [--json] ...

Calculates X-ray diffraction patterns of a structure file (.xyz, .pdb or
.mvm; `--select` takes atom numbers from 1, such as `1-40,57`); talks to no
ECCE session. `fibre` is the pattern of molecules aligned on one axis and
rotated about it, as a film or an (R, Z) map. `powder`, `layer` and `frame`
need a periodic structure: a .cif, an .mvm with lattice vectors, or an .xyz
whose comment line has `Lattice="ax ay az bx by bz cx cy cz"`. `powder` gives
intensity against 2θ with the K-alpha1/K-alpha2 doublet when `--source` is
given, and draws a measured pattern (text columns, .xrdml or .brml) as a
second curve with `--measured`. `layer` is a reciprocal-lattice layer (hk0,
h0l, 0kl or a higher layer) with spots sized by |F|^2. `frame` is the
detector frame of a rotating crystal. `--out` takes `.png`, `.csv` or, for
`fibre` and `frame`, `.npy`, and may be repeated. Wavelengths and cell
lengths are in Å, angles in degrees. `ecce-xrd <subcommand> --help` lists
every option; the help page **Diffraction** describes the views.

`debye` needs no lattice. It calculates the orientational average
`I(q) = sum_i f_i(q)^2 + 2 sum_{i<j} f_i(q) f_j(q) sinc(q r_ij)`, with
`sinc(x) = sin(x)/x`, `sinc(0) = 1` and `q = 4*pi*sin(theta)/lambda` in Å⁻¹.
The neutral-atom IT92 factors and B damping are shared with Powder; no
Lorentz/polarisation factor is added. Periodic inputs contribute the cell
contents once, without images. At most 300 atoms use exact distances;
larger inputs use an element-pair histogram of 0.002 Å bins (a 1441-atom protein
in seconds rather than minutes). Pair generation
is O(N²) for both methods.
In the Builder, Diffraction is never the pane a structure opens on and never computes
until selected. A run longer than 2 s shows a progress bar with Cancel in the pane;
Builder script hooks `xrd cancel` and `xrd progress` do the same. The CLI summary and JSON state which was used.
JSON includes q, raw intensity and relative intensity arrays; `.csv` holds
`two_theta,intensity` (maximum 100 in the requested range), and `.png` plots
the same curve. With `--source`, the default doublet sums Ka1 and Ka2 in a
2:1 ratio. There is no Bragg reflection table or peak-width option.
The Builder script hook `debye OUT.png OUT.csv STRUCTURE.xyz [SOURCE [DOUBLET]]`
exports its current structure, switches from the initial view to Debye, and
saves the curve through the dialog. SOURCE is 0 Cu, 1 Mo, 2 Co or 3 custom;
DOUBLET is 0 off or 1 on. This hook supports isolated GUI tests.

Fibre **Transform** / `--transform auto|helical|finite` defaults to Auto:
fit a screw from consecutive backbone P or peptide N atoms in file order
(at least six, rigid-fit RMSD < 0.15 Å), rationalize the twist to t/u turns
(u <= 20, within 1°), and calculate only orders with l - n t divisible by u
on Z = l/c, c = u times the measured rise. Interior steps are folded and
averaged into a screw unit. Auto falls back to the finite-model sum when the
fit fails; forced Helical errors. Select one regular helix in a mixed model.
Finite model is available for other fibres. `--json` reports `transform` and
`helix` parameters; `--geometry FILE` writes `repeat C` (0 for finite).
The window fills **Repeat** from that result. Length broadening is Gaussian
FWHM 1/length, followed by angular misalignment and the existing display.
Teaching B-DNA: 10 units/turn, rise 3.387 Å, c 33.867 Å; polyalanine:
18 units/5 turns, rise 1.503 Å, c 27.061 Å, pitch 5.412 Å.
The DNA fourth layer is suppressed by motif interference, not forced to zero.

    ecce-xrd fibre helix.mvm --map rz --rmax 0.7 --zmax 0.7 --out rz.png
    ecce-xrd powder nacl.cif --source cu --measured scan.xy --out powder.png
    ecce-xrd debye molecule.xyz --source cu --out debye.csv --out debye.png --json

### Other command-line tools

On Linux these commands are on PATH. In the macOS app they are in
`ECCE.app/Contents/Resources/bin`; add that folder to PATH to use them from a
terminal. The Windows package currently supplies only the main `ecce.cmd`
launcher, without command wrappers for these tools.

| Command | Purpose |
|---|---|
| `ecce-localdata` | Print the local data folder |
| `ecce-series` | Work with a series of calculations (the Organizer does the same with Tools > New Calculations in Another Code) |
| `ecce-check-props` | Check stored property dimensions |
| `ecce-si-export` | Export structures and a Supporting Information table; `--checks` makes the Checks table (with core-hours), `--rank` the Compare Energies table. Command line only |
| `ecce-displacement` | Compare geometries and aligned RMSD |
| `ecce-bandshift` | Match vibrational bands and report shifts |
| `ecce-spectrum` | Process spectral curves. `--exp FILE` and `--parse-exp FILE` read the experimental spectrum as text columns or JCAMP-DX (`.jdx`, `.dx`, `.jcamp`; units from the file, so `--exp-kind` is ignored for JCAMP; NMR/MS files give a one-line error); `--parse-exp` prints `kind`, a `label` line (title, state, origin) and the points. The reader alone: `python3 scripts/jcampdx.py FILE` (#986) |
| `ecce-displace` | Displace a structure along a normal mode. The Vibrational Frequencies panel of a GeoVib run with imaginary modes below 100 cm-1 shows a note that the optimisation stopped at a saddle (Builder script hook `vibnote` prints it; #1042). The Builder's Properties list groups its rows as Vibrational spectra, Electronic spectra, Magnetic resonance and Diffraction and scattering (#1079); the Builder script command `plist` prints the list as `GROUP/NAME` lines |
| `ecce-thermo` | Recompute thermochemistry from stored results; `--json` gives per treatment H, S, G and the contributions `U_trans/U_rot/U_vib/U_elec`, `ZPE` (kJ/mol), `S_*`, `Cv_*`, `Cv`, `Cp` (J/mol/K); `--contributions` prints them as text |
| `ecce-rate` | TST rate constants with Wigner/Eckart tunnelling from reactant, TS and product calculations |
| `ecce-force-compare` | Compare stored forces (command line only) |
| `ecce-mlp` | Run an ML potential job from its input file |
| `ecce-confsearch` | Run a simple conformer search from its input file |
| `ecce-qm` | Run the bundled quantum chemistry program, when built |
| `ecce-results` | Convert and inspect results files, when built with HDF5 |
| `ecce-fragorb` | Fragment-orbital analysis (`analyse <complex> <fragment>...`) and PDOS/OPDOS (`dos`) from results.h5 files of a complex and its fragment calculations at one geometry; `--json`, `--help` has the syntax |
| `ecce-npa` | Natural population analysis (Reed–Weinstock–Weinhold NPA) computed by ECCE from a calculation's results.h5 (`ecce-npa <calc URL\|folder\|results.h5> [--json]`): natural charges, core/valence/Rydberg populations, spin populations (open shell), natural electron configuration; agrees with NBO 7's NPA within 0.005 e; refuses semiempirical, periodic and Cartesian-d/f calculations with a one-line reason; the Viewer's Properties list has the same charges as **Natural (NPA) Charges, computed by ECCE** (property key `NPAECCE`, computed on first selection and stored in results.h5 as `charges/npa_ecce`), beside the code's own NBO row **Natural (NPA) Charges** (key `NPA`). results.h5 now records ECP cores (`ecp` group: atom, name, ncore; `nucleus/charge`) from the stored basis or the ORCA/Gaussian output; an ORCA basis named only on the route card now also gets a results.h5 (the generated deck carries `PrintBasis`, and the basis is read from the printed block; ORCA runs made before this have no block and get no file) |
| `ecce-morse` | Morse fit of a stored bond-length scan: De, a, re, ω_e, ω_e x_e, D0 (`ecce-morse <calc> [--window KJ_PER_MOL] [--atoms I,J] [--masses M1,M2] [--json]`) |
| `ecce-gmxanalysis` | GROMACS analysis of a stored GROMACS calculation, as text, CSV or JSON: `<calc> groups`, `rdf`, `energy`, `rms`, `gyrate`, `density`, `hbond`; selections like `"residue:SOL & element:O"`; needs `gmx` |
| `ecce-mocomp` | Atomic-orbital composition of one molecular orbital (`CALC --mo N\|homo\|lumo [--beta] [--method mulliken\|c2] [--min PCT] [--by ao,shell,type,atom] [--tsv]`), `--list` the orbitals, `--spin` Mulliken spin populations |
| `ecce-density-field` | ELF, LOL, reduced density gradient, sign(λ2)ρ or density from a calculation's stored orbitals as a cube file (`CALC --field F [--spacing A] [--margin A] [--threads N] -o out.cube`), or all fields at points (`CALC --at X,Y,Z`); local data |
| `ecce-pdbqt` | Prepare AutoDock PDBQT files for docking: `--frag FILE --atoms LIST --as ligand\|receptor -o OUT`, `--ligand FILE.sdf`, `--receptor FILE.pdb`, `--info FILE` |
| `ecce-site-screen` | Interstitial or vacancy site screening of a periodic host: `make <template> interstitial\|vacancy --element X`, then `results <folder>` for E - E(host) in eV |

See the matching help page for inputs, options and units.

### Administration

These run on the machine that hosts or connects to a data server, not on a
calculation.

| Command | Purpose |
|---|---|
| `ecce-diagnose [-o directory] [run-directory]` | Collects what is needed to diagnose a problem into one text file, `ecce-diagnostics-<host>-<time>.txt`; on macOS it also copies the newest five crash reports of ECCE's programs from the last 7 days (`~/Library/Logs/DiagnosticReports`, `/Library/Logs/DiagnosticReports`) into `crash-reports/` beside it. Help > Report a Problem... logs each step on stderr as `Report a Problem: ...` (Organizer hook `reportproblem`). Changes nothing and collects no passwords, but the file holds host names, user names and paths: read it before attaching it to an issue. `ecce --bug` runs it too |
| `ecce-remote-setup <server-host> [data-port]` | Points this client installation at a central server; run once per client installation, as the owner of the installation. Users then start `ecce -remote` |
| `ecce-dataserver-start`, `ecce-dataserver-status`, `ecce-dataserver-stop` | Start, check and stop this user's data server (`ecce-dataserver-stop --if-unused` refuses while another session or a job still uses it) |
| `ecce-dataserver-adduser` | Add an account to this user's data server (`-b USER PASS [FIRST] [LAST]` or `--stdin USER [FIRST] [LAST]` without prompts) |
| `ecce-site-publish [DIR]` | Copy this install's site machine list to where the data server serves it to `-remote` clients |

## Checking a result

In this order:

1. **State.** `.ecce-meta` `state`: Completed means the code finished
   normally, not that the result is right. For Killed, Incomplete, Monitor
   Error or System Error read `runStatusReason`.
2. **The run log.** `Outputs/eccejobstorelog.ecce_run_log` shows submit,
   queue and monitor events; a failed submit leaves the calculation Ready with
   the reason there.
3. **The output.** The end of `Outputs/` file and the code's own convergence
   lines.
4. **The parsed properties.** Read `Props/` first, units are in the XML. A
   scalar is `<value name="TE" units="Hartree">...</value>`, a vector
   `<vector name="VIBFREQ" rows="30" units="cm-1">...</vector>`; orbitals and
   geometry traces are tables.
5. **The chemistry.** An optimisation converged by the code's criteria rather
   than stopped; 0 imaginary frequencies for a minimum and exactly 1 for a
   transition state, along the intended coordinate; the same method, basis,
   dispersion and solvent model before energies are compared; electronic,
   ZPE-corrected and Gibbs energies not mixed; open shells with spin
   multiplicity as intended and S2 near its ideal value; the basis set listed
   once; the geometry sensible (bond lengths, nothing dissociated); for MD,
   equilibration reached before averages are taken.

State what was checked and what was not. Do not claim agreement with the
literature or another program without having compared.

### Properties

`Props/<KEY>` files. Common keys:

- Energies: TE (Hartree), TEVEC (per optimisation step), DELTAE, EGIBBS, ETIME.
- Geometry: GEOMTRACE (per step); SCANENERGY, SCANGEOM, SCANCOORD (relaxed
  scans); IRCCOORD, IRCENERGY, IRCGEOM (IRC path). The Builder lists
  Geometry Trace when GEOMTRACE is stored, and Geometry Step Plots (per-step
  vectors such as TEVEC, PRESSVEC) only when it is not (GROMACS tasks).
- Gradient: EGRAD, EGRADVEC.
- Vibrational spectrum files (#986): the Spectrum panel's Experimental...
  (script `vibspectrum <csv> <calc|-> <diff> <file|-> <zoom|->`,
  `ECCE_SPECTRUM_EXPERIMENTAL=<file>`) accepts JCAMP-DX besides text columns.
  Export .csv... writes JCAMP-DX when the file name ends in `.jdx`, `.dx` or
  `.jcamp` (`spectrumcsv`, `ECCE_SPECTRUM_CSV=<path>.jdx`, or the first
  argument of `vibspectrum`): one LINK file, block 1 the broadened curve
  (XYDATA, width and scaling factor in `##$` lines), block 2 the sticks
  (PEAK TABLE, mode number as a `$$` comment); width 0 gives the peak block
  only. Reading it back takes the first block, the curve.
- Vibrations: VIBFREQ (cm-1; leading near-zeros are translations and
  rotations), VIBIR (km/mol), VIBRAM, VIBSYM.
- Orbitals: MO, ORBENG, ORBOCC, MOAOORDER; LOCMO, LOCMOENG, LOCMOOCC,
  LOCMOLABEL (localised orbitals, ECCE-QM).
- Charges: MULLIKEN, LOEWDIN, HIRSHFELD, CHELPG, MK, APT where computed.
- ORCA EPR: EPRG (one row: g1, g2, g3, g_iso, dimensionless), EPRA
  (per requested nucleus: A1, A2, A3, A_iso in MHz), EPRQ (per nucleus
  with a quadrupole result: Cq, Q1, Q2, Q3 in MHz). In the Builder's
  Magnetic resonance group these are **EPR g-tensor**, **EPR Hyperfine
  Couplings**, and **EPR Quadrupole Couplings**. Nuclear row labels retain
  ORCA's zero-based output indices and element symbols; only computed nuclei
  appear. Q1..Q3 use ORCA's printed spin-Hamiltonian prefactor and eta.
  Import or reparse an ORCA output containing EPR sections to populate them.
  See the help page `orca-epr` for conventions and the required input.
- Other: DIPOLE, S2; for MD TIMEVEC, TEMPVEC, PRESSVEC, MDPEVEC, MDKEVEC.

The full list with units is `data/client/config/properties` in the ECCE
source. `ecce-results` converts orbitals and Hessians from Molden, fchk and
ORCA files, and CP2K's Molden file plus its output (`ecce-results convert
cp2k-MOS-1_0.molden r.h5`, then `ecce-results convert cp2k.out r.h5`; the
orbitals are valence only and the file records the GTH core charges), into one
HDF5 file and checks the conversion (help page "The
results file format").

## Documentation shipped with ECCE

The help pages are in `<install>/doc/help/` (sources `help/src/*.md` in the
repository). Read the page before explaining a feature, for example
`launch-selected`, `reconnect-selected`, `run-states`, `ecce-qm-files`,
`machines`, `register-machines`, `importing-a-calculation`. Do not guess menu
names.

## Keeping this file current

A new command or option is added to this file in the same change that adds it,
as for the help pages. `tests/ai/agents_md_test.py` fails when a command
named under "Commands" is not in the build.

### NMR chemical shifts (#384)

Builder **Properties > Magnetic resonance > NMR spectrum** uses `ISOSHIELD`
(ppm; isotropic shielding from the parsed tensors). Choose H (1H), C (13C)
or another element and supply its reference shielding. Defaults are empty;
there is no implicit TMS value. `delta_ppm = sigma_reference - sigma_atom`.
Reference calculation chooses a sibling calculation in the project folder
and averages its shieldings of the selected element. Use the same method,
basis and solvent; ECCE does not verify this. References are separate per
element and live only as long as the panel. The reversed ppm plot groups
atoms whose shifts span at most 0.01 ppm, at their mean shift, with atom
count as stick height. Clicking selects the group's atoms (1-based numbers
in the table and exports). J-couplings are outside this view.

With `ECCE_BUILDER_SCRIPT`, after `property NMR spectrum`:

- `nmr element H|C|SYMBOL` selects an element present in the structure.
- `nmr reference VALUE|empty` enters a shielding in ppm or clears it.
- `nmr calculation NAME` takes that element's mean shielding from a sibling
  calculation with the given name.
- `nmr width PPM` sets Lorentzian FWHM (0: sticks only).
- `nmr click N` clicks group N, numbered from 1, and logs selected atoms.
- `nmr csv PATH` exports reference provenance, grouped and per-atom data,
  plus an area-normalised envelope in nuclei per ppm when width is positive.
- `nmr png PATH` exports the current plot at 1200 by 760 pixels.
- `nmr status` prints element, reference, width, axis direction and groups.

The script logs `NMR: ok=0|1`; export is refused without a reference. Put
`background 1 1 1` before screenshots. Help: [NMR spectrum](../help/src/importing-a-calculation.md#nmr-spectrum).

## Scripted large ECCE-QM launches

The stand-alone Launcher asks **Large ECCE-QM calculation** in a wx modal
dialog. Its `ECCE_LAUNCHER_SCRIPT` hook accepts `answer yes` after
`launch` (or `sizecheck`) to choose **Launch anyway**; `answer cancel`
keeps the calculation Ready. `answer ok` and `answer no` handle other
wx modal dialogs. Commands run in order, including while a dialog is open.

`ecce-launch --accept-large-qm CALCULATION` explicitly accepts the size
warning. The Organizer's batch submission already proceeds without this
question, including when the option is omitted. `ecce-launch --help` lists
the option. This does not change the calculation's resource settings.
