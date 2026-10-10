# ECCE overview

## How ECCE is organised

![Two ways to run ECCE. With an ECCE server (the default on one computer, or a central server for a lab or class): the client saves to and loads from the server, which holds the data server and message broker. On its own (optional): the client keeps its data in a local folder with its own broker, and there is no ECCE server. In both, the client's job agent submits jobs over ssh to a compute machine and follows them.](images/ecce-architecture.svg)

The **client** (`ecce-client`) is the windows you work in, plus a job agent
that submits and follows jobs. Normally it works with an **ECCE server**
(`ecce-server`: a data server and a message broker) that stores projects
and results. That is the default: both packages on one computer, started
for you. For a lab or class, one central server serves many clients. The
client can instead run **on its own**, keeping its data in a folder on
your disk (Edit → Preferences → Data folder; off by default on Linux,
the default on macOS and Windows); it still
starts a private broker, which needs the `mosquitto` package, but no ECCE
server. Neither server nor client runs calculations: they run on a
**compute machine**, a workstation or an HPC cluster, where the client
submits each job over ssh.

## General features

* **Build molecular models**, or import a structure and work from that.
* **Set up calculations through one interface** for **NWChem, Gaussian
  16, ORCA and MOPAC** — built, submitted, monitored and parsed back,
  each actively tested against the real code. Further codes can be
  registered without changing ECCE itself.
* **Check the input before it goes out**: Verify inspects the
  generated input file before it is submitted.
* **Choose basis sets graphically**, with the code's own built-in sets
  used where they match.
* **Submit to workstations, clusters and supercomputers**, through PBS,
  LSF, Slurm, Moab, SGE and HTCondor, or by running directly on the
  machine without a batch system.
* **Watch results arrive while the job is still running** — energies,
  geometry traces and convergence are parsed live, not only at the end.
* **Visualise molecular data in 3-D**: molecular orbitals, electron
  density, electrostatic potential maps, vibrational modes with
  animation, and geometry optimisation traces.
* **Import output from jobs run outside ECCE**, for centres where
  ECCE cannot submit directly.
* **Run one server for a group**, with students or colleagues connecting
  to it as clients.

## Screenshots

Click any image for the full-size version.

<a href="images/organizer.png"><img src="images/organizer.png" width="300" alt="The Organizer, with a completed ORCA calculation selected"></a>

*The Organizer is the front door: projects and calculations on the left,
each with a run-state icon, and a summary of the selected one in the
middle, with buttons for its editor, builder, basis set tool, launcher
and viewer — here a completed ORCA calculation on benzene.*

<a href="images/viewer.png"><img src="images/viewer.png" width="300" alt="The viewer, showing a molecular orbital of benzene"></a>

*The viewer: a calculation's orbitals and energies beside the structure
they were computed for — benzene, from ORCA.*

| | | |
|---|---|---|
| <a href="images/orbital-benzene.png"><img src="images/orbital-benzene.png" width="170" alt="Benzene's highest occupied molecular orbital"></a> | <a href="images/esp-benzene.png"><img src="images/esp-benzene.png" width="170" alt="The electrostatic potential on benzene's surface"></a> | <a href="images/vectors-water.png"><img src="images/vectors-water.png" width="170" alt="A vibrational mode of water, drawn as displacement vectors"></a> |
| Benzene's π HOMO (ORCA) | Electrostatic potential on the surface (ORCA) | A vibrational mode of water, as displacement vectors (Gaussian 16) |

<a href="images/mo-diagram-water.png"><img src="images/mo-diagram-water.png" width="380" alt="A qualitative MO correlation diagram for water"></a>

*A qualitative MO correlation diagram — water's orbitals, from Gaussian
16, against the oxygen on one side and the hydrogens' symmetry orbitals
on the other, with the non-bonding lone pair picked out. The same
diagram is drawn from any of the supported codes. Experimental.*
