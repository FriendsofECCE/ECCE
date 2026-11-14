# The Organizer's menu bar, toolbar and Preferences

This page lists, menu by menu, what the Organizer's menu bar and toolbar
offer, and the tabs of the Preferences window. The right-click menus of the
tree and the project table are on
[The Organizer's right-click menus](organizer-menus.md).

Several menus are rebuilt each time the selection changes, so what they offer,
and which items are grey, depends on what is selected and, for a calculation,
on its run state.

## File

- **New**: what can be made in the nearest project or folder at or above the
  selection, or in the home folder when nothing is selected: a project, a
  calculation for each registered code, an MD study, a workflow.
- **Upload File...**: copies a file from this computer into the selected
  calculation. Offered when a calculation is selected.
- **Download File...**: saves the selected file to this computer. Offered
  when a file is selected.
- **New Structure...** (Ctrl+N): opens an empty Builder.
- **Import Calculation from Output File...**: makes a calculation from the
  output of a run made outside ECCE; see
  [Importing a calculation](importing-a-calculation.md).
- **Import GROMACS Run...**: makes a GROMACS study from the run directory of
  a GROMACS simulation made outside ECCE.
- **Open Example Calculations**: shows the example calculations that come
  with ECCE; see [Example calculations](example-calculations.md).
- **Export > Supporting Information...**: writes the selected calculations
  as one multi-structure XYZ file, with an optional CSV or LaTeX table; see
  [Export for Supporting Information](si-export.md).
- **Quit** (Ctrl+Q): closes the Organizer and ends the session, asking first
  when **Ask for confirmation on exit** is set in Preferences.

## Edit

- **Duplicate for Rerun**: a copy of the selected calculation with the same
  structure and settings, ready to launch. The same as **New from This >
  Rerun** in the right-click menu.
- **Duplicate Setup with Last Geometry**: a copy that starts from the
  geometry the calculation ended on. The same as **New from This > Continue
  from Last Geometry**.
- **Duplicate with Final Orbitals**: as the previous item, and the copy also
  starts from the final orbitals. The same as **New from This > Continue from
  Final Orbitals**.
- **Cut** (Ctrl+X), **Copy** (Ctrl+C), **Paste** (Ctrl+V): move or copy the
  selection into the selected project or folder.
- **Rename**: renames the selected item; one item only.
- **Delete** (Del): deletes the selection, asking first when **Confirm
  Deletes** is on. In the tree, Backspace does the same.
- **Annotation...**: edits the annotation, a free-text note, of the selected
  item.
- **Find...** (Ctrl+F): lists the items below the folder in **From** whose
  name, or the path to them from that folder, contains the text typed
  (upper and lower case differ). **Open** selects the chosen result in the
  tree.
- **Change Access Control...**: sets who may read or change a project on a
  data server. It is grey for a local folder, which has one user, and with
  more than one item selected.
- **Preferences...**: opens the Preferences window (below).

## Options

The check items are kept between sessions; (on) marks those on by default.

- **Show Toolbar** (on), **Show Run State Legend** (on): show or hide them.
- **Show Lines in Tree View** (on): draws the lines that join the items of
  the tree.
- **Show Run States in Tree** (on): shows the run-state icon of each
  calculation in the tree.
- **Show ECCE Internal Files**: shows the files ECCE keeps inside a
  calculation for its own use.
- **Set Home as Root**: makes your home folder the top of the tree.
- **Ask for File Name at Creation** (on): asks for a name when an item is
  made; when off, the item is named after its type.
- **Invoke Default Tool at Creation**: opens the default tool of a new item
  as soon as it is made.
- **Confirm Deletes** (on): asks before **Delete** removes anything.
- **Confirm Resets** (on): asks before an editor resets a calculation to
  rerun, which removes the output of the previous run.
- **Sort Tree by**: **Type**, **Name** or **Date**, each also reversed.
- **Project View**: the columns of the project table: **Basic Properties**,
  **Setup Properties**, **Launch Properties** or **All Properties**.

## Go

- **Home** (Alt+Home): selects your home folder on the current server.
- **Up** (Alt+Up): selects the folder above the selection.
- **Back** (Alt+Left), **Forward** (Alt+Right): move through the places
  selected before.
- Below a separator, the last 15 places selected, newest first; choosing
  one selects it.

## Bookmarks

- **Add to Bookmarks** (Ctrl+D): bookmarks the selected item.
- **Manage Bookmarks...**: opens the Bookmark Editor, where a bookmark's
  name and URL are edited, and **Add**, **Delete**, **Move Up** and **Move
  Down** change the list.
- Below a separator, one item per bookmark; choosing one selects that item
  in the tree.

## Run Mgmt

The menu acts on the selected calculation, and its items depend on the run
state; see [Run states](run-states.md). With several calculations selected
it holds only **Launch Selected...**, **Reconnect Selected...**, **Terminate
Selected...** and **Re-Parse Outputs**.

- **Tail -f on Output File...** (Alt+L): a terminal that follows the output
  file as the job writes it.
- **Shell in Calculation Run Directory...** (Alt+S): a terminal in the run
  directory on the compute machine.
- **Terminate Calculation...**: stops the job on the compute machine.
- **Check Job Monitoring**: says whether the job monitor of the calculation
  is still running.
- **Check Job on Machine**: asks the compute machine whether the process or
  queue job is still there and when the output was last written. It only
  reports; it does not change the state.
- **Reconnect Job Monitoring**: starts a new job monitor for a job started
  earlier; grey, with the reason as its help text, when the job cannot be
  reconnected.
- **Force State to Failed**: sets the state of a submitted, running or
  waiting job to failed, without touching the job itself.
- **Reset to Rerun** (Alt+R): resets a finished calculation so that it can
  be launched again; the outputs of the previous run are removed. Imported
  calculations are skipped, since the reset would delete the only copy of
  their results; use **Edit > Duplicate for Rerun** for them.
- **Reset for Restart**: sets the state back to ready; the outputs stay.
- **Reset State**: changes the state alone, to completed, incomplete,
  imported or killed.
- **Mark as Reviewed**, **Mark as Unreviewed**: set or remove the reviewed
  mark and its date.
- **Cleanup Run Directory Files**: removes the files of the calculation from
  its run directory on the compute machine.
- **View Run Log...**: the log ECCE kept of the launch and the monitoring of
  the job.
- **View Input file...** (Alt+I), **View Output File...** (Alt+O): open the
  file in the editor set in Preferences.
- **Launch Selected...**: see [Launch Selected](launch-selected.md).
- **Reconnect Selected...**: see [Reconnect Selected](reconnect-selected.md).
- **Terminate Selected...**: stops the jobs of the selected calculations.
- **Re-Parse Outputs**: see [Re-Parse Outputs](reparse-outputs.md).

## Tools

The upper part is the same whatever is selected. The lower part lists the
tools of the selected item under the heading **For** *name*. With two
calculations selected only the upper part is shown; with more than two the
menu is grey.

- **Register Machines...**: defines a compute machine; see
  [Registering machines](register-machines.md).
- **Machine Browser...**: shows the registered machines.
- **Allocations...**: see [Allocations](allocations.md).
- **Periodic Table...** (Ctrl+P).
- **Energy Calculator...**: see [Energy Calculator](energy-calculator.md).
- **New Calculations in Another Code...**: one new calculation in another
  code from each selected calculation, with a setup you make once; see
  [New Series](new-series.md).
- **Batch Add from SMILES...**: see
  [Batch Add from SMILES](batch-from-smiles.md).
- **QSAR Table...**: see [QSAR Table](qsar-table.md).
- **QSAR Analysis...**: see [QSAR Analysis](qsar-analysis.md).
- **Builder**: shown when the selection has no tools of its own; opens an
  empty Builder.
- **Electronic Structure Editor...** (Ctrl+E): the Calculation Editor of the
  selected calculation.
- **Builder...** (Ctrl+B): the structure of the selected calculation in the
  Builder.
- **Basis Set Tool...** (Ctrl+S).
- **Launcher...** (Ctrl+L): from the ready state on.
- **Viewer...** (Ctrl+R): the results of the selected calculation.
- For an MD task, the editor of its kind (Prepare, Optimize, Energy or
  Dynamics), named after the code of the task.

## Help

- **ECCE Help** (Ctrl+H): these pages, in the browser set in Preferences.
- **Running Codes on This Computer**: the help page for your system, such
  as [Running codes on Linux](running-codes-on-linux.md).
- **Report a Problem...**: collects diagnostics into a zip archive in your
  home folder; ECCE sends nothing itself. See
  [Reporting a problem](installation.md#reporting-a-problem).
- **How to Cite ECCE...**: see [Citing ECCE](citing-ecce.md).
- **About ECCE...**: the version of ECCE and, for a development build, its
  commit.

## Developer

Shown only when the environment variable `ECCE_DEVELOPER` is set.

- **Generate Theory/RunType Test Suite**: makes a project *name*_TestSuite
  beside the selected calculation, with one calculation for each combination
  of theory and run type of its code.
- **View Properties...**: the stored properties of the selected item.
- **Manage Data Servers...**: opens the list of data servers; the tree is
  rebuilt when it is closed.

## Toolbar

From left to right, named by the tooltip of each button:

- **Go to user home on current server**, **Go up**, **Go back**, **Go
  forward**: as in the **Go** menu.
- **Cut**, **Copy**, **Paste**, **Rename**, **Delete**: as in the **Edit**
  menu.
- **Search File...**: as **Edit > Find...**.
- **Refresh**: reads the selected part of the tree again.
- **Upload File...**, **Download File...**: as in the **File** menu.
- **View Properties...**: as **Developer > View Properties...**, and here
  also without `ECCE_DEVELOPER`.
- **Servers**: your remembered SSH folders and ECCE servers; see
  [Servers on the toolbar](organizer-menus.md#servers-on-the-toolbar).

## Preferences

**Edit > Preferences...** opens the Preferences window. Each change is saved
as it is made; **Close** hides the window and **Reset All** returns every
setting to its default. The settings apply to all ECCE programs.

### General

- **Font Size**: Small, Medium, Large or Extra Large.
- **Use the system font size**: the desktop's font size instead of ECCE's
  10 pt; applies when the programs are next started.
- **Date Format**, **Time Format**: how dates and times are written.
- **Units**: the units in which energies, lengths and angles are shown:
  AU (Hartree, Bohr, radian), CGS (kcal/mol, Å, degree) or SI (kJ/mol, nm,
  radian).
- **Beep on errors**, **Beep on warnings**.
- **Focus follows mouse over input fields**: a field takes the keyboard when
  the pointer is over it.
- **Ask for confirmation on exit**: asks before the session ends.
- **Close remote shells on exit**: closes the terminals opened on compute
  machines when the session ends.
- **Save data and compute server passwords between sessions**.
- **Check for new releases**: at most once a day asks GitHub which releases
  exist, sending only ECCE's version; nothing is downloaded or installed.
- **Also tell me about previews (alpha and beta releases)**.
- **Quick transparency**: draws transparent orbitals and isosurfaces faster,
  and less accurately where lobes overlap.
- **Show the MO diagram for ECCE-QM calculations**: for the other codes it
  is always offered.

### Colours

- **Colour scheme**: Jmol or ECCE (classic), the element colours of the 3-D
  view, the periodic table and thumbnails.
- **Style**: Flat or Shaded run-state icons in the tree and legend.

### External programs

Each row offers presets and **Other**, which takes a command with its
arguments. `ECCE_EDITOR`, `ECCE_TERMINAL` and `ECCE_BROWSER`, when set, win
over these settings, and the tab says so.

- **Editor**: for input and output files. **Default** uses `VISUAL` or
  `EDITOR` when set, otherwise `vi` (`open -t` on macOS).
- **Terminal**: for terminal editors and for the shells the programs open.
  **Default** is `xterm` on Linux and Terminal on macOS.
- **Web browser**: for Help. **Automatic** uses `xdg-open`,
  `x-www-browser` or `sensible-browser`, whichever is found first.

### Data

- **Folder**: where the calculations on this computer are kept. **Change...**
  picks another folder; **Open** shows it in the file manager.
- **Servers**: the remembered servers. **Add Server...** and **Edit...** open
  the server dialog, whose **Connect at start** chooses whether the Organizer
  connects when it starts; **Remove** takes a server out of the list without
  deleting anything on it.
- **Discard the programs' own binary files once results.h5 is written**:
  for jobs that finish afterwards, and for Gaussian 16 checkpoint files only;
  see [The results file](results-file-format.md).

### Advanced

Switches for testing and troubleshooting; see
[Installation](installation.md). A variable already set in the environment
wins, and the tab says so.

- **OpenGL drawing**: Automatic, Software-safe or Hardware.
- **Native (C++) Details dialogs**: the theory Details dialogs from the C++
  code instead of the Python one.
- **Per-pixel lighting off**: for when the 3-D picture is wrong or blank.
- **No progress records for older calculations**: the Organizer does not
  work out a progress record for a calculation that has none.
- **3-D view diagnostics**, **Window and panel diagnostics**: write details
  to the error output.
- **MO-diagram symmetry log**, **Data-server login log**: append details to
  the file given.
- **Remembered ssh logins**: the hosts whose SSH passwords are kept;
  **Forget** removes one. Shown where a password store is available.

### Gateway window

Present only when the Gateway window is in use (`ECCE_GATEWAY_WINDOW=1`).

- **Show system busy icon** (on): the ECCE icon of the Gateway window moves
  while work is going on.
- **Always on top**: the window manager may override it.
- **Left click invokes new tool**: swaps what a click on a tool button does,
  between bringing up the open copy of the tool and starting a new one.
- **Horizontal Orientation** or **Vertical Orientation**: the tool buttons
  in a row or in a column.
