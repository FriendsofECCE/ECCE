"""
The central `Topology` data model (decision §14.5: stdlib dataclasses, zero-dep).

Per-instance-first: every bonded term stores its own ``params`` inline; shared
type tables are *derived on demand* (see ``typing.extract_types``). Canonical
units are nm / kJ·mol⁻¹ / degrees / e / amu (see ``units``).

The on-disk JSON is just ``Topology.to_dict()`` serialized.
"""
from __future__ import annotations

import math
from dataclasses import dataclass, field, asdict
from typing import Any, Dict, List, Optional

import numpy as np

SCHEMA_VERSION = "1.0"


# ---------------------------------------------------------------------------
# Box (triclinic-capable; stored internally as a 3x3 matrix of row vectors, nm)
# ---------------------------------------------------------------------------
@dataclass
class Box:
    """Periodic cell. Internally the GROMACS-style lower-triangular box vectors
    (rows v1,v2,v3) in **nm**: v1=(lx,0,0), v2=(xy,ly,0), v3=(xz,yz,lz)."""

    matrix: List[List[float]] = field(
        default_factory=lambda: [[0.0, 0.0, 0.0]] * 3
    )

    # --- constructors -------------------------------------------------------
    @classmethod
    def from_box_dim(cls, box_dim: List[float]) -> "Box":
        """GROMACS Box_dim (nm). Accepts 3 (orthogonal lx,ly,lz) or 9
        (lx,ly,lz,0,0,xy,0,xz,yz)."""
        v = [float(x) for x in box_dim]
        if len(v) == 3:
            lx, ly, lz = v
            return cls([[lx, 0, 0], [0, ly, 0], [0, 0, lz]])
        if len(v) == 9:
            lx, ly, lz, _, _, xy, _, xz, yz = v
            return cls([[lx, 0, 0], [xy, ly, 0], [xz, yz, lz]])
        raise ValueError(f"Box_dim must have 3 or 9 entries, got {len(v)}")

    @classmethod
    def from_cell(cls, a: float, b: float, c: float,
                  alpha: float, beta: float, gamma: float) -> "Box":
        """Cell params (lengths in **nm**, angles in degrees) -> lower-triangular
        box vectors (GROMACS convention: a along x, b in xy-plane)."""
        al, be, ga = (math.radians(x) for x in (alpha, beta, gamma))
        lx = a
        xy = b * math.cos(ga)
        ly = b * math.sin(ga)
        xz = c * math.cos(be)
        if abs(math.sin(ga)) < 1e-12:
            yz = 0.0
        else:
            yz = c * (math.cos(al) - math.cos(be) * math.cos(ga)) / math.sin(ga)
        lz = math.sqrt(max(c * c - xz * xz - yz * yz, 0.0))
        return cls([[lx, 0.0, 0.0], [xy, ly, 0.0], [xz, yz, lz]])

    # --- views --------------------------------------------------------------
    def to_box_dim(self) -> List[float]:
        m = self.matrix
        return [m[0][0], m[1][1], m[2][2], 0.0, 0.0, m[1][0], 0.0, m[2][0], m[2][1]]

    def cell_params(self) -> List[float]:
        v = np.array(self.matrix, dtype=float)
        a, b, c = (float(np.linalg.norm(x)) for x in v)
        def ang(u, w):
            d = np.dot(u, w)
            n = np.linalg.norm(u) * np.linalg.norm(w)
            return math.degrees(math.acos(max(-1.0, min(1.0, d / n)))) if n else 90.0
        alpha = ang(v[1], v[2]); beta = ang(v[0], v[2]); gamma = ang(v[0], v[1])
        return [a, b, c, alpha, beta, gamma]

    def lammps_bounds(self, origin=(0.0, 0.0, 0.0)):
        """Return ((xlo,xhi),(ylo,yhi),(zlo,zhi),(xy,xz,yz)) in **nm**.
        Caller converts to the LAMMPS length unit."""
        ox, oy, oz = origin
        m = self.matrix
        lx, ly, lz = m[0][0], m[1][1], m[2][2]
        xy, xz, yz = m[1][0], m[2][0], m[2][1]
        return ((ox, ox + lx), (oy, oy + ly), (oz, oz + lz), (xy, xz, yz))

    def is_triclinic(self) -> bool:
        m = self.matrix
        return any(abs(m[i][j]) > 1e-9 for i, j in ((1, 0), (2, 0), (2, 1)))


# ---------------------------------------------------------------------------
# Atom types (optional shared table) and atoms (the sites)
# ---------------------------------------------------------------------------
@dataclass
class AtomType:
    name: str
    element: Optional[str] = None
    atomic_number: Optional[int] = None
    mass: Optional[float] = None
    charge: Optional[float] = None
    ptype: str = "A"
    lj: Dict[str, float] = field(default_factory=dict)   # {sigma,epsilon} or {c6,c12}/{A,C}
    comment: str = ""


@dataclass
class Atom:
    id: int                                  # 0-based internal index
    type: Optional[str] = None               # AtomType.name (or None in fully-explicit mode)
    site_label: Optional[str] = None         # unique per-site label, e.g. "Mgo_1"
    name: str = ""
    element: Optional[str] = None
    atomic_number: Optional[int] = None
    mass: Optional[float] = None             # per-site (may differ from type default)
    charge: Optional[float] = None           # per-site (MINFF site charges live here)
    residue_id: int = 1
    residue_name: str = ""
    molecule_id: int = 1
    charge_group: int = 1                    # GROMACS cgnr
    position: Optional[List[float]] = None   # [x,y,z] nm, optional
    extra: Dict[str, Any] = field(default_factory=dict)


