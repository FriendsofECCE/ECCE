---
type: pitfall
title: "QE before 7.1 ignores the HUBBARD card with a one-line warning and runs without U; ECCE has a syntax choice for it"
area: codereg
section: "Pitfalls found more than once (check for siblings)"
paths: ["scripts/parsers/ai.qe", "scripts/codereg/qetheory.py", "scripts/parsers/verifyinput", "tests/qe_features/run_tests.py"]
issues: [329]
---
Debian 13 ships pw.x 6.7. Given a deck with `HUBBARD {ortho-atomic}` /
`U Ni-3d 4.0` it prints `Warning: card HUBBARD {ORTHO-ATOMIC} ignored` and
finishes with exactly the U = 0 energy (NiO, -384.69379577 Ry both ways).
Nothing a deck checker can see tells the two apart, so Theory Details has an
"Input for:" choice: QE 7.1 or later (the `HUBBARD` card, the default) or QE
6.x to 7.0 (`lda_plus_u`, `lda_plus_u_kind = 0`, `U_projection_type`,
`Hubbard_U(i)` in `&SYSTEM`). Only the second form can be run on a machine
with the Debian package; the first has been tested as a deck only.

In the old form pw.x picks the Hubbard orbital from the element (Ni 3d,
Ce 4f, O 2p), so the dialog's orbital box only reaches the card. Orbital
occupations are printed between `--- enter write_ns ---` and `--- exit
write_ns ---`; the term of the energy is `Hubbard energy` (`EHUBBARD`).

The dialog is never told the elements, so the U rows are typed (element,
orbital, U); `ai.qe` ignores a row whose element is not in the structure,
with a note on stderr. `verifyinput` warns (UNSURE, never BAD) when a
transition metal has U and `nspin` is not 2.

pw.x 7.5 refuses the old keywords loudly ("DFT+Hubbard input syntax has
changed since v7.1"); 6.7 ignores a HUBBARD card silently. 7.x also prints
the occupations under "HUBBARD OCCUPATIONS" (6.x: "enter write_ns") and
Tkatchenko-Scheffler as plain "Dispersion Correction" (6.x: "Dispersion T-S
Correction"). tests/qe_features passes against both (a serial 7.5 build is
in /opt/QE/qe-7.5/build/bin on niobium). The form should follow the
machine's pw.x version, not a dialog choice.
