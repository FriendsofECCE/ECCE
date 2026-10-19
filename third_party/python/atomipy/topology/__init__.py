"""
atomipy.topology — hub-and-spoke topology interchange.

A single per-instance-first `Topology` data model with format readers/writers
that convert to/from it. See the package modules:

  model            Topology, Atom, Bond, Angle, ... + JSON (de)serialization
  units            canonical nm/kJ/deg unit system + conversions
  functional_forms registry of bonded forms + units + the ½ prefactor handling
  typing           extract_types / expand_types (per-instance <-> typed)
  validate         semantic validation

Readers/writers live in atomipy.import_topology / atomipy.write_topology.
"""
from .model import (
    Topology, Box, Atom, AtomType, Bond, Angle, Dihedral, Improper,
    Pair, Exclusion, PairType, Molecule, Defaults, Meta, SCHEMA_VERSION,
)
from . import units, functional_forms, typing, validate, elements, reduce, forcefield
from .forcefield import ForceField
from .builder import build_topology_from_atoms

__all__ = [
    "Topology", "Box", "Atom", "AtomType", "Bond", "Angle", "Dihedral",
    "Improper", "Pair", "Exclusion", "PairType", "Molecule", "Defaults", "Meta",
    "SCHEMA_VERSION", "ForceField", "units", "functional_forms", "typing",
    "validate", "elements", "reduce", "forcefield", "build_topology_from_atoms"
]
