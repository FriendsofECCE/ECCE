"""
Rules-based oxidation-state / formal-charge guessing for atomipy.

This is a *parallel alternative* to atomipy's two existing approaches:

  * ``bond_valence`` — geometry based (bond-valence sums from interatomic
    distances + Shannon radii). Needs good coordinates.
  * ``charge_formal`` — force-field oriented (half formal charges from a single
    most-common oxidation state per element).

The method here instead applies the classic IUPAC-style oxidation-state rules
together with overall charge balance, which works directly from element
identities (for ionic crystals / minerals) and, when bond connectivity is
available, from an electronegativity bond-partitioning scheme (rigorous for
molecules). Nothing here needs accurate distances.

Two engines
-----------
1. ``ionic`` — assign reliable oxidation states to the electropositive and
   electronegative elements, then solve the remaining "variable" elements
   (C, N, S, transition metals, …) from the constraint that each charge group
   sums to its net charge (0 by default — "an inorganic lattice of metal +
   oxygen (+ H) is neutral"). Best for crystals / unit cells.

2. ``electronegativity`` — the rigorous definition: every bond's electrons go
   to the more electronegative partner. ox(atom) = Σ_bonds(±order) + formal
   charge. Needs connectivity (``atom['bonds']`` with bond orders, e.g. from a
   cjson import, or ``atom['neigh']`` assuming single bonds). Best for molecules.

``method='auto'`` (default) uses ``electronegativity`` when bond orders are
present, otherwise ``ionic``.

Rules used (ionic engine)
-------------------------
  * Group 1 (Li, Na, K, Rb, Cs)      → +1
  * Group 2 (Be, Mg, Ca, Sr, Ba)     → +2
  * Group 3 / 13 (Sc, Y, La, B, Al, Ga) → +3
  * Zn, Cd → +2 ; Ag → +1
  * Si, Ge, Ti, Zr, Hf → +4  (the user's Si=+4, Ti=+4)
  * F → −1 (always; most electronegative element)
  * O → −2 (peroxide O–O → −1 if connectivity shows it)
  * Cl, Br, I → −1 (unless bonded to O/F, when known)
  * H → +1 (or −1 in a metal hydride, when connectivity shows H bonded only to metals)
  * everything else is "variable" and solved by charge balance, preferring each
    element's most common oxidation state.

The main entry point is :func:`guess_oxidation_states`.
"""
from itertools import product

# --- Pauling electronegativities (for the bond-partition engine & exceptions) ---
PAULING_EN = {
    'H': 2.20, 'Li': 0.98, 'Be': 1.57, 'B': 2.04, 'C': 2.55, 'N': 3.04,
    'O': 3.44, 'F': 3.98, 'Na': 0.93, 'Mg': 1.31, 'Al': 1.61, 'Si': 1.90,
    'P': 2.19, 'S': 2.58, 'Cl': 3.16, 'K': 0.82, 'Ca': 1.00, 'Sc': 1.36,
    'Ti': 1.54, 'V': 1.63, 'Cr': 1.66, 'Mn': 1.55, 'Fe': 1.83, 'Co': 1.88,
    'Ni': 1.91, 'Cu': 1.90, 'Zn': 1.65, 'Ga': 1.81, 'Ge': 2.01, 'As': 2.18,
    'Se': 2.55, 'Br': 2.96, 'Rb': 0.82, 'Sr': 0.95, 'Y': 1.22, 'Zr': 1.33,
    'Nb': 1.60, 'Mo': 2.16, 'Ag': 1.93, 'Cd': 1.69, 'In': 1.78, 'Sn': 1.96,
    'Sb': 2.05, 'Te': 2.10, 'I': 2.66, 'Cs': 0.79, 'Ba': 0.89, 'La': 1.10,
    'Hf': 1.30, 'Ta': 1.50, 'W': 2.36, 'Pt': 2.28, 'Au': 2.54, 'Hg': 2.00,
    'Pb': 2.33, 'Bi': 2.02,
}

