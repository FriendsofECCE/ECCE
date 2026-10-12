# Running codes on Linux

ECCE does not include a computational chemistry code, and it does not install
one for you. On Linux, most of the codes ECCE works with are in the
distribution's own package archive, so they install with one command and are
updated with the rest of the system. ORCA and Gaussian are not, and come from
their makers. ECCE-QM, the small quantum chemistry program that comes with
ECCE, needs nothing extra.

The calculations run on your own computer: the machine to choose in the
Launcher is `localhost`. For a cluster, see
[Register Machines](register-machines.md).

## From the package archive (Debian, Ubuntu)

Debian 13 has these codes in its archive:

| Code | Package | Version in Debian 13 |
|---|---|---|
| NWChem | `nwchem` | 7.2.3 |
| MOPAC | `mopac` | 23.1.2 |
| GROMACS | `gromacs` | 2025.2 |
| Quantum ESPRESSO | `quantum-espresso` | 6.7 |
| CP2K | `cp2k` | 2025.1 |

Install the ones you need, for example:

```
sudo apt install nwchem mopac
```

The programs are then in `/usr/bin`, where ECCE's **Find** button looks.
Ubuntu 24.04 has the same packages, some in older versions.

**Do not install the package called `orca`.** In Debian and Ubuntu, `orca` is
the GNOME screen reader, not the quantum chemistry program. ORCA comes from
its own web site; see below.

On Fedora, Rocky Linux and RHEL, some of these codes are in Fedora or EPEL;
`dnf search nwchem` (and so on) shows what your system offers.

## ORCA

ORCA is free for academic use but is not in any distribution's archive.

1. Download ORCA from FACCTS (<https://www.faccts.de/orca/>; you need a free
   academic account to log in), choosing ORCA for Linux: x86-64 for a PC,
   arm64 for an ARM machine.
   For x86-64 there are two archives,
   `orca_6_1_1_linux_x86-64_shared_openmpi418.tar.xz` and
   `orca_6_1_1_linux_x86-64_shared_openmpi418_nodmrg.tar.xz`; the second
   leaves out the DMRG module, which ECCE does not use. Either works.
2. Unpack it, for example in your home folder:

   ```
   mkdir -p ~/orca && tar xf orca_6_1_1_linux_x86-64_shared_openmpi418_nodmrg.tar.xz -C ~/orca
   ```

   or, for every user of the computer, under `/opt` with `sudo`.
3. The program is the file `orca` in the unpacked folder. Give ECCE its full
   path: ORCA finds its helper programs next to itself.

For a calculation on more than one core, ORCA needs the Open MPI version
named in the archive's file name (for example, `openmpi418` is Open MPI
4.1.8). ORCA's download page and manual describe how to install it
if your distribution's `openmpi-bin` is a different version.

## Gaussian

Gaussian is a commercial program. A site with a licence installs it; ask
your system administrator for the location of `g16` and of its setup file
(`g16.profile`). ECCE needs the full path of `g16`.

## NBO (optional)

NBO 6 or 7 adds natural bond orbital analysis to ORCA and Gaussian jobs. It is
licensed separately from the University of Wisconsin. After installing it:

- **ORCA:** in Register Machines, on the Codes tab, select ORCA and add to
  **Environment variables** the line `NBOEXE /path/to/nbo7/bin/nbo7.i8.exe`.
- **Gaussian:** add the line `PATH /path/to/nbo7/bin` for Gaussian 16, and
  set `BINDIR` in NBO's own `gaunbo7` script to the same folder.

## Tell ECCE where the codes are

1. Choose **Register Machines** and select `localhost`.
2. On the **Codes** tab select a code and press **Find**. For a code from the
   package archive this finds the program in `/usr/bin`. For ORCA and
   Gaussian, enter the full path of the program in the **Program** field.
3. Save. ECCE writes your choices to `~/.ECCE/CONFIG.localhost`.

Then make a calculation as in [Your first calculation](first-calculation.md)
and choose `localhost` in the Launcher.
