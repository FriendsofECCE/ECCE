---
type: map
title: "Per-atom charge schemes: one parser helper, one panel class, keys named in the descriptor"
area: codereg
section: "How a property gets from disk into the Properties menu"
paths: ["scripts/parsers/atomcharges.pl", "scripts/parsers/orca.desc", "scripts/parsers/gaussian-16.desc", "src/apps/builder/MullikenPanel.C", "data/client/config/PropertyPanelDescriptor.xml", "data/client/config/properties"]
issues: [272]
---
`MULLIKEN`, `LOEWDIN`, `HIRSHFELD`, `CM5`, `MBIS`, `NPA`, `APT` and `ESPFIT`
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
  (`! CHELPG`, feeds `ESPFIT` and the older `ESPCHARGE` table) on request;
  NPA only with the licensed NBO (`! NBO`), whose output cannot be checked in.
- Gaussian 16: APT with `Freq`; ESP fit (label is "ESP charges" for MK,
  CHELPG and HLY alike, so one key `ESPFIT`); Hirshfeld and CM5 need
  `Pop=Hirshfeld` or `Pop=CM5` on its own -- `Pop=(Full,Hirshfeld,CM5)` and
  `Pop=(...,Hirshfeld,CM5,MK)` printed nothing; NPA with `Pop=NPA`.
- NWChem: nothing new. The `ecce_print` trace the monitor parses carries
  only Mulliken atomic/shell charges (and Car-Parrinello atomic charges);
  Lowdin and ESP charges exist only in the normal stdout, at two decimals,
  which is not what the monitor reads.
