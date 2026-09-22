# Adding Quantum ESPRESSO as a registered code — implementation roadmap

Status: **IMPLEMENTED — this document is now historical.** Written
2026-09-21 against `main` at `cc82254` as a design document, before any
code existed. Since then QE has been registered end to end: see
`data/client/cap/QuantumESPRESSO.edml`, `scripts/parsers/ai.qe`,
`qe.tpl`, `qe.desc`, `qe.launchpp`, the `qe.*` parse scripts, and
`scripts/codereg/qe{theory,runtype}.py`.

Quantum ESPRESSO 6.7 **is now installed on this machine**
(`/usr/bin/pw.x`), and four real fixtures live in
`tests/parsers/fixtures/qe/` — three single points and, as of
2026-09-22, a genuine 6-step BFGS optimisation used to build and verify
`GEOMTRACE`. So where the sections below say a statement is inferred
from documentation rather than from a binary, prefer the fixtures and
the test suite, which are checked against the real program.

Properties extracted today: VERSION, TE, EWVEC, DELTAE, EGRADVEC,
CPUSEC/ETIME, GEOMTRACE. Known gaps, in rough order of value: TEVEC (see
the alignment warning in `qe.desc` — energies and geometries are offset
by one), LATTICEVEC for `vc-relax` (the machinery to animate a changing
cell already exists, per §1 below, and has no producer for QE), and
anything requiring `ph.x`, which is a separate executable and a separate
integration.

Original preamble, kept for context: every statement about Quantum
ESPRESSO's behaviour below came from its published input documentation
(`INPUT_PW`, `INPUT_PH`) and from output snippets in third-party
teaching material — *not* from a binary. Statements about
ECCE come from reading this tree and from one real NWChem 7.2.3
plane-wave fixture (`tests/parsers/fixtures/nwchem/cp_h2o.eprint`) and
are marked where they are inferences rather than verified facts.

