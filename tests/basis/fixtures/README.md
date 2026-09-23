Hand-written `.gbs` inputs in the format `TGBSConfig::dump()` produces: a
`NameBasis` section giving each element's library basis name, then a
`NumericalBasis` section with the explicit exponents and coefficients, and
optionally an `ECP` block.

* `water_same.gbs`    every atom on one mappable basis -- the route-card case
* `water_mixed.gbs`   O on 6-31G* (mappable), H on IGLO-II (not) -- per-element
* `water_numonly.gbs` no `NameBasis` section at all -- everything explicit
* `pth_ecp.gbs`       Pt carrying an ECP beside a plain H

The numbers are real (6-31G*/O, 6-31G/H, LANL2DZ/Pt) but trimmed to a few
shells: these fixtures exercise the writers' DECISIONS, not the chemistry,
and a full basis would bury the thing being checked.
