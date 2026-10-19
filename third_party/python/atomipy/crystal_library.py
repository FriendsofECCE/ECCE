"""Bundled inorganic crystal library (CIF).

A categorised set of ~517 inorganic crystal structures — oxides, halides,
sulfides, sulfates, carbonates, hydroxides, nitrides, silicates, zeolites,
clays, pure elements/metals, intermetallics, etc. — vendored from the Avogadro2
crystals library (public domain; zeolites from the IZA Structure Commission,
others from the Crystallography Open Database).

Many of these are NOT typeable by the built-in force fields (MINFF/CLAYFF) —
transition-metal oxides (MnO, NiO, Cr2O3), sulfides, halides, pure metals, … —
and are intended to be run with the Dummy FF (see :mod:`atomipy.dummy_mineral`).
Use atomipy's curated UC_conf presets for force-field-ready clays/zeolites.

Typical use::

    import atomipy as ap
    ap.crystal_categories()                 # ['carbonates', 'elements', 'oxides', ...]
    ap.list_crystals('oxides')              # [{'name': 'MnO-Manganosite', ...}, ...]
    atoms, cell = ap.load_crystal('MnO-Manganosite')   # symmetry-expanded atoms
"""
import json
import os

_LIB_DIR = os.path.join(os.path.dirname(__file__), 'structures', 'crystals')
_INDEX = os.path.join(_LIB_DIR, 'index.json')
_cache = None


def library_dir():
    """Absolute path to the bundled crystal library directory."""
    return _LIB_DIR


def _load_index():
    global _cache
    if _cache is None:
        with open(_INDEX, 'r', encoding='utf-8') as f:
            _cache = json.load(f)
    return _cache


def crystal_categories():
    """Sorted list of category names (e.g. 'oxides', 'halides', 'elements')."""
    return sorted(_load_index().get('categories', {}).keys())


def list_crystals(category=None):
    """List available crystals, optionally filtered to one category.

    Returns a list of dicts: {name, file, formula, mineral, elements, category}.
    """
    cats = _load_index().get('categories', {})
    out = []
    for cat, entries in cats.items():
        if category and cat != category:
            continue
        for e in entries:
            out.append({**e, 'category': cat})
    return sorted(out, key=lambda e: (e['category'], e['name'].lower()))


def _resolve(name_or_file):
    """Resolve a crystal name or 'category/file.cif' to an absolute path."""
    cand = os.path.join(_LIB_DIR, name_or_file)
    if os.path.isfile(cand):
        return cand
    if not name_or_file.endswith('.cif'):
        cand_cif = cand + '.cif'
        if os.path.isfile(cand_cif):
            return cand_cif
    target = os.path.splitext(os.path.basename(name_or_file))[0].lower()
    for e in list_crystals():
        if e['name'].lower() == target:
            return os.path.join(_LIB_DIR, e['file'])
    raise FileNotFoundError(
        f"crystal '{name_or_file}' not found in bundled library ({_LIB_DIR}). "
        f"Use ap.list_crystals() to see available names.")


def load_crystal(name_or_file, expand_symmetry=True):
    """Load a bundled crystal by name or 'category/file.cif' path.

    Parameters
    ----------
    name_or_file : str
        Crystal name (e.g. 'MnO-Manganosite'), bare file ('MnO-Manganosite.cif'),
        or category-qualified path ('oxides/MnO-Manganosite.cif').
    expand_symmetry : bool
        Apply space-group symmetry operations to fill the unit cell (default True).

    Returns
    -------
    (atoms, Cell) : tuple
        Same return signature as ``ap.import_cif`` — atoms (Cartesian Å) and the
        cell [a, b, c, alpha, beta, gamma].
    """
    from .import_conf import cif as _cif
    return _cif(_resolve(name_or_file), expand_symmetry=expand_symmetry)
