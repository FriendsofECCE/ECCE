# Your first calculation

In this tutorial you optimise the geometry of a water molecule with NWChem
at the Hartree-Fock level, then look at the energy, the optimisation steps
and, optionally, the vibrational frequencies.

You need:

- ECCE installed and started with `ecce` (see [Installation](installation.md)).
- NWChem installed, so that the command `nwchem` works in a terminal. The
  machine `localhost` runs it for you (see [Machines](machines.md)).

The calculation takes seconds.

## 1. Create a project

A project is a folder that holds calculations. Every calculation belongs to
a project.

1. In the Organizer, select your home folder in the tree on the left.
2. Choose **File > New Project...**.
   You can also right-click the folder and choose **New... > Project...**.
3. In the window "New Project Name", enter `tutorial`.
4. Click **OK**.

The project appears in the tree. Names may contain letters, digits, `.`,
`_` and `-`. [TO CHECK: that a new user's home folder offers **New
Project...**; the resource types allow a project only inside a project.]

![The Organizer with the new project](img/organizer-project.png)

<!-- capture: Organizer with the project "tutorial" selected in the tree -->

## 2. Create a calculation

1. Select the project `tutorial`.
2. Choose **File > New NWChem Calculation...**.
   You can also right-click the project and choose **New... > NWChem
   Calculation...**.
3. In the window "New Calculation Name", enter `water-opt`.
4. Click **OK**.

The calculation appears under the project, with the run state **Created**.
It has no molecule yet.

By default ECCE does not open an editor for the new calculation. (The
Organizer menu **Options** has the entry **Invoke Default Tool at
Creation** if you prefer that.)

## 3. Build the molecule

1. Select `water-opt`.
2. Choose **Tools > Builder...** (Ctrl+B).

The Builder opens on this calculation. To add water from the structure
library:

1. In the **Mode Toolbar**, click the button with the tooltip **Import from
   Structure Library**. You can also choose **Mode > Add Structure**
   (Ctrl+7).
2. In the **Structure Library** panel, open the folder `SimpleStructures`,
   then the folder `Miscellaneous2`. Open a folder by double-clicking it.
   [TO CHECK: the name the library is listed under in the **Libraries**
   drop-down, and that folders are shown by these names.]
3. Click `h2o` in the list. A preview appears in the panel.
4. Click in the empty 3D view. The molecule is added there.

If you prefer to draw the molecule yourself:

1. Choose **Mode > Atom** (Ctrl+5), or click **Choose Build Element** in the
   Mode Toolbar.
2. Choose the element O.
3. Click in the empty 3D view to place the oxygen atom.
4. Choose **Build > Add Hydrogen** (Ctrl+F) to complete the valence with
   hydrogen atoms.
5. Choose **Build > Clean** (Ctrl+K) to tidy the geometry.
   [TO CHECK: that **Add Hydrogen** gives two hydrogens on a bare oxygen.]

Then save the structure:

1. Choose **File > Save** (Ctrl+S).
2. Close the Builder with **File > Quit** (Ctrl+Q). If it asks "Save Builder
   Changes?", save.

![The Builder with water](img/builder-water.png)

<!-- capture: Builder window, water in ball-and-stick, Structure Library panel closed -->

## 4. Set up the calculation

1. Select `water-opt` in the Organizer.
2. Choose **Tools > Electronic Structure Editor...** (Ctrl+E).

The editor shows what you built under **Chemical System**: **Formula:**
`H2O`, **Atoms:** 3, **Electrons:** 10. **Charge:** is 0 and **Spin
Mult.:** is Singlet, which is correct for water. [TO CHECK: that the
**Formula:** field shows the formula in this form.]

Set the rest:

1. Under **Code**, check that the button shows NWChem.
2. In the **NWChem Settings** box, choose `RHF` in **Theory:**.
   `RHF` is restricted Hartree-Fock for a closed-shell molecule.
3. In **Runtype:**, choose `Geometry`. This optimises the geometry.
4. Under **Basis Set**, click **Quick Basis Menu**, then choose `6-31G*`.
   The fields **Name:**, **Polarization:**, **Functions:** and
   **Primitives:** show the chosen basis set.
