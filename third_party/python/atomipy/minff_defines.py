"""
Preprocessor defines that select a MINFF parameter set in GROMACS.

Since MINFF v1.0 the angle force constant is a define of its own, and the mineral
and the force constant are separate concerns:

* **General (GMINFF)** parameters are selected by the angle force constant alone::

      define = -DMINFF_k500 -DOPC3 -DOPC3_IOD_LM

* **Tailored (TMINFF)** parameters need two defines, the mineral and the force
  constant, both required::

      define = -DMontmorillonite -DMINFF_k500 -DOPC3 -DOPC3_IOD_LM

The old ``-DGMINFF_k500`` and ``-DMontmorillonite_k500`` no longer select anything.
A define that selects nothing is a bad failure: ``grompp`` then stops on undeclared
atomtypes and not on the define, so the error does not point at the cause. This
module builds the right defines and turns the old spellings into the new ones
(with a ``FutureWarning``), so that they cannot silently select nothing.

Do not mix this up with the *JSON block keys* of the bundled parameter files (see
:mod:`atomipy.ffparams`): the general keys were renamed in the same way
(``GMINFF_k500`` -> ``MINFF_k500``), but the tailored keys are unchanged
(``Montmorillonite_k500`` stays as it is, and selecting that block is equivalent
to passing both defines).
"""
from __future__ import annotations

import re
import warnings
from typing import Iterable, List, Optional, Sequence, Union

#: Angle force constants (kJ/mol/rad2) that the parameter sets are made for.
FORCE_CONSTANTS = ("k0", "k250", "k500", "k1500")

#: Minerals with a tailored (TMINFF) parameter set, from the #ifdef blocks of
#: ffparams/min.ff/ffnonbonded_tminff.itp. Used only to tell a mineral name from
#: any other define, so that a mineral given without an angle force constant is
#: reported rather than passed through. Add to this if a mineral is added.
MINERALS = (
    "Akdalaite", "Anatase", "Boehmite", "Brucite", "CaF2", "CaO", "Coesite",
    "Corundum", "Cristobalite", "Diaspore", "Dickite", "Forsterite",
    "Gibbsite", "Goethite", "Hectorite-F", "Hectorite-H", "Hematite",
    "Imogolite", "Kaolinite", "Lepidocrocite", "Li2O", "Maghemite",
    "Magnetite", "Montmorillonite", "Muscovite", "Nacrite", "Nontronite",
    "Periclase", "Portlandite", "Pyrophyllite", "Quartz", "Rutile", "Talc",
    "Wustite", "cis_Oct_Fe2_cis", "cis_Oct_Fe2_trans",
    "cis_Oct_Mg2cis_Fe3cis", "cis_Oct_Mg2cis_Fe3trans",
    "cis_Oct_Mg2trans_Fe3cis", "cis_Oct_Mg2trans_Fe3trans", "cis_Tet_Fe3",
    "trans_Oct_Fe2_cis", "trans_Oct_Mg2cis_Fe3cis", "trans_Tet_Fe3",
)

_K = r"k(?:0|250|500|1500)"
# <Mineral>_k500, as in the tailored JSON block keys and the old combined defines.
# The mineral may itself contain underscores and hyphens (cis_Oct_Fe2_cis, Hectorite-F).
_COMBINED = re.compile(rf"^(?P<mineral>[A-Za-z][A-Za-z0-9_-]*)_(?P<k>{_K})$")
_MINERAL = re.compile(r"^[A-Za-z][A-Za-z0-9_-]*$")
_GENERAL = re.compile(rf"^MINFF_{_K}$")
# Anything shaped like a force-constant define, so that a wrong value such as
# MINFF_k999 or Montmorillonite_k42 is reported instead of passed through.
_LOOKS_LIKE_K = re.compile(r"^(?P<stem>[A-Za-z][A-Za-z0-9_-]*?)_k(?P<n>\d+)$")

#: The default parameter set: the general MINFF at the 500 kJ/mol/rad2 angle force constant.
DEFAULT_VARIANT = "MINFF_k500"


