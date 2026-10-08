---
type: map
title: "ECCE-QM is a bundled code: where it is wired, and the per-code flags it introduced"
area: codereg
section: "New-code checklist (gotchas found integrating ORCA, issue #38)"
paths: ["data/client/cap/ECCE-QM.edml", "scripts/parsers/ai.ecceqm", "scripts/parsers/ecceqm.desc", "scripts/parsers/ecceqm.parse", "scripts/parsers/std2ECCEQM", "scripts/parsers/ECCE-QM.expt", "scripts/codereg/ecceqmtheory.py", "scripts/gensub", "src/tdat/resources/RefMachine.C", "src/apps/calced/CalcEd.C", "src/apps/builder/MoDiagramPanel.C", "src/dsm/xml/JCode.C", "src/qm", "tests/launch/ecceqm_test.py"]
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
  pictures are untouched. Lifting it for ECCE-QM is deleting that line.
* **MO columns** come in ECCE's canonical order (atoms, shells stably sorted by
  l), marked `MOAOORDER angular-momentum`; the m-order inside a shell is
  `ECCE-QM.edml`'s `MOOrdering` (libcint real harmonics, fitted to monomials,
  checked with `ecce-mocomp`'s c.S.c = 1 on every orbital, the same as
  trace(P S) = N; see `the-mo-diagram-needs-the-codes-mo`).
* **Restricted open-shell** output is one orbital set with occupations 2, 1, 0
  (ORCA's ROKS does the same), unrestricted has alpha and beta sets.
* **Runtype:** Energy only. `ai.ecceqm`'s `RunType()` is the one place that
  refuses the others, and the `.edml` theories list the runtypes; adding Opt
  is both, plus an `opt` keyword and gradient in the engine, plus
  `GEOMTRACE`/`TEVEC` in `ecceqm.desc`.
* **No Theory Details beyond** the DFT functional (DFT only) and an
  "unrestricted" tick; no runtype dialog; no convergence, grid, memory or
  thread settings. `ECCEQM_EXTRA` in the environment appends lines to the input
  for tests.