5. Click **Verify**. This checks the input for obvious problems before you
   submit it.
6. Choose **File > Save** (Ctrl+S).

The buttons **Theory Details...** and **Runtype Details...** open further
options for the chosen theory and run type. The defaults are right for
this calculation.

To see the input file ECCE will send to NWChem, click **Final Edit...**.
[TO CHECK: what **Final Edit...** opens and whether closing it is safe
without changes.]

![The Electronic Structure Editor](img/calced-water.png)

<!-- capture: Electronic Structure Editor with NWChem, RHF, Geometry, 6-31G* set for water -->

## 5. Launch

1. In the editor, click **Launch...**.
   Or select the calculation in the Organizer and choose **Tools >
   Launcher...** (Ctrl+L).
2. In the Launcher, choose `localhost` in **Machine:**.
3. In **Run Directory:**, enter a folder in your home directory where the
   job may run, for example `/home/<you>/ecce-runs`. The field is marked
   `*`, which means it is required. [TO CHECK: whether the field is
   pre-filled and whether the folder is created if it does not exist.]
4. Leave **Username:** empty. An empty user name, or your own, runs the job
   on this computer.
5. Click **Launch**.

The Launcher shows progress messages and, at the end, "Successfully
submitted job." [TO CHECK: the processor, queue and memory fields are not
shown for `localhost`.]

![The Launcher](img/launcher-localhost.png)

<!-- capture: Launcher with Machine: localhost, Run Directory filled, before Launch -->

## 6. Watch the run

Return to the Organizer. The icon of `water-opt` shows its run state.
**Options > Show Run State Legend** (on by default) lists the states and
their icons.

The states you see, in order, are **Submitted**, **Running** and
**Complete**. A calculation that ended with an error is **Failed** or
**Unsuccessful**. You can also select the calculation and read its summary
in the panel next to the tree.

To read the files while the job runs, select the calculation and use the
**Run Mgmt** menu:

- **Tail -f on Output File...** follows the output as it grows.
- **View Output File...** and **View Input file...** open the files.
- **View Run Log...** shows ECCE's log of the job.

If the state is **Failed**, open **View Output File...** and read the end
of the file. The most common causes are a wrong path to the code (see
[Machines](machines.md)) and an input error that **Verify** would have
reported.

## 7. Look at the results

1. Select `water-opt`.
2. Choose **Tools > Viewer...** (Ctrl+R).

The Viewer shows the final geometry. Open the results from its
**Properties** menu. Each entry shows or hides a panel.

- **Calculation Summary** lists the theory, run type and basis set, and
  when and where the job ran.
- **Energies** lists the energies of the calculation, including **Total
  Energy**. [TO CHECK: the exact rows shown for an NWChem RHF geometry
  run.]
- **Geometry Trace** shows how the energy changed during the optimisation,
  one point for each geometry step. Click a point on the plot to see the
  molecule at that step, or use the playback control to step through them.
  **Delay:** sets the time between steps during playback.

For a converged optimisation the energy decreases from step to step and
levels off in the last steps.

![The Viewer with the Geometry Trace panel](img/viewer-geometry-trace.png)

<!-- capture: Viewer on the finished water-opt calculation, Geometry Trace panel open with the energy plot -->

### Vibrations (optional)

To also compute vibrational frequencies, repeat the tutorial with a second
calculation:

1. In the project, select `water-opt` and choose **Edit > Duplicate for
   Rerun**. Or create a new calculation as in step 2.
2. In the Electronic Structure Editor, choose `GeoVib` in **Runtype:**.
   `GeoVib` optimises the geometry and then computes the frequencies.
   [TO CHECK: that **Duplicate for Rerun** is under **Edit** and keeps the
   structure and settings.]
3. Save and launch as before.
4. In the Viewer, choose **Properties > Vibrational Frequencies**.
5. Use the **Table** and **Graph** tabs to see the three frequencies.
6. Select a frequency and click the button with the tooltip **animate
   normal mode** to see the motion. **Scale:** sets the size of the
   displacement and **Delay:** the speed. **stop animation** ends it.

