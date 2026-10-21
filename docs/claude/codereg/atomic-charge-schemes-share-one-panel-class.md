---
type: map
title: "Per-atom charge schemes: one parser helper, one panel class, keys named in the descriptor"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["scripts/parsers/atomcharges.pl", "scripts/parsers/orca.desc", "scripts/parsers/gaussian-16.desc", "src/apps/builder/MullikenPanel.C", "data/client/config/PropertyPanelDescriptor.xml", "data/client/config/properties"]
issues: [272]
---
`MULLIKEN`, `LOEWDIN`, `HIRSHFELD`, `CM5`, `MBIS`, `NPA`, `APT`, `AIM` and `ESPFIT`
are all `PropVector<Atom>` in units of e. Each has its own descriptor entry
and menu title, but they are all `class MullikenPanel`: the panel reads the
key from `getDescription()->properties`, so a new per-atom scheme needs a
`.desc` entry, a `properties` line and a descriptor entry, no C++. Only
`MULLIKEN` also reads `MLKNSHELL`.

Parse scripts are three-line wrappers around `atomcharges.pl`, which picks
out atom rows by shape, so a new scheme usually needs no new regex.

What each code actually prints (checked by running ORCA 6.1.1, Gaussian 16
Rev C.01, NWChem 7.2.3):
- ORCA: Loewdin always; Hirshfeld (`! Hirshfeld`), MBIS (`! MBIS`), CHELPG
  (`! CHELPG`, parsed into `ESPFIT` only) on request; NPA only with the
  licensed NBO (`! NBO`), whose output cannot be checked in. `ESPCHARGE`
  (a table, NWChem's key) is not emitted for ORCA: it listed the same
  charges a second time under Charges. The ESP surface takes `ESPCHARGE`,
  else `ESPFIT`, else Mulliken.
- Gaussian 16: APT with `Freq`; ESP fit (label is "ESP charges" for MK,
  CHELPG and HLY alike, so one key `ESPFIT`); the Hirshfeld/CM5 table is
  printed when exactly one of `Hirshfeld` and `CM5` is named, beside any
  other Pop keywords (`Pop=(Full,CM5,MK)` and `Pop=(NPA,Hirshfeld)` print
  it, checked on Rev C.01); naming both (`Pop=(Full,Hirshfeld,CM5)`) prints
  neither. The parse entry has no Pop condition. NPA with `Pop=NPA`.
- MOPAC: `ESP` keyword (Theory Details box `ES.Theory.SCF.ESPCharges`) prints
  "ELECTROSTATIC POTENTIAL CHARGES" at four decimals, parsed into `ESPFIT`
  (`mopac.espcharge`). Checked on MOPAC 23.1.2, PM7 water: -0.5889 0.2944 0.2944.
- NWChem ESP: the Theory Details box `ES.Theory.NWChem.ESPCharges` (SCF/DFT,
  not the ESP runtype) appends `task esp`; the fit lands in `<job>.q`, which
  `nwchem.esp` (a File= entry, not the ecce_print trace) publishes as the
  `ESPCHARGE` table, not `ESPFIT`. Checked on NWChem 7.2.3, H2O HF/6-31G*:
  -0.8093 0.4043 0.4049.
- NWChem: nothing new. The `ecce_print` trace the monitor parses carries
  only Mulliken atomic/shell charges (and Car-Parrinello atomic charges);
  Lowdin and ESP charges exist only in the normal stdout, at two decimals,
  which is not what the monitor reads.
- Gaussian 16 AIM (`AIM=(CP)`): the `AIM` entry's block runs from
  "I. ATTRACTORS" (attractor to element) to the Total row of "III.
  PROPERTIES OF ATTRACTORS" (charges). Attractors are taken in printed order
  as atom order (checked on CO and an H,O,H water with unequal bonds); the
  charges sum to about -0.002, not 0, so tests must not use a `sum` check.
- Gaussian does one ESP fit per job: `Pop=(MK,CHelpG)` fits once, so the
  dialog offers a choice (`ES.Theory.ESPCharges`), not two tick boxes.

Checked end to end for NPA (G16 `Pop=NPA`, ORCA `! NBO`; the ORCA output in the
parser fixtures is a real NBO 7 run, values identical): parse golden, Props
file, Properties menu entry "Natural (NPA) Charges", per-atom table and red/blue
atom colouring all work with no C++ change. To look at any scheme headless:
`PANELS_REPO_CLIENT=1 ECCE_CAPTURE_CALC=<dir from tests/spectrum/makecalc.py>
tests/panels/capture.py --builder ... --descriptor data/client/config/PropertyPanelDescriptor.xml
--open-panel "Natural (NPA) Charges"` (makecalc writes MULLIKEN/NPA/APT/ESPFIT
when the case has them; PANELS_REPO_CLIENT is needed when the builder is newer
than /opt/ecce, else pixmap dialogs block the capture).