# --- Reliable single oxidation states (the "fixed" set) ---
_FIXED = {
    'Li': 1, 'Na': 1, 'K': 1, 'Rb': 1, 'Cs': 1, 'Fr': 1,        # group 1
    'Be': 2, 'Mg': 2, 'Ca': 2, 'Sr': 2, 'Ba': 2, 'Ra': 2,       # group 2
    'Sc': 3, 'Y': 3, 'La': 3, 'B': 3, 'Al': 3, 'Ga': 3,         # group 3 / 13
    'Zn': 2, 'Cd': 2, 'Ag': 1,
    'Si': 4, 'Ge': 4, 'Ti': 4, 'Zr': 4, 'Hf': 4,
    'F': -1,
}
_HALOGENS = {'Cl', 'Br', 'I'}
_METALS_FOR_HYDRIDE = set(_FIXED) | {'Fe', 'Mn', 'Cu', 'Cr', 'V', 'Co', 'Ni'}

# --- Common oxidation states for "variable" elements, most-common first ---
# Used to solve charge balance and to score candidate assignments.
_COMMON = {
    'H': [1, -1], 'C': [4, -4, 2, -2, 0], 'N': [-3, 5, 3, -2, 4, 2, 1],
    'O': [-2, -1, 0], 'S': [-2, 6, 4, 2], 'Se': [-2, 6, 4], 'Te': [-2, 4, 6],
    'P': [5, 3, -3], 'As': [3, 5, -3], 'Sb': [3, 5], 'Bi': [3, 5],
    'Cl': [-1, 1, 3, 5, 7], 'Br': [-1, 1, 5], 'I': [-1, 1, 5, 7],
    'Fe': [3, 2], 'Mn': [2, 4, 3, 7, 6], 'Cu': [2, 1], 'Cr': [3, 6, 2],
    'V': [5, 3, 4, 2], 'Co': [2, 3], 'Ni': [2, 3], 'Mo': [6, 4], 'W': [6, 4],
    'Nb': [5], 'Ta': [5], 'Sn': [4, 2], 'Pb': [2, 4], 'Au': [3, 1],
    'Pt': [2, 4], 'Hg': [2, 1], 'Ti': [4, 3], 'In': [3, 1],
}


def _norm_element(atom):
    """Best-effort element symbol from an atom dict."""
    el = (atom.get('element') or '').strip()
    if not el:
        # Fall back to the atom type, stripping digits/charges/suffixes.
        t = str(atom.get('type') or atom.get('fftype') or '').strip()
        t = t.strip('0123456789+-').split('_')[0]
        el = (t[:2] if len(t) > 1 and t[1].islower() else t[:1])
    if not el:
        return ''
    return el[0].upper() + el[1:].lower() if len(el) > 1 else el.upper()


def _neighbors(atom):
    """Return list of (neighbor_1based_index, bond_order). Prefers 'bonds'
    (with order), falls back to 'neigh' (order assumed 1)."""
    out = []
    for nb in atom.get('bonds', []) or []:
        if isinstance(nb, (tuple, list)):
            out.append((int(nb[0]), int(nb[1]) if len(nb) > 1 else 1))
        else:
            out.append((int(nb), 1))
    if not out:
        for nb in atom.get('neigh', []) or []:
            out.append((int(nb), 1))
    return out


def _has_bond_orders(atoms):
    """True if any atom carries explicit bond-order connectivity."""
    for a in atoms:
        for nb in a.get('bonds', []) or []:
            if isinstance(nb, (tuple, list)) and len(nb) > 1:
                return True
    return False


