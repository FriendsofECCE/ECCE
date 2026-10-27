---
type: map
title: "`[PROGRESS]` entries in a `.desc`: job progress, not properties; matched on every line, folded into a snapshot, sent at most once a second (#284)"
area: codereg
section: "Getting a calculation set up (the "code registration" system)"
paths: ["scripts/eccejobmonitor", "scripts/parsers/ecceqm.desc", "src/qm/output.cpp", "src/qm/main.cpp", "tests/progress/monitor_test.py", "tests/parsers/eccejobmonitor_sim.py", "src/comm/commxt/JobStore.C"]
issues: [284, 303]
---
A code gets progress in the Organizer by `.desc` entries alone:

    [PROGRESS]
    Begin=^\s+(\d+)\s+(-?\d+\.\d+)\s+(\S+)      any regex, groups $1..$9
    Progress=scf cycle=$1 energy=$2 dE=$3/1e-8  "<stage> key=value ..."
    History=dE                                  optional, kept for a plot
    [END]

- **Not in the property table.** `PDFileRead` diverts them to
  `@gProgList`; no `Script`, `End`, `Lines`, `Frequency` (an error if
  given). They are tested against *every* line, inside other entries'
  blocks too, and never consume a line, so the Begin-priority and
  End-starvation traps of property entries do not apply either way.
  `JCode::parseScript` skips them (no `Script`), and the parser test
  simulator keeps them in `Desc.progress`.
- **The snapshot.** Per stage the monitor keeps the latest fields. A record
  key (`cycle`, `step` or `point`, the first one present) that changes
  clears the stage's other fields; one that goes down (a new SCF) also
  clears its history. So a multi-line block (ORCA's convergence table) is
  one entry per line, all setting fields of the same `opt` stage, with the
  step line first. A value `a/b` is a quantity and its target (the box
  shows a tick or a cross); `point=k/n` gets `eta=` seconds from the
  monitor's own clock. History keeps the last 60 values; the message is
  trimmed to one packet (2042 bytes).
- **Sending.** `jmPROG <text>` only with `-progress 1`, which
  eccejobstore puts on the monitor's command line: an older job store would
  restart on an unknown message type, an older monitor ignores the flag.
  At most one a second, latest wins; a held-back snapshot goes at the next
  end-of-file wait (`timePauseReadLine`, 2 s local, 10 s otherwise) and the
  last one when the output ends.
- **Stage names the box knows:** `opt`, `freq`, `scf` (titles, order,
  plots: the optimisation's max and RMS force per step with their limits
  on a log axis, |dE| per cycle likewise). Field labels it knows:
  `energy`, `dE`, `grad`, `maxgrad`, `rmsgrad`, `maxstep`, `rmsstep`,
  `converged=yes|no`, `done=yes real= imaginary= zpe=` (freq summary).
  Other names are shown as they are; new labels go in
  `src/util/genutil/JobProgress.C`.
- **`History=a,b,c`** keeps several keys of one stage, each as its own
  `stage~ key=...` line; the first is the stage's main history. An `opt`
  stage with `maxgrad`/`rmsgrad` histories is plotted as those forces (log
  |value|, the limit from the current `value/limit` field, Hartree/bohr when
  the code prints a limit); without them it plots its energy as before.
- **Targets printed once: `Remember=`.** A `[PROGRESS]` entry with
  `Begin=` and `Remember=<name>` (no `Progress`) keeps its `$1`; other
  entries write it as `{name}`, e.g. `maxgrad=$3/{gmax}`. A name not yet
  seen becomes empty, so the box shows the value without a tick or cross.
  Fortran `1.0D-04` in a message becomes `1.0e-04`. Used for NWChem's
  thresholds and ORCA's SCF tolerance, which follow the user's settings.
  `Value=<word>` beside `Remember=` keeps that word instead of `$1`; with
  `Progress={name} ...` (a template may start with `{name}` as the stage) it
  switches an entry on (`Value=freq`) or off (`Value=-`, no stage) from
  another line: ORCA's CP-SCF lines count only after "Nuclear geometric
  perturbations ... YES", Gaussian's CPHF passes only when the route has
  Freq, NWChem's linear-equation table only inside the analytic Hessian.
  `Times=<n>` beside `Remember=` keeps `$1` times n: MOPAC prints the atom
  count, and its FORCE runs 3 steps per atom (`freq point=$1/{nforce}`).
- **A second file.** When `jobOutputFile2` (stdout) is not the parse file,
  the monitor also feeds its complete lines to the `[PROGRESS]` entries
  (`ProgressTail`, every 0.5 s, and before each wait and the last
  snapshot); never for properties. NWChem needs it: the `ecce_print` trace
  has SCF energies and gradients, but each value on a line of its own and
  no optimiser table or thresholds.
- **What a code can give.** Steps and criteria where the output prints
  them per step; analytic frequencies only a closing `done=yes zpe=`
  (no counts of real/imaginary modes, the monitor keeps no counters);
  numerical ones (ORCA `NumFreq`) `point=k/n`. Gaussian: the freq job of
  `Opt Freq` repeats "Step number 1 out of a maximum of 2", which would
  restart the opt record, so that wording is skipped.
- **MOPAC** prints SCF iterations only with `PL` (ECCE's inputs do not set
  it), so an SCF is normally just `scf converged=yes` and the summary says
  "SCF converged" without a count; the opt history is the gradient, so
  the summary takes an energy from the history only when it is `energy`.
  CP2K (#329): the SCF table and the optimiser summary are read as in ORCA; see `cp2k-code-registration-follows-qe.md`.
- **GROMACS** names the stage by the integrator (`Progress={integrator}
  point=$1/{nsteps}`): `md` is "Molecular dynamics", `steep` "Minimisation".
  QE prints the ionic step after its criteria, so it is sent as `istep`
  (not a record, which would clear them).
- **ECCE-QM** writes its own `progress scf|opt|freq ...` lines
  (`write_progress_*` in `src/qm/output.cpp`), flushed per line.

`tests/progress/monitor_test.py` (ctest `progress-monitor`) runs the real
monitor over a made-up output; `tests/launch/ecceqm_test.py` checks the
messages of a real GeoVib run as the Organizer receives them.

- **`opt` history spans the job (#457).** A step number that goes down
  (a relaxed scan or IRC starting the next point) clears an SCF's history
  but not the `opt` stage's, so the box's energy plot shows one descent per
  point; `opt` keeps up to 150 values, other stages 60, and the message
  trimming drops the oldest first. A `scan point=k/n` stage (ORCA: the scan
  header's `steps =`; Gaussian 16: `on scan point k out of n`) gives the
  "point 4 of 11" line; for several scanned parameters ORCA's total is the
  last parameter's, so the box drops a total smaller than the point. A
  code whose `opt` history is not an energy (Gaussian 16: dE) plots that.
- **Counting modes: `Count=<name>` (#611).** A `[PROGRESS]` entry with
  `Progress=` and `Count=fq` adds every number its regex captures on a line
  (all matches, so `(?!^)\G` walks along a row of frequencies) to
  `{fq_real}` or `{fq_imag}` by sign; |value| < 1 (translations, rotations)
  is skipped. `When=<name>` counts only while `{name}` is `on`. Reset the
  counts with `Remember=fq_real Value=0` (and `fq_imag`) at the block's
  header. Gaussian, ORCA, NWChem and MOPAC use it for `real=`/`imaginary=`;
  NWChem prints its zero-point line before the table, so the counts are set
  after `done=yes` there. CP2K and QE have no fixture with vibrations.
