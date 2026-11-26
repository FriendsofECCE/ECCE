**ECCE 9.6.1.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora 44 packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

A bug-fix release for 9.6.0, the first release in which ECCE is one program with tools in tabs. Two of the fixes are for faults that could lose work, and one of those has been present in every released version of ECCE.

## Work is no longer lost without warning

- **Generate Bonds after a save was not tracked** (#1308). Build > Generate Bonds adds bonds and removes every nub, but it never told the Builder that the structure had changed. So after saving, pressing Ctrl+B and closing the Builder discarded that change with no prompt. The file on disk kept the last saved structure, so nothing already written was damaged, but the work since the save was gone. `GenBondsCmd` now announces the change as every other editing command does. The fault dates from the original code and is in every earlier release.
- **Opening a Viewer marked an unedited Builder as modified** (#1307). In one program, loading a calculation into any tab announced a geometry change to every other tab, and a Builder of a completely different calculation marked itself modified. Closing it then asked to save work that was never done. The Builder now ignores these announcements from other tools. Panels that announce changes directly -- Periodic Builder, Slice, Trajectory, Scan and Point Calculation -- are not yet covered.
- **A docking run no longer searches empty space without saying so** (#1296). The search box is placed automatically around the ligand, which is right when the ligand came from the receptor's own structure file and wrong for one that was drawn or added from another file: the box then sits wherever that molecule's coordinates put it, and Vina returns ordinary-looking poses and scores for a volume that holds no protein. Launch now counts the receptor atoms inside the box and warns when there are almost none, naming the count and pointing at Tools > Docking Box Around Selection. It warns; it does not block.

## Fixes

- **An SSH folder is listed without freezing the window** (#1220). Selecting a folder on a remote machine ran its listing on the interface thread, so the window was dead for as long as the remote took: a few seconds on a quiet server, far longer on a loaded one, with nothing on screen to say why. The listing now runs in the background, with a progress bar and Cancel when it takes more than about a second. Connecting a folder for the first time after starting ECCE still blocks for one round trip (#1303).
- **Carbon monoxide gets its triple bond** (#1292). Build > Clean gave CO a double bond and left the carbon unresolved, because the rule had no way to give oxygen a third bond. An atom left unresolved now retries with a dative bond from a lone-pair donor, and the bond note says so (`C1#O2 triple (both terminal; uses a lone pair, C1-/O2+)`). Nitrogen, nitriles and radicals are unchanged.
- **Two Substitution vs elimination examples had no orbitals** (#1282). The stored results of `f_etcl_e2_irc` and `f_etcl_sn2_irc` were written without their orbital section, so opening them gave no Density Map and no MO composition. Both are regenerated. A calculation whose orbitals are read as empty now says so instead of being stored silently as one that has none.
- **Diffraction details reach a script again** (#1282). Moving the hover and click details into the readout line under the image left the `xrd hover/click` script hook returning nothing, so the reflection, its multiplicity and the structure factor were missing from anything driven by script.
- **A job that finishes while stalled is reported correctly** (#1282).
- Windows command-line tools reach the Organizer from any Command Prompt (#1287), drive-letter file URLs work (#1289), and launching to a single-connection or front-end machine works (#1284).
- The Calculation Editor after a launch (#1290), a new calculation no longer opening read-only (#1288), and the forked job store's broker link (#1283).

## For people who script or test ECCE

- The release-round runner could report success having checked nothing: a failing harness still exited 0 when some steps had recorded no failure, a round with no steps at all exited 0, and a workflow that returned early said nothing about the steps it skipped. Each workflow now has an expected number of steps, a step that is not reported is a failure, and the summary names the machine, the platform and the commit under test (#1279).
- Every route out of ECCE now logs why it did or did not ask before quitting, to help find a fault in which the window's close button quits without asking (#1291). Ask for it with `ECCE close:` in the output.
- `selectoverlay` works in Viewer tabs, not only in the Builder (#1279).
