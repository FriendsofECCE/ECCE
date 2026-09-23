Re-sourcing the basis set library from the Basis Set Exchange
=============================================================

**Status: validated prototype, not yet a production re-sourcing tool.**
`bse2bas.py` converts one BSE basis into ECCE's `.BAS` format and has been
checked against the shipped library. It does **not** yet handle ECPs,
aggregate basis sets, `.meta` descriptors, or the per-type alias files
(`pople`, `correlation_consistent`, ...) that give the GUI its names.
See issue #116.

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

What a full re-sourcing still needs
------------------------------------
1. ECP handling (`ecp_potentials` in the JSON; the `lmax` reordering logic
   in `gbsNWChemConverter` is worth preserving from it).
2. Aggregate basis sets -- the shipped library composes base + polarization
   + diffuse into one named set via `.descriptor` files.
3. Regenerating the per-type alias files the GUI reads for its names.
4. A diff of every regenerated basis against the current one, of the kind
   above, reviewed before anything is replaced. A silent numerical change
   here would not surface until someone's published results were wrong.
