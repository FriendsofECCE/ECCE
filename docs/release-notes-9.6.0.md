**ECCE 9.6.0.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Upgrading from 9.5
- **One program with tabs** is the default on Windows, macOS and Linux (#983, #1067). The Organizer, Builder, Calculation Editor, Viewer and the other tools open as tabs of one window. `ECCE_ONE_PROCESS=0 ecce` brings back one window per tool.
- **Built-in broker for local sessions.** On Windows, local sessions use a small message broker inside ECCE instead of `mosquitto.exe` (#990). On Linux and macOS, local sessions use ECCE's own broker program, `eccebroker`, instead of a per-user Mosquitto, so a client installation needs no Mosquitto (#1007). A central server and a shared system broker still use Mosquitto, unchanged. `ECCE_BROKER=mosquitto` keeps the previous local route.
- The Calculation Editor's tab is a **Set-up** tab: Theory Details, Runtype Details, the basis set and the input file share one detail area, with a live preview of the input (#983).
- **New from This** offers Rerun, Continue from Last Geometry, Continue from Final Orbitals and Continue in Another Code..., with the same names in the Edit and right-click menus. **Run Mgmt > Reset for Rerun** is named as in the right-click menu (#1089).

## One program

<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.6.0/linux-tabs-builder.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.6.0/linux-tabs-builder.png" width="400" alt="ECCE on Linux as one window with Organizer, Builder, ECCE-QM Editor and Viewer tabs; the Builder tab is in front with ethanol"></a> <a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.6.0/linux-editor-setup.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.6.0/linux-editor-setup.png" width="400" alt="The ECCE-QM Editor tab on Linux for ethanol (B3LYP, 6-31G*), with Theory Details open in the right panel"></a>
<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.6.0/windows-tabs.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.6.0/windows-tabs.png" width="400" alt="The Organizer as one window with its tab strip, on Windows"></a> <a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.6.0/macos-tabs.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.6.0/macos-tabs.png" width="400" alt="The Organizer as one window with its tab strip, on macOS"></a>

- **Windows and macOS:** one icon and one menu bar; Quit ends ECCE; a second start of ECCE raises the running window; structure files open in a Builder tab, and the packages register the file types (#1069).
- **Edit > Preferences > Windows and tabs:** Normal, Small and Wide layouts, and for each tool whether it opens as a tab or in its own window. A tab can be moved to its own window and back. A fresh installation starts with the Normal layout (#983).
- A tab names its calculation in a tooltip and in the Window menu; there is one tab per calculation and tool; **Next Tab** and **Previous Tab** (Ctrl+PgDn, Ctrl+PgUp) cycle through the tabs (#1256).
- The Basis Set Tool, the MD editors, MD Prepare and the Launcher open as tabs too (#983, #1066). The basis panel in the Set-up tab lists the library sets with their coverage of the system, assigns sets per element, and handles ECPs and fitting sets (#1063).
- **Launch:** the Launch panel opens in the Set-up area, with warnings inline and a **Launch anyway** button. The Organizer's Launch button with one calculation selected opens that calculation's editor with the Launch panel (#1269).
- Dialogs of a tool in a tab open centred over the main window and return the focus to it.

## Other changes
- **Edit > Preferences > Codes** chooses which codes are offered for new calculations; hidden codes stay listed as "N more codes (hidden)..." (#1049).
- **Organizer:** double-click a file in a folder, or right-click it and choose **View**, to read it in a text window, in local data, SSH folders and ECCE servers; large files open at the end, with Load more, Follow and Find (#1084). The details pane of a selected calculation fills at once (#1237), and listing an SSH folder is faster (#1082).
- **Diffraction** is a docked Properties pane, recalculated on every change, with its settings behind **Options...** and a breakout window (#1013).
- **Thermochemistry** is split into translation, rotation, vibration and electronic contributions, with C_V and C_P; the Energy Calculator gives H and G at a chosen temperature (#1044).
- **Natural population analysis:** ECCE computes NPA charges itself from the stored orbitals, without the NBO program. **ORCA EPR** results are read and shown as tables.
- **Counterpoise correction:** for a completed ECCE-QM or ORCA relaxed scan with fragments defined, **Counterpoise single points** creates and launches an Energy calculation that runs the complex and each fragment in the complex basis (#1091).
- **Stress tensor** of a periodic calculation (Quantum ESPRESSO, CP2K) under Properties > Geometry, drawn as principal stresses on the cell, with pressure and shear (#1015, #1021).
- **Builder:** **Build > Cut Cluster...** turns a periodic structure into a capped cluster around selected atoms (#1033). **Edit > Select Ligand** lists the ligand candidates, the likely ligand first, and **Edit > Select Heteroatoms** selects everything that is not protein; Add Hydrogen fills ligands and structures without residues from their geometry (#1244, #1251).
- **Vibrational spectra:** **Experimental...** reads JCAMP-DX files, including those of the NIST WebBook, and the calculated spectrum can be exported as JCAMP-DX (#986). An optimisation with frequencies that ends with imaginary modes below 100 cm⁻¹ in magnitude gets a warning line in the panel.
- **QSAR:** one window per folder with Table, PCA and PLS, a report with R²X, Q², a scree plot and the explained scaling, copyable tables, and point selection in the plots (#996, #1012).
- **OPDOS:** groups A and B can be taken from the selection, the settings fold under **Options ▸**, and **Open in window...** shows the plots in a resizable window.
- **CREST:** **Select Distinct...** selects the distinct members of an ensemble, lowest energy first, by the rules of `ecce-ensemble pick`.
- **GROMACS:** a calculation from your own .mdp/.gro/.top or .tpr files is its own calculation type, with hand-offs to and from other codes (#1038). NPT on a mineral stack built in the Layer Builder uses a solid's compressibility when the default was left in place (#1140).
- **Workflows:** MACE (MLP) is a workflow step, and the geometry link carries the periodic cell into and out of it (#1019).
- **Allocations:** Refresh logs in to a machine with no open connection, as a launch does, and each row says why a check did not run (#1226).
- **General Chemistry examples:** a new topic, Molecular and covalent solids, with seven Quantum ESPRESSO crystals (ice, dry ice, urea, benzene, sucrose, diamond, graphite), each with its powder pattern.

## Fixes
- Undo of a delete draws the restored atoms again, so undoing the deletion of the solvent brings the water back (#1248). The Delete key deletes the selection after Edit > Select Solvent (#1240). Loading a PDB no longer flashes the window black (#999). A PDB opened from the file manager can be edited, and Save writes it under a new name (#1259).
- A Viewer reopened on a calculation after its tab was closed shows the structure (#1209). The Viewer's Depth cueing and Perspective toggles work and can be switched off again (#1214). ELF and LOL surfaces are closed (#1076). `results.h5` keeps the ECP of a calculation (#1055).
- One program: no crash when a Viewer tab is closed with the Density Map panel open (#1135); Quit after tabs were closed while a job's watchers were connected no longer crashes on Windows (#1131); a log alert no longer blocks Quit on macOS (#1123); with `ECCE_ONE_PROCESS=0`, Quit ends the separate tools too (#1106, #1120). A tool tab's close cross works on the first click (#1234).
- The Organizer no longer stays blank or "not responding" at start while a remembered SSH data server logs in (#1272), or on a right-click on an SSH folder (#1273). SSH logins no longer send an empty or repeated password, which made a cluster refuse a correct one with "Too many authentication failures" (#1274). The Quit question at start can be clicked (#1252).
- On a package upgrade, the first Viewer no longer stays at "Starting the Viewer…" with its panels greyed on an X server (#1249).
- Vina: Runtype Details opens in the Set-up area with a list of the structure's molecules to choose the ligand from, the editor no longer shows Charge, Spin Multiplicity, Open Shells or symmetry, and the input preview works before a theory is stored (#1261, #1276, #1277). The Launch panel's fields keep to the panel width and the editor no longer greys out for each line of the launch log (#1265, #1266). Run-state icons have no light square background (#1241). Diffraction on a protein runs in seconds, with a progress bar and Cancel for slow runs (#1246); a structure without calculation data no longer opens on the fibre diffraction view (#1245).

## Known issues, fixed in 9.6.1
- Half-drawn frames when a tool tab opens or closes; partly fixed (#1210).
- Opening an SSH folder takes 20-30 s to connect although ssh to the same host takes under a second (#1220).
- Vina: the ligand-candidates side file can read as empty (#1271).

## Known issues
- **Windows:** uninstalling takes long (#815).
- **GROMACS, CP2K, Quantum ESPRESSO** and the **ML potential** are experimental. **The MO diagram** is experimental.

## Packages
Each package is built by the project's CI on the system it is meant for:

| File | Built on |
|---|---|
| `ecce-client_9.6.0_amd64.deb`, `ecce-server_9.6.0_amd64.deb` | Debian 13 (trixie) |
| `ecce-client_9.6.0_ubuntu24.04_amd64.deb`, `ecce-server_9.6.0_ubuntu24.04_amd64.deb` | Ubuntu 24.04 |
| `ecce-client-9.6.0-1.el9.x86_64.rpm`, `ecce-server-9.6.0-1.el9.x86_64.rpm` | Rocky Linux 9 (for RHEL 9 and compatible systems) |
| `ecce-client-9.6.0-1.fedora.x86_64.rpm`, `ecce-server-9.6.0-1.fedora.x86_64.rpm` | Fedora 45 |
| `ECCE-9.6.0-arm64.dmg` | macOS on Apple silicon; runs on macOS 11 or later |
| `ECCE-9.6.0-x86_64.dmg` | macOS 15 on Intel; runs on macOS 10.15 or later |
| `ECCE-9.6.0-x86_64-macos10.13.dmg` | macOS 15 on Intel; runs on macOS 10.13 and 10.14 (experimental) |
| `ECCE-windows-9.6.0.msi`, `ECCE-windows-9.6.0.zip` | Windows, MSYS2 UCRT64; for Windows 10 and 11 |

How to install each: [Get ECCE](https://github.com/FriendsofECCE/ECCE#get-ecce).
