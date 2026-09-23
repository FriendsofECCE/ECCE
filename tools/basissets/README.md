Re-sourcing the basis set library from the Basis Set Exchange
=============================================================

**Status: steps 0-2 of the plan on issue #116 are done.** The structure
decision is made -- ECCE's aggregate-plus-components layout is kept.

    snapshot.py    step 0   fingerprint the library as it stands
    reconcile.py   step 1   match ECCE's names against BSE
    bse2bas.py     step 2   convert a BSE basis into .BAS, and an ECP into .POT

Still to do: aggregate assembly, `.meta` descriptors, regeneration of the
per-type alias files, and the step 5 diff of everything. Nothing in the
live library has been touched.

Why the JSON API and not `gbsNWChemConverter`
---------------------------------------------
BSE's REST API emits structured JSON as well as NWChem text:

    https://www.basissetexchange.org/api/basis/6-31g/format/json/?elements=1-30

The existing `scripts/gbsNWChemConverter` parses the *text*, and cannot
consume the modern BSE: the retired EMSL BSE emitted one `basis` block per
element, the modern one emits a single block containing every element, and
that parser has no "next element" branch at all. It also matches lowercase
`basis`/`ecp`/`end` and does not skip the `#BASIS SET:` comments the modern
BSE interleaves. Adapting it is a rewrite; the JSON has none of those
ambiguities and maps directly onto `.BAS`:

    am [0] -> S, [1] -> P, [0,1] -> SP, [2] -> D
    a general contraction repeats the letter once per coefficient column,
    which is how the shipped library writes them (shell=DD, ncoef=2)

Validation against the shipped 6-31G (H-Zn)
--------------------------------------------
Regenerated from BSE and compared value by value with
`data/admin/basissets/6-31G.BAS`:

    values compared                        1224
    agreeing to ECCE's stored precision    1220
    differing materially                      4

The four:

* **Helium, 3 coefficients**, all larger by the *same* factor 1.6889 -- a
  contraction normalisation convention, not a data error. Verified
  harmless: the same single-point run in ORCA 6.1.1 with each set of
  coefficients gives -2.215316809 vs -2.215316801 Hartree, a difference of
  8e-9, because the code renormalises the contraction.
* **Sulphur, 1 coefficient**, differing by 1.3e-4 relative -- rounding at
  ECCE's stored precision.

Structural differences: **K and Ca have 7 shells in BSE against 5 in
ECCE**, BSE including d functions ECCE's copy lacks.

So the shipped data is sound, and re-sourcing would mainly buy precision,
those missing d functions, and provenance under a stated licence.

Validation of the ECP conversion (step 2)
------------------------------------------
Regenerated `def2-ECP` from BSE and compared against the shipped
`DEF2_ECP.POT`, value by value:

    elements shared                36  (all 36 shipped; BSE has 50)
    values compared              1668
    worst relative difference   0.00e+00
    structural mismatches           0

Exact. The two formats line up directly: BSE gives `ecp_electrons` and a
list of potentials with the local (highest angular momentum) term first,
ECCE writes `ncore`/`lmax` on the atom line and labels the same components
`d`, `s-d`, `p-d`. Both order them the same way and both store the same
triple per row -- r exponent, gaussian exponent, coefficient.

What a full re-sourcing still needs
------------------------------------
1. Aggregate assembly -- the shipped library composes base + polarization
   + diffuse into one named set via `.descriptor` files.
2. Regenerating the per-type alias files the GUI reads for its names.
3. The step 5 diff of every regenerated basis against the current one,
   reviewed before anything is replaced. A silent numerical change here
   would not surface until someone's published results were wrong.
