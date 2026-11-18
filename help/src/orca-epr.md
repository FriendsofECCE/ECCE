# ORCA EPR results

Import an ORCA output containing EPR results, or reparse the outputs of an
existing calculation. Open it in the Viewer and choose a table from the
**Properties** list, under **Magnetic resonance**.

## EPR g-tensor

**EPR g-tensor** lists the three principal g values (`g1`, `g2`, `g3`)
and the isotropic value (`g_iso`). These are dimensionless, from ORCA's
`g(tot)` row. ORCA reports principal values as the positive square roots
of the eigenvalues of g times its transpose.

## EPR Hyperfine Couplings

**EPR Hyperfine Couplings** lists the three principal values and `A_iso`
from ORCA's `A(Tot)` row for each computed nucleus. All values are in MHz,
with their printed signs and order, using ORCA's SAI spin-Hamiltonian
convention. These are total couplings, including the contributions requested
in the input.

The row labels contain ORCA's zero-based output atom index and element:
`0-N` is the first atom, nitrogen. A table may contain only selected nuclei.

## EPR Quadrupole Couplings

**EPR Quadrupole Couplings** appears when ORCA prints a nuclear quadrupole
result. Each row lists `Cq` (ORCA's e²qQ) and the three principal values
`Q1`, `Q2`, `Q3`, all in MHz. The latter are the coefficients of ORCA's
spin-Hamiltonian term I·Q·I, calculated as
`k * [-(1-eta), -(1+eta), 2]`, where `k = Cq / (4I(2I-1))`.
They are not the electric field gradient eigenvalues in atomic units.
The isotope and nuclear spin are those selected in the ORCA calculation.

## Requesting the results

For this results feature, supply the EPR requests in the ORCA input yourself.
For example, for an open-shell molecule containing N and H, place this block
after the coordinates:

```
%eprnmr
  gtensor true
  Nuclei = all N { aiso, adip, fgrad }
  Nuclei = all H { aiso, adip }
end
```

Use a separate `Nuclei` line for each element; `fgrad` requests the electric
field gradient and quadrupole coupling. A missing section produces no table.
The tables show the most recent complete EPR sections in the output.
