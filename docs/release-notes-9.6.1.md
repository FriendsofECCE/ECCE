**ECCE 9.6.1.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

A bug-fix release for 9.6.0, the first release in which ECCE is one program with tools in tabs. Two of the fixes are for faults that could lose work, and one of those has been present in every released version of ECCE.

## Work is no longer lost without warning

- **Generate Bonds after a save was not tracked** (#1308). Build > Generate Bonds adds bonds and removes every nub, but it never told the Builder that the structure had changed. So after saving, pressing Ctrl+B and closing the Builder discarded that change with no prompt. The file on disk kept the last saved structure, so nothing already written was damaged, but the work since the save was gone. `GenBondsCmd` now announces the change as every other editing command does. The fault dates from the original code and is in every earlier release.
- **Opening a Viewer marked an unedited Builder as modified** (#1307). In one program, loading a calculation into any tab announced a geometry change to every other tab, and a Builder of a completely different calculation marked itself modified. Closing it then asked to save work that was never done. The Builder now ignores these announcements from other tools. Panels that announce changes directly -- Periodic Builder, Slice, Trajectory, Scan and Point Calculation -- are not yet covered.
- **A docking run no longer searches empty space without saying so** (#1296). The search box is placed automatically around the ligand, which is right when the ligand came from the receptor's own structure file and wrong for one that was drawn or added from another file: the box then sits wherever that molecule's coordinates put it, and Vina returns ordinary-looking poses and scores for a volume that holds no protein. Launch now counts the receptor atoms inside the box and warns when there are almost none, naming the count and pointing at Tools > Docking Box Around Selection. It warns; it does not block.


**Still open:** with a tool tab selected, the window's close button can quit without asking (#1291). This release adds logging for it, not a fix: run ECCE from a terminal and look for lines beginning `ECCE close:`, which name the route taken and why it did or did not ask.

## Fixes

- **Dragging in the Organizer tree works again** (#983 follow-up). Dragging a calculation or folder did nothing and left `gtk_drag_begin: assertion 'gtk_widget_get_realized (widget)' failed` in the terminal. In one program the Organizer's own window is never shown, and the drag was being started from that window rather than from the tree the pointer is actually on. The tree is now its own drag source.
- **Docking poses open at once, and their labels can be read** (#1293). Opening a set of Vina poses took minutes, because the coordination-pattern scan ran over the whole receptor; it is skipped for docking poses and for any structure over 500 atoms. The residue labels were drawn in white on a white background and now follow the Foreground colour.
- **An SSH folder is listed without freezing the window** (#1220). Selecting a folder on a remote machine ran its listing on the interface thread, so the window was dead for as long as the remote took: a few seconds on a quiet server, far longer on a loaded one, with nothing on screen to say why. The listing now runs in the background, with a progress bar and Cancel when it takes more than about a second. Connecting a folder for the first time after starting ECCE still blocks for one round trip (#1303).
- **The Calculation Editor fits a 768-pixel-high screen** (#189). On a screen that height the editor's window could end one pixel below the bottom edge: it is positioned while it still has its pre-layout height and then grows by a row as it is drawn. It is now moved back onto the screen once its final size is known.
- **Carbon monoxide gets its triple bond** (#1292). Build > Clean gave CO a double bond and left the carbon unresolved, because the rule had no way to give oxygen a third bond. An atom left unresolved now retries with a dative bond from a lone-pair donor, and the bond note says so (`C1#O2 triple (both terminal; uses a lone pair, C1-/O2+)`). Nitrogen, nitriles and radicals are unchanged.
- **Two Substitution vs elimination examples open with their orbitals** (#1211). The stored results of `f_etcl_e2_irc` and `f_etcl_sn2_irc` were written before an ECCE-QM reaction path stopped being mistaken for one whose orbitals are kept only at the transition state, so they shipped without their basis and orbital section and gave no Density Map and no MO composition. Both are regenerated.
- **Diffraction details reach a script again** (#1282). Moving the hover and click details into the readout line under the image left the `xrd hover/click` script hook returning nothing, so the reflection, its multiplicity and the structure factor were missing from anything driven by script.
- **A job that finishes while stalled is reported correctly** (#1282).
- Windows command-line tools reach the Organizer from any Command Prompt (#1287), drive-letter file URLs work (#1289), and launching to a single-connection or front-end machine works (#1284).
- The Calculation Editor after a launch (#1290), a new calculation no longer opening read-only (#1288), and the forked job store's broker link (#1283).

## Also in this release

- **Open and Add Structure from File use the platform's file dialog** (#342, #1275); the old chooser becomes "... from Remote Machine...".
- **Fixes to the tabbed window itself**: a run calculation's Builder button opens its Viewer rather than a read-only Builder beside it (#1254); Next/Previous Tab no longer draws every tab it steps past (#1267); switching MOs to Surfaces no longer paints the old table over the new panel (#1263); a flushed feedback message no longer greys the whole window (#1268); missing Organizer thumbnails refresh when rendering completes (#1238).
- **Panels fit a small window** (#1282). The Set-up area's Launch fields have a minimum width, so the panel fits the narrow column of a 1024-pixel window without a horizontal scrollbar, and the Vina run type's ligand-atoms label is shorter for the same reason.
- **Docking and MD set-up**: the Vina scoring function (vina or vinardo) is chosen in Theory Details (#1278); GROMACS Prepare reports substitutions it could not make and explains genion's "No more replaceable solvent" (#1228); the bundled MINFF force field is a single tailored file (#1280).
- **Diffraction and playback**: crystal views are greyed without a cell, Debye comes first and is the default, and the last view is remembered (#1230); Playback offers "Repeat off / from the start / back and forth" in place of Loop (#1229).
- **Windows**: the data-folder check ignores letter case and accepts a Windows path inside the data folder (#1285), and live monitoring works for single-connection OpenSSH jobs.
- **Less noise in the terminal**: the broker reads a private copy of its ACL file, so Mosquitto no longer warns about permissions (#1257); Organizer start-up and desk diagnostics appear only with `ECCE_DIAG`; the Builder's Clean report stays in the Builder.

## For people who script or test ECCE

- The release-round runner could report success having checked nothing: a failing harness still exited 0 when some steps had recorded no failure, a round with no steps at all exited 0, and a workflow that returned early said nothing about the steps it skipped. Each workflow now has an expected number of steps, a step that is not reported is a failure, and the summary names the machine, the platform and the commit under test (#1279).
- Every route out of ECCE now logs why it did or did not ask before quitting, to help find a fault in which the window's close button quits without asking (#1291). Ask for it with `ECCE close:` in the output.
- `selectoverlay` works in Viewer tabs, not only in the Builder (#1279).
- The Builder's `diffraction` and `calcdiffraction` script hooks take `view=fibre|powder|single|frame|debye`, so a script can ask for a view instead of taking whichever one the dialog opens on; a crystal view asked for without a unit cell is refused rather than quietly replaced (#1230).
- The same hooks reported a Debye view as `fibre`, because the test for a powder view is false in Debye mode. Every view is now named (#1230).
- A test that measures windows against a small screen reported a window query that failed as an app that had opened no window; the query is retried and the two are now told apart (#1322).
- An offload `run` left the command pointed at the installed ECCE rather than the tree it had just built, so a tool could read an older install's data without saying so.
- A GUI test script, or a harness run, that reached none of its checks reported success; a script now declares how many checks it should run and fails with "only N of M checks ran" otherwise (#1309).
- A test run that tested nothing no longer reports success: an offload `ctest`/`ctesti` whose `-R` pattern matched no test exited 0, so a typo in the pattern looked like a clean run (#1301).
