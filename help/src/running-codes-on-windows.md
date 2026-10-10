# Running codes on Windows

ECCE for Windows does not include a computational chemistry code, and it
does not install one for you. This page shows how to install MOPAC and ORCA
on a Windows PC, how to add Microsoft MPI so that ORCA can use more than one
core, and how to tell ECCE where the programs are. ECCE-QM, the small
quantum chemistry program that comes with ECCE, needs nothing extra.

The calculations run on your own PC: the machine to choose in the Launcher
is `localhost`. Codes that have no Windows version (NWChem, Quantum
ESPRESSO, GROMACS) run on a Linux machine instead, registered in Register
Machines and reached over ssh.

## MOPAC

1. Download the Windows installer (a file such as `mopac-…-win.exe`) from
   the MOPAC web site and run it.
2. The default folder is `C:\Program Files\MOPAC`, which needs an
   administrator. Without administrator rights, choose a folder in your
   user folder, for example `C:\Users\<you>\mopac`.
3. The program is `C:\Program Files\MOPAC\bin\mopac.exe` (or `bin\mopac.exe`
   in the folder you chose).

## ORCA

1. Download ORCA for Windows from the ORCA forum site (you need a free
   account): the 64-bit Windows build.
2. Install or unpack it into a folder in your user folder, for example
   `C:\Users\<you>\orca`. Keep all of its files together: ORCA finds its
   helper programs next to itself.
3. The program is `orca.exe` in that folder, for example
   `C:\Users\<you>\orca\orca.exe`.

ORCA runs on one core without anything else. For more than one core it
needs Microsoft MPI.

## Microsoft MPI (for ORCA on more than one core)

ORCA's parallel parts are started with `mpiexec`, which Microsoft MPI
provides.

1. Download **Microsoft MPI** from Microsoft (search for "Microsoft MPI
   download"). Only the runtime is needed: `msmpisetup.exe`, not the SDK.
2. Run `msmpisetup.exe`. It needs an administrator, and installs into
   `C:\Program Files\Microsoft MPI\Bin`, which it adds to the `PATH`.
3. **Close ECCE and start it again.** A running ECCE does not see programs
   installed after it started; after the restart it finds `mpiexec`.

To check, open a Command Prompt and type `where mpiexec`. It should print
`C:\Program Files\Microsoft MPI\Bin\mpiexec.exe`.

Then set the number of cores in the Launcher. If a code that needs MPI is
started on more than one core and ECCE cannot find `mpiexec`, the Launcher
warns: **Launch anyway** starts the job as set, **Run on 1 core** changes
it to one core. A job that finds no `mpiexec` when it runs uses one core
and says so in its output.

## Tell ECCE where the codes are

1. Choose **Register Machines** and select `localhost`.
2. On the **Codes** tab select a code and enter the full path of its
   program in the **Program** field, for example
   `C:\Users\<you>\orca\orca.exe`. A path with spaces, such as
   `C:\Program Files\MOPAC\bin\mopac.exe`, is fine.
3. Save. ECCE writes your choices to `.ECCE\CONFIG.localhost` in your user
   folder.

Then make a calculation as in [Your first calculation](first-calculation.md)
and choose `localhost` in the Launcher.
