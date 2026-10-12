---
type: map
title: "ECCE-QM is a bundled code: where it is wired, and the per-code flags it introduced"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["data/client/cap/ECCE-QM.edml", "scripts/parsers/ai.ecceqm", "scripts/parsers/ecceqm.desc", "scripts/parsers/ecceqm.parse", "scripts/parsers/std2ECCEQM", "scripts/parsers/ECCE-QM.expt", "scripts/codereg/ecceqmtheory.py", "scripts/gensub", "src/tdat/resources/RefMachine.C", "src/apps/calced/CalcEd.C", "src/apps/launcher/WxLauncher.C", "src/apps/builder/MoDiagramPanel.C", "src/dsm/xml/JCode.C", "src/qm", "tests/launch/ecceqm_test.py"]
issues: []
---
**ECCE-QM** (program `ecce-qm`, `src/qm`, `docs/qm/README.md`) is the HF/DFT
engine shipped with ECCE. The name is written in `ECCE-QM.edml` (the file
name), `ResourceDescriptor*.xml` (`ecceqm_es`), `siteconfig/Machines`,
`RefMachine::bundledCode()`, `scripts/gensub` (`sub ecceqm`, the
normal-termination marker `end_of_output`) and `data/client/config/mimetypes`
and `httpd.conf.ecce` (`.qmin`/`.qmout`); the rest of the tree uses `ecceqm`.

How it differs from the codes before it, each a thing to look at again for the
next bundled code:

* **No registration.** `RefMachine::offersCode()` is true for the machine
  named `localhost` whatever its Machines line or CONFIG says (a MyMachines
  line shadows the site one, see `launcher-machine-list-per-code`), and
  `gensub`'s `ecceqm()` runs `$ECCE_HOME/bin/ecce-qm` unless `ECCE_ECCEQM` or
  an `ecce-qm:` line in CONFIG names another. It runs on the machine ECCE
  runs on, in local-data mode and server mode alike.
* **The basis is always written out** (`wrECCEQMGBS.pm`, a `basis_data` block),
  so the engine needs no library at run time and a basis edited in ECCE is
  honoured. Elements that need a core potential are refused by the writer and
  by the engine (`.POT` file next to the `.BAS`).
* **Fixed lists, in `.edml` attributes of `<Editor>`:** `basisSetPicks` (the
  Calculation Editor's quick-pick menu; `JCode::getBasisSetPicks()`),
  `basisSetDefault` (a new calculation starts with it, `CalcEd::
  applyDefaultBasis()`), `basisSetToolHidden` (no Basis Set Tool button). A
  basis that does not cover the molecule is reported by element name in
  `CalcEd::uncoveredElementsMessage()` before launch. The DFT functionals are
  `XC_FUNCTIONALS` in `ecceqmtheory.py` and `%XCKeyword` in `ai.ecceqm`
  (`tests/dialogs` compares the two).
* **`<SupportsMODiagram>false</SupportsMODiagram>`** (any code; default true)
  makes `MoDiagramPanel::isRelevant()` false, which drops the panel silently
  from the Properties menu. The MO list, energies, occupations and orbital
  pictures are untouched, unless the user ticks Preferences > "Show the MO
  diagram for ECCE-QM calculations" (`EcceQmMoDiagram`, default ON since
  2026-10-12, Andy "for now"; a lab where students draw the diagram turns it
  off there or site-wide with `ECCE_HIDE_MO_DIAGRAM yes`). The default lives
  in two places, GlobalPrefs.H and MoDiagramPanel::isRelevant(); keep them equal.
* **MO columns** come in ECCE's canonical order (atoms, shells stably sorted by
  l), marked `MOAOORDER angular-momentum`; the m-order inside a shell is
  `ECCE-QM.edml`'s `MOOrdering` (libcint real harmonics, fitted to monomials,
  checked with `ecce-mocomp`'s c.S.c = 1 on every orbital, the same as
  trace(P S) = N; see `the-mo-diagram-needs-the-codes-mo`).
* **Restricted open-shell** output is one orbital set with occupations 2, 1, 0
  (ORCA's ROKS does the same), unrestricted has alpha and beta sets.
* **Runtype:** Energy, Gradient, Geometry, Vibration, GeoVib (named as ORCA's, so the Calculation Editor reads the same). `ai.ecceqm`'s `RunType()` maps them
  to the engine's `runtype` keyword (and is the one place that refuses the
  rest), the `.edml` theories list them, and `ecceqm.desc` has one block per
  step (`begin geometry`, `begin gradient`, `energy_step`) for GEOMTRACE, TEVEC,
  EGRAD/EGRADVEC, all Frequency=all, while TE is the last `energy_total`.
  A frequency run adds `begin vibrations` (VIBFREQ and VIBIR from one entry,
  all 3N modes with the translations/rotations first at 0, as ORCA lists
  them), `begin normal_modes` (VIB) and the keys `zero_point_energy`,
  `thermal_energy`, `enthalpy`, `entropy_ts`, `gibbs_energy` (EZEROPT, ETHERM,
  ENTHALPY, ENTROPY, EGIBBS; one Begin each, none may match another's line),
  and `thermo_temperature`/`thermo_pressure_atm` give THERMOTEMP/THERMOPRES. No
  runtype dialog, so the engine's `maxsteps`/`opt_conv` are not set from ECCE.
* **Launch warns before a large run** (`WxLauncher::confirmQmSize()`, #248):
  Vibration/GeoVib with N^2 x nbf above 5000 (DFT) or 15000 (HF), or any run
  beyond 25 atoms or 150 functions (memory, nbf^4 bytes). Cancel or "Launch
  anyway" (the default); never a block. `tests/apps/ecceqm_size_warning_test.py`.
* **No Theory Details beyond** the DFT functional (DFT only) and an
  "unrestricted" tick; no runtype dialog; no convergence, grid, memory or
  thread settings. `ECCEQM_EXTRA` in the environment appends lines to the input
  for tests.