def _warn_old(old: str, new: Sequence[str]) -> None:
    flags = " ".join(f"-D{n}" for n in new)
    warnings.warn(
        f"'{old}' is the pre-v1.0 MINFF spelling and no longer selects any parameters in "
        f"GROMACS; using {flags} instead. Use the new spelling to silence this warning.",
        FutureWarning,
        stacklevel=4,
    )


def _normalize(name: str) -> List[str]:
    """Return the defines (without ``-D``) that one variant string stands for."""
    name = name.strip()
    if name.startswith("-D"):
        name = name[2:]
    if _GENERAL.match(name):
        return [name]
    m = _COMBINED.match(name)
    if m:
        mineral, k = m.group("mineral"), m.group("k")
        if mineral == "GMINFF":  # old general spelling: GMINFF_k500
            new = [f"MINFF_{k}"]
        elif mineral == "MINFF":
            return [name]
        else:  # old tailored spelling: Montmorillonite_k500 -> two defines
            new = [mineral, f"MINFF_{k}"]
        _warn_old(name, new)
        return new
    bad = _LOOKS_LIKE_K.match(name)
    if bad and f"k{bad.group('n')}" not in FORCE_CONSTANTS:
        raise ValueError(
            f"{name!r} names the angle force constant k{bad.group('n')}, which MINFF does "
            f"not provide; the available ones are {', '.join(FORCE_CONSTANTS)}")
    return [name]  # CLAYFF_EXT, OPC3, ... are defines of their own


def minff_defines(variant: Union[str, Iterable[str], None] = DEFAULT_VARIANT,
                  mineral: Optional[str] = None) -> List[str]:
    """Return the preprocessor defines (without ``-D``) that select a MINFF parameter set.

    Parameters
    ----------
    variant : str or sequence of str, optional
        The angle force constant define of the set, e.g. ``'MINFF_k500'`` (default), or
        ``None`` for none. A sequence is taken as several defines. The pre-v1.0
        spellings ``'GMINFF_k500'`` and ``'<Mineral>_k500'`` are accepted and turned
        into the new ones, with a ``FutureWarning``.
    mineral : str, optional
        The mineral of a tailored (TMINFF) set, e.g. ``'Montmorillonite'``. Needs a
        ``variant`` that is an angle force constant define.

    Returns
    -------
    list of str
        ``['MINFF_k500']`` for a general set and ``['Montmorillonite', 'MINFF_k500']``
        for a tailored one, in the order that they are best written.

    Raises
    ------
    ValueError
        For a tailored set that lacks the force constant, for a mineral that is not a
        valid define name, or if the mineral is given twice.
    """
    if variant is None or variant == "":
        names: List[str] = []
    elif isinstance(variant, str):
        names = _normalize(variant)
    else:
        names = [d for v in variant for d in _normalize(v)]

    if mineral:
        mineral = mineral.strip()
        if not _MINERAL.match(mineral) or _COMBINED.match(mineral):
            raise ValueError(
                f"mineral={mineral!r} is not a valid MINFF mineral name (give the mineral "
                "only, for example 'Montmorillonite', and the force constant as the variant)")
        if mineral in names:
            raise ValueError(f"mineral {mineral!r} was given twice")
        if len(names) != 1 or not _GENERAL.match(names[0]):
            raise ValueError(
                "a tailored (TMINFF) parameter set needs both a mineral and an angle force "
                f"constant define such as 'MINFF_k500'; got variant={variant!r}, "
                f"mineral={mineral!r}")
        names = [mineral] + names

    # A mineral on its own selects nothing: the tailored blocks are nested inside
    # the force-constant guard, so both defines are needed. Catch it here as well as
    # in the mineral= branch above, since the mineral may arrive as the variant.
    if not any(_GENERAL.match(n) for n in names):
        stray = [n for n in names if n in MINERALS]
        if stray:
            raise ValueError(
                f"{stray[0]!r} is a tailored (TMINFF) mineral and selects nothing on its own; "
                f"it needs an angle force constant define as well, for example "
                f"minff_defines('MINFF_k500', {stray[0]!r})")

    out: List[str] = []
    for n in names:  # keep the order, drop repeats
        if n not in out:
            out.append(n)
    return out
