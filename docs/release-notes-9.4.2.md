**ECCE 9.4.2.** Please report problems as [issues](https://github.com/FriendsofECCE/ECCE/issues).

**Platforms.** Linux (Debian 13; Ubuntu 24.04, RHEL/Rocky 9 and Fedora packages are built on those systems but not tested). Windows 10 and 11. macOS, Apple silicon and Intel: experimental.

## Fixes
- **A run that stops without converging ends Unsuccessful, not Complete** (#614): an ORCA or MOPAC optimisation at its cycle limit, a CP2K or Quantum ESPRESSO SCF or optimisation that did not converge, and ECCE-QM. The calculation summary gives the line found as the reason; outputs and parsed properties are kept.
- **Return in a message box presses the highlighted button on Linux** (#634). Since 9.1 it pressed Cancel, for example in Quit ECCE.
- **A Slurm job cancelled on the machine** (scancel) ends as Killed instead of staying Submitted (#636). Reconnect after a machine's queue manager changed watches the right job (#635).
- **Calculation Summary:** labels stay on the left and long values wrap after the allocation line is filled in.
- **Density Profile, average around an atom:** on large systems it shows a progress bar with Cancel and uses fewer directions; it ran for minutes without feedback (#629). One atom picked in the viewer opens the average only where it is quick.
- **Launch Selected** finds machines registered after the Organizer started (#588).
- **Register Machines:** Find no longer crashes on a new machine whose name has an old preferences entry, and logs in as the user name typed in the form.
- **Organizer:** no crash removing the old example projects at the first start after an upgrade; Getting Started is imported when an upgrade finds no example set.
- **macOS and Windows:** names such as Löwdin and Å show correctly.
- **macOS:** imported GROMACS runs keep their structure, trajectory and MD analysis (#630).
- **MD examples:** argon atoms are drawn; water molecules stay whole across the cell; noble gases have their usual colours (argon was invisible on a white background) (#560).
- **After a dropped connection:** one login per compute machine for all jobs and all ECCE sessions (#599, #631); Reconnect asks for the login again right after a cancel.
- **Jobs on a Mac reached over ssh** start (macOS has no setsid).
- **The queue system's error file** (slurm.err) comes back into Outputs and is named in the failure reason (#594).

## New
- **Tools > Compare Energies:** each side can be a sum of calculations with coefficients; E, E+ZPE, H and G rows; a solvation-energy label for one molecule in gas and solvent; checks when code, method or basis differ; energies rounded to what the methods carry (#612).
- **Allocation profiles:** several allocations (account, run and scratch directories) per compute machine, managed in Register Machines; the Launcher offers the profile. The calculation summary shows the allocation and how much is left (#541, #613).
- **Request lines per queue** in Register Machines (for example `#SBATCH --constraint=zen4`), added to the job script header; one machine entry per host. Slurm machines always offer Memory in the Launcher (#624).
- **Register Machines: Browse** for a code's program on another computer lists that computer's files over its login.
- **Launch Selected: Launch on** another machine and profile, with the settings carried over and the changes listed per calculation; Reconnect Selected for several jobs at once (#601).
- **Progress box:**
  - Queued jobs: since when, the job id, when the queue was last checked, lost contact (#606).
  - Frequencies: the number of real and imaginary modes for ORCA, Gaussian 16, NWChem, MOPAC and ECCE-QM, next to 'converged', in red when it does not fit a minimum or a transition state (#611).
  - ECCE-QM's analytic Hessian reports its progress; ORCA's names the part of the Hessian it finished last.
  - CP2K 2023 geometry optimisations show their steps, energy, forces and step sizes.
- **Calculation summary:** a DFT calculation names its functional (#611).
- **Vibrations:** an imaginary mode's row in red; the mode description and the IRC point status in boxes (#611).
- **MO Diagram:** the Löwdin step is fast on large clusters.
- **Help:** Arranging the Viewer's panels (a panel in its own window, Dock Floating Panels, fold, close).

<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/compare-energies-sums.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/compare-energies-sums.png" width="400" alt="Compare Energies with sums of calculations" title="Compare Energies with sums of calculations"></a>
<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/progress-frequency-counts.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/progress-frequency-counts.png" width="400" alt="Progress box: real and imaginary modes (transition state)" title="Progress box: real and imaginary modes (transition state)"></a>
<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/register-machines-request-lines.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/register-machines-request-lines.png" width="400" alt="Register Machines: request lines per queue" title="Register Machines: request lines per queue"></a>
<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/progress-queued.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/progress-queued.png" width="400" alt="Progress box for a queued job" title="Progress box for a queued job"></a>
<a href="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/launch-selected-launch-on.png"><img src="https://raw.githubusercontent.com/wiki/FriendsofECCE/ECCE/release-images/v9.4.2/launch-selected-launch-on.png" width="400" alt="Launch Selected: Launch on another machine" title="Launch Selected: Launch on another machine"></a>

## Known issues
- **Density Profile:** the average around an atom on a large cluster is still slow (now with progress and Cancel); a background computation follows (#629).
- **Density Map:** each zoom step re-evaluates the map behind a progress dialog (#645).
- **Launch Selected** keeps the Organizer busy until the batch is submitted (#640).
- **Ubuntu 26.04:** ECCE's message broker is blocked by Ubuntu's AppArmor profile for mosquitto, so the apps do not start; the Ubuntu package is for 24.04 (#646).
- **Windows:** the first start after installing takes about a minute on slower computers, with progress shown (#444).
- **GROMACS, CP2K and Quantum ESPRESSO** remain experimental. **The MO diagram** is experimental.
