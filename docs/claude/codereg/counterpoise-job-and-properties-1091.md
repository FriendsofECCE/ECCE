---
type: map
title: "Counterpoise job: one input file, monomer decks split by gensub, energies parsed from an appended block"
area: codereg
paths: ["scripts/parsers/cpfrag.pl", "scripts/parsers/ai.ecceqm", "scripts/parsers/ai.orca", "scripts/gensub", "scripts/parsers/counterpoise.parse", "scripts/parsers/ecceqm.desc", "scripts/parsers/orca.desc", "data/client/config/properties", "src/apps/builder/KeyValuePropertyPanel.C", "tests/launch/counterpoise_test.py"]
issues: [1091]
---
Only the primary input file is staged to the run folder, so the N+1(+N) decks
travel in it: the complex deck first, each monomer deck after a `# ecce-run
<name>` line (a comment in `.qmin` and ORCA input). `cpPlan` (cpfrag.pl) reads
`fragment_states` / `atom_fragment_list` from the `.frag`, checks the charge
sum and dies with a message if there are fewer than two fragments.

gensub `cpSplit` keeps the whole file as `<input>.cp-all`, leaves the complex deck
in the input (the code's normal command runs on it) and writes
`ecce-cp-<name>.<ext>`. After the main command `cpRuns` runs those in sequence
(ORCA gets the same `%pal` line) and appends `begin counterpoise / complex E /
fragment-1 E ... / end counterpoise` to the output; it keeps the main command's
exit status unless a monomer fails. The block goes AFTER the code's normal
ending; the normal-termination check only greps for the marker.

`counterpoise.parse` is code-independent (both `.desc` files call it). Raw and
BSSE need the `-own` runs (E_X(X)); dE_CP does not. ECCE-QM monomer decks drop
`write_orbitals`, `read_orbitals` and `charges`, which belong to the complex.
