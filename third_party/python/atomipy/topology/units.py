"""
Units subsystem for the topology interchange layer.

Canonical internal units (decision §14.2): **nm, kJ·mol⁻¹, degrees, e, amu**
(GROMACS-aligned). Never Å internally. Readers convert *into* canon on load;
writers convert *out of* canon per backend.

Every numeric field carries a *quantity kind* so conversions are explicit. The
notorious **factor-of-2 prefactor** for harmonic terms (GROMACS ½·k·x² vs.
CHARMM/LAMMPS K·x²) is NOT handled here — it lives in ``functional_forms`` because
it is a property of the functional form + backend convention, not of units.
"""
from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict

# --- Quantity kinds -----------------------------------------------------------
LENGTH = "length"
ENERGY = "energy"
ENERGY_PER_LENGTH2 = "energy/length^2"   # harmonic bond force constant
ENERGY_PER_RAD2 = "energy/rad^2"         # harmonic angle force constant
ANGLE = "angle"                          # degrees in all our backends
CHARGE = "charge"                        # e everywhere
MASS = "mass"                            # amu everywhere
DIMENSIONLESS = "dimensionless"

# Base conversion constants
KJ_PER_KCAL = 4.184
KJ_PER_MOL_PER_EV = 96.48533212        # 1 eV = 96.485 kJ/mol
NM_PER_ANGSTROM = 0.1
ANGSTROM_PER_NM = 10.0


@dataclass(frozen=True)
class UnitSystem:
    """A target unit system, defined by its base length/energy factors *relative
    to canonical* (nm, kJ/mol). ``length_factor`` = (value in this system) /
    (value in nm); ``energy_factor`` likewise for kJ/mol. Composite kinds are
    derived so there is a single source of truth."""

    name: str
    length_factor: float            # canonical nm  -> system length unit
    energy_factor: float            # canonical kJ  -> system energy unit
    length_unit: str = "nm"
    energy_unit: str = "kJ/mol"

    def factor(self, kind: str) -> float:
        """Multiplicative factor to convert a canonical value of *kind* into this
        system's units."""
        if kind in (ANGLE, CHARGE, MASS, DIMENSIONLESS):
            return 1.0
        if kind == LENGTH:
            return self.length_factor
        if kind == ENERGY:
            return self.energy_factor
        if kind == ENERGY_PER_LENGTH2:
            return self.energy_factor / (self.length_factor ** 2)
        if kind == ENERGY_PER_RAD2:
            return self.energy_factor          # angle in rad both sides
        raise ValueError(f"Unknown quantity kind: {kind!r}")

    def to_canonical(self, value: float, kind: str) -> float:
        return value / self.factor(kind)

    def from_canonical(self, value: float, kind: str) -> float:
        return value * self.factor(kind)


# Canonical system (identity) and the three backends.
CANONICAL = UnitSystem("canonical", length_factor=1.0, energy_factor=1.0,
                       length_unit="nm", energy_unit="kJ/mol")
GROMACS = UnitSystem("gromacs", length_factor=1.0, energy_factor=1.0,
                     length_unit="nm", energy_unit="kJ/mol")
CHARMM = UnitSystem("charmm", length_factor=ANGSTROM_PER_NM,
                    energy_factor=1.0 / KJ_PER_KCAL,
                    length_unit="A", energy_unit="kcal/mol")
LAMMPS_REAL = UnitSystem("lammps_real", length_factor=ANGSTROM_PER_NM,
                         energy_factor=1.0 / KJ_PER_KCAL,
                         length_unit="A", energy_unit="kcal/mol")
LAMMPS_METAL = UnitSystem("lammps_metal", length_factor=ANGSTROM_PER_NM,
                          energy_factor=1.0 / KJ_PER_MOL_PER_EV,
                          length_unit="A", energy_unit="eV")

_SYSTEMS: Dict[str, UnitSystem] = {
    s.name: s for s in (CANONICAL, GROMACS, CHARMM, LAMMPS_REAL, LAMMPS_METAL)
}
# Friendly aliases
_SYSTEMS["lammps"] = LAMMPS_REAL
_SYSTEMS["real"] = LAMMPS_REAL
_SYSTEMS["metal"] = LAMMPS_METAL


def get_system(name: str) -> UnitSystem:
    key = str(name).strip().lower()
    if key not in _SYSTEMS:
        raise ValueError(f"Unknown unit system {name!r}; known: {sorted(_SYSTEMS)}")
    return _SYSTEMS[key]


def convert(value: float, kind: str, src: str | UnitSystem, dst: str | UnitSystem) -> float:
    """Convert *value* of *kind* from *src* system to *dst* system."""
    s = src if isinstance(src, UnitSystem) else get_system(src)
    d = dst if isinstance(dst, UnitSystem) else get_system(dst)
    return d.from_canonical(s.to_canonical(value, kind), kind)


# Common literal conversions for adapters / tests.
def angstrom_to_nm(x: float) -> float:
    return x * NM_PER_ANGSTROM


def nm_to_angstrom(x: float) -> float:
    return x * ANGSTROM_PER_NM
