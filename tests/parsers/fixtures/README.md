# Fixture provenance

Every file here is genuine output from a real binary, captured on
**niobium** (Debian 13 "trixie") on **2026-09-21**.  Nothing is hand-edited
or trimmed: a trimmed file can change how `eccejobmonitor`'s Begin/Skip/End
feeding behaves, and the point of the suite is that it behaves identically
to production.  Each output file is accompanied by the exact input deck
used to produce it, so any of them can be regenerated.

Total size of this tree is around 800 KB — keep it in that order of
magnitude; prefer STO-3G and a handful of atoms.  Every job below runs
in seconds.

## Codes and versions

| code      | version                                  | invocation |
|-----------|------------------------------------------|------------|
| Gaussian  | `ES64L-G16RevC.01  3-Jul-2019` (`/opt/gaussian/g16/g16`) | `g16 <deck>.gjf` |
| NWChem    | Debian package `nwchem 7.2.3-10`         | `nwchem <deck>.nw > <deck>.out` |
| ORCA      | `6.1.1` (`/opt/orca/orca_6_1_1_linux_x86-64_shared_openmpi418_nodmrg`) | `orca <deck>.inp > <deck>.out` |
| MOPAC     | `23.1.2` (Debian `/usr/bin/mopac`)       | `mopac <deck>.mop` → `<base>.out` |
| Quantum ESPRESSO | Debian package `quantum-espresso 6.7-3+b1` (`/usr/bin/pw.x`), pseudopotentials from `quantum-espresso-data-sssp 1.3.0-3` | `pw.x -in <deck>.pwin > <deck>.pwout` |

NWChem fixtures are `ecce_print` *traces*, not the normal stdout log: the
`.desc` entries for NWChem match the `%begin%`/`%end%` markers that the
`ecce_print <file>` directive writes to a separate file.  That is what the
live monitor parses, so that is what is checked in.  The stdout log of the
same run is not kept.

---

## gaussian-16/

### `h2o_optfreq.log` + `h2o_optfreq.gjf`
Water, HF/STO-3G, `Opt Freq`, closed shell.  ECCE runtype **GeoVib**.

    %chk=h2o.chk
    #p HF/STO-3G Opt Freq

    water opt freq test

    0 1
    O   0.000000   0.000000   0.000000
    H   0.000000   0.000000   0.960000
    H   0.926000   0.000000  -0.240000

