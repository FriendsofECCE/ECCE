# The Organizer's right-click menus

## A calculation

Right-click a calculation in the tree or in the project table.

- **Cut**, **Copy**, **Paste**, **Rename** and **Delete** work as for any item.
- **Move to...** and **Copy to...** open a list of folders, and put the
  selection in the one you choose, as Cut or Copy followed by Paste would. The
  folder the item is in is selected to begin with, and **OK** stays grey for a
  folder that cannot take it, with the reason below the list. The list shows
  every tree the Organizer shows, so it can also move work to a server.
- **New from This** makes a new calculation from this one:
  - **Rerun**: a copy with the same structure and settings, reset and ready
    to launch.
  - **Continue from Last Geometry**: a copy that starts from the geometry the
    calculation ended on.
  - **Continue from Final Orbitals**: the same, and the new calculation also
    starts from the final orbitals. It is grey when the code or the stored
    results cannot give a starting guess; the tooltip says why.
- **Run** collects what is done with the job: **Launch...**, **Reconnect...**,
  **Terminate**, **Re-Parse Outputs** and **Shell in Run Directory**. Items
  that do not fit the state of the calculation are grey. Reconnect is
  available when the job was started from this installation and has a job id
  and a run directory.

The Builder, the Calculation Editor, the Basis Set Tool and the Viewer are
opened from the toolbar and the **Tools** menu.

## Several calculations, or a folder

With two or more calculations selected in the project table, or with a folder
selected in the tree, **Compare Energies...** lists their energies relative to
each other; see [Compare Energies](compare-energies.md). **Tools > Energy
Calculator...** is a different window, for the energy of sums of calculations
such as a reaction; see [Energy Calculator](energy-calculator.md).

## The project table

The table shows the type and application of each calculation in one column
(**Type**), its name, state, progress, **Total energy**, dates, formula,
theory, run type and machine. **Total energy** is the final energy of a
finished calculation in Eh, as the Progress box shows it; it is empty
when the calculation has none, and for a MOPAC calculation, whose result is a
heat of formation.

Some functions are only on the command line; see
[Scripting and AI assistants](scripting-and-ai.md).

## Servers on the toolbar

The **Servers** button lists your remembered SSH folders and ECCE servers.
A tick means connected. Tick an entry to connect; clear its tick to disconnect.
**Add Server...** remembers another location. **Manage Servers...** opens
**Preferences > Data**, where you can edit the list.

Right-click a remembered server's tree root for **Connect** or **Disconnect**
and **Remove from list**. Disconnect closes the Organizer's logins and folds
its tree. Jobs launched from there keep their monitors. Remove forgets the
entry without deleting its data. The configured central server belongs to
the session and is not offered in this menu.

In **Add Server** and **Edit Server**, **Connect at start** chooses whether
to connect automatically when the Organizer starts. New entries start with
this option off; existing entries default to on. With it off, choose the
server in the Servers menu or use **Connect** on its tree root.