# ---------------------------------------------------------------------------
# Engine 1: electronegativity bond-partitioning (rigorous, needs connectivity)
# ---------------------------------------------------------------------------
def _electronegativity_states(atoms):
    elements = [_norm_element(a) for a in atoms]
    states = [None] * len(atoms)
    for i, atom in enumerate(atoms):
        el = elements[i]
        en_i = PAULING_EN.get(el)
        ox = 0.0
        for (j1, order) in _neighbors(atom):
            j = j1 - 1
            if not (0 <= j < len(atoms)):
                continue
            el_j = elements[j]
            en_j = PAULING_EN.get(el_j)
            if en_i is None or en_j is None:
                continue
            if el == el_j or abs(en_i - en_j) < 1e-9:
                continue  # homonuclear / equal EN → shared evenly, no change
            ox += order if en_i < en_j else -order  # less EN → oxidized (+)
        ox += float(atom.get('formal_charge', 0) or 0)
        states[i] = int(round(ox)) if abs(ox - round(ox)) < 1e-6 else ox
    return states


# ---------------------------------------------------------------------------
# Engine 2: ionic rules + charge balance
# ---------------------------------------------------------------------------
def _fixed_state(atom, atoms, elements, idx):
    """Return a reliable oxidation state for `atom`, or None if it is variable."""
    el = elements[idx]
    if el in _FIXED:
        return _FIXED[el]
    if el == 'O':
        # Peroxide: O bonded to another O → −1 (only trust if connectivity known)
        for (j1, _o) in _neighbors(atom):
            j = j1 - 1
            if 0 <= j < len(atoms) and elements[j] == 'O':
                return -1
        return -2
    if el == 'H':
        nbrs = _neighbors(atom)
        if nbrs:
            nb_els = [elements[j1 - 1] for (j1, _o) in nbrs if 0 <= j1 - 1 < len(atoms)]
            if nb_els and all(e in _METALS_FOR_HYDRIDE for e in nb_els):
                return -1  # metal hydride
        return 1
    if el in _HALOGENS:
        # Cl/Br/I default −1 unless bonded to O or F (oxoanion / interhalogen)
        for (j1, _o) in _neighbors(atom):
            j = j1 - 1
            if 0 <= j < len(atoms) and elements[j] in ('O', 'F'):
                return None  # variable, let balance solve it
        return -1
    return None


