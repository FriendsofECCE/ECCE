**ECCE 9.4.3.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Fixes
- **A machine that does not answer no longer blocks ECCE** (#666). Reconnect, Reconnect Selected and the check at start-up log in and copy the job monitor in the background, with a Cancel that works at once; a host that does not answer fails after the connect timeout, without a second try through the built-in ssh client. A failed reconnect no longer deletes the calculation's stored results.
- **ECCE no longer hangs silently at start** when a stopped ECCE process (for example after Ctrl+Z) holds the session lock: the start names the process and says how to release it. A new start keeps its shared ssh connection directory, which the clean-up of the previous session could remove.
- **A job the queue system ends** (out of memory, wall time reached, node failure, preemption) is **Incomplete** instead of System Error, with the cause as the reason, also at the end of the Progress box. A new help page, Run states, says what each state means.
- **Comp. Date** is when the job ended, also after a later Reconnect.
- **Density Profile:** the line profile, its ranking of the orbitals and the average around an atom run in the background and are several times faster; the window no longer freezes on large clusters (#629).
- **Density Map:** zooming draws a coarse map at once and the full one when the wheel rests (#645); a diatomic starts on the molecular plane through its axis.
- **Atoms selected before** the Density Map or Density Profile opens define its plane or line.
- **Theory and Runtype Details** of every code can be resized; settings scroll and Close and Reset stay visible; no internal setting names as tooltips.
- **Launch Selected** leaves the Organizer usable while a batch is submitted, with a Stop button (#640).
- **Calculation summary:** the values of all blocks line up; the second Launch Info column follows the first; dates in the Viewer follow the date preference (#654, #657).
- **Ubuntu 26.04:** the package lets ECCE's message broker run under Ubuntu's AppArmor profile for mosquitto (#646).
- **ORCA:** a geometry optimisation of a single atom runs as a single point (with frequencies) instead of stopping without an energy.
- **Cost** of a running job no longer counts an extra hour of its cores (job dates were read as local time).
- **macOS:** the viewer's text and measurement labels are readable on Retina screens (#668).
- **Playback:** Stop keeps the current frame instead of returning to the first (#661).
- **Select Molecule** after Select All and a click in wireframe selects that molecule, not everything again (#660).
- **The input check** counts CP2K atoms with a kind label (such as Nb_e).

## New
- **Tools > Allocations** lists each cluster account of your machine profiles and how much of it is used (projinfo at NAISS centres such as HPC2N, sshare elsewhere). The calculation summary shows **Remaining on submission** under the allocation, taken when the job was submitted (#613).
- **Launch Info:** Job ID and Cost (cores × wall time in CPU-h) rows; a waiting Slurm job shows its **Queue state**, the reason in plain words and the estimated start (#641, #648).
- **Run Mgmt > Re-Parse Outputs** reads a finished calculation's stored outputs again with the current parsers, without logging in to the machine (#651). **Run Mgmt > Terminate Selected** terminates several selected jobs at once.
- **ORCA:** an integration grid choice (DefGrid1/2/3) in Theory Details (#644). **ORCA and Gaussian 16:** a custom solvent given by its dielectric constant and refractive index (#642).
- **CP2K:** an outer SCF loop (&OUTER_SCF) with the OT minimiser, on by default.
- **Saving in the Calculation Editor** asks before regenerating an input file that was edited by hand; Keep my input is the default (#622).
- **Dipole Moment:** the arrow runs from pole to pole along the dipole moment; each pole carries the atoms' total positive charge and is marked by a sphere, and the poles lie as far apart as that charge needs to give the moment. Where the atomic charges point the other way (carbon monoxide), the text says so.
- **Density Map:** drag with the right mouse button to move a zoomed map.
- **Mode composition** is a read-only assignment table: the three largest contributions of every mode, C–H bends included (a checkbox), Copy table and Export .csv.
- **General Chemistry:** argon at 40, 100, 130 and 200 K (solid, liquid, liquid near its critical point, gas), with speed distributions that follow Maxwell-Boltzmann in every phase (#626).
- **Register Machines** no longer shows Vendor, Model and Processor.
- **Bundled for GROMACS Prepare (MINFF):** atomipy 1.1 (was 0.98); min.ff unchanged at commit d12b895, since its newest layout is not read yet (#667).

<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.3/dipole-poles-water.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.3/dipole-poles-water.png" width="400" alt="Dipole Moment: poles and arrow for water" title="Dipole Moment: poles and arrow for water"></a>
<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.3/dipole-poles-co.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.3/dipole-poles-co.png" width="400" alt="Dipole Moment: carbon monoxide, the ESP charges point the other way" title="Dipole Moment: carbon monoxide, the ESP charges point the other way"></a>

## Known issues
- **Density Map:** the full map of a large calculation still takes a while after zooming.
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
