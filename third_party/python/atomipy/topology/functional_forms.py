"""
Functional-form registry + cross-engine convention conversion.

Each (category, form) declares its parameter *quantity kinds* (for unit
conversion) and which parameters are **force constants subject to the
factor-of-2 prefactor**: canonical/GROMACS harmonic terms use ½·k·(x−x₀)²,
while CHARMM and LAMMPS ``harmonic`` use K·(x−x₀)² (no ½). So going canonical→
CHARMM/LAMMPS divides the force constant by 2 (after unit conversion); the
reverse multiplies by 2. Canonical convention == GROMACS.
"""
from __future__ import annotations

from typing import Dict, Set

from . import units as U

# Backend families that keep the ½ in harmonic terms (canonical convention).
_KEEPS_HALF = {"canonical", "gromacs"}


# (category, form) -> {param_name: quantity_kind}
_PARAM_KINDS: Dict[tuple, Dict[str, str]] = {
    ("bond", "harmonic"):            {"b0": U.LENGTH, "k": U.ENERGY_PER_LENGTH2},
    ("angle", "harmonic"):           {"theta0": U.ANGLE, "k": U.ENERGY_PER_RAD2},
    # Urey-Bradley: optional 1-3 sub-term carried on the angle.
    ("angle", "urey-bradley"):       {"theta0": U.ANGLE, "k": U.ENERGY_PER_RAD2,
                                      "r0": U.LENGTH, "k_ub": U.ENERGY_PER_LENGTH2},
    ("dihedral", "periodic"):        {"phi0": U.ANGLE, "k": U.ENERGY, "n": U.DIMENSIONLESS},
    ("dihedral", "ryckaert-bellemans"): {f"c{i}": U.ENERGY for i in range(6)},
    ("dihedral", "fourier"):         {"phi0": U.ANGLE, "k": U.ENERGY, "n": U.DIMENSIONLESS},
    ("improper", "harmonic"):        {"xi0": U.ANGLE, "k": U.ENERGY_PER_RAD2},
    ("improper", "periodic"):        {"phi0": U.ANGLE, "k": U.ENERGY, "n": U.DIMENSIONLESS},
    ("pair", "lj14"):                {"sigma": U.LENGTH, "epsilon": U.ENERGY},
    ("pair", "lj14_c"):              {"c6": U.ENERGY, "c12": U.ENERGY},   # c6,c12 carry length powers; see note
    ("nonbonded", "lj"):             {"sigma": U.LENGTH, "epsilon": U.ENERGY},
}

# Force-constant params subject to the ½ prefactor.
_HALF_PARAMS: Dict[tuple, Set[str]] = {
    ("bond", "harmonic"): {"k"},
    ("angle", "harmonic"): {"k"},
    ("angle", "urey-bradley"): {"k", "k_ub"},
    ("improper", "harmonic"): {"k"},
}

# GROMACS funct codes
_GROMACS_FUNCT = {
    ("bond", "harmonic"): 1,
    ("angle", "harmonic"): 1,
    ("angle", "urey-bradley"): 5,
    ("dihedral", "periodic"): 9,          # 9 = allows multiple terms per quartet
    ("dihedral", "ryckaert-bellemans"): 3,
    ("dihedral", "fourier"): 5,
    ("improper", "harmonic"): 2,
    ("improper", "periodic"): 4,
}
_FUNCT_TO_FORM = {
    ("bond", 1): "harmonic",
    ("angle", 1): "harmonic",
    ("angle", 5): "urey-bradley",
    ("dihedral", 1): "periodic",
    ("dihedral", 9): "periodic",
    ("dihedral", 3): "ryckaert-bellemans",
    ("dihedral", 5): "fourier",
    ("improper", 2): "harmonic",
    ("improper", 4): "periodic",
}

# LAMMPS style names (for the companion input snippet)
_LAMMPS_STYLE = {
    ("bond", "harmonic"): "harmonic",
    ("angle", "harmonic"): "harmonic",
    ("angle", "urey-bradley"): "charmm",
    ("dihedral", "periodic"): "charmm",
    ("dihedral", "ryckaert-bellemans"): "opls",
    ("improper", "harmonic"): "harmonic",
    ("improper", "periodic"): "cvff",
}


def is_registered(category: str, form: str) -> bool:
    return (category, form) in _PARAM_KINDS


def param_kinds(category: str, form: str) -> Dict[str, str]:
    try:
        return _PARAM_KINDS[(category, form)]
    except KeyError:
        raise KeyError(f"Unregistered functional form: ({category!r}, {form!r})")


def half_params(category: str, form: str) -> Set[str]:
    return _HALF_PARAMS.get((category, form), set())


def gromacs_funct(category: str, form: str) -> int:
    return _GROMACS_FUNCT[(category, form)]


def form_from_funct(category: str, funct: int) -> str:
    return _FUNCT_TO_FORM[(category, int(funct))]


def lammps_style(category: str, form: str) -> str:
    return _LAMMPS_STYLE.get((category, form), "harmonic")


def to_backend(category: str, form: str, params: Dict[str, float], system) -> Dict[str, float]:
    """Convert canonical params -> backend units + prefactor convention."""
    sys = system if isinstance(system, U.UnitSystem) else U.get_system(system)
    keep_half = sys.name in _KEEPS_HALF
    kinds = param_kinds(category, form)
    halves = half_params(category, form)
    out: Dict[str, float] = {}
    for name, val in params.items():
        kind = kinds.get(name, U.DIMENSIONLESS)
        v = sys.from_canonical(float(val), kind)
        if (not keep_half) and (name in halves):
            v = v / 2.0
        out[name] = v
    return out


def from_backend(category: str, form: str, params: Dict[str, float], system) -> Dict[str, float]:
    """Inverse of :func:`to_backend` (backend units/convention -> canonical)."""
    sys = system if isinstance(system, U.UnitSystem) else U.get_system(system)
    keep_half = sys.name in _KEEPS_HALF
    kinds = param_kinds(category, form)
    halves = half_params(category, form)
    out: Dict[str, float] = {}
    for name, val in params.items():
        kind = kinds.get(name, U.DIMENSIONLESS)
        v = float(val)
        if (not keep_half) and (name in halves):
            v = v * 2.0
        out[name] = sys.to_canonical(v, kind)
    return out