def _solve_variable(var_counts, required):
    """Pick oxidation states for variable elements so Σ state*count == required.

    var_counts: dict {element: count}. Returns {element: state} (states may be
    fractional for a single mixed-valence element) or None if unsolved.
    """
    if not var_counts:
        return {} if abs(required) < 1e-6 else None

    items = list(var_counts.items())

    # Single variable element: try uniform integer, then a mixed-valence multiset,
    # then a fractional average.
    if len(items) == 1:
        el, n = items[0]
        states = _COMMON.get(el, [_default_state(el)])
        if required % n == 0 and (required // n) in states:
            return {el: required // n}
        # Mixed valence: search small multisets of allowed states summing to required.
        allowed = sorted(set(states))
        if n <= 8:
            for combo in product(allowed, repeat=n):
                if sum(combo) == required:
                    avg = required / n
                    return {el: avg if len(set(combo)) > 1 else combo[0]}
        return {el: required / n}  # fractional average (e.g. Fe3O4 → +8/3)

    # Multiple variable elements: enumerate one common state per element and pick
    # the charge-balanced combination with the best (most-common) preference score.
    candidates = []
    choice_lists = [_COMMON.get(el, [_default_state(el)]) for el, _ in items]
    for combo in product(*choice_lists):
        total = sum(state * items[k][1] for k, state in enumerate(combo))
        if total == required:
            score = sum(_COMMON.get(items[k][0], [combo[k]]).index(state)
                        if state in _COMMON.get(items[k][0], []) else 9
                        for k, state in enumerate(combo))
            candidates.append((score, {items[k][0]: combo[k] for k in range(len(items))}))
    if candidates:
        candidates.sort(key=lambda c: c[0])
        return candidates[0][1]
    return None


def _default_state(el):
    common = _COMMON.get(el)
    return common[0] if common else 0


def _ionic_states(atoms, total_charge, verbose):
    elements = [_norm_element(a) for a in atoms]
    states = [None] * len(atoms)
    fixed_sum = 0.0
    var_idx = []
    for i, atom in enumerate(atoms):
        fs = _fixed_state(atom, atoms, elements, i)
        if fs is None:
            var_idx.append(i)
        else:
            states[i] = fs
            fixed_sum += fs

    required = total_charge - fixed_sum
    var_counts = {}
    for i in var_idx:
        var_counts[elements[i]] = var_counts.get(elements[i], 0) + 1

    solution = _solve_variable(var_counts, required)
    if solution is None:
        # Could not balance: fall back to each variable element's most common state.
        if verbose:
            print(f"[oxidation] could not balance (required={required:+g} over "
                  f"{var_counts}); using most-common states. System may be charged "
                  f"or contain an unsupported element.")
        for i in var_idx:
            states[i] = _default_state(elements[i])
    else:
        for i in var_idx:
            states[i] = solution.get(elements[i], _default_state(elements[i]))
    return states


def _group_key(atom, group_by):
    if group_by == 'molid':
        return atom.get('molid', 1)
    if group_by == 'resname':
        return str(atom.get('resname', ''))
    return 'all'


def guess_oxidation_states(atoms, total_charge=0, group_by='all', method='auto',
                           write=True, key='oxidation_state', verbose=False):
    """Guess per-atom oxidation states with rules + charge balance.

    Parameters
    ----------
    atoms : list of dict
        atomipy atoms. Needs 'element' (or a typeable 'type'); the
        electronegativity engine additionally uses 'bonds'/'neigh'.
    total_charge : float, optional
        Net charge of each charge group (default 0 — neutral lattice/molecule).
    group_by : {'all', 'molid', 'resname'}, optional
        How to partition atoms into charge groups for the ionic engine. 'all'
        (default) treats the whole input as one neutral assembly — correct for a
        unit cell or a single molecule. Use 'molid'/'resname' for multi-fragment
        neutral assemblies. (Ignored by the electronegativity engine, which is
        per-atom from connectivity.)
    method : {'auto', 'ionic', 'electronegativity'}, optional
        'auto' (default) → electronegativity if bond orders are present, else ionic.
    write : bool, optional
        If True (default) store the result in ``atom[key]``.
    key : str, optional
        Atom-dict key to write into (default 'oxidation_state').
    verbose : bool, optional
        Print a per-group balance report.

    Returns
    -------
    list of (int|float)
        Oxidation state per atom, aligned with `atoms`.

    Examples
    --------
    >>> import atomipy as ap
    >>> atoms, cell = ap.import_auto('UC_conf/Pyrophyllite.pdb')
    >>> ap.guess_oxidation_states(atoms)          # Si=+4, Al=+3, O=-2, H=+1
    >>> atoms, _ = ap.load_molecule('L-alanine')  # has bond orders → EN engine
    >>> ap.guess_oxidation_states(atoms)
    """
    if not atoms:
        return []

    use_en = method == 'electronegativity' or (method == 'auto' and _has_bond_orders(atoms))

    if use_en:
        states = _electronegativity_states(atoms)
    else:
        # Partition into charge groups and solve each independently.
        groups = {}
        for i, atom in enumerate(atoms):
            groups.setdefault(_group_key(atom, group_by), []).append(i)
        states = [None] * len(atoms)
        for gkey, idxs in groups.items():
            sub = [atoms[i] for i in idxs]
            sub_states = _ionic_states(sub, total_charge, verbose)
            for local, i in enumerate(idxs):
                states[i] = sub_states[local]

    if write:
        for atom, ox in zip(atoms, states):
            atom[key] = ox

    if verbose:
        total = sum(s for s in states if s is not None)
        engine = 'electronegativity' if use_en else f'ionic (group_by={group_by})'
        print(f"[oxidation] engine={engine}; Σ oxidation states = {total:+g}")

    return states