Exercises: `MULLIKEN` (-0.330647 / 0.165312 / 0.165335), the combined
`[VIB][VIBFREQ][VIBIR][VIBRAM][VIBSYM]` block (2169.8577 / 4141.5426 /
4392.6459 cm-1, symmetries A' A' A'), `GEOMTRACE`/`GEOMTRACE1` across
several optimisation steps (and that `Skip=5` lands exactly on the first
atom row), `ESCF`/`TE`, `DELTAE`, `RMSDP`, the multipoles, `PNTGRP`.

### `co_freq.log` + `co_freq.gjf`
CO, HF/STO-3G, `Freq` only.  ECCE runtype **Vibration**.  This is the
issue #85 regression case: a *diatomic* frequency job, whose single normal
mode Gaussian prints in a different shape from the usual three-mode block.
Before the fix it produced no vibrational data at all.

    %chk=co_freq.chk
    #p HF/STO-3G Freq

    CO diatomic frequency test

    0 1
    C  0.0  0.0  0.0
    O  0.0  0.0  1.128

Pinned values: VIBFREQ 2600.5673, VIBIR 38.4596, VIBRAM 5.7433, VIBSYM SG,
VIB row labels `1-C 2-O`.

### `oh_uhf.log` + `oh_uhf.gjf`
OH radical, UHF/STO-3G single point with `Pop=Full`, doublet.  ECCE runtype
**Energy**, open shells 1.  Exercises the alpha/beta orbital path
(`ORBOCC`/`ORBOCCBETA`), `FERMICON`, and the archive-block parser
`gaussian-16.db` (which for an open-shell job emits TE, ESCF, S2, S2A,
CPUSEC).

    %chk=oh.chk
    #p UHF/STO-3G SP Pop=Full

    OH radical UHF single point

    0 2
    O   0.000000   0.000000   0.000000
    H   0.000000   0.000000   0.970000

This is also the fixture that reproduces the open-shell Mulliken bug
described in `../README.md`.

---

## nwchem/

### `h2o_opt.eprint` + `h2o_opt.nw`
Water, SCF/STO-3G geometry optimisation with `ecce_print`.  ECCE runtype
**Geometry**.  Exercises `GEOMTRACE` across optimisation steps, `TE`,
`SYMLAB`, `MOLAB`, the dipole/multipole entries.

### `oh_uhf.eprint` + `oh_uhf.nw`
OH radical, UHF/STO-3G single point, `nopen 1`.  ECCE runtype **Energy**.
Pinned value: `S2` = 7.53263362179062e-01 (the trace prints
0.753263362179062; `nwchem.scalar` reprints it in `%e` form).  This guards
the S² regex that used to match any line containing the digit 2.

### `oh_udft.eprint` + `oh_udft.nw`
Same radical, UB3LYP/STO-3G with `mulliken`, `mult 2`.  ECCE runtype
**Energy**, category DFT.  Exercises the beta-spin orbital-symmetry path
(`MOLAB4`/`MOLAB5` -> ORBSYM/ORBSYMBETA).

### `cp_h2o.eprint` + `cp_h2o.nw`
Water, plane-wave PSPW Car-Parrinello, `loop 5 5` — five CP steps.  ECCE
runtype **Car-Parrinello**.  This is the issue #84 regression case: the
`task_car-parrinello%begin%total gradient` block must fire five times and
the parser must emit the key **`TGRADCPVEC`**, not `EGRADVEC`.

Note: this run writes pseudopotential files (`H.psp`, `O.vpp`, ...) and a
`scratch/` directory next to the deck; they are not part of the fixture.

### `h2o_prop.eprint` + `h2o_prop.nw`
Water, STO-3G, `task scf property` with
`property; shielding; spinspin 1 2 3; efield; efieldgrad; electrondensity; end`.
ECCE runtype **Property**.  Added 2026-09-21 for issue #45, to cover the
`property`-module entries no earlier fixture reached: `SHIELDTENSOR` /
`ISOSHIELD` / `ANISOSHIELD` / `SHIELDEIGVAL`, `EFIELD`, `EFIELDGRAD` (+
`EFIELDASYMM`, `EFIELDEIGVAL`), `EDENS`, `SPINSPIN` (+ `SSISOTROPY`).

The SCF block is `uhf` with `nopen 0` — a **closed-shell UHF** wavefunction
— on purpose, and this is the only way to get all five properties out of
one NWChem 7.2.3 run: `spinspin` refuses an RHF reference ("Indirect
spin-spin coupling not calculated for RHF wave functions: needs UHF wave
function") *and* refuses a genuinely open-shell one ("...not calculated for
open shell wave functions: needs closed shell UHF!").  The molecule is C1
as written (the geometry is not symmetrised), so NWChem emits no "group
irrep names" block and `[MOLAB2]`/`[MOLAB3]` fire with nothing to map —
recorded as `silent_ok` on the case.

### `oh_hfine.eprint` + `oh_hfine.nw`
OH radical, UHF/STO-3G `nopen 1`, `task scf property` with
`property; hyperfine 2 1 2; end`.  ECCE runtype **Property**.  Added
2026-09-21 for issue #45: `hyperfine` needs a genuinely open-shell
wavefunction, so it cannot share `h2o_prop` above.  Covers `FERMI` and the
`[SPINDIPOLE]` entry (whose real keys are `DIPOLETENSOR` /
`DIPOLEEIGVAL`).  Also the regression guard for the `nwchem.mlknshell`
`"\nn"` typo that corrupted the `units:` header of `MLKNSHELL`.

### `h2o_mp2ccsd.eprint` + `h2o_mp2ccsd.nw`
Water, 6-31G, `task mp2` **then** `task ccsd(t)` in one deck.  ECCE runtype
**Energy**.  Added 2026-09-21 for issue #45.  Covers `EMP2`,
`ECCSDTPERT`, `ECCSDTPERT2` (all three share `nwchem.emp2`, which picks its
key off the matched line), and the two `nwchem.scalar` branches nothing had
ever triggered:

* `RESIDNORM` — NWChem 7.2.3 tags this `ccsd rms error`, never plain
  `rms error` (checked against every string in the binary), so the entry
  could not fire at all before the `.desc` fix.  Nine CCSD iterations here,
  so nine blocks.
* `CORRELTN` — the `mp2` and `dft` modules tag `correlation energy`, but
  `ccsd` tags `ccsd correlation energy`, so a plain `task ccsd(t)` job used
  to produce no Correlation Energy.  Both tasks are in this one deck
  deliberately, so the case pins that the entry fires twice and that the
  surviving scalar is the later (CCSD) value.

### `h2o_freq.eprint` + `h2o_freq.nw`
Water, SCF/STO-3G, `task scf freq`.  ECCE runtype **Vibration**.  Added
2026-09-21 for issue #45: `VIB`, `VIBFREQ` and `VIBIR` were hand-checked
against real output but had no fixture.  All 3N = 9 modes are kept,
including the six near-zero translational/rotational ones, exactly as the
trace prints them; the three real modes are 2043.29 / 4488.45 / 4767.59
cm-1.  `[VIBIR]`'s `Begin` has to stay pinned to the `(KM/mol)` variant —
NWChem prints a bare `projected intensities` block and a
`projected intensities (debye/ang)2` block immediately before it.

`[EZEROPT]` (`Begin=begin%zero point`) does **not** fire on this job:
NWChem 7.2.3 emits no `zero point` `ecce_print` tag at all (the string
exists in the binary only as an rtdb key, `vib:zero point energy`), so that
entry appears to be dead for this version.

---

## orca/

### `h2o_opt.out` + `h2o_opt.inp`
Water, RHF/STO-3G geometry optimisation, `%pal nprocs 1`.  ECCE runtype
**Geometry**.

    ! RHF STO-3G Opt
    %pal nprocs 1 end
    * xyz 0 1
    O   0.000000   0.000000   0.100000
    H   0.000000   0.789000  -0.500000
    H   0.000000  -0.789000  -0.500000
    *

Exercises ORCA's `GEOMTRACE` — the property whose `Skip=` off-by-one made
it silently produce zero output through the real monitor pipeline while
testing fine against hand-picked line ranges.

### `h2o_optfreq.out` + `h2o_optfreq.inp`
Water, RHF/STO-3G `Opt Freq`.  ECCE runtype **GeoVib**.  Captured
2026-09-21.  This is the fixture that covers most of `orca.desc`: 20 of
its 24 parse types fire on it.

    ! RHF STO-3G Opt Freq
    %output
      Print[P_MOs] 1
    end
    %elprop
      Quadrupole true
    end
    %pal nprocs 1 end
    * xyz 0 1
    O   0.000000   0.000000   0.100000
    H   0.000000   0.789000  -0.500000
    H   0.000000  -0.789000  -0.500000
    *

The `%output Print[P_MOs]` and `%elprop Quadrupole` blocks are not
optional decoration: `ai.orca` writes both unconditionally, and without
them ORCA prints no MO coefficients and computes no quadrupole moment at
all, so `MO`/`ORBENG`/`ORBOCC` and `QUADPOLE` would have nothing to
parse.  Keep them in any replacement deck.

Exercises: `GEOMTRACE` (5 frames), `TE`/`TEVEC`, `NNREPUL`/`ONEELEC`/
`TWOELEC`, `PE`/`KE`, `MO`/`ORBENG`/`ORBOCC`, `EGRAD`/`EGRADVEC`,
`DIPOLE` (au → Debye conversion), `QUADPOLE`, `MULLIKEN`, `VIBFREQ`/
`VIB`/`VIBIR`, `PNTGRP`, `EZEROPT`/`ETHERM`/`ENTHALPY`/`ENTROPY`/
`EGIBBS`, `DELTAE` (28 SCF iterations), `ETIME`.

This is also the regression fixture for the `[VIB]` End-line starvation
bug this suite found on 2026-09-21: `[VIBFREQ]`'s `End=NORMAL MODES`
consumed the line `[VIB]`'s `Begin` needed, so `[VIB]` could never fire
on any ORCA job and the Vibrational Frequencies panel never appeared.
Pinned: VIBFREQ = 9 rows (3N, translations/rotations as 0.00), VIB =
9 × 3 × 3, VIBIR = 9 rows — all three the same length, because
`NModePanel` indexes them against each other by row.

### `oh_uhf.out` + `oh_uhf.inp`
OH radical, UHF/STO-3G single point, doublet.  ECCE runtype **Energy**,
open shells 1.  Captured 2026-09-21.  The only fixture that can reach
ORCA's open-shell-only output: `S2` (0.753262) and the beta-spin orbital
path (`MOBETA`/`ORBENGBETA`/`ORBOCCBETA`).

    ! UHF STO-3G
    %output
      Print[P_MOs] 1
    end
    %elprop
      Quadrupole true
    end
    %pal nprocs 1 end
    * xyz 0 2
    O   0.000000   0.000000   0.000000
    H   0.000000   0.000000   0.970000
    *

Also guards the open-shell charge table: for a UHF job ORCA's
`MULLIKEN ATOMIC CHARGES` grows a second column (the spin population),
`   0 O :   -0.164453    1.069760`, and only the first must be taken —
the same trap that produced #80 and the open-shell Gaussian Mulliken bug.

### `h2o_nmr_chelpg.out` + `h2o_nmr_chelpg.inp`
Water, RHF/STO-3G `NMR CHELPG` at a fixed geometry.  ECCE runtype
**Magnetic**.  Captured 2026-09-21.

    ! RHF STO-3G NMR CHELPG
    %output
      Print[P_MOs] 1
    end
    %elprop
      Quadrupole true
    end
    %pal nprocs 1 end
    * xyz 0 1
    O   0.000000   0.000000   0.117300
    H   0.000000   0.757200  -0.469200
    H   0.000000  -0.757200  -0.469200
    *

Exercises `ISOSHIELD`/`ANISOSHIELD`, `SHIELDTENSOR`/`SHIELDEIGVAL` and
`ESPCHARGE`.  Two jobs were merged into one deck to keep the tree small;
they are independent property calculations on the same wavefunction.

`CHELPG` is in the deck **by hand**: `ai.orca` has no CHELPG path at all
(its `Magnetic` runtype emits only `! NMR`), so `ESPCHARGE` is currently
unreachable from the ECCE GUI.  The parser is covered here anyway, so it
is ready if a runtype ever requests it.

This is also the regression fixture for the second End-line starvation
bug found on 2026-09-21: `[ISOSHIELD][ANISOSHIELD]`'s `Begin` was
`CHEMICAL SHIELDING SUMMARY \(ppm\)`, which `[SHIELDTENSOR]`'s `End`
consumes, so isotropic shielding and anisotropy were silently never
extracted from any ORCA NMR job while the tensor from the very same
section parsed fine.

---

## mopac/

MOPAC 23.1.2 (`/usr/bin/mopac`, Debian), invoked `mopac <deck>.mop`,
which writes `<base>.out` (plus a `.arc` summary that is not part of any
fixture).  All captured 2026-09-21, the day MOPAC was integrated (#86).

MOPAC is semiempirical, so there is no basis set anywhere in these decks
and the energies are **heats of formation in kcal/mol**, which is the
ECCE Energy converter's own base unit — the parsers must pass those
numbers through unconverted.  Theory category is `SE`, theory `RPM7` /
`UPM7`.  The three positional header lines (keywords, title, comment)
are what `ai.mopac` writes, and lines 2 and 3 are never blank.

### `ch4_opt.mop` + `ch4_opt.out`
Methane, PM7 `EF` geometry optimisation.  ECCE runtype **Geometry**.

    PM7 EF CHARGE=0 SINGLET PRECISE
    methane geometry optimization
    ECCE MOPAC regression fixture
    C    0.000000    0.000000    0.000000
    H    0.629000    0.629000    0.629000
    H   -0.629000   -0.629000    0.629000
    H   -0.629000    0.629000   -0.629000
    H    0.629000   -0.629000   -0.629000

`PRECISE` matters for the fixture, not just for taste: without it this
optimisation stops loose enough that MOPAC reports the point group as
**D2d**, not Td.  Pinned values: heat of formation −14.40441 kcal/mol
(published simultaneously as `TE`, `TEVEC` and `HFENERGY`), point group
**Td**, ionization potential 13.727659 eV, Mulliken (really Coulson net
atomic) charges −0.603058 / 4 × 0.150764.  `GEOMTRACE` is two frames —
the 4-decimal input echo and the 9-decimal final geometry, MOPAC's two
different `CARTESIAN COORDINATES` layouts, both of which have to parse
from one `Skip=` value.

### `ch4_force.mop` + `ch4_force.out`
**Methane**, PM7 `FORCE THERMO` at the optimised geometry from the run
above.  ECCE runtype **Vibration**.

    PM7 FORCE THERMO CHARGE=0 SINGLET
    methane vibrational analysis
    ECCE MOPAC regression fixture
    C    0.000000000    0.000000000    0.000000000
    H    0.626527549    0.626527282    0.626539585
    H   -0.626527549   -0.626527282    0.626539585
    H   -0.626527514    0.626527248   -0.626539269
    H    0.626527514   -0.626527247   -0.626539269

**Methane, deliberately, and not water.**  MOPAC's readable
`DESCRIPTION OF VIBRATIONS` section collapses symmetry-degenerate modes
— CH4 has 9 modes but only 4 distinct frequencies there — while
`NORMAL COORDINATE ANALYSIS` lists all 9.  Sourcing `VIBFREQ` from the
first and `VIB` from the second silently misaligns them row by row,
which is what the first cut of this integration did.  A C2v molecule
such as water has no degeneracies and cannot expose it.  Pinned: 9
modes, frequencies 1291.2 ×2 / 1306.5 ×3 / 2689.9 ×3 / 2813.7, symmetry
labels `1E 1E 1T2 1T2 1T2 2T2 2T2 2T2 1A1` (the degeneracy pattern
itself), `VIB` = 9 × 5 × 3, zero point energy 24.854 kcal/mol.

The geometry must already be a stationary point: MOPAC refuses a `FORCE`
run otherwise ("Gradient in FORCE is too large"), and the refusal is a
normal termination, so it would look like a parsing problem rather than
an input problem.  This run also prints the *shorter* `HEAT OF FORMATION
=` wording (no "FINAL", `KCALS/MOLE` not `KCAL/MOL`), twice — which is
why `mopac.desc`'s energy `Begin` has an optional `FINAL `, and why
`TEVEC` is deliberately not emitted for that wording.

A `FORCE`-only run prints no `NET ATOMIC CHARGES` and no
`IONIZATION POTENTIAL` at all, so `[MULLIKEN][DIPOLE]` and `[IP]`
correctly never fire on this fixture.

### `ch4_geovib.mop` + `ch4_geovib.out`
Methane, PM7 `EF` followed by a second `OLDGEO FORCE THERMO` data set in
the same file.  ECCE runtype **GeoVib**.

    PM7 EF CHARGE=0 SINGLET PRECISE
    methane optimization then vibrational analysis
    ECCE MOPAC regression fixture
    C    0.000000    0.000000    0.000000
    H    0.629000    0.629000    0.629000
    H   -0.629000   -0.629000    0.629000
    H   -0.629000    0.629000   -0.629000
    H    0.629000   -0.629000   -0.629000

    PM7 FORCE THERMO OLDGEO CHARGE=0 SINGLET PRECISE
    methane optimization then vibrational analysis
    Vibrational analysis at the optimized geometry

This two-data-set form is the only way MOPAC can express "optimise then
compute the Hessian", and it is exactly what `ai.mopac` writes for
GeoVib (note the blank separator line and that the second data set has
no geometry block of its own).  The case pins the invariant
`mopac.desc` relies on: three energy matches but only **one** `TEVEC`
frame against **three** `GEOMTRACE` frames, so the Geometry Trace plot
can never have more points than the trace has frames.

### `h2o_grad.mop` + `h2o_grad.out`
Water, PM7 `1SCF GRADIENTS`.  ECCE runtype **Gradient**.

    PM7 1SCF GRADIENTS CHARGE=0 SINGLET
    water single point gradient
    ECCE MOPAC regression fixture
    O    0.000000    0.000000    0.000000
    H    0.000000    0.000000    0.960000
    H    0.926000    0.000000   -0.240000

The only runtype that produces MOPAC's `PARAMETER / ATOM / TYPE`
gradient table, and therefore the only one that can exercise
`EGRADVEC`.  Pinned at full precision, because a dropped unit conversion
still yields a plausible-looking gradient: MOPAC prints
kcal/(mol·Å) and ECCE wants Hartree/Bohr, so −1.252503 × 0.001593601451
× 0.52917725 = −0.0010562327.  Also the one fixture with a genuinely
non-zero dipole (1.690 / −0.000 / 1.315 Debye), so the X/Y/Z split of
the combined `[MULLIKEN][DIPOLE]` block is actually tested.

### `ch3_uhf.mop` + `ch3_uhf.out`
Methyl radical, PM7 `UHF 1SCF DOUBLET`.  ECCE runtype **Energy**, theory
`UPM7`, open shells 1.

    PM7 UHF 1SCF CHARGE=0 DOUBLET
    methyl radical UHF doublet
    ECCE MOPAC regression fixture
    C    0.000000    0.000000    0.000000
    H    1.078000    0.000000    0.000000
    H   -0.539000    0.933000    0.000000
    H   -0.539000   -0.933000    0.000000

The open-shell path.  Unlike Gaussian's and ORCA's, MOPAC's charge table
has the same shape open- or closed-shell (no spin-density column), so the
charge list must be exactly one entry per atom — the guard against the
open-shell-charge-table bug family (#80, `g16-oh-uhf`, `orca-oh-uhf`).
Pinned: heat of formation 28.40785 kcal/mol, point group D3h, IP
9.910583 eV, charges −0.411272 / 0.137160 / 0.137056 / 0.137056.

MOPAC prints no ⟨S²⟩, and `mopac.desc` declares no `S2` entry; that is
correct, not a gap.

---

## Regenerating

From a scratch directory, with the code on `PATH`:

    g16     h2o_optfreq.gjf          # -> h2o_optfreq.log
    g16     co_freq.gjf              # -> co_freq.log
    g16     oh_uhf.gjf               # -> oh_uhf.log
    nwchem  h2o_opt.nw  > /dev/null  # -> h2o_opt.eprint   (via ecce_print)
    nwchem  oh_uhf.nw   > /dev/null  # -> oh_uhf.eprint
    nwchem  oh_udft.nw  > /dev/null  # -> oh_udft.eprint
    nwchem  cp_h2o.nw   > /dev/null  # -> cp_h2o.eprint
    orca    h2o_opt.inp        > h2o_opt.out
    orca    h2o_optfreq.inp    > h2o_optfreq.out
    orca    oh_uhf.inp         > oh_uhf.out
    orca    h2o_nmr_chelpg.inp > h2o_nmr_chelpg.out
    mopac   ch4_opt.mop        # -> ch4_opt.out    (+ ch4_opt.arc, discard)
    mopac   ch4_force.mop      # -> ch4_force.out
    mopac   ch4_geovib.mop     # -> ch4_geovib.out
    mopac   h2o_grad.mop       # -> h2o_grad.out
    mopac   ch3_uhf.mop        # -> ch3_uhf.out

(The only edit made to any checked-in deck: the `ecce_print <file>` line of
each `.nw` was renamed to match the fixture's filename, so regenerating in
place produces the right name.  Nothing else, and no output file, was
touched.)

Then `../run_tests.py --update` and **review the golden diff**: a new code
version will legitimately shift timings and formatting, but a change in a
*value* or in which parse types fire is exactly the thing this suite is for.

### gaussian-16/h2o_nmr.{gjf,log}

Gaussian 16 RevC.01, `#P RHF/STO-3G NMR Pop=(MK) NoSymm`, water at its
STO-3G geometry. Generated 2026-09-21 on niobium with
`/opt/gaussian/g16/g16`.

Added to cover `[SHIELDTENSOR][ISOSHIELD][ANISOSHIELD][SHIELDEIGVAL]`,
which no fixture exercised before — `gaussian-16.desc` had 30 of its 50
parse types never firing. The ORCA equivalent of this entry turned out
to be silently dead (see `orca.desc`'s `[ISOSHIELD]` End-starvation
fix), so it was worth establishing that Gaussian's is not. **It is
not**: all four keys come out of the one block, values matching the log
by hand (O isotropic 365.6728, anisotropy 4.1792, eigenvalues 363.1113
/ 365.4481 / 368.4589; both H isotropic 33.6821).

`Pop=(MK)` is in the deck but `[ESP]` still does not fire: it is a
`File=`-rule parse type reading `grid.dat`, which this run does not
produce, and `File=` rules are excluded from line matching anyway. ESP
coverage would need a different setup.

### gaussian-16/h2o_ccsdt.{gjf,log}

Gaussian 16 RevC.01, `#P CCSD(T)/6-31G NoSymm`, water. Generated
2026-09-21 on niobium with `/opt/gaussian/g16/g16`.

Covers the correlated-method energy branches, which no fixture
exercised. Six of them — `[EMP21]`, `[EMP3]`, `[EMP4DQ]`, `[EMP4SDQ]`,
`[ECCSDTPERT]`, `[ECCD][ECCSD]` — match a block here and emit nothing,
which looks exactly like #85 from the outside but is correct:
`gaussian-16.energy` gates its whole body on `runtype =~ /Geo/i` (line
114, documented in its own header), and for a non-optimisation job the
energies come from the archive block via `gaussian-16.db`.

That fallback is asserted rather than assumed: the case pins all seven
archive energies (ESCF, EMP2, EMP3, EMP4DQ, EMP4SDQ, ECCSD,
ECCSDTPERT), cross-checked against the per-line values in the log
(`EUMP2 = -0.76111680142148D+02` and `CCSD(T)= -0.76119325411D+02`
agree with EMP2 and ECCSDTPERT).

Also confirms the documented `[EMP21]`/`[EMP22]` EUMP2 Begin collision
is harmless in practice.

---

## qe/

Quantum ESPRESSO 6.7 (`/usr/bin/pw.x`, Debian `quantum-espresso
6.7-3+b1`), invoked `pw.x -in <deck>.pwin > <deck>.pwout`.  All captured
2026-09-21, the day QE was integrated (phase 1 of `docs/QE_ROADMAP.md`).

**These decks are not hand-written.**  Every `.pwin` here was GENERATED BY
`scripts/parsers/ai.qe` from a real `.param` / `.frag` pair whose values
are exactly what `scripts/codereg/qetheory.py` and `qeruntype.py` export
(captured from the real dialogs over their own UDP export socket), and
then run to completion.  So these fixtures test the input generator and
the parsers as one pipeline, which is not true of any other code's
fixtures here.  To regenerate one:

    ECCE_HOME=<repo> cp scripts/parsers/qe.tpl <name>.pwin
    ECCE_HOME=<repo> scripts/parsers/ai.qe -p -f -n <name> -t <name>.pwin
    pw.x -in <name>.pwin > <name>.pwout

QE is a plane-wave pseudopotential code: there is no basis set anywhere
(the theory is `needsBasis="false"`), **periodicity is mandatory** — each
`.frag` carries a `lattice_vectors:` block, which `ai.qe` refuses to
proceed without — and the pseudopotential file per element is resolved by
scanning `/usr/share/espresso/pseudo`, never typed.  Theory category is
`PW`, theory `PW`, runtype **Energy** (`calculation='scf'`).

All energies in the outputs are **Rydberg**, and
`src/util/units/EnergyConverter.C` has no Rydberg entry, so every parser
converts to Hartree (x 0.5).  Forces are **Ry/au**, and `EGRADVEC` is a
gradient rather than a force, so `qe.egradvec` multiplies by **-0.5**
(sign flip *and* unit conversion in one factor).  Those two conversions
are what the pinned values in `cases.py` exist to protect.

### `si_scf.pwin` + `si_scf.pwout`
**Bulk silicon**, primitive fcc cell (a = 5.43 A), 4x4x4 Monkhorst-Pack
grid, `occupations='fixed'`, `ecutwfc = 18 Ry`, forces and stress both
on.  The reference periodic case: a real crystal with a real k-point
grid.

Pinned values: total energy -11.411697375 Hartree (from
`!    total energy              =     -22.82339475 Ry`), a 4-point
`EWVEC` SCF trace ending at -11.41169429, a 5-point `DELTAE` trace ending
at 4e-07, and a 2x3 `EGRADVEC` (all zero -- silicon's forces vanish by
symmetry, which is why the water fixture below exists).

The 4 `EWVEC` blocks against 5 SCF iterations is the point of the
fixture: pw.x marks the converged energy with a leading `!` and prints
the same phrase without it for each iteration, so `[TE]` (anchored `^!`)
and `[EWVEC]` (anchored `^\s+`) split one phrase into two entries that
cannot match the same line.

### `h2o_gamma.pwin` + `h2o_gamma.pwout`
**Water in a 10 A cubic box**, `K_POINTS gamma`, `ecutwfc = 30 Ry`,
forces on.  Two species, so `ATOMIC_SPECIES` has two rows and two
differently-named pseudopotential files were resolved out of
`pseudo_dir` (`O.pbe-n-kjpaw_psl.0.1.UPF` and
`H.pbe-rrkjus_psl.1.0.0.UPF` -- note they share no naming convention,
which is exactly why the filename is never typed by hand).

This is the fixture with **non-zero forces**, and its `EGRADVEC` is
pinned at full precision because a dropped sign flip, a missing 0.5, or
one applied twice all still look like a perfectly ordinary water
gradient.

Pinned values: total energy -21.985236255 Hartree, 9-point `EWVEC`,
10-point `DELTAE`, 3x3 `EGRADVEC` starting
`0.0000000000 0.0000000000 0.0229597350`.

### `al_smear.pwin` + `al_smear.pwout`
**fcc aluminium** -- a **metal**, `occupations='smearing'` with
Marzari-Vanderbilt ("cold") smearing, `degauss = 0.02 Ry`, 4x4x4 grid,
`ecutwfc = 25 Ry`, stress on.

`docs/QE_ROADMAP.md` section 7 calls for this explicitly.  A metal's
output is not the same shape as an insulator's: it prints
`the Fermi energy is ...` and a `smearing contrib. (-TS)` term and no
`highest occupied level`, and (with a PAW pseudopotential) a
`total all-electron energy =` line that is one word away from matching
`[EWVEC]`'s Begin.  Testing only insulators would be the same mistake as
testing MOPAC frequencies only on water.

Pinned values: total energy -19.740479745 Hartree, 2-point `EWVEC`,
3-point `DELTAE`, 1x3 `EGRADVEC`.
