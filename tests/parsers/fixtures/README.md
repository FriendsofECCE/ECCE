# Fixture provenance

Every file here is genuine output from a real binary, captured on
**niobium** (Debian 13 "trixie") on **2026-09-21**.  Nothing is hand-edited
or trimmed: a trimmed file can change how `eccejobmonitor`'s Begin/Skip/End
feeding behaves, and the point of the suite is that it behaves identically
to production.  Each output file is accompanied by the exact input deck
used to produce it, so any of them can be regenerated.

Total size of this tree: ~460 KB.  Keep it that way — prefer STO-3G and a
handful of atoms.  Every job below runs in seconds.

## Codes and versions

| code      | version                                  | invocation |
|-----------|------------------------------------------|------------|
| Gaussian  | `ES64L-G16RevC.01  3-Jul-2019` (`/opt/gaussian/g16/g16`) | `g16 <deck>.gjf` |
| NWChem    | Debian package `nwchem 7.2.3-10`         | `nwchem <deck>.nw > <deck>.out` |
| ORCA      | `6.1.1` (`/opt/orca/orca_6_1_1_linux_x86-64_shared_openmpi418_nodmrg`) | `orca <deck>.inp > <deck>.out` |

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
    orca    h2o_opt.inp > h2o_opt.out

(The only edit made to any checked-in deck: the `ecce_print <file>` line of
each `.nw` was renamed to match the fixture's filename, so regenerating in
place produces the right name.  Nothing else, and no output file, was
touched.)

Then `../run_tests.py --update` and **review the golden diff**: a new code
version will legitimately shift timings and formatting, but a change in a
*value* or in which parse types fire is exactly the thing this suite is for.