# ---------------------------------------------------------------------------
# Bonded terms — per-instance core
# ---------------------------------------------------------------------------
@dataclass
class Bond:
    i: int
    j: int
    form: str = "harmonic"
    params: Dict[str, float] = field(default_factory=dict)   # inline, per-instance
    type_ref: Optional[str] = None
    comment: str = ""


@dataclass
class Angle:
    i: int
    j: int
    k: int
    form: str = "harmonic"
    params: Dict[str, float] = field(default_factory=dict)
    type_ref: Optional[str] = None
    comment: str = ""


@dataclass
class Dihedral:
    i: int
    j: int
    k: int
    l: int
    form: str = "periodic"        # periodic | ryckaert-bellemans | fourier
    params: Dict[str, float] = field(default_factory=dict)
    type_ref: Optional[str] = None
    comment: str = ""             # multiple entries per quartet are allowed


@dataclass
class Improper:
    i: int
    j: int
    k: int
    l: int
    form: str = "harmonic"        # harmonic | periodic
    params: Dict[str, float] = field(default_factory=dict)
    type_ref: Optional[str] = None
    central: int = 0              # which of i,j,k,l is the central atom (convention!)
    comment: str = ""


@dataclass
class Pair:
    i: int
    j: int
    form: str = "lj14"
    params: Dict[str, float] = field(default_factory=dict)
    comment: str = ""


@dataclass
class Exclusion:
    i: int
    excluded: List[int] = field(default_factory=list)


@dataclass
class PairType:
    """Off-diagonal nonbonded override (NBFIX / [nonbond_params] / PairIJ)."""
    i_type: str
    j_type: str
    params: Dict[str, float] = field(default_factory=dict)
    comment: str = ""


@dataclass
class Molecule:
    """Optional templating layer for GROMACS molecule re-compression on write."""
    name: str
    count: int = 1
    atom_ids: List[int] = field(default_factory=list)   # ids of the first instance


@dataclass
class Defaults:
    nb_func: int = 1            # 1 = LJ
    comb_rule: int = 2         # 2 = sigma/epsilon, lorentz-berthelot
    gen_pairs: str = "yes"
    fudgeLJ: float = 1.0
    fudgeQQ: float = 1.0


@dataclass
class Meta:
    name: str = ""
    source_format: str = ""
    source_file: str = ""
    generator: str = "atomipy.topology"
    comment: str = ""


# ---------------------------------------------------------------------------
# Topology — the hub
# ---------------------------------------------------------------------------
@dataclass
class Topology:
    meta: Meta = field(default_factory=Meta)
    units: str = "canonical"                      # name of the canonical UnitSystem
    box: Optional[Box] = None
    defaults: Defaults = field(default_factory=Defaults)
    atom_types: List[AtomType] = field(default_factory=list)
    atoms: List[Atom] = field(default_factory=list)
    bonds: List[Bond] = field(default_factory=list)
    angles: List[Angle] = field(default_factory=list)
    dihedrals: List[Dihedral] = field(default_factory=list)
    impropers: List[Improper] = field(default_factory=list)
    pairs: List[Pair] = field(default_factory=list)
    exclusions: List[Exclusion] = field(default_factory=list)
    nonbonded_overrides: List[PairType] = field(default_factory=list)
    molecules: List[Molecule] = field(default_factory=list)
    # Derived type tables, populated by typing.extract_types (not serialized by default)
    derived_types: Dict[str, List[Any]] = field(default_factory=dict)

    # --- convenience --------------------------------------------------------
    @property
    def n_atoms(self) -> int:
        return len(self.atoms)

    def atom_type_map(self) -> Dict[str, AtomType]:
        return {t.name: t for t in self.atom_types}

    def has_positions(self) -> bool:
        return bool(self.atoms) and all(a.position is not None for a in self.atoms)

    # --- (de)serialization (JSON-ready nested dicts) ------------------------
    def to_dict(self) -> Dict[str, Any]:
        d: Dict[str, Any] = {
            "schema_version": SCHEMA_VERSION,
            "meta": asdict(self.meta),
            "units": self.units,
            "defaults": asdict(self.defaults),
            "box": {"matrix": self.box.matrix} if self.box is not None else None,
            "atom_types": [asdict(t) for t in self.atom_types],
            "atoms": [asdict(a) for a in self.atoms],
            "bonds": [asdict(b) for b in self.bonds],
            "angles": [asdict(a) for a in self.angles],
            "dihedrals": [asdict(x) for x in self.dihedrals],
            "impropers": [asdict(x) for x in self.impropers],
            "pairs": [asdict(p) for p in self.pairs],
            "exclusions": [asdict(e) for e in self.exclusions],
            "nonbonded_overrides": [asdict(p) for p in self.nonbonded_overrides],
            "molecules": [asdict(m) for m in self.molecules],
        }
        return d

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "Topology":
        def mk(klass, items):
            return [klass(**it) for it in (items or [])]
        box = None
        if d.get("box"):
            box = Box(matrix=[[float(x) for x in row] for row in d["box"]["matrix"]])
        return cls(
            meta=Meta(**(d.get("meta") or {})),
            units=d.get("units", "canonical"),
            box=box,
            defaults=Defaults(**(d.get("defaults") or {})),
            atom_types=mk(AtomType, d.get("atom_types")),
            atoms=mk(Atom, d.get("atoms")),
            bonds=mk(Bond, d.get("bonds")),
            angles=mk(Angle, d.get("angles")),
            dihedrals=mk(Dihedral, d.get("dihedrals")),
            impropers=mk(Improper, d.get("impropers")),
            pairs=mk(Pair, d.get("pairs")),
            exclusions=mk(Exclusion, d.get("exclusions")),
            nonbonded_overrides=mk(PairType, d.get("nonbonded_overrides")),
            molecules=mk(Molecule, d.get("molecules")),
        )
