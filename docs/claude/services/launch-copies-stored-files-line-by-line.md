---
type: pitfall
title: "Launch copies the stored deck to its cache line by line and stores that copy back; a 255-byte line buffer cut decks at the first long line"
area: services
paths: ["src/comm/commxt/Launch.C", "tests/launch/long_lines_test.py"]
issues: [455, 957]
---
`Launch::validateCalculation` copies the primary input (and, for every code
but GROMACS, the auxiliary inputs and the MD Prepare PDB) from the data
server into the local launch cache with `copyStoredFile`, one line at a
time. `Launch::postProcessInput` then runs the code's launch preprocessor
and **stores the cache copy back over the calculation's deck**, whatever the
preprocessor did.

Until 2026-11 the copy was `char buf[256]; while (is->getline(buf, 255))`.
`istream::getline` with a full buffer sets failbit, so the loop did not break
the long line: it **stopped there**, dropping that line and everything after
it, and the write-back made the cut permanent. Nothing reported it. A Vina
deck for 1IEP (2267 atoms) went from 257 kB to 431 bytes, ending just before
its 10055-character `#| RECEPTOR_ATOMS=` line; a 51-atom deck, whose lines
are all short, was unaffected. Any deck with a line of 254 characters or more
was exposed. The copy now reads into a `std::string`; `launch_long_lines`
(tests/launch/long_lines_test.py) launches decks with 300- and
5000-character lines and compares both copies byte for byte.

The line copy still ends the file with a newline and turns CR LF into LF
when read in text mode; a file that must arrive byte for byte goes `whole`
(GROMACS does).

The same `char buf[N]; getline(buf, N-1)` shape stops silently wherever it
is used on a file the user wrote, and in a `while (!eof()) { if
(!getline(...)) continue; }` loop it spins forever instead, since failbit
never becomes eof. Read user files into a `std::string`.
