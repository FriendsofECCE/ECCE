---
type: map
title: "A Gaussian basis keyword is selectable only if the library has an entry; tools/basissets/g16_basis_dump.py makes one from the keyword"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["data/admin/basissets", "tools/basissets/g16_basis_dump.py", "tests/basisload/named_basis_check.py", "scripts/parsers/wrGaussian16GBS.pm"]
issues: [154]
---
The picker lists what `data/admin/basissets/<type>` indexes, so a name
alone (no .BAS) cannot be chosen; a keyword-only entry does not exist.
To add a set Gaussian calls by keyword:
- `g16_basis_dump.py <keyword>` runs one `gfinput` job per element, stops
  at the end of link 301 (no SCF) and writes `.BAS` and, if Gaussian
  prints one, `.POT`. Prefer BSE (`bse2bas.py`) where it carries the set.
- Add the index entry (`ECPOrbital` + an `<name>-ecp` entry in `ecp` for
  ECP sets: `GBSNameRules::pairedECP` pairs on that name), `.meta`
  files, then the `%NameToBasis` line, then run
  `named_basis_check.py --code gaussian --ecp --only "<name>"`.
- `--ecp` checks an ECP set on HI and the closed-shell atoms Zn and Hg and
  also runs the writer's own "named" deck for HI (light atom named,
  iodine explicit with its ECP) against the explicit one.
- The DFT Orbital sets (`DZVP`, `DZVP2`, `TZVP`) equal Gaussian's
  DGDZVP/DGDZVP2/DGTZVP but are deliberately not in the table: they come
  with DFT fitting bases that a route-card name would drop.
- def2-SV/TZV/QZV carry Gaussian's ECP block for Rb and heavier (g local
  term, lmax 4). It is a different decomposition from BSE's `def2-ECP`
  that `DEF2_ECP.POT` holds (lmax 3), so those sets have their own `.POT`.
