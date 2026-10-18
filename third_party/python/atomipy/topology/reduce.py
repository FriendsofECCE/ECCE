"""
Reduction transforms: collapse a per-instance `Topology` toward representative
types. These are `Topology -> Topology` passes that run *before* a writer, so the
same `write_itp` / `write_data` then emit harmonized / type-averaged output (and
`typing.extract_types` yields few types instead of one-per-term).

This brings the legacy `write_top` capabilities — `harmonize_angles`,
`detect_bimodal`, and the `lmp()` type-averaging — into the new architecture as
an explicit, optional step, while the default Topology stays fully explicit
(per-instance). `cluster_values` is ported from the legacy `cluster_angles`.
"""
from __future__ import annotations

from typing import Dict, List, Sequence, Tuple

# Equilibrium parameter clustered/harmonized per category.
_EQUILIBRIUM = {"bond": "b0", "angle": "theta0",
                "dihedral": "phi0", "improper": "xi0"}
_ATTR = {"bond": "bonds", "angle": "angles",
         "dihedral": "dihedrals", "improper": "impropers"}


def cluster_values(values: Sequence[float], threshold: float = 30.0
                   ) -> List[Tuple[float, List[float]]]:
    """Gap-based clustering, ported verbatim from legacy `write_top.cluster_angles`.
    Returns [(mean, members), ...]: one cluster, or two if the spread exceeds
    `threshold` and the largest gap exceeds `threshold/2`."""
    if not values:
        return []
    sorted_vals = sorted(values)
    spread = sorted_vals[-1] - sorted_vals[0]
    if spread <= threshold or len(values) < 4:
        return [(sum(values) / len(values), list(values))]
    max_gap = 0.0
    split_idx = 0
    for i in range(len(sorted_vals) - 1):
        gap = sorted_vals[i + 1] - sorted_vals[i]
        if gap > max_gap:
            max_gap = gap
            split_idx = i + 1
    if max_gap > threshold / 2:
        c1 = sorted_vals[:split_idx]
        c2 = sorted_vals[split_idx:]
        return [(sum(c1) / len(c1), c1), (sum(c2) / len(c2), c2)]
    return [(sum(values) / len(values), list(values))]


def _type_key(tof: Dict[int, str], term, category: str):
    """Atom-type tuple (legacy convention: for angles, order the two end types so
    A-B-C and C-B-A share a key; for bonds, unordered pair)."""
    if category == "bond":
        return frozenset((tof.get(term.i), tof.get(term.j)))
    if category == "angle":
        t1, t2, t3 = tof.get(term.i), tof.get(term.j), tof.get(term.k)
        if t1 is not None and t3 is not None and t1 > t3:
            t1, t3 = t3, t1
        return (t1, t2, t3)
    if category in ("dihedral", "improper"):
        return (tof.get(term.i), tof.get(term.j), tof.get(term.k), tof.get(term.l))
    raise ValueError(category)


def _groups(topology, category: str):
    tof = {a.id: a.type for a in topology.atoms}
    groups: Dict = {}
    for t in getattr(topology, _ATTR[category]):
        groups.setdefault(_type_key(tof, t, category), []).append(t)
    return groups


def harmonize(topology, *, categories: Sequence[str] = ("angle",),
              detect_bimodal: bool = True, threshold: float = 30.0,
              harmonize_k: bool = False):
    """Replace each term's equilibrium value (theta0 / b0) with the **cluster
    mean** for its atom-type tuple — the legacy `harmonize_angles` semantics.
    Bimodal tuples (when `detect_bimodal`) split into two clusters (e.g. cis /
    trans) and each term is assigned to its nearest cluster mean. With
    `harmonize_k`, the force constant is also averaged within each cluster.
    Mutates and returns `topology`."""
    for category in categories:
        eq = _EQUILIBRIUM[category]
        for group in _groups(topology, category).values():
            members = [t for t in group if eq in t.params]
            if not members:
                continue
            vals = [t.params[eq] for t in members]
            clusters = (cluster_values(vals, threshold) if detect_bimodal
                        else [(sum(vals) / len(vals), vals)])
            means = [c[0] for c in clusters]
            # k averaged per cluster (optional)
            kmean: Dict[int, float] = {}
            if harmonize_k:
                buckets: Dict[int, List[float]] = {i: [] for i in range(len(means))}
                for t in members:
                    ci = min(range(len(means)), key=lambda i: abs(means[i] - t.params[eq]))
                    if "k" in t.params:
                        buckets[ci].append(t.params["k"])
                kmean = {i: (sum(v) / len(v) if v else None) for i, v in buckets.items()}
            for t in members:
                ci = min(range(len(means)), key=lambda i: abs(means[i] - t.params[eq]))
                t.params[eq] = means[ci]
                if harmonize_k and kmean.get(ci) is not None:
                    t.params["k"] = kmean[ci]
    return topology


def average_by_type(topology, *, categories: Sequence[str] = ("bond", "angle")):
    """Set **all** params of every term to the per-atom-type-tuple average (a
    single cluster, no bimodal split) — the legacy `lmp()` default. After this,
    `extract_types` yields ~one type per atom-type tuple. Mutates and returns
    `topology`."""
    for category in categories:
        for group in _groups(topology, category).values():
            keys = set()
            for t in group:
                keys.update(t.params.keys())
            for pk in keys:
                vals = [t.params[pk] for t in group if pk in t.params]
                if not vals:
                    continue
                avg = sum(vals) / len(vals)
                for t in group:
                    if pk in t.params:
                        t.params[pk] = avg
    return topology