A geometry optimisation that has reached a minimum has no imaginary
frequencies.

## Where next

Change the theory to `RDFT`, or the basis set to `cc-pVTZ`, and compare the
total energy. The next chapters describe each window in more detail.

<!-- sources:
  data/client/config/ResourceDescriptor.xml: line 177 (New Project...), 241 (New NWChem Calculation...), 2859-2900 (tool labels: Electronic Structure Editor..., Builder..., Launcher..., Viewer...), 2961-3011 (Run Mgmt labels: Tail -f on Output File..., View Run Log..., View Input file..., View Output File...), 270-340 (default tools, states)
  src/apps/organizer/CalcMgr.C: 2380-2420 and 4780-4830 (New... submenu with "New " removed; name dialog "New <Label> Name", "Please enter the name for the new <Label>:"; default Label "Project"/"Calculation"), 2480-2570 (File menu: New items, New Structure..., Import Calculation from Output File...), 2435 (Duplicate for Rerun in the New... submenu)
  src/apps/organizer/CalcMgrGUI.C lines 279-298 and CalcMgrGUI.pjd 782-1010 (Options menu: Show Run State Legend, Invoke Default Tool at Creation, Ask for File Name at Creation checked by default; Invoke Default Tool unchecked)
  src/apps/organizer/CalcMgrGUI.pjd 457-760 (Edit menu items, including Duplicate for Rerun; the pjd has it under Edit)
  src/apps/builder/Builder.C: 286-313 (panel and mode names), 625-650 (Mode toolbar tooltips), 760-790 (Mode menu items and shortcuts), 1181-1210 (Add Structure behaviour), 1900-1915 (Structure Library panel shown in Add Structure mode)
  src/apps/builder/BuilderGUI.C 171-182 (File menu: Save, Quit)
  src/wxviz/viztools/ViewerEvtHandler.C 866-883 (Build menu: Clean, Add Hydrogen)
  src/apps/builder/StructLibGUI.C 126-147 (Libraries, No Current Selection), StructLib.C 440-475 (folder activation)
  data/client/StructureLibrary/SimpleStructures/Miscellaneous2/h2o.mvm (water structure)
  src/apps/calced/CalcEdGUI.C 196-215 (File menu: Save, Quit), 221-540 (Code, Chemical System, Charge:, Spin Mult.:, Formula:, Atoms:, Electrons:, Symmetry:, Basis Set, Basis, Quick Basis Menu, Name:, Polarization:, Functions:, Primitives:, Settings, Theory:, Theory Details..., Runtype:, Runtype Details..., Verify, Final Edit..., Launch...)
  src/apps/calced/CalcEd.C 118-122 (quick basis list, includes 6-31G* and cc-pVTZ), 528 ("<Code> Settings" box title), 2231-2290 (Theory and Runtype lists show the names from the code's .edml)
  data/client/cap/NWChem.edml 82-165 (theories RHF, RDFT, ...; run types Energy, Gradient, Geometry, Vibration, GeoVib, Property)
  src/apps/launcher/WxLauncherGUI.C 216-231, 249, 349-700 (Job menu, Machine:, Run Directory:, Username:, Launch)
  src/apps/launcher/WxLauncher.C 1640-1690 (local run when Username empty or own), 2160-2235 (launch messages)
  siteconfig/Machines (localhost options WS) and src/tdat/resources/MachineOptions.C 31-33, 50-53 (WS shows Machine, Username, Password, Priority, Run Directory, Scratch Directory)
  data/client/config/PropertyPanelDescriptor.xml 52-80, 131-137, 186-191 (Calculation Summary, Energies, Geometry Trace, Vibrational Frequencies)
  data/client/config/properties line 60 (Total Energy)
  src/apps/builder/GeomTracePropertyPanel.C 178-187 (Delay:, playback control)
  src/apps/builder/NModesGUI.C 171-247 (Animation/Vector, Graph/Table, Frequencies, Scale:, Delay:, animate normal mode, stop animation)
  src/apps/builder/Builder.C 4598-4730 (Properties menu is filled from the panels the calculation's properties select)
-->
