---
type: map
title: "ecce-contacts: coordination shells, H-bonds and the pattern key used to group ensembles (#693, #708)"
area: codereg
paths: ["scripts/ecce_contacts.py", "tests/contacts/run_tests.py"]
issues: [693, 708, 682]
---
`scripts/ecce_contacts.py` (installed as `bin/ecce-contacts`) is wx-free Python.
Input: xyz (multi-frame; `energy: <Eh> relative: <kcal>` or a leading energy in the
comment) or pdb (MODEL frames). Output text/CSV/JSON; `--group` gives one lowest-energy
representative per pattern with count; `--hbonds` lists D-H...A (and adds them to the key
in an ensemble).
- Shell cutoff r_cov(M)+r_cov(X)+0.5 A; donors N,O,F,P,S,Cl,Se,Br,I; ligand = connected
  component of the non-metal atoms (covalent bond r+r+0.4); denticity = donors from one ligand.
- Key: `Na: 4×O(H2O), 1×O2(ClO4, bidentate)`; tokens sorted, so atom order is irrelevant.
- H-bond: H...A <= 2.01 A (Builder's limit) and D-H...A >= 120 deg; the Builder's own
  angle test (>= 60 deg) accepts almost everything and was not reused.
- Not done: reading GEOMTRACE from a stored calculation, the panel (mock-up only).
