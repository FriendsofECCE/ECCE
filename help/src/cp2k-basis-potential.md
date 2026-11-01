# CP2K: basis sets and potentials

In **Theory Details** the **Basis Set and Potential**, **Library Files** and
**Per-element Basis and Potential** boxes choose the MOLOPT basis and the GTH
pseudopotential of each element. ECCE picks a basis and a potential per
element itself, because the dialog is not told which elements the structure
has: the **Functional** fixes the GTH family (PBE and revPBE use the GTH-PBE
potentials), the **Basis level** names the MOLOPT family
(`DZVP-MOLOPT-SR-GTH` and so on), and **Library Files** says where to find
them — the directory (CP2K's own data directory on the machine that runs the
job by default) and the basis and potential file names (`BASIS_MOLOPT` and
`GTH_POTENTIALS` by default).

**Per-element Basis and Potential** overrides that choice for the few
elements that need it. Each of the six rows names an element and, optionally,
its basis and potential; a blank basis or potential is picked from the files
as usual. A row can also name its own **Basis file** and **Potential file** —
a blank file means the **Library Files** above — so one element can come from
a different library. The usual case is valence-only sodium, whose `GTH-PBE-q1`
potential is in `POTENTIAL_UZH` and whose basis is in `BASIS_MOLOPT_UZH`,
while the other elements stay on `GTH_POTENTIALS` and `BASIS_MOLOPT`. The
generator writes one `BASIS_SET_FILE_NAME` line for each basis file in use
and the row's potential file inside that element's `&KIND` section.