Read `CLAUDE.md`'s "code registration" section and its **New-code
checklist** (ORCA #38, extended by MOPAC #86) first. This document
assumes them and does not repeat them; §7 walks the checklist and says
which items are irrelevant for QE and which need new treatment.

---

## 1. Executive summary

**Feasibility: yes, and on better foundations than expected.** The
surprise of this investigation is that ECCE already contains a real,
working periodic-boundary subsystem — not a stub. There is a
first-class `LatticeDef` owned by `Fragment`, a dedicated Builder tool
panel ("Periodic Builder", `src/apps/builder/PBC.C`, 1109 lines) in
which a user can type either a/b/c/α/β/γ or nine Cartesian lattice
vector components, pick one of 230 space groups and symmetry-expand an
asymmetric unit, replicate, fold, build a supercell, transform the cell
by Miller indices and slice a surface; a scene-graph node (`SGLattice`)
that draws the cell *and* n₁·n₂·n₃ replicated cell boxes with
colour-coded a1/a2/a3; a `LATTICEVEC` time-series property; and
`GTStepCmd` already assigns a per-step 3×3 cell so a **variable-cell
optimization can be animated with a changing box**. The lattice
round-trips through ECCE's own `.frag`/MVM format
(`lattice_vectors:`), and `ai.nwchem` already reads it back and emits
`system crystal cartesian` into a generated deck. So the gating
question "can ECCE even express a crystal?" answers **yes** — and the
answer was not obvious from the NWPW theory dialog alone, which is what
a reader would look at first.

**Scale: materially bigger than ORCA (#38) or MOPAC (#86), but not by
an order of magnitude.** For calibration, the whole ORCA integration
was 27 new files (1 `.edml`, 2 Python dialogs, `ai.orca` at 475 lines,
`orca.tpl`, `orca.desc`, 22 parser scripts) plus `ResourceDescriptor`
×2, `httpd.conf.ecce`, a `gensub` sub and an icon. MOPAC was 15 files.
QE needs all of that, at roughly ORCA's file count, **plus three things
neither of them needed**:

1. **A pseudopotential-file selection subsystem.** This is the single
   largest genuinely-new piece and the main risk. See §3.1 and §8.Q3.
2. **k-point / Brillouin-zone specification.** ECCE has *none*,
   anywhere — verified: zero matches for `brillouin|monkhorst|kpoint|
   k_point` across `scripts/` and `data/client/`. NWChem's `BAND`
   theory is offered in `NWChem.edml` and generates no k-point block at
   all, i.e. every ECCE-generated band-structure run to date has been
   Γ-only. This is new UI plus new input generation, but it is small
   and well-understood (a Monkhorst–Pack grid is four to six integers).
3. **Human-readable output parsing for a plane-wave code.** 99% of
   `nwchem.desc`'s 80 `Begin=` patterns match NWChem's `ecce_print`
   machine-tagged stream (`task_car-parrinello%begin%total energy%1%
   double`), which is self-describing in name, dimensions *and* type.
   **QE has no equivalent facility.** Every existing plane-wave parse
   script (`nwchem.cp`, `nwchem.latvec`, `nwchem.geomtrace`,
   `nwchem.egradvec`) reads that tag line and is therefore **not
   reusable**. QE's `.desc` has to be written the way `orca.desc` and
   `mopac.desc` are — banner-prose `Begin`/`Skip`/`End` line matching,
   which is exactly the family of bugs `tests/parsers/` exists to catch.

**Largest risks, in order:**

| Risk | Why it is the risk it is |
|---|---|
| **Pseudopotentials** | QE will not run without a UPF file named per element in `ATOMIC_SPECIES`, resolved against `pseudo_dir`. ECCE has a Gaussian *basis-set* library (918 files under `data/admin/basissets`) with a per-element selection tool, but nothing whose content model fits a pseudopotential, no notion of a PP family/table, and no validation that a chosen PP covers the elements present. NWChem sidestepped this entirely: `nwpw` *generates* its own pseudopotentials from `$NWCHEM_NWPW_LIBRARY`, which ECCE never sets (the only occurrence in the whole tree is one hand-written site file, `siteconfig/CONFIG.chinook:76`). There is no precedent to copy. |
| **`ph.x` is a second executable** | Vibrational frequencies in QE require `pw.x` (scf) → `ph.x` → `dynmat.x`, sharing `prefix`/`outdir`. ECCE's machine registration stores **one path per code** (`$::config{$::lccode}` in `scripts/gensub`), and a calculation has **one** `<Input type="primary">`, one parse stream and one `.desc`. The launch side can be made to work (a `gensub` sub can emit three invocations in one submit script); the *registration* and *parse* sides cannot without a decision. See §3.2 and §8.Q2. |
| **Two unsynchronized places to enter a cell** | The Builder's `LatticeDef` and the calculation editor's own `ES.Theory.NWPW.Cell*` widgets are independent, and `ai.nwchem` honours the fragment's lattice *only when* `NWPW.CellType eq "None"`. Repeating that design for QE would reproduce a known wart. QE should take the cell from the fragment only. |
| **`.desc` prose matching** | See the four distinct silent-failure modes catalogued in `CLAUDE.md` and `tests/parsers/README.md`. Budget real time for `tests/parsers/` fixtures, and write them *before* believing anything works. |

**Not risks, contrary to expectation:**

- **Namelist input generation is easy.** `nwch.tpl`/`orca.tpl` are plain
  `##Token##` substitution and `ai.<code>` is Perl that, per token,
  either substitutes a dictionary value or calls a Perl sub of the same
  name returning arbitrary multi-line text (unresolved tokens delete
  the whole line — `ai.nwchem:79-125`). `&SYSTEM … /` is just a sub
  returning a string. There is nothing about Fortran namelists the
  existing engine cannot express. Do not treat this as a design problem.
- **The data model does not need extending for the common cases.** A
  fixed-cell scf/relax run needs no new C++ at all. A `vc-relax` trace
  needs only that `LATTICEVEC` be emitted, which `GTStepCmd` already
  consumes.

### 1.x Update, 2026-09-21 — two findings that soften the worst parts

Added after the analysis above was written. Both improve it; neither
changes the overall shape.

**1. Debian packages the pseudopotential library, which largely
dissolves §3's hardest problem.** On trixie:

| Package | Provides | Installed size |
|---|---|---|
| `quantum-espresso` 6.7-3+b1 | `/usr/bin/pw.x`, **`/usr/bin/ph.x`**, `/usr/bin/dynmat.x` and the rest | ~281 MB |
| `quantum-espresso-data-sssp` 1.3.0-3 | **103 UPF files covering ~102 elements** in `/usr/share/espresso/pseudo/` | ~162 MB |
| `quantum-espresso-data` | documentation | — |

Verified present in the SSSP package: H, C, N, O, Si, Fe, Al, Cu — i.e.
the ordinary cases, not just exotica.

This means **phase 1 should NOT make the user type UPF filenames**, as
§3 and §5 recommend. Instead default `pseudo_dir` to
`/usr/share/espresso/pseudo/` and populate a per-element combo by
scanning that directory. That is much closer to how the basis-set
library already behaves, and a far better first experience. The
"pseudopotentials are an unbounded new subsystem" framing in §3 was
written before this was checked and is too pessimistic — treat the
library as *provided* and the remaining work as *selection UI*. The
harder sub-problems (multiple PPs per element, functional consistency
between PP and `input_dft`, missing-element handling) all remain real.

**2. QE 7.5 source is unpacked at `/opt/QE/qe-7.5/` — unbuilt, but it
carries two assets worth more than the binaries.**

- `test-suite/` holds **643 `benchmark.out.*` reference outputs** across
  scf, relax, vc-relax, ph, cp and more. These are real QE outputs, so
  the parsing phases (2–4) can be developed and regression-tested
  against them **before any QE binary is installed**, exactly the way
  `tests/parsers/` already works for the other codes. This removes the
  "QE not installed, so output claims are unverified" caveat from most
  of the parsing work.
- `Doc/INPUT_PH.txt` and `PW/Doc/` give the input documentation offline.

Already verified against a real reference output rather than
documentation: the total-energy anchor this roadmap quotes appears as
`!    total energy              =     -15.77714687 Ry`.

**Version gap to keep in mind:** Debian ships **6.7**, the local
reference outputs and docs are **7.5**. For basic `scf`/`relax`/
`vc-relax` the input and output shapes are stable across that gap, but
any `.desc` rule developed against a 7.5 benchmark output must be
re-checked against 6.7's actual output before it is trusted — that is
the same version-drift trap that produced the Gaussian-16 Mulliken bug
(#80) and the NWChem `ccsd`-prefix bugs (#45). Generate the regression
fixtures from the **installed** binary, and use the 7.5 benchmarks only
for development and for cases the install cannot produce.

**3. Now verified against the real installed binary.** The Debian
packages were installed and a real `pw.x` scf run completed (silicon,
`ibrav=2`, 4×4×4 Monkhorst–Pack, the packaged
`Si.pbe-n-rrkjus_psl.1.0.0.UPF`; deck and output kept at
`~/ecce-test-logs/qe/si_scf.{in,out}`). **Every parse anchor this
roadmap quoted from documentation appears verbatim in real 6.7 output**,
so the version gap does not affect the core ones:

| Anchor | Real 6.7 output |
|---|---|
| Total energy | `!    total energy              =     -22.82211442 Ry` |
| Forces | `Forces acting on atoms (cartesian axes, Ry/au):` |
| Stress | `total   stress  (Ry/bohr**3)                   (kbar)     P=  ...` |
| SCF iterations | `iteration #  1     ecut=    18.00 Ry     beta= 0.70` |
| k-points | `number of k points=     8` |

Two practical notes that follow:

- **`<Output type="parse" verifypattern="...">` should be
  `Program PWSCF`**, not a version string: the banner reads
  `Program PWSCF v.6.7MaX starts on ...`, so anchoring on the version
  would break on every upgrade. (Compare MOPAC, where `MOPAC v` was
  safe because the name and `v` are adjacent.)
- **The pseudopotential in `ATOMIC_SPECIES` must be the filename**, and
  the packaged names are long and functional-specific
  (`Si.pbe-n-rrkjus_psl.1.0.0.UPF`). This confirms a per-element combo
  populated by scanning `pseudo_dir` is the right UI: nobody will type
  those correctly, and picking the wrong functional family silently
  gives a physically inconsistent calculation.

`~/ecce-test-logs/qe/si_scf.out` is a ready first fixture for
`tests/parsers/` when phase 2 starts.

---

## 2. How nwpw works today — file by file

This is the existing plane-wave path in full, so a reader does not have
to rediscover it.

### 2.1 `data/client/cap/NWChem.edml`

Two plane-wave theories, bracketed by `<!-- Start/End Eric Bylaska's
planewave module integration -->`:

```xml
<Theory category="NWPW" name="PSPW" needsBasis="false">
  <runtype>Energy</runtype> <runtype>Gradient</runtype>
  <runtype>Geometry</runtype> <runtype>Vibration</runtype>
  <runtype>GeoVib</runtype> <runtype>Property</runtype>
  <runtype>Car-Parrinello</runtype>
</Theory>
<Theory category="NWPW" name="BAND" needsBasis="false">
  … same minus Car-Parrinello …
</Theory>
```

`needsBasis="false"` is the pre-existing switch that turns off the
whole basis-set machinery (`JCode.C:488`,
`CalcEd::updateBasisWidgets()`; `ESInputController`'s `write_gbsconfig()`
skips the `.basis` file, so `ai.nwchem` is never handed `-b`). MOPAC
reused it; QE will too.

`<IntegrationFiles>`: `nwch.tpl`, `ai.nwchem`, `NWChem.expt`,
`nwchem.desc`, `nwchem.launchpp`, `std2NWChem` (the last is **unused**
on the plane-wave path). Dialogs: `nedtheory.py` / `nedruntype.py`.

**What the `.edml` does *not* do for plane waves:** no plane-wave
`<DataFiles>` entry of any kind — no `.movecs`/`.emovecs` wavefunction,
no pseudopotential file, no CP trajectory; the `type="restart"` entry
has an empty filename. And neither `<TheorySummary>` nor
`<RuntypeSummary>` lists a single NWPW or CPMD key, so nothing
plane-wave ever appears in the calculation summary panel.

### 2.2 `scripts/codereg/nedtheory.py` — the plane-wave theory dialog

All plane-wave widgets are under `if EcceGlobals.Category == "NWPW"`.
Exact `GUIValues` keys:

*Theory Options – NWPW* (lines 384-501): `ES.Theory.NWPW.XCFunctionals`
(lda, pbe96, revpbe, blyp, hf, pbe0, revpbe0 + `-sic` variants),
`.UseCutoff` + `.Cutoff` (default 30.0 Hartree), `.UseNpDimensions` +
`.NpDimensionsRows` + `.NpDimensionsCols`, `.UseRcut` + `.EwaldRcut`,
`.UseNcut` + `.EwaldNcut`, `.mapping`, `.Mulliken` (None/LCAO/Kawai).
Elsewhere: `.Spintype` (Restricted/Unrestricted), `.InitialGuess`
(LCAO/LCAO_skip), `.ConvergenceDensity.Value`, `.ConvergenceEnergy.Value`,
and shared `ES.Theory.SCF.ConvergenceAlgorithm` (CG/LMBFGS for NWPW).

*Unit Cell* box (lines 503-688): `ES.Theory.NWPW.CellType`
(`None`/`SC`/`FCC`/`BCC`/`LatticeVectors`), `.CellBoundary`
(`periodic`/`aperiodic`), `.CellL`, and nine `.a1x … .a3z` floats.

**Dead / disabled in this dialog, with the original comments intact:**

- A sixth cell mode `"Lattice"` (a/b/c/α/β/γ entry) and its six inputs
  are commented out: `#Lattice support not in current NWChem 5.1.1
  GDB 4/23/09`. `ai.nwchem:2108` still handles the value.
- An `ngrid` (FFT grid) panel, commented out: `# Eric doesn't use this
  to generate an input file so I'm going to comment it out for now
  until I hear back from him. GDB 4/22/09`.
- **There is no k-point widget, and never was.** `BAND` therefore has
  no way to specify a Brillouin-zone sampling from the GUI.

### 2.3 `scripts/codereg/nedruntype.py` — Car-Parrinello

One block, `if RunType == "Car-Parrinello" and Category == "NWPW"`
(lines 322-435): `ES.Runtype.CPMD.Algorithm` (Constant Energy /
Constant Temperature), `.TimeStep` (5.0), `.FakeMass` (500.0),
`.InnerIterations` (10), `.OuterIterations` (100), `.electronscaling`,
`.ionscaling`, `.electronPeriod`, `.electronTemperature`, `.ionPeriod`,
`.ionTemperature`, `.Translation`, `.Rotation`, `.Restart`. Nothing
else in `nedruntype.py` is plane-wave aware.

### 2.4 `scripts/parsers/nwch.tpl` + `ai.nwchem` — input generation

Template block (`nwch.tpl:40-56`):

```
nwpw
  ##NWPWSimulationCell##
  ##NWPW.InitialGuess##
  ##SCF.ConvergenceAlgorithm##
  mult ##Multiplicity##
  ##NWPW.SpinType##
  vectors ##InitialGuess##
  xc ##NWPW.XCFunctionals##
  cutoff ##NWPW.Cutoff##
  ewald_rcut ##NWPW.EwaldRcut##
  ewald_ncut ##NWPW.EwaldNcut##
  ##NWPWDimensions2##
  mapping ##NWPW.mapping##
  ##NWPWTolerances2##
  mulliken ##NWPW.Mulliken##
  ##NWPW_CPMD##
end
```

`ai.nwchem:2040-2069` (`removeUnusedSections`) keeps the `nwpw` block
only for `Category == "NWPW"` and deletes the `scf` block for it.

- `sub NWPWSimulationCell` (`:2104-2152`) emits `simulation_cell` with
  `boundary_conditions aperiodic` if asked, then either three
  `lattice_vectors` rows or one of `SC|FCC|BCC <L>`.
- `sub NWPW_CPMD` (`:2154-2203`) emits the `car-parrinello` sub-block:
  optional `nose-hoover`, `time_step`, `fake_mass`, `loop <inner>
  <outer>`, `scaling`, plus trailing `rotation off` / `translation off`.
- `sub NWTheoryKey` (`:208`) maps `Theory` → `pspw` or `band`;
  `sub RunType` (`:341`) maps ECCE runtypes → `energy|gradient|
  optimize|saddle|freq|property|car-parrinello`; `sub Tasks` (`:139`)
  composes `task <key> <runtype>` lines, appends a second `task <key>
  freq` for `GeoVib`, and prepends a `task <key> energy` when
  `CPMD.Restart` is set.
- **The fragment's own lattice** (`:906-914`): when
  `$useLatticeVectors` (set from the `.frag`'s `lattice_vectors:`
  section, `:1780-1784`) *and* `Category eq "NWPW"` *and*
  `NWPW.CellType eq "None"`, the geometry block gains
  `system crystal cartesian / lattice_vectors / …3 rows… / end`.
  So the Builder's cell *does* reach the deck — but only in the one
  cell mode where the theory dialog declines to override it.
- Basis: `needsBasis="false"` means `$BasisFILE` is empty, `sub basis`
  writes nothing, and `##basis##`'s line is deleted. No plane-wave code
  path consults basis data at all.

**Broken GUI→deck wiring found while reading this** (pre-existing, not
QE's problem, but worth a separate issue):

- `##NWPW.SpinType##` in the template vs `ES.Theory.NWPW.Spintype` in
  the dialog — case differs, `%AbiDict` lookup is case-sensitive, no
  `sub` of either name exists, so **the line is silently deleted and
  spin type never reaches the deck**.
- `##SCF.ConvergenceAlgorithm##` inside the `nwpw` block routes to
  `sub ConvergenceAlgorithm`, which only understands the DFT choices —
  the NWPW `CG`/`LMBFGS` selection is **discarded**.
- `vectors ##InitialGuess##` routes to `sub InitialGuess`, which only
  understands `Core Hamiltonian`/`NWChem Atomic Guess`, so that line is
  blanked; `##NWPW.InitialGuess##` emits a bare `LCAO` with no
  directive name.
- All four `Use*` gate checkboxes (`UseCutoff`, `UseRcut`, `UseNcut`,
  `UseNpDimensions`) are **never consulted** by `ai.nwchem` — zero
  matches. `mulliken None` and `ewald_rcut 0.0` can reach the deck.

### 2.5 `scripts/parsers/nwchem.desc` — the plane-wave parse entries

Nine Car-Parrinello entries (`nwchem.desc:522-583`), all
`Frequency=all`, `End=task`, no `Skip`/`Lines`, all routed to
`nwchem.cp` except one:

| Section | `Begin=` | Script |
|---|---|---|
| `[TECPVEC]` | `task_car-parrinello%begin%total energy` | `nwchem.cp` |
| `[KECPVEC]` | `…%begin%total kinetic` | `nwchem.cp` |
| `[PECPVEC]` | `…%begin%potential energy` | `nwchem.cp` |
| `[EKCPVEC]` | `…%begin%electron kinetic` | `nwchem.cp` |
| `[IKCPVEC]` | `…%begin%ion kinetic` | `nwchem.cp` |
| `[TIMECPVEC]` | `…%begin%time` | `nwchem.cp` |
| `[TGRADCPVEC]` | `…%begin%total gradient` | `nwchem.egradvec` |
| `[GRADNORMCPVEC]` | `…%begin%gradient norm` | `nwchem.cp` |
| `[OGRADNORMCPVEC]` | `…%begin%orbital gradient norm` | `nwchem.cp` |

Plus `[GEOMCAR]` (`task_car-parrinello%begin%cartesian coordinates` →
`nwchem.geomtrace`, which emits `GEOMTRACE`), and three lattice entries
(`:59-78`): `[LATVECNULL]` (`task_hessian task_energy%begin%lattice
vectors` → `nwchem.null`), `[LATVECPROP]`
(`^task_energy%begin%lattice vectors`, `Frequency=last`) and
`[LATVECTRACE]` (`task_gradient%begin%lattice vectors`,
`Frequency=all`), both → `nwchem.latvec`.

`nwchem.cp` is 58 lines and expects exactly *the tag line, then one
number*; it dispatches to a property key by regex on the tag line's own
text. `nwchem.latvec` reads the tag line for dimensions (then discards
them — "Lattice vectors are always 3x3"), reads 9 numbers, gates on
runtype against the `task_*` prefix, and emits `key: LATTICEVEC`,
`size: 1 3 3`, `units: Angstrom`.

**Why these `Begin` patterns are easy, and QE's will not be.** With
`ecce_print` on, NWChem replaces its human report with a machine-tagged
stream. Real fixture lines:

```
task_energy pspw%begin%iteration counter%1%int
10
task_energy pspw%end%iteration counter%1%int
task_car-parrinello%begin%total gradient%3 3%double
```

Every datum is fenced and self-describing in producing-task chain,
name, **dimensions** and **type**. 67 of `nwchem.desc`'s 80 `Begin=`
patterns contain the literal `begin%`; of the remaining 13, five still
match machine tags by substring, seven are bare ecce_print module
markers (`^nmr`, `^efg`, `^fermi_contact`, …) and one is a side file
(`##CalcName##.q`). **Effectively 79/80 ≈ 99% of `nwchem.desc` is
machine-tag driven and zero patterns match human prose** — unique among
the descs in `scripts/parsers/`. It is turned on by exactly one thing:
the line `ecce_print ##parseFile##` in `nwch.tpl:13`.

### 2.6 `scripts/gensub`'s `nwchem()` and `nwchem.launchpp`

`sub nwchem()` (`gensub:1027-1077`): resolve the binary from
`$::config{$::lccode}` (die if absent), emit an `$ECCE_NWCHEM`
override hook, fix `LD_LIBRARY_PATH` for a bundled build, set
`SCRATCH_DIR` from `$::scratchDir` or `$TMPDIR`, then run either a
configured `NWChemCommand` block verbatim, or serial `$nwchem $inFile`,
or `mpirun -np $totalprocs`.

**Plane-wave-specific content: none.** Zero matches for
`nwpw|pseudo|movecs|permanent` in the whole of `scripts/gensub`. No
`NWCHEM_NWPW_LIBRARY`, no pseudopotential directory, no `permanent_dir`,
no wavefunction restart-file staging. `siteconfig/submit.site`'s
`NWChemPrelimFilesToRemove` neither cleans nor protects `*.movecs`,
which the CPMD "Restart" checkbox silently depends on.

`nwchem.launchpp` (90 lines) does exactly two rewrites: replace or
prepend `scratch_dir <dir>`, and make `ecce_print`'s path absolute
against `runDir` (idempotently). Nothing plane-wave-specific; in
particular it never emits `permanent_dir`.

### 2.7 Periodicity in C++ — what exists

- `include/tdat/LatticeDef.H` / `src/tdat/chemistry/LatticeDef.C` (953
  lines). Canonical storage is **the three Cartesian lattice vectors**;
  `assign(a,b,c,α,β,γ)` converts in, `getLattice(...)` re-derives out.
  Also reciprocal vectors, `toLines()` (12 cell edges),
  `getPlaneNormal(h,k,l)`, `getLatticePlane`, `transformCell`,
  `getReplicationFactors`/`setReplicationFactors`, centring on the
  fragment centroid. **Absent from the class:** space group, symmetry
  operators, centring type, periodicity dimensionality (always a 3-D
  box — no 2-D slab or 1-D wire), Bravais-lattice enum,
  fractional-coordinate model.
- `Fragment` owns a nullable `LatticeDef *p_lattice`
  (`Fragment.H:114/217/333`); every ctor inits it to 0, so non-periodic
  is the default and periodicity is opt-in. Deep-copied by the copy
  ctor. **Space group is smuggled into `Fragment::p_pointGroup`**, the
  same string field used for molecular point groups, and disambiguated
  by `PointGroup::isSpaceGroup()` against
  `data/client/config/SpaceGroups` (230 names, taken from NWChem's
  `spnames.f90`). A genuine design wart.
- **Persistence works for free.** `Fragment::dumpMVM` (`:4901-4910`)
  emits `lattice_vectors:` + 3 rows and `point_group:` in the
  attributes block; `restoreMVM` (`:5485+`) reads both
  `lattice_vectors:` and a one-row `lattice:`. The datastore hook is
  `src/dsm/edsiimpl/ChemistryTask.C:109/241`, so lattices persist in
  EDSI with no lattice-specific DSM code. **Not persisted:** replication
  factors, centring flag, fragment centre.
- **The viewer really draws it.** `SGContainer.C:131-139` builds a
  `latticeswitch`/`SGLattice` pair; `SGLattice::setLattice()` triple-loops
  the replication factors and draws n₁·n₂·n₃ cell boxes, colouring a1
  red, a2 green, a3 blue. `SGContainer::touchLattice()` (`:1841-1878`,
  14 call sites) recentres and forces the rebuild.
- **The Builder can build a crystal.** `src/apps/builder/PBC.C` +
  `PBCGUI.C`, registered as the "Periodic Builder" tool panel
  (`Builder.C:252`, `:919`, not shown by default). Widgets: Create
  Lattice; Cell Type (`Lattice` | `Lattice Vectors`); nine vector
  fields; six a/b/c/α/β/γ fields; Lock / Center Lattice / Show
  Lattice; Fold, Delete Lattice, Equiv. Rectangle, Replicate, Unit
  Cell, Super Lattice; a 230-entry space-group combo plus **Generate**,
  which shells out via `SymmetryOps::generateLatticeFragment()` to the
  Fortran `./genmollat` to expand the asymmetric unit; and three
  replication spin controls (1..100) with a Uniform lock. A "Slicer"
  panel (`Slice.C`) is enabled whenever a lattice exists.
- **Variable-cell traces are already wired.** `GTStepCmd.C:57-91` reads
  `LATTICEVEC` (`PropTSVecTable<Geometry Step,Coordinate,Coordinate>`,
  `data/client/config/properties:181`) and calls `lattice->assign(a1,
  a2,a3)` + `sg->touchLattice()` per step;
  `GeomTracePropertyPanel::processStep()` un-replicates, steps, folds
  and re-replicates around it.

### 2.8 What nwpw support does **not** do — state this plainly

1. **No k-points.** No widget, no input generation, no parse entry.
   `BAND` runs are Γ-only. (Verified: zero `brillouin|monkhorst|kpoint`
   matches under `scripts/` and `data/client/`.)
2. **No pseudopotential involvement of any kind.** NWChem generates its
   own from `$NWCHEM_NWPW_LIBRARY`, which ECCE never sets.
3. **No a/b/c/α/β/γ cell entry in the calculation editor** (disabled
   since 2009), and the editor's cell inputs are a **second,
   unsynchronized** place to enter a cell alongside the Builder's
   `LatticeDef`.
4. **No lattice parse route for Car-Parrinello.** The CP fixture
   contains five `task_car-parrinello%begin%lattice vectors` blocks and
   `expected/nwchem-cp.txt` contains no `LATTICEVEC` — no `.desc` entry
   matches them, which makes `nwchem.latvec`'s CP branch dead code.
5. **No `task_pspw`- or `task_band`-specific parse entries at all**; the
   plane-wave path is picked up only via generic `task_energy`/
   `task_gradient` tags.
6. **No plane-wave restart/wavefunction file management** —
   `.movecs`/`.emovecs` are absent from `<DataFiles>`, from `gensub`
   and from `submit.site`'s keep/remove lists; no `permanent_dir` is
   ever emitted.
7. **No plane-wave property panel.** `PropertyPanelDescriptor.xml` has
   **zero** lattice references; `LATTICEVEC` has no viewer and is
   consumed only programmatically. There is no cell-parameter,
   cell-volume, stress or pressure property in
   `data/client/config/properties`.
8. `GTStepCmd` silently discards per-step lattice vectors if the
   fragment has no `LatticeDef` (`//TODO … create one?`, `:71`) and
   publishes no `LatticeChange`, so the PBC and Slicer panels go stale
   during playback.
9. `SGLattice::GLRender`'s axis labels are broken — literal `"b"/"c"/
   "d"` from an admitted off-by-one, with the font display lists
   `#if 000`-disabled, so they render garbage or nothing.
10. Replication materializes **real atoms** and re-perceives all bonds
    (`PBCReplicateCmd`, not undoable): a 5×5×5 replication of a
    100-atom cell creates 12,500 `TAtm`s. No instanced periodic-image
    rendering.
11. `PBCCreateCmd` hardcodes a 5 Å cube regardless of molecule size
    ("Totally brain dead command…").
12. **No crystal-structure importers.** No CIF, no POSCAR/VASP, and
    `Fragment::restorePDB` does not parse `CRYST1` (nor does `dumpPDB`
    emit it) — reading a periodic PDB loses the cell. Only ECCE's own
    MVM and BIOSYM `.car` carry a lattice. This is the biggest
    practical barrier to getting *real* crystals into ECCE, and it is
    not QE-specific.

### 2.9 The one precedent worth copying: `MetaDyn.edml`

`data/client/cap/MetaDyn.edml` registers NWChem plane-wave
metadynamics as its **own code**: `category="NWPW" name="PSPW"` with
the single runtype `Car-Parrinello`, its own dialogs
(`metathry.py`/`metartyp.py`, 270+195 lines), its own input generator
(`pw.metadyn`), its own `metadyn.desc` — while **reusing `nwch.tpl`**
and `nwchem.launchpp`. This is structurally the closest existing thing
to what QE needs: a plane-wave-only code registration with dedicated
dialogs. Read it before writing `QuantumESPRESSO.edml`.

---

## 3. Gap analysis

### 3.0 Summary table

| Concern | Carries over as-is | Needs extending | Must be built from nothing | nwpw does it, QE doesn't need it |
|---|---|---|---|---|
| `.edml` registration | ✔ (MOPAC/MetaDyn as models) | | | |
| `ResourceDescriptor.xml` **and** `…Rxn.xml` | ✔ (mechanical) | | | |
| Apache MIME (`httpd.conf.ecce` `AddType`) | ✔ | | | |
| `gensub` per-code sub | ✔ pattern | multi-executable chain (§3.2) | | |
| `<LaunchPreprocessor>` | ✔ (required unconditionally) | `outdir`/`pseudo_dir` rewrite | | |
| `.tpl` + `ai.<code>` engine | ✔ — namelists are expressible | | | |
| Basis-set machinery | ✔ turn off via `needsBasis="false"` | | | ✔ irrelevant |
| **Pseudopotential selection** | | | **✔ entire subsystem (§3.1)** | |
| **k-points / Brillouin zone** | | | **✔ (ECCE has none at all)** | |
| Cell definition (Builder `PBC`) | ✔ — a/b/c/α/β/γ, vectors, 230 space groups, replicate, fold, slice | fractional coords; `ibrav` mapping | | |
| Cell → deck (`.frag` `lattice_vectors:`) | ✔ — `ai.nwchem:906` is the pattern | | | |
| Cell rendering (`SGLattice`) | ✔ | broken axis labels (#§2.8.9) | | |
| `LATTICEVEC` per-step cell | ✔ — `GTStepCmd` already assigns it | emit it for QE; null-lattice guard | | |
| `.desc` Begin/Skip/End model | ✔ mechanism | | **✔ every pattern, from prose (§3.3)** | |
| Plane-wave parse scripts (`nwchem.cp`, `.latvec`, `.geomtrace`, `.egradvec`) | output protocol only | | **✔ QE equivalents** | |
| Property keys (`TE`, `GEOMTRACE`, `EGRADVEC`, `LATTICEVEC`, `VIB*`, `TECPVEC` family) | ✔ reuse, do not invent | | | |
| Stress / pressure property | | | ✔ new `properties` entry if wanted | |
| `ecce_print` machine-tagged stream | | | | ✔ **QE has no equivalent** |
| Car-Parrinello fictitious-mass dynamics | | | | ✔ (QE's `cp.x` is a separate code; out of scope) |
| `np_dimensions`, `mapping`, Ewald `rcut`/`ncut` | | | | ✔ NWChem-specific |
| `.movecs` restart files | | | ✔ QE's `outdir` equivalent | |
| Phonons | | | **✔ `ph.x` + `dynmat.x` chain (§3.2)** | |

### 3.1 Pseudopotentials — the one genuinely new subsystem

QE's requirement is concrete and unavoidable: `ATOMIC_SPECIES` names a
UPF file per element, resolved against `&CONTROL pseudo_dir`:

```
ATOMIC_SPECIES
  Si  28.086  Si.pbe-n-rrkjus_psl.1.0.0.UPF
  O   15.999  O.pbe-n-kjpaw_psl.1.0.0.UPF
```

What ECCE has that is *structurally* analogous, and what it lacks:

| ECCE basis-set library | What QE needs |
|---|---|
| 918 files under `data/admin/basissets`, served from the per-user WebDAV dataserver | A directory of UPF files, either on the *compute* host (`pseudo_dir`) or staged into the run dir |
| `MASTER.TBL` — a per-basis element×row capability matrix (`MAXL=`, `MXC1..3=`) | A per-PP-family element coverage table (which elements a family covers, at what cutoff) |
| Content model: Gaussian exponents and contraction coefficients | Content model: an opaque binary-ish XML/text file ECCE must never parse |
| Per-element assignment through the Basis Set Tool, written to a `.basis` file | Per-element assignment of a *filename* |
| `wr<Code>GBS.pm` translation to each code's syntax | No translation — the filename goes through verbatim |
| Gated off wholesale by `needsBasis="false"` | Needs the gate **off** for basis and **on** for something new |

Honest assessment: the *concept* transfers (per-element selection from a
library, validated against the elements present), but **no code
transfers**. The `.basis` file, `rdStandardGBS.pm`, the `wr*GBS.pm`
writers, the Basis Set Tool's C++ (`src/apps/basistool`) and
`CodeFactory::getBasisCodes()` are all about Gaussian primitives and
are the wrong shape. Note also that the "PP" basis sets already in
the library (`CC-PVDZ-PP.BAS` etc.) are *effective-core-potential
Gaussian basis sets* with no ECP block in the file at all — they are
not a precedent.

Four options, in increasing cost, for §8.Q3 to choose between:

1. **Type the filenames.** A small grid in the theory dialog: one row
   per distinct element in the fragment, one text field for the UPF
   filename, plus one field for `pseudo_dir`. Entirely Python, no C++,
   no library, no packaging. Ugly and unvalidated, but it makes QE
   *work* and it is honest about who owns the PP choice. **Recommended
   for phase 1.**
2. **A naming-convention picker.** Ship a small table of
   family→filename patterns (SSSP efficiency/precision, pslibrary,
   GBRV) and a functional/cutoff hint per element, and synthesize the
   filename. Cheap, but silently wrong whenever the installed set
   differs from the table — exactly the failure shape this codebase
   keeps getting bitten by.
3. **Read the real `pseudo_dir` at edit time.** Have the theory dialog
   list what is actually installed. This is the correct answer and the
   expensive one: the dialogs are local Python processes launched by
   `calced` and `pseudo_dir` lives on a *remote* compute host, so this
   needs a remote directory listing through the existing comm layer
   that the Python dialogs have no access to today.
4. **A real PP library subsystem** mirroring the basis-set library:
   ship UPFs in the `.deb`, serve them from the dataserver, add a
   per-element assignment tool, stage the chosen files into the run
   directory at launch. This is a multi-week project of its own, it
   makes the package much larger, and UPF licensing/provenance would
   need checking. Do not start here.

### 3.2 `ph.x` — the second-executable problem

QE's phonon workflow (from `INPUT_PH`): `ph.x` requires "the same
`outdir`/`prefix` as specified in the calculation of the unperturbed
system", i.e. a **completed `pw.x` scf run must precede it**, and
extracting Γ-point frequencies and normal modes needs a third pass
(`dynmat.x`). Three executables, one logical calculation.

Where ECCE's model bends and where it breaks:

- **Launch: bends, fine.** `gensub` generates one csh submit script per
  calculation and each code's sub emits whatever it likes into it. A
  `quantumespresso()` sub can emit `pw.x`, then `ph.x`, then
  `dynmat.x` sequentially. No framework change.
- **Machine registration: breaks.** `$::config{$::lccode}` is **one
  path per code**; `siteconfig/CONFIG.*` has one `NWChem: /path/...`
  line. Options: (a) derive `ph.x`/`dynmat.x` as siblings of the
  registered `pw.x` path — pragmatic, works for every normal QE
  install, and is the recommendation; (b) add `<code>Environment`
  entries per site (already supported by `gensub`'s `doEnvironment()`);
  (c) register a second code. (a) is one line of Perl.
- **Input files: breaks mildly.** `<DataFiles>` allows several `<Input>`
  entries but exactly one `type="primary"`. A vibration run needs
  three input decks. Cleanest resolution: keep `qe.pwin` primary and
  have `ai.qe` also write `qe.phin`/`qe.dmin` as non-primary inputs (or
  have the launch preprocessor generate them, which is where
  launch-time-only knowledge belongs anyway — see `nwchem.launchpp`'s
  contract).
- **Parsing: breaks properly.** One parse stream, one `.desc`. The
  three executables write three separate outputs. Either concatenate
  them into one declared output file in the submit script (simple,
  works with `Frequency`/`Begin`/`End`, and is effectively what
  `gensub`'s existing "append the ECCE log to the output file" tail
  already does), or declare `<Output type="parse">` on the concatenated
  file only. Concatenation is the pragmatic answer; note the ordering
  hazard — `dynmat.x`'s frequency table and `pw.x`'s own output must
  not produce colliding `Begin` patterns (see `CLAUDE.md` on
  `Begin`-as-hash-key and `PDMatchBegin()` starvation).

**Recommendation: leave `ph.x` out of phase 1 entirely** and ship QE
with no `Vibration`/`GeoVib` runtype, exactly as MOPAC shipped with no
transition-state runtype. Say so in the `.edml` comment.

### 3.3 Human-readable output — what this actually costs

QE prints a conventional human report. Exact line shapes (from
published teaching material, **not** verified against a binary here):

```
!    total energy              =     -15.87574986 Ry
     Forces acting on atoms (cartesian axes, Ry/au):
     atom    1 type  1   force =     0.00000620    0.00000000    0.00002841
     Total force =     0.024421     Total SCF correction =     0.000247
          total   stress  (Ry/bohr**3)                   (kbar)     P=   -0.06
     bfgs converged in   4 scf cycles and   3 bfgs steps
     End of BFGS Geometry Optimization
     Final energy   =     -15.8757498591 Ry
ATOMIC_POSITIONS (angstrom)
C       -0.000016545   0.000000000   0.000113593
Begin final coordinates
CELL_PARAMETERS (alat= ...)
End final coordinates
```

This is *well suited* to the Begin/Skip/End model — arguably better
than ORCA's, because `vc-relax` even brackets its final structure with
literal `Begin final coordinates` / `End final coordinates` markers.
The cost is not difficulty, it is **the absence of any safety net**:
NWChem's tags carry their own dimensions and types, so a `nwchem.*`
script can assert its shape; a QE script has to count lines and trust a
banner. Every hazard in `CLAUDE.md`'s pitfall list applies at full
force:

- `Skip=N` counts the `Begin` line itself (ORCA's `GEOMTRACE`/`VIBFREQ`
  bug). QE's forces block has a blank line after the banner — a
  guaranteed off-by-one trap.
- `End=` consumes its own matching line, so an `End` landing on the
  next entry's `Begin` starves it forever (ORCA `VIBFREQ`/`VIB`,
  `SHIELDTENSOR`/`ISOSHIELD`). QE's output is densely sectioned;
  `Forces acting on atoms` is immediately followed by `Total force =`
  which a naive `End` would eat.
- A `Begin` that is also the hash key: `!    total energy` appears once
  per scf cycle *and* as the converged value, exactly the "one marker
  phrase, two logical purposes" case `CLAUDE.md` says **not** to split
  into two entries. Use one combined `[TE][TEVEC]`-style entry at
  `Frequency=all` and emit both keys every invocation. And per the
  MOPAC lesson, keep `len(TEVEC) <= len(GEOMTRACE)` or
  `GeomTracePropertyPanel`'s click handler walks off the end of
  `PropTSVecTable`.
- `relax` vs `vc-relax` vs `scf` print different banners for the same
  quantity (`Final energy` vs `!    total energy`), which is precisely
  the MOPAC `FINAL HEAT OF FORMATION` / `HEAT OF FORMATION` trap. **Run
  every runtype's output through `tests/parsers/`, not just the richest
  one.**

### 3.4 Can ECCE define a QE crystal? — yes, with two mapping decisions

The Builder gives a cell and a space group. QE needs, in `&SYSTEM`,
either `ibrav != 0` + `celldm(1..6)` / `A,B,C,cosAB,...`, or
`ibrav = 0` + a `CELL_PARAMETERS` card. Two decisions:

1. **Always emit `ibrav = 0` + `CELL_PARAMETERS angstrom` with the
   three vectors straight out of `LatticeDef::toVectors()`.** This is
   the right call: `LatticeDef`'s canonical storage *is* the three
   Cartesian vectors, the `.frag` already carries exactly that
   (`lattice_vectors:`), and `ai.nwchem:906-914` is a working precedent
   for reading it. Attempting `ibrav` inference would require a Bravais
   classifier ECCE does not have, and would need the axis conventions
   to match QE's — and `Fragment::dumpCAR`'s history (multiple dead
   row/column-swap blocks with contradictory comments about NWChem
   conventions) is a warning about exactly that.
2. **Coordinates:** emit `ATOMIC_POSITIONS angstrom` with the
   Cartesians ECCE already has. ECCE has **no fractional-coordinate
   model at all**, so `crystal` units are not available without new
   `LatticeDef` math (it has the reciprocal vectors, so the conversion
   is easy to add later, but do not need it for phase 1).
3. **Space group:** do **not** pass it to QE. ECCE's space-group name
   lives in `Fragment::p_pointGroup` and is already consumed by
   `genmollat` to expand the asymmetric unit into a full cell, so by
   the time a deck is generated the cell is complete and P1 is correct.
   Set `nosym`/leave QE to find its own symmetry.

---

## 4. Workflow comparison — does QE take the same steps, and the same number?

Walked side by side for the two cases that matter.

### 4.1 Fixed-cell single-point energy (nwpw PSPW Energy vs QE `calculation='scf'`)

| # | nwpw today | QE | Same? |
|---|---|---|---|
| 1 | Build/import structure in Builder | same | ✔ |
| 2 | *(optional)* open **Periodic Builder**, create lattice, enter a/b/c/α/β/γ or vectors, optionally pick space group + Generate | same, but **required** — QE has no aperiodic mode | ✚ same step, now mandatory |
| 3 | Organizer → New NWChem Calculation | New Quantum ESPRESSO Calculation | ✔ |
| 4 | Pick Theory = PSPW (category NWPW), Runtype = Energy | Theory = PW/PBE (category PW), Runtype = Energy | ✔ |
| 5 | Basis Set Tool — **greyed out** (`needsBasis="false"`) | greyed out too | ✔ |
| 6 | **Theory Details**: XC functional, cutoff, Ewald rcut/ncut, np_dimensions, mapping, Mulliken, convergence, **and the unit cell again** | XC via functional-in-PP or `input_dft`, `ecutwfc`, `ecutrho`, `occupations`/`smearing`/`degauss`, `conv_thr`, `mixing_beta`, `electron_maxstep`, `nspin` | ✔ same step, different fields |
| 7 | — | **NEW: k-points.** Monkhorst–Pack `nk1 nk2 nk3` + offsets, or Γ | ✚ **extra step** |
| 8 | — | **NEW: pseudopotential per element** + `pseudo_dir` | ✚ **extra step** |
| 9 | Runtype Details (nothing plane-wave for Energy) | nothing for scf | ✔ |
| 10 | Launcher: machine, processors, queue | same | ✔ |
| 11 | Save → input uploaded to dataserver; Launch | same | ✔ |
| 12 | `gensub` writes submit script; `nwchem.launchpp` fixes `scratch_dir` and `ecce_print` path | `qe.launchpp` fixes `outdir`, `pseudo_dir`, `nproc`-dependent parallel keywords | ✔ same step |
| 13 | `eccejobmonitor` tails `ecce.out`, matches `%begin%` tags, feeds `nwchem.*` scripts | tails `qe.pwout`, matches prose banners, feeds `qe.*` scripts | ✔ same step, riskier content |
| 14 | Results appear in Organizer / Builder property panels | same | ✔ |

**Verdict: two extra steps (k-points, pseudopotentials), one step that
changes from optional to mandatory (the cell), and one step that
becomes strictly harder to get right (parsing).** Everything else is
step-for-step identical. That is a genuinely encouraging answer.

### 4.2 Vibrational frequencies (nwpw `Vibration` vs QE)

| nwpw | QE |
|---|---|
| One deck. `task pspw freq` appended by `sub Tasks`. One binary, one output, one parse stream. | **Three decks, three binaries, three outputs.** `pw.x` scf → `ph.x` (`&INPUTPH`, `prefix`/`outdir` matching, `fildyn`) → `dynmat.x` to turn the dynamical matrix into frequencies and modes. |

This is where the workflows **genuinely diverge**, and it is the one
place QE does not fit ECCE's "one calculation = one input = one binary
= one output" model. See §3.2. In terms of *user-visible* steps it can
be made to look identical (pick Runtype = Vibration, launch); in terms
of *implementation* steps it is not. Ship without it first.

### 4.3 Variable-cell optimization — a case nwpw doesn't really have

`vc-relax` has no nwpw equivalent exposed in ECCE (NWChem's
`set nwpw:stress`/cell optimization is not wired up). But ECCE's
`LATTICEVEC` + `GTStepCmd` + `GeomTracePropertyPanel` machinery is
*already built for exactly this* and currently has no producer for it
(the CP lattice blocks aren't even matched — §2.8.4). Emitting
`LATTICEVEC` per step from QE's `vc-relax` output would light up
existing, untested-in-anger code. Attractive, low cost, and a real
capability ECCE does not have today — but expect to hit the
`GTStepCmd` null-lattice `TODO` and the missing `LatticeChange`
publish while doing it.

---

## 5. Phased implementation plan

The model is ORCA's: a deliberately **reduced-scope first cut** that
works end to end, then extend. Each phase says how to verify it and
whether that verification is headless.

### Phase 0 — Decisions (no code)

Answer §8.Q1–Q5. Nothing below should start before Q3
(pseudopotentials) is settled, because it determines the theory
dialog's shape.

### Phase 1 — Registration skeleton, `calculation='scf'` only

Scope: fixed cell, `scf` only, Γ-point or a typed Monkhorst–Pack grid,
typed UPF filenames, no relax, no vc-relax, no phonons, no MD.

Create:
- `data/client/cap/QuantumESPRESSO.edml` — one `<Theory
  category="PW" name="PW" needsBasis="false">` with `<runtype>Energy
  </runtype>` only. `<Editor theorydialog="qetheory.py"
  runtypedialog="qeruntype.py">`. `<IntegrationFiles>`: `qe.tpl`,
  `ai.qe`, `qe.desc`, `qe.launchpp` (**`<LaunchPreprocessor>` is
  mandatory even if trivial** — `Launch::postProcessInput()` has no
  empty-string check). No `<Importer>`, no
  `<BasisTranslationScript>`, no `<GaussianBasisSetRules>`. **No `--`
  inside XML comments** (it made `MOPAC.edml` non-well-formed).
- `scripts/parsers/qe.tpl`, `scripts/parsers/ai.qe` — the namelist
  writer. One Perl sub per namelist (`sub ControlNamelist`,
  `SystemNamelist`, `ElectronsNamelist`) and one per card
  (`AtomicSpecies`, `AtomicPositions`, `KPoints`, `CellParameters`).
  Read the cell from the `.frag`'s `lattice_vectors:` section, copying
  `ai.nwchem:1780-1784` + `:906-914`.
- `scripts/codereg/qetheory.py`, `qeruntype.py` — functional,
  `ecutwfc`/`ecutrho`, `occupations`/`smearing`/`degauss`, `conv_thr`,
  `mixing_beta`, `electron_maxstep`, `nspin`/`starting_magnetization`,
  k-point grid, PP filenames + `pseudo_dir`. **Memory in GB** per the
  standing preference, converted at the point closest to the deck.
- `scripts/parsers/qe.launchpp` — rewrite `outdir` and `pseudo_dir` to
  real launch-time paths; model on `nwchem.launchpp`'s
  read-`-p`-paramfile, rewrite-in-place, idempotent contract.
- `scripts/parsers/qe.desc` + parser scripts: start with `[TE]`
  (`qe.energy`), `[ETIME]`/`[CPUSEC]`, `[VERSION]`. Model on
  `orca.desc`/`mopac.desc`, never on `nwchem.desc`.
- `scripts/gensub`: `sub quantumespresso()` — note the dispatch is
  `$fct = lc($applicationType)` with hyphens stripped
  (`gensub:200-210`), so an `applicationType` of `Quantum-ESPRESSO` or
  `QuantumESPRESSO` both land on `quantumespresso()`. Emit
  `pw.x -in <inFile> > <outFile>` (or `mpirun -np N pw.x -nk ...`).

Modify:
- `data/client/config/ResourceDescriptor.xml` **and**
  `ResourceDescriptorRxn.xml` — a full `<ResourceType name="qe_es">`
  block in each **and** a `<ContainsResource name="qe_es"/>` on
  `project` in each. Missing either silently omits the code from the
  New-Calculation menu with no error. (Note the latent bug recorded in
  `CLAUDE.md`: `ResourceDescriptor.xml`'s project `<Contains>` list is
  already missing `gaussian16_es`/`gaussian09_es`/`orca_es`, masked
  only because `ResourceDescriptor.C` prefers the Rxn variant when
  `bin/dirdyed` exists. Add to both regardless.)
- `packaging/dataserver/httpd.conf.ecce` — `AddType chemical/x-qe-input
  .pwin` and `chemical/x-qe-output .pwout`. Generic `.in`/`.out` would
  fall through to `DefaultType text/plain` and break the primary-input
  lookup in a way that looks exactly like a save failure. Testing this
  needs `ecce-dataserver-stop` first, not just an app relaunch.
- An icon (`calcqe.xpm`) — ORCA needed this (#73) to avoid the generic
  `calcany.xpm`.
- **No `CMakeLists.txt` change** for files under `scripts/parsers`,
  `scripts/codereg` or `data/` — those are installed as whole
  directories.

Verification:
- **Headless:** nothing yet — `tests/parsers/` needs real QE output,
  which does not exist on this machine. Hand-write a `qe.tpl` + `ai.qe`
  smoke check by running `ai.qe` directly on a captured `.param`/`.frag`
  pair and eyeballing the deck. (There is **no** test harness for the
  generation side of any code today; see §6.)
- **Live GUI:** the code appears in the New-Calculation menu; the two
  dialogs open without crashing (watch for the wx3.2/GTK3 layout
  reentrancy and `CreateGrid()`-fires-`SELECT_CELL` hazards in
  `CLAUDE.md` — a per-element PP grid is *exactly* the shape that bit
  `NModesGUI`); Save writes the input to the dataserver (a 409/"input
  file copy to DAV failed" here is a genuinely new bug, not the fixed
  `VDoc::isCurrentVdoc()` one); a deck generates with the expected
  namelists and a `CELL_PARAMETERS` card matching what the Periodic
  Builder shows.
- **Needs a real QE install** for anything past deck generation.

### Phase 1 — DONE (2026-09-21), and three corrections to what follows

Implemented and committed. Four generated decks ran to `JOB DONE` in
real `pw.x` 6.7 (bulk Si, water/Γ, fcc Al with smearing, triplet O₂),
`qe.desc` declares six parse types and all six fire, suite at 963
checks. Corrections to the plan below, from doing it:

1. **§5 Phase 1's "Verification: headless — nothing yet" is obsolete.**
   QE is installed, so the fixtures test **generation and parsing as one
   pipeline** — a deck built by `ai.qe`, run through the real binary,
   then replayed through the monitor's algorithm. No other code's
   fixtures in this suite do that.
2. **Q3's per-element combo is not implementable in the dialog.** Codereg
   dialogs are standalone processes whose entire input is `globals.py`'s
   fixed argv; nothing tells them which elements the structure contains,
   so a per-species row cannot be built there. The resolution lives in
   `ai.qe`, which reads the `.frag` and scans the pseudopotential
   directory itself. The dialog contributes `pseudo_dir`, a type
   preference applied with fallback, and a status line. This gap blocks
   **any** future per-element UI, not just this one.
3. **No `TEVEC`/`GEOMTRACE` in phase 1, deliberately.** scf has no
   geometry steps, so a Geometry-Step-indexed vector would have nothing
   to index against — the `len(TEVEC) <= len(GEOMTRACE)` invariant in
   §7. Per-iteration energies go to `EWVEC` (`PropTSVector<Wave Step>`),
   which is the correct axis and was previously produced only by
   NWChem's `ecce_print`.

Worth knowing for phase 3+: a `!` comment is fine inside a QE namelist
but **fatal inside `ATOMIC_SPECIES`**; and an odd valence-electron count
with `occupations='fixed'`, `nspin=1` makes `pw.x` stop with "charge is
wrong: smearing is needed" seconds after launch — `ai.qe` warns at
generation time.

### Phase 2 — Parsing, against real output

Scope: get properties out of a real `scf` run.

- Capture real `pw.x` output for a small periodic system (e.g. bulk Si,
  8 Ry, 2×2×2 grid) into `tests/parsers/fixtures/qe/`.
- Write `qe.desc` entries + parser scripts for: `TE` (+ `TEVEC` from
  the per-iteration `!    total energy` lines, as one combined entry per
  §3.3), `EGRADVEC` (forces), `DELTAE`, `CPUSEC`/`ETIME`, `VERSION`.
- Add a `tests/parsers/cases.py` case per runtype.

Verification: **fully headless** — `tests/parsers/run_tests.py --case
qe-si-scf`, and `ctest -R parsers`. This is the phase where the
regression suite earns its keep; do not skip to live testing.

### Phase 3 — `relax` (ECCE `Geometry`) and geometry traces

- `&IONS` namelist, `calculation='relax'`, `ion_dynamics='bfgs'`,
  `nstep`, `forc_conv_thr`, `etot_conv_thr`.
- `qe.geomtrace` parsing `ATOMIC_POSITIONS` blocks per step (model on
  `orca.geomtrace`/`mopac.geomtrace`). Watch the `TEVEC`-length rule.
- `Gradient` runtype (`tprnfor=.true.` on an scf run).

Verification: headless via `tests/parsers/` with a real relax log;
**live** for the geometry-trace 3-D playback (and note #74 is open
there — geometry-trace 3-D view not updating — so a failure to animate
may not be QE's fault).

### Phase 4 — `vc-relax` and per-step lattice

- `&CELL` namelist, `cell_dynamics`, `press`, `cell_dofree`.
- `qe.latvec` emitting `LATTICEVEC` per step from `CELL_PARAMETERS`
  blocks, plus a final-structure entry from the
  `Begin final coordinates` / `End final coordinates` bracket.
- Optionally a new `stress`/`pressure` property in
  `data/client/config/properties` (find something close in the ~250
  existing entries rather than inventing a shape).

Verification: headless for extraction; **live** to confirm the cell
actually animates. Expect to hit `GTStepCmd`'s null-lattice `TODO`
(`:71`) — a QE-created calculation's fragment may have no `LatticeDef`
attached at load time even though `LATTICEVEC` parsed fine, in which
case the per-step cell is silently dropped.

### Phase 5 — k-point UI properly, `nscf`/`bands`

- A real k-point widget set (grid vs explicit path vs Γ), used by
  **both** QE and, as a bonus, retrofittable to NWChem's `BAND`.
- `calculation='nscf'` and `'bands'` as ECCE runtypes, plus `bands.x`
  post-processing if band plotting is wanted (a second executable
  again).

Verification: mostly live; band-structure display has no existing
property panel, so scope carefully.

### Phase 6 (optional, defer) — `ph.x`

Only after §8.Q2 is answered. Three-executable chain, concatenated
parse stream, `dynmat.x` frequency table → `VIB`/`VIBFREQ`/`VIBIR`.
Reuse the existing property keys so the Vibrational Frequencies panel
lights up — and note that panel is gated on **`VIB`**, not `VIBFREQ`
(the ORCA `End`-starvation bug), and that its Animation/Vector radio
box is **currently broken under wx3.2/GTK3** (#81, open) so animation
cannot be verified end to end regardless.

### Phase 7 (independent, high value) — a CIF reader

Not required by QE, but §2.8.12 makes it the practical bottleneck on
*using* any of this: today a real crystal structure can only be typed
in by hand or come from a BIOSYM `.car`. A CIF reader feeding
`Fragment::setLattice` + `pointGroup` + `SymmetryOps::
generateLatticeFragment` would make QE genuinely usable and would
improve the existing nwpw path at the same time. Worth its own issue.

---

## 6. Verification: what can be done headlessly, and what cannot

**Headless (`tests/parsers/`)** — read `tests/parsers/README.md`. The
harness replays a captured output file through
`eccejobmonitor_sim.py` (a faithful re-implementation of
`eccejobmonitor`'s `PDFileRead` + Begin/Skip/Lines/End/Frequency
feeding loop) into the **real** `.desc` and the **real** parser
scripts, invoked exactly as `JobParser::storeProperty` does. Needs only
python3 and perl — no build, no GUI, no server, no chemistry code. This
covers **all of phases 2-4's parsing work**, and it is the only place
the four silent-failure modes in §3.3 are visible. It requires **real
captured QE output**, which is the one external dependency.

**Not covered by any harness today:** input generation (`ai.qe` +
`qe.tpl`), `qe.launchpp`, and `gensub`. There is no test for the
generation side of *any* code. Verification there is "run the script by
hand on a captured `.param`/`.frag` and read the deck", then "run the
deck through a real binary".

**Needs live GUI work:** dialog construction and layout (and the wx3.2
hazards); the New-Calculation menu; Save/upload to the dataserver;
property panels appearing; geometry-trace and cell animation; the
Periodic Builder round-trip.

**Needs a real QE install** (not available here): everything from "does
the generated deck actually run" onward. The MOPAC integration's
practice of confirming every emitted keyword against the binary's own
keyword banner has no QE analogue, but `pw.x` does echo unrecognised
namelist variables as errors, which is nearly as good.

---

## 7. The new-code checklist, walked for QE

From `CLAUDE.md`. Marked **[irrelevant]**, **[applies]**, or **[needs
new treatment]**.

| Checklist item | For QE |
|---|---|
| Don't trust `ai.<code>`'s comments for `.frag`/`.param`/`.basis` format; verify against `ESInputController.C`'s `write_cs()`/`write_setup()`/`write_gbsconfig()` | **[applies]** — and doubly so, because QE must read the `.frag`'s `lattice_vectors:` section, which only `ai.nwchem` reads today |
| Two resource-graph files, not one (`ResourceDescriptor.xml` *and* `…Rxn.xml`), each needing a `<ResourceType>` **and** a `<ContainsResource>` | **[applies]** verbatim |
| `<DataFiles>` filenames need a distinctive extension + matching `AddType` in `httpd.conf.ecce`; test with `ecce-dataserver-stop` first | **[applies]**. MOPAC's counter-lesson (let the *code* dictate the filename) does **not** bite here: `pw.x -in <file>` accepts any name and writes to stdout, so `.pwin`/`.pwout` are free |
| `<LaunchPreprocessor>` required unconditionally | **[applies]** — and QE genuinely needs one (`outdir`, `pseudo_dir`, `-nk` from the real processor count) |
| `rdStandardGBS.pm`'s NameBasis quirks (trailing `print`, unindented library lines) | **[irrelevant]** — `needsBasis="false"`, no basis file |
| No `CMakeLists.txt install()` change needed for a new code's own files | **[applies]** — confirmed, `scripts/parsers`, `scripts/codereg` and `data/` install as whole directories |
| A `.desc` parse-type's `Begin` is also its hash key **and** its match priority; don't split one marker phrase into two entries | **[needs new treatment]** — QE's `!    total energy` is exactly the "one phrase, two purposes" case. Use one combined `Frequency=all` entry emitting both keys |
| `Skip=N` counts the `Begin` line itself | **[applies]**, and QE's blank-line-after-banner blocks make it a live hazard |
| `End=` consumes its own line → starves the next entry's `Begin` | **[applies]**, high risk given QE's dense sectioning |
| `gensub` needs a `sub <lccode>()` or the job cannot launch at all | **[applies]** — `sub quantumespresso()` |
| Test the `.desc` against a molecule with degenerate vibrations, not just water | **[deferred with `ph.x`]**, but the equivalent for QE is: test against a *metal* (smearing, `occupations='smearing'`) as well as an insulator, since the output differs |
| Run **every** runtype's output through the simulation, not just the richest | **[applies]**, strongly — `scf`/`relax`/`vc-relax` print the converged energy under three different banners |
| XML comments may not contain `--` | **[applies]** |
| Keep `len(TEVEC) <= len(GEOMTRACE)` | **[applies]** — QE prints an scf energy per *iteration*, many per geometry step, so a naive `TEVEC` will be far longer than `GEOMTRACE` and will trip `PropTSVecTable::value()`'s bounds check |
| `ResourceDescriptor.xml`'s project `<Contains>` list already misses three codes | **[applies]** — add `qe_es` to both files |
| Copy-after-run rather than symlink-before-run for output files (#86) | **[irrelevant]** unless output concatenation for `ph.x` reintroduces it |

Plus, not on the checklist because no previous code needed it:

- **Periodicity is mandatory, not optional.** A QE calculation whose
  fragment has no `LatticeDef` cannot generate a valid deck. `ai.qe`
  must fail loudly (stderr, like `ai.orca`'s memory plausibility check)
  rather than emit a deck QE will reject with a cryptic namelist error.
- **The cell must come from exactly one place.** Do not repeat nwpw's
  two-unsynchronized-cell-inputs design. Take it from the fragment.

---

## 8. Open questions / decisions needed from the maintainer

**Q1 — Which `calculation=` modes, in which order?**
Recommendation: `scf` alone in phase 1; then `relax`; then `vc-relax`;
then `nscf`/`bands`. `md`/`vc-md` probably never (QE's `cp.x` is a
separate program and ECCE's Car-Parrinello runtype is NWChem-shaped).
Confirm, and confirm whether `vc-relax` is wanted at all — it is the
most attractive because it lights up existing unused machinery, but it
is also the least-exercised code path in ECCE.

**Q2 — Attempt `ph.x` at all?**
Recommendation: **no**, not in the first several phases. It is the only
part of QE that does not fit "one calculation, one input, one binary,
one output", and the Vibrational Frequencies panel it would feed is
itself currently broken under wx3.2/GTK3 (#81). Shipping QE with no
Vibration runtype is honest and has precedent (MOPAC shipped with no
TS runtype). Decide whether that is acceptable.

**Q3 — Pseudopotentials: which of the four options in §3.1?**
This is the single decision that most shapes the work.
Recommendation: **option 1** (typed UPF filenames + a typed
`pseudo_dir`) for phase 1, with option 3 (read the real `pseudo_dir`)
as the eventual goal if QE gets real use. Option 4 (a shipped PP
library) should not be attempted without a clear demand, and would need
its own licensing review.
Sub-question: should `ai.qe` validate that a UPF filename was supplied
for every element present, and refuse otherwise? (Recommendation: yes,
loudly, on stderr.)

**Q4 — k-points: how much UI?**
Minimum viable is `K_POINTS automatic` with `nk1 nk2 nk3` + three
offsets, plus a `gamma` option. Explicit k-point lists and band paths
can wait for phase 5. Should the same widget be retrofitted onto
NWChem's `BAND` (which has been Γ-only forever)? That is a separate,
arguably more valuable fix.

**Q5 — Registration identity.**
`applicationType` (and hence the `gensub` sub name and the `_es`
resource name): `Quantum-ESPRESSO` / `quantumespresso` / `qe_es`? And
should the code be named for the *package* (Quantum ESPRESSO) or the
*executable* (PWscf)? The `ph.x` question makes this less cosmetic than
it looks: if phonons are ever added as a separate registered code,
"Quantum ESPRESSO" as the name for `pw.x` alone becomes awkward.

**Q6 — Is a CIF reader (phase 7) in scope?**
It is not required by QE but it is what makes QE *useful*, and it
improves nwpw at the same time. Worth a separate issue either way.

---

## 9. Honest conclusions

- **This is worth doing and is not as hard as it first looks**, because
  the periodic infrastructure — the part that would have been genuinely
  expensive — already exists and mostly works.
- **The pseudopotential question is the real project.** Everything else
  is a competent application of the two existing worked examples. If
  the answer to Q3 is "build a real PP library", the scale changes
  category; if it is "let the user type filenames", QE is a
  slightly-larger-than-ORCA integration.
- **Do not reuse the NWChem plane-wave parsers.** They are
  `ecce_print`-shaped. Model `qe.desc` and the `qe.*` scripts on ORCA
  and MOPAC, and write the `tests/parsers/` cases first.
- **Phonons do not fit ECCE's model.** Say so, ship without them, and
  revisit as a deliberate framework change rather than smuggling a
  three-binary chain in as if it were one calculation.
- **Two things in this document are worth fixing regardless of whether
  QE ever happens**: the broken NWPW GUI→deck wiring in §2.4 (spin
  type, convergence algorithm and initial guess are all silently
  dropped today) and the unmatched Car-Parrinello lattice blocks in
  §2.8.4. Both are the same silent-failure shape the parser regression
  suite exists to catch, and both are cheap.
