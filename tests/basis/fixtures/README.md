Hand-written `.gbs` inputs in the format `TGBSConfig::dump()` produces: a
`NameBasis` section giving each element's library basis name, then a
`NumericalBasis` section with the explicit exponents and coefficients, and
optionally an `ECP` block.

* `water_same.gbs`    every atom on one mappable basis -- the route-card case
* `water_mixed.gbs`   O on 6-31G* (mappable), H on IGLO-II (not) -- per-element
* `water_gencontr.gbs` / `water_mixed_orca.gbs`  general contraction (cc-pVDZ-style s); H on 6-31++G, which ORCA rejects (IGLO-II is now mappable in ORCA).
* `water_numonly.gbs` no `NameBasis` section at all -- everything explicit
* `water_ccpvdz_explicit.gbs` the whole cc-pVDZ for water, explicit only: the
  function count ORCA gets (24) is checked, the one fixture not trimmed
* `water_aug_ccpvdz.gbs`, `water_aug_pwcvdz.gbs`  one shared name each, to
  exercise NWChem's name blocklist (named vs. forced explicit)
* `pth_ecp.gbs`       Pt carrying an ECP beside a plain H

The numbers are real (6-31G*/O, 6-31G/H, LANL2DZ/Pt) but trimmed to a few
shells: these fixtures exercise the writers' DECISIONS, not the chemistry,
and a full basis would bury the thing being checked.
