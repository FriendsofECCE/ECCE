# Running codes on macOS

ECCE for macOS does not include a computational chemistry code, and it does
not install one for you. This page shows how to put MOPAC, ORCA and NWChem
on a Mac without administrator rights, and how to tell ECCE where they are.
ECCE-QM, the small quantum chemistry program that comes with ECCE, needs
nothing extra.

The calculations run on your own Mac: the machine to choose in the Launcher
is `localhost`.

## Python for ECCE's own windows

The detail windows of the Calculation Editor (**Theory Details...** and
**Runtype Details...**) and the first-start window are Python programs.
ECCE.app contains its own Python with wxPython and uses it before any
Python of yours, so nothing needs to be installed for them, however ECCE is
started. Miniforge below is only for NWChem.

## MOPAC

1. Download the macOS installer (a `.dmg` such as `mopac-23.2.5-mac.dmg`)
   from the MOPAC web site and open it.
2. Run the installer and choose a folder in your home folder, for example
   `~/mopac`. No administrator password is needed for that.

   The same can be done from a terminal, without the installer window:

   ```
   /Volumes/<the dmg>/mopac-23.2.5-mac.app/Contents/MacOS/mopac-23.2.5-mac \
       --accept-licenses --accept-messages --confirm-command --root ~/mopac install
   ```
3. The program is `~/mopac/bin/mopac`.

## ORCA

1. Download ORCA for macOS from the ORCA forum site (you need a free
   account). Choose the Intel build on an Intel Mac and the arm64 build on
   an Apple-silicon Mac.
2. Unpack it in your home folder:

   ```
   mkdir -p ~/orca && tar xjf orca_6_1_1_macosx_intel_openmpi411.tar.bz2 -C ~/orca
   ```
3. The program is `~/orca/orca_6_1_1_macosx_intel_openmpi411/orca`. Give the
   full path: ORCA finds its helper programs next to itself.

ORCA 6.1.1 for macOS needs macOS 12.3 or newer. On older systems it stops
at once with a `dyld: Symbol not found` message.

## NWChem

NWChem for macOS comes from conda-forge. Miniforge is a small conda
installer that needs no administrator rights.

1. Download the Miniforge installer for your Mac from
   `github.com/conda-forge/miniforge/releases` (`MacOSX-x86_64` for an Intel
   Mac, `MacOSX-arm64` for Apple silicon). The newest installers need
   macOS 11 or newer; on macOS 10.15 use release 25.3.1-0.
2. In a terminal:

   ```
   bash Miniforge3-MacOSX-x86_64.sh -b -p ~/miniforge3
   ~/miniforge3/bin/conda install -y nwchem
   ```
3. The program is `~/miniforge3/bin/nwchem`.
4. NWChem needs to know where its basis-set library is. Conda sets this
   when its environment is activated, which ECCE does not do. Enter it in
   Register Machines (below), on the **Codes** tab, in the NWChem
   **Environment variables** box:

   ```
   NWCHEM_BASIS_LIBRARY /Users/<you>/miniforge3/share/nwchem/libraries/
   ```

## Tell ECCE where the codes are

1. Choose **Register Machines** and select `localhost`.
2. On the **Codes** tab select a code and enter the full path of its program
   in the **Program** field, or press **Find** after you have put the
   program's folder on the `PATH` in the script that **Script run at login**
   (Connection tab) names. For example, a file `~/ecce-login.sh` with

   ```
   PATH=$HOME/mopac/bin:$HOME/miniforge3/bin:$PATH; export PATH
   ```

3. Save. ECCE writes your choices to `~/.ECCE/CONFIG.localhost`.

Then make a calculation as in [Your first calculation](first-calculation.md)
and choose `localhost` in the Launcher.
