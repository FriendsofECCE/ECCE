"""Bundled organic molecule library (Chemical JSON).

A curated set of ~428 small organic molecules — amino acids, nucleobases,
sugars, alcohols, fatty acids, steroids, functional-group representatives, etc.
— vendored from the Avogadro2 molecules library (BSD-3-Clause, (c) 2016
Geoffrey Hutchison, University of Pittsburgh). Every molecule is composed only
of elements GAFF / OpenFF (Sage) can parameterize, so they can feed directly
into atomipy's organic force-field pipeline.

Typical use::

    import atomipy as ap
    ap.molecule_categories()                 # ['alcohols', 'amino_acids', ...]
    ap.list_molecules('amino_acids')         # [{'name': 'L-alanine', ...}, ...]
    atoms, cell = ap.load_molecule('L-alanine')   # -> atom dicts (resname LALA)
"""
import json
import os
import re

_LIB_DIR = os.path.join(os.path.dirname(__file__), 'structures', 'molecules')
_INDEX = os.path.join(_LIB_DIR, 'index.json')
_cache = None


def library_dir():
    """Absolute path to the bundled molecule library directory."""
    return _LIB_DIR


def _load_index():
    global _cache
    if _cache is None:
        with open(_INDEX, 'r', encoding='utf-8') as f:
            _cache = json.load(f)
    return _cache


def molecule_categories():
    """Sorted list of category names (e.g. 'amino_acids', 'alcohols')."""
    return sorted(_load_index().get('categories', {}).keys())


def list_molecules(category=None):
    """List available molecules, optionally filtered to one category.

    Returns a list of dicts: {name, file, formula, natoms, category}.
    """
    cats = _load_index().get('categories', {})
    out = []
    for cat, entries in cats.items():
        if category and cat != category:
            continue
        for e in entries:
            out.append({**e, 'category': cat})
    return sorted(out, key=_display_sort_key)


_STEREO_RE = re.compile(r'^(dl|ld|d|l)-', re.IGNORECASE)
_STEREO_RANK = {'l': 0, 'dl': 1, 'ld': 1, 'd': 2}


def _display_sort_key(e):
    """Sort within a category by the base (stereo-stripped) name, then order
    enantiomers L < (achiral/racemic) < D — so e.g. L-alanine appears before
    D-alanine, and each amino acid's L form leads its D form."""
    name = str(e['name'])
    m = _STEREO_RE.match(name)
    if m:
        base = name[m.end():].lower()
        rank = _STEREO_RANK.get(m.group(1).lower(), 1)
    else:
        base, rank = name.lower(), 1
    return (e['category'], base, rank, name.lower())


def _resolve(name_or_file):
    """Resolve a molecule name or 'category/file.cjson' to an absolute path."""
    # Direct relative path within the library.
    cand = os.path.join(_LIB_DIR, name_or_file)
    if os.path.isfile(cand):
        return cand
    if not name_or_file.endswith('.cjson'):
        cand_cjson = cand + '.cjson'
        if os.path.isfile(cand_cjson):
            return cand_cjson
    # Match by molecule name (case-insensitive) across the index.
    target = os.path.splitext(os.path.basename(name_or_file))[0].lower()
    for e in list_molecules():
        if e['name'].lower() == target:
            return os.path.join(_LIB_DIR, e['file'])
    raise FileNotFoundError(
        f"molecule '{name_or_file}' not found in bundled library "
        f"({_LIB_DIR}). Use ap.list_molecules() to see available names.")


def load_molecule(name_or_file, resname=None):
    """Load a bundled molecule by name or 'category/file.cjson' path.

    Parameters
    ----------
    name_or_file : str
        Molecule name (e.g. 'L-alanine'), bare file ('L-alanine.cjson'), or
        category-qualified path ('amino_acids/L-alanine.cjson').
    resname : str, optional
        Residue name to assign (default: derived from the molecule name).

    Returns
    -------
    (atoms, Cell) : tuple
        Same return signature as ``ap.import_cjson``.
    """
    from .import_conf import cjson as _cjson
    return _cjson(_resolve(name_or_file), resname=resname)
