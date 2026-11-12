"""Molecular descriptors from a SMILES string, for the QSAR table (#961).

Wx-free and without third-party packages.  The definitions follow published
ones, named after RDKit/MOE where the definition is the same:

  SlogP, SMR   Wildman and Crippen, J. Chem. Inf. Comput. Sci. 39 (1999) 868
               (table in ecce_qsar_crippen.py)
  TPSA         Ertl et al., J. Med. Chem. 43 (2000) 3714, N and O only
  Gasteiger    Gasteiger and Marsili, Tetrahedron 36 (1980) 3219, PEOE
  LabuteASA, *_VSA   Labute, J. Mol. Graph. Model. 18 (2000) 464
  chi*, Kier*  Hall and Kier; balabanJ  Balaban, Chem. Phys. Lett. 89 (1982) 399

The graph is the hydrogen-suppressed heavy-atom graph unless a descriptor
says otherwise.  Hydrogens are implicit (or the bracket count).
"""
import bisect
import math
import re

# symbol: (Z, mass, Rb0 (Labute radius, A), valence electrons, default valences)
ELEMENTS = {
    "H": (1, 1.008, 0.33, 1, (1,)),
    "B": (5, 10.812, 0.82, 3, (3,)),
    "C": (6, 12.011, 0.77, 4, (4,)),
    "N": (7, 14.007, 0.70, 5, (3, 5)),
    "O": (8, 15.999, 0.66, 6, (2,)),
    "F": (9, 18.998, 0.611, 7, (1,)),
    "Si": (14, 28.085, 0.937, 4, (4,)),
    "P": (15, 30.974, 0.89, 5, (3, 5)),
    "S": (16, 32.06, 1.04, 6, (2, 4, 6)),
    "Cl": (17, 35.45, 0.997, 7, (1,)),
    "Br": (35, 79.904, 1.167, 7, (1,)),
    "I": (53, 126.904, 1.387, 7, (1,)),
}
BY_Z = {v[0]: k for k, v in ELEMENTS.items()}
AROMATIC_OK = {"b", "c", "n", "o", "p", "s"}
EV = 27.211386245988


class SmilesError(ValueError):
    pass


class Atom:
    def __init__(self, sym, arom=False, charge=0, h=None, chiral=False):
        if sym not in ELEMENTS:
            raise SmilesError("unsupported element %s" % sym)
        self.sym = sym
        self.Z = ELEMENTS[sym][0]
        self.arom = arom
        self.charge = charge
        self.h = h            # None: from the valence
        self.chiral = chiral
        self.nbrs = []        # (atom index, bond index)


class Mol:
    def __init__(self):
        self.atoms = []
        self.bonds = []       # [i, j, order, aromatic]; order 1, 2, 3
        self.rings = []

    def add_bond(self, i, j, order, arom):
        k = len(self.bonds)
        self.bonds.append([i, j, order, arom])
        self.atoms[i].nbrs.append((j, k))
        self.atoms[j].nbrs.append((i, k))

    def bond_between(self, i, j):
        for n, k in self.atoms[i].nbrs:
            if n == j:
                return k
        return None


_ORGANIC = {"B", "C", "N", "O", "P", "S", "F", "Cl", "Br", "I"}
_BRACKET = re.compile(
    r"^(\d*)([A-Z][a-z]?|[bcnops]|se|as)(@{1,2}(?:TH\d|SP\d|TB\d+|OH\d+)?)?"
    r"(?:(H\d*))?([+-]+\d*)?(?:(H\d*))?(?::\d+)?$")


def _charge(txt):
    if not txt:
        return 0
    sign = 1 if txt[0] == "+" else -1
    m = re.match(r"^[+-](\d+)$", txt)
    return sign * (int(m.group(1)) if m else len(txt))


def parse(smiles):
    """SMILES -> Mol with implicit hydrogens, rings and aromaticity set."""
    m = Mol()
    s = smiles.strip().split()[0] if smiles.strip() else ""
    if not s:
        raise SmilesError("empty SMILES")
    stack, prev, bond, ringopen, i = [], None, None, {}, 0
    while i < len(s):
        c = s[i]
        if c == "(":
            if prev is None:
                raise SmilesError("branch without an atom")
            stack.append(prev)
            i += 1
        elif c == ")":
            if not stack:
                raise SmilesError("unbalanced )")
            prev = stack.pop()
            i += 1
        elif c == ".":
            prev, bond = None, None
            i += 1
        elif c in "-=#:/\\":
            bond = c
            i += 1
        elif c.isdigit() or c == "%":
            if c == "%":
                num, i = int(s[i + 1:i + 3]), i + 3
            else:
                num, i = int(c), i + 1
            if prev is None:
                raise SmilesError("ring closure without an atom")
            if num in ringopen:
                j, b = ringopen.pop(num)
                b = bond or b
                if j == prev:
                    raise SmilesError("ring closure to itself")
                _bond(m, prev, j, b)
            else:
                ringopen[num] = (prev, bond)
            bond = None
        else:
            if c == "[":
                e = s.find("]", i)
                if e < 0:
                    raise SmilesError("unclosed [")
                mt = _BRACKET.match(s[i + 1:e])
                if not mt:
                    raise SmilesError("cannot read [%s]" % s[i + 1:e])
                sym = mt.group(2)
                arom = sym.islower()
                sym = sym.capitalize()
                hs = mt.group(4) or mt.group(6)
                h = 0 if not hs else (int(hs[1:]) if len(hs) > 1 else 1)
                a = Atom(sym, arom, _charge(mt.group(5)), h, bool(mt.group(3)))
                i = e + 1
            else:
                two = s[i:i + 2]
                if two in ("Cl", "Br"):
                    sym, i = two, i + 2
                elif c in "BCNOPSFI":
                    sym, i = c, i + 1
                elif c in "bcnops":
                    sym, i = c.upper(), i + 1
                else:
                    raise SmilesError("cannot read %r" % c)
                a = Atom(sym, c in "bcnops")
            m.atoms.append(a)
            n = len(m.atoms) - 1
            if prev is not None:
                _bond(m, prev, n, bond, arom_pair=(m.atoms[prev].arom and a.arom))
            prev, bond = n, None
    if ringopen:
        raise SmilesError("unclosed ring")
    if stack:
        raise SmilesError("unbalanced (")
    _implicit_h(m)
    _perceive(m)
    return m


def _bond(m, i, j, sym, arom_pair=None):
    if arom_pair is None:
        arom_pair = m.atoms[i].arom and m.atoms[j].arom
    if sym in ("-", "/", "\\"):
        m.add_bond(i, j, 1, False)
    elif sym == "=":
        m.add_bond(i, j, 2, False)
    elif sym == "#":
        m.add_bond(i, j, 3, False)
    elif sym == ":":
        m.add_bond(i, j, 1, True)
    else:
        m.add_bond(i, j, 1, bool(arom_pair))


def _implicit_h(m):
    for a in m.atoms:
        if a.h is not None:
            continue
        s = sum(m.bonds[k][2] for _, k in a.nbrs)
        v = ELEMENTS[a.sym][4]
        if a.arom:
            a.h = max(0, v[0] - (s + 1))
        else:
            a.h = 0
            for x in v:
                if x >= s:
                    a.h = x - s
                    break


def bond_sum(m, i):
    """Valence: sum of bond orders (aromatic bonds 1.5) with the hydrogens."""
    a = m.atoms[i]
    return a.h + sum(1.5 if m.bonds[k][3] else m.bonds[k][2] for _, k in a.nbrs)


def sssr(m):
    """Smallest set of smallest rings, as lists of atom indices in ring order."""
    n = len(m.atoms)
    nb = len(m.bonds)
    seen, comps = [False] * n, 0
    for s in range(n):
        if not seen[s]:
            comps += 1
            todo = [s]
            seen[s] = True
            while todo:
                x = todo.pop()
                for y, _ in m.atoms[x].nbrs:
                    if not seen[y]:
                        seen[y] = True
                        todo.append(y)
    want = nb - n + comps
    if want <= 0:
        return []
    cands = {}
    for k, (i, j, _, _) in enumerate(m.bonds):
        # shortest path i->j without bond k closes the smallest ring through k
        prev = {i: None}
        q = [i]
        found = False
        while q and not found:
            nxt = []
            for x in q:
                for y, kk in m.atoms[x].nbrs:
                    if kk == k or y in prev:
                        continue
                    prev[y] = x
                    if y == j:
                        found = True
                        break
                    nxt.append(y)
                if found:
                    break
            q = nxt
        if found:
            path, x = [], j
            while x is not None:
                path.append(x)
                x = prev[x]
            key = frozenset(_ring_bonds(m, path))
            cands[key] = path
    out, basis = [], []
    for key, path in sorted(cands.items(), key=lambda kv: (len(kv[1]), sorted(kv[0]))):
        v = 0
        for k in key:
            v |= 1 << k
        for b in basis:
            v = min(v, v ^ b)
        if v:
            basis.append(v)
            out.append(path)
            if len(out) == want:
                break
    return out


def _ring_bonds(m, path):
    return [m.bond_between(path[x], path[(x + 1) % len(path)]) for x in range(len(path))]


def _perceive(m):
    """Rings, and aromaticity for Kekule rings (6- and 5-membered, Hueckel)."""
    m.rings = sssr(m)
    for ring in m.rings:
        rb = _ring_bonds(m, ring)
        if all(m.bonds[k][3] for k in rb):
            continue
        if len(ring) not in (5, 6) or any(m.bonds[k][3] for k in rb):
            continue
        e = 0
        ok = True
        for x, i in enumerate(ring):
            a = m.atoms[i]
            ib = [m.bonds[rb[x]], m.bonds[rb[x - 1]]]
            dbl_in = any(b[2] == 2 for b in ib)
            exo = [(n, k) for n, k in a.nbrs if k not in (rb[x], rb[x - 1])]
            exo_dbl = [n for n, k in exo if m.bonds[k][2] == 2]
            if dbl_in:
                e += 1
            elif exo_dbl:
                other = m.atoms[exo_dbl[0]]
                if ELEMENTS[other.sym][3] >= 6 and a.sym == "C":
                    e += 0
                else:
                    ok = False
            elif a.sym in ("N", "O", "S") and a.charge == 0 and len(ring) in (5, 6):
                e += 2
            elif a.sym == "C" and a.charge == -1:
                e += 2
            elif a.sym in ("C", "N") and a.charge == 1 and not dbl_in:
                e += 0
            else:
                ok = False
        if ok and e % 4 == 2:
            for k in rb:
                m.bonds[k][3] = True
            for i in ring:
                m.atoms[i].arom = True


# ---- graph helpers ---------------------------------------------------------

def heavy_n(m):
    return len(m.atoms)


def distance_matrix(m):
    n = len(m.atoms)
    D = [[0 if i == j else None for j in range(n)] for i in range(n)]
    for s in range(n):
        q = [s]
        while q:
            nxt = []
            for x in q:
                for y, _ in m.atoms[x].nbrs:
                    if D[s][y] is None:
                        D[s][y] = D[s][x] + 1
                        nxt.append(y)
            q = nxt
    return D


def sym_classes(m):
    """Topological symmetry classes (Morgan refinement) with element, charge,
    aromaticity and bond order in the starting labels."""
    n = len(m.atoms)
    cls = [hash((a.Z, a.charge, a.arom, a.h)) for a in m.atoms]
    for _ in range(n):
        new = []
        for i, a in enumerate(m.atoms):
            nb = sorted((cls[j], m.bonds[k][2], m.bonds[k][3]) for j, k in a.nbrs)
            new.append(hash((cls[i], tuple(nb))))
        if len(set(new)) == len(set(cls)):
            cls = new
            break
        cls = new
    return cls


def chiral_centres(m):
    """Atoms with four topologically different substituents (C, or N/P/S
    cations), the hydrogens counted as one kind."""
    cls = sym_classes(m)
    cnt = 0
    for i, a in enumerate(m.atoms):
        if a.sym == "C" and not a.arom:
            pass
        elif a.sym in ("N", "P") and a.charge == 1:
            pass
        else:
            continue
        if any(m.bonds[k][2] != 1 for _, k in a.nbrs):
            continue
        subs = [cls[j] for j, _ in m.atoms[i].nbrs] + ["H"] * a.h
        if len(subs) == 4 and len(set(subs)) == 4:
            cnt += 1
    return cnt


# ---- counts ----------------------------------------------------------------

def _rotatable(m):
    """Single, acyclic bonds between non-terminal atoms, not at a triple bond
    and not the N-C bond of an amide N-H (the Open Babel pattern)."""
    ringb = set()
    for ring in m.rings:
        ringb.update(_ring_bonds(m, ring))
    n = 0

    def ok(i):
        a = m.atoms[i]
        if len(a.nbrs) < 2:
            return False
        if any(m.bonds[k][2] == 3 for _, k in a.nbrs):
            return False
        if a.sym == "N" and a.h == 1:
            for j, k in a.nbrs:
                if k in ringb:
                    continue
                c = m.atoms[j]
                if c.sym == "C" and any(m.bonds[kk][2] == 2 and m.atoms[jj].sym == "O"
                                        for jj, kk in c.nbrs):
                    return False
        return True

    for k, (i, j, o, ar) in enumerate(m.bonds):
        if o == 1 and not ar and k not in ringb and ok(i) and ok(j):
            n += 1
    return n


def counts(m):
    d = {}
    heavy = len(m.atoms)
    d["weight"] = sum(ELEMENTS[a.sym][1] + a.h * ELEMENTS["H"][1] for a in m.atoms)
    d["a_heavy"] = heavy
    d["a_nH"] = sum(a.h for a in m.atoms)
    d["a_count"] = heavy + d["a_nH"]
    for e in ("C", "N", "O", "F", "Cl", "Br", "I", "S", "P", "B", "Si"):
        d["a_n" + e] = sum(1 for a in m.atoms if a.sym == e)
    d["a_aro"] = sum(1 for a in m.atoms if a.arom)
    d["b_heavy"] = len(m.bonds)
    d["b_count"] = len(m.bonds) + d["a_nH"]
    d["b_ar"] = sum(1 for b in m.bonds if b[3])
    d["b_double"] = sum(1 for b in m.bonds if b[2] == 2 and not b[3])
    d["b_triple"] = sum(1 for b in m.bonds if b[2] == 3)
    d["b_single"] = sum(1 for b in m.bonds if b[2] == 1 and not b[3])
    d["b_rotN"] = _rotatable(m)
    d["b_rotR"] = d["b_rotN"] / len(m.bonds) if m.bonds else 0.0
    d["rings"] = len(m.rings)
    d["aro_rings"] = sum(1 for r in m.rings
                         if all(m.bonds[k][3] for k in _ring_bonds(m, r)))
    d["chiral"] = chiral_centres(m)
    d["FCharge"] = sum(a.charge for a in m.atoms)
    d["lip_don"] = sum(1 for a in m.atoms if a.sym in ("N", "O") and a.h > 0)
    d["lip_acc"] = sum(1 for a in m.atoms if a.sym in ("N", "O"))
    return d


# ---- topological indices -----------------------------------------------------

def _delta(m, i):
    return len(m.atoms[i].nbrs)


def _delta_v(m, i):
    a = m.atoms[i]
    z, zv = a.Z, ELEMENTS[a.sym][3]
    zv = zv - a.charge if a.sym not in ("H",) else zv
    h = a.h
    if z <= 10:
        return zv - h
    return (zv - h) / (z - zv - 1.0)


def topology(m):
    n = len(m.atoms)
    d = {}
    if n == 0:
        return d
    D = distance_matrix(m)
    dd = [_delta(m, i) for i in range(n)]
    dv = [_delta_v(m, i) for i in range(n)]
    finite = [D[i][j] for i in range(n) for j in range(n) if D[i][j] is not None]
    d["weinerPath"] = sum(D[i][j] for i in range(n) for j in range(i + 1, n)
                          if D[i][j] is not None)
    d["weinerPol"] = sum(1 for i in range(n) for j in range(i + 1, n) if D[i][j] == 3)
    d["diameter"] = max(finite)
    ecc = [max(x for x in D[i] if x is not None) for i in range(n)]
    d["radius"] = min(ecc)
    d["petitjean"] = (d["diameter"] - d["radius"]) / float(d["radius"]) if d["radius"] else 0.0
    d["zagreb1"] = sum(x * x for x in dd)
    d["zagreb2"] = sum(dd[i] * dd[j] for i, j, _, _ in m.bonds)
    d["chi0"] = sum(1 / math.sqrt(x) for x in dd if x > 0)
    d["chi0v"] = sum(1 / math.sqrt(x) for x in dv if x > 0)
    d["chi1"] = sum(1 / math.sqrt(dd[i] * dd[j]) for i, j, _, _ in m.bonds
                    if dd[i] * dd[j] > 0)
    d["chi1v"] = sum(1 / math.sqrt(dv[i] * dv[j]) for i, j, _, _ in m.bonds
                     if dv[i] * dv[j] > 0)
    # Balaban J: bonds q, rings mu = q - n + components
    q = len(m.bonds)
    comps = _components(m)
    mu = q - n + comps
    s = [sum(x for x in D[i] if x is not None) for i in range(n)]
    tot = sum(1 / math.sqrt(s[i] * s[j]) for i, j, _, _ in m.bonds if s[i] * s[j] > 0)
    d["balabanJ"] = q / float(mu + 1) * tot if q else 0.0
    # Kier kappa shape indices, plain and alpha-modified
    for tag, alpha in (("Kier", [0.0] * n), ("KierA", _alpha(m))):
        a = sum(1 + x for x in alpha)
        p1 = q
        p2 = _paths(m, 2)
        p3 = _paths(m, 3)
        k1 = a * (a - 1) ** 2 / p1 ** 2 if p1 else 0.0
        k2 = (a - 1) * (a - 2) ** 2 / p2 ** 2 if p2 else 0.0
        if p3:
            k3 = ((a - 3) * (a - 2) ** 2 / p3 ** 2 if n % 2 == 0
                  else (a - 1) * (a - 3) ** 2 / p3 ** 2)
        else:
            k3 = 0.0
        d[tag + "1"], d[tag + "2"], d[tag + "3"] = k1, k2, k3
    d["KierFlex"] = d["KierA1"] * d["KierA2"] / n
    # information content of the symmetry classes (Basak IC0)
    cls = sym_classes(m)
    cnt = {}
    for c in cls:
        cnt[c] = cnt.get(c, 0) + 1
    d["ic0"] = -sum(v / n * math.log2(v / n) for v in cnt.values())
    return d


def _components(m):
    n, seen, c = len(m.atoms), set(), 0
    for s in range(n):
        if s in seen:
            continue
        c += 1
        todo = [s]
        seen.add(s)
        while todo:
            x = todo.pop()
            for y, _ in m.atoms[x].nbrs:
                if y not in seen:
                    seen.add(y)
                    todo.append(y)
    return c


def _alpha(m):
    rc = ELEMENTS["C"][2]
    return [ELEMENTS[a.sym][2] / rc - 1.0 for a in m.atoms]


def _paths(m, length):
    """Number of simple paths of the given bond length (each counted once)."""
    cnt = 0

    def walk(path):
        nonlocal cnt
        if len(path) == length + 1:
            cnt += 1
            return
        for y, _ in m.atoms[path[-1]].nbrs:
            if y not in path:
                walk(path + [y])

    for i in range(len(m.atoms)):
        walk([i])
    return cnt // 2


# ---- Wildman-Crippen (SMARTS) ---------------------------------------------------

class _Q:
    """Atom query: nested ("and"|"or"|"not"|prim, ...) tuples."""


def _parse_atom_expr(t):
    """Parse the inside of [...]: ; lowest, then ',', then '&'/implicit, then '!'."""
    pos = [0]

    def prim():
        c = t[pos[0]]
        if c == "!":
            pos[0] += 1
            return ("not", prim())
        if c == "#":
            mt = re.compile(r"#(\d+)").match(t, pos[0])
            pos[0] = mt.end()
            return ("Z", int(mt.group(1)))
        if c in "+-":
            mt = re.compile(r"([+-])(\d*)").match(t, pos[0])
            pos[0] = mt.end()
            n = int(mt.group(2)) if mt.group(2) != "" else 1
            return ("chg", n if mt.group(1) == "+" else -n)
        if c in "HXD":
            mt = re.compile(r"([HXD])(\d*)").match(t, pos[0])
            pos[0] = mt.end()
            return (mt.group(1), int(mt.group(2)) if mt.group(2) else 1)
        if c == "A":
            if t[pos[0]:pos[0] + 2] in ("Al", "As", "Au", "Ag"):
                raise SmilesError("unsupported SMARTS element")
            pos[0] += 1
            return ("aliph",)
        if c == "a":
            pos[0] += 1
            return ("arom",)
        if c == "*":
            pos[0] += 1
            return ("true",)
        two = t[pos[0]:pos[0] + 2]
        if two in ("Cl", "Br"):
            pos[0] += 2
            return ("and", ("Z", ELEMENTS[two][0]), ("aliph",))
        if c.isupper():
            pos[0] += 1
            return ("and", ("Z", ELEMENTS[c][0]), ("aliph",))
        if c in AROMATIC_OK:
            pos[0] += 1
            return ("and", ("Z", ELEMENTS[c.upper()][0]), ("arom",))
        raise SmilesError("SMARTS primitive %r" % c)

    def and_hi():
        parts = [prim()]
        while pos[0] < len(t) and t[pos[0]] not in ",;":
            if t[pos[0]] == "&":
                pos[0] += 1
            parts.append(prim())
        return parts[0] if len(parts) == 1 else ("and",) + tuple(parts)

    def or_():
        parts = [and_hi()]
        while pos[0] < len(t) and t[pos[0]] == ",":
            pos[0] += 1
            parts.append(and_hi())
        return parts[0] if len(parts) == 1 else ("or",) + tuple(parts)

    def and_lo():
        parts = [or_()]
        while pos[0] < len(t) and t[pos[0]] == ";":
            pos[0] += 1
            parts.append(or_())
        return parts[0] if len(parts) == 1 else ("and",) + tuple(parts)

    r = and_lo()
    if pos[0] != len(t):
        raise SmilesError("SMARTS %r" % t)
    return r


def _parse_smarts(s):
    """-> (atom queries, [(parent, child, bond symbol or None)])."""
    atoms, edges, stack, prev, bond, i = [], [], [], None, None, 0
    while i < len(s):
        c = s[i]
        if c == "(":
            stack.append(prev)
            i += 1
        elif c == ")":
            prev = stack.pop()
            i += 1
        elif c in "-=#:":
            bond = c
            i += 1
        else:
            if c == "[":
                e = s.index("]", i)
                q = _parse_atom_expr(s[i + 1:e])
                i = e + 1
            else:
                two = s[i:i + 2]
                tok = two if two in ("Cl", "Br") else c
                q = _parse_atom_expr(tok)
                i += len(tok)
            atoms.append(q)
            if prev is not None:
                edges.append((prev, len(atoms) - 1, bond))
            prev, bond = len(atoms) - 1, None
    return atoms, edges


class _HGraph:
    """Heavy atoms plus explicit hydrogens, for the SMARTS typing."""

    def __init__(self, m):
        self.Z, self.arom, self.charge, self.nbrs = [], [], [], []
        for a in m.atoms:
            self.Z.append(a.Z)
            self.arom.append(a.arom)
            self.charge.append(a.charge)
            self.nbrs.append([])
        for i, j, o, ar in m.bonds:
            b = "a" if ar else str(o)
            self.nbrs[i].append((j, b))
            self.nbrs[j].append((i, b))
        for i, a in enumerate(m.atoms):
            for _ in range(a.h):
                k = len(self.Z)
                self.Z.append(1)
                self.arom.append(False)
                self.charge.append(0)
                self.nbrs.append([(i, "1")])
                self.nbrs[i].append((k, "1"))

    def nH(self, i):
        return sum(1 for j, _ in self.nbrs[i] if self.Z[j] == 1)


def _eval(q, g, i):
    t = q[0]
    if t == "and":
        return all(_eval(x, g, i) for x in q[1:])
    if t == "or":
        return any(_eval(x, g, i) for x in q[1:])
    if t == "not":
        return not _eval(q[1], g, i)
    if t == "Z":
        return g.Z[i] == q[1]
    if t == "chg":
        return g.charge[i] == q[1]
    if t == "H":
        return g.nH(i) == q[1]
    if t == "X":
        return len(g.nbrs[i]) == q[1]
    if t == "D":
        return sum(1 for j, _ in g.nbrs[i] if g.Z[j] != 1) == q[1]
    if t == "aliph":
        return not g.arom[i]
    if t == "arom":
        return g.arom[i]
    return True


def _bond_ok(sym, b):
    if sym is None:
        return b in ("1", "a")
    return {"-": "1", "=": "2", "#": "3", ":": "a"}[sym] == b


def _match_at(pat, g, root):
    atoms, edges = pat
    if not _eval(atoms[0], g, root):
        return False
    kids = {}
    for p, c, b in edges:
        kids.setdefault(p, []).append((c, b))
    order = []

    def lin(p):
        for c, b in kids.get(p, []):
            order.append((p, c, b))
            lin(c)

    lin(0)

    def go(idx, mp):
        if idx == len(order):
            return True
        p, c, b = order[idx]
        for j, bt in g.nbrs[mp[p]]:
            if j in mp.values() or not _bond_ok(b, bt):
                continue
            if _eval(atoms[c], g, j):
                mp[c] = j
                if go(idx + 1, mp):
                    return True
                del mp[c]
        return False

    return go(0, {0: root})


_CRIPPEN_COMPILED = None


def crippen_contribs(m):
    """Per heavy atom (logP, MR) and the hydrogens' totals (logP, MR)."""
    global _CRIPPEN_COMPILED
    if _CRIPPEN_COMPILED is None:
        from ecce_qsar_crippen import CRIPPEN
        _CRIPPEN_COMPILED = [(t, _parse_smarts(s), lp, mr) for t, s, lp, mr in CRIPPEN]
    g = _HGraph(m)
    res = []
    for i in range(len(g.Z)):
        hit = None
        for t, pat, lp, mr in _CRIPPEN_COMPILED:
            if _match_at(pat, g, i):
                hit = (lp, mr)
                break
        if hit is None:
            raise SmilesError("no Crippen type for atom %d" % i)
        res.append(hit)
    heavy = res[:len(m.atoms)]
    hl = sum(x[0] for x in res[len(m.atoms):])
    hm = sum(x[1] for x in res[len(m.atoms):])
    return heavy, hl, hm


# ---- TPSA, Gasteiger, Labute ------------------------------------------------------

def _bond_counts(m, i):
    sing = doub = trip = arom = 0
    for _, k in m.atoms[i].nbrs:
        o, ar = m.bonds[k][2], m.bonds[k][3]
        if ar:
            arom += 1
        elif o == 1:
            sing += 1
        elif o == 2:
            doub += 1
        else:
            trip += 1
    return sing, doub, trip, arom


def tpsa_contribs(m):
    """Ertl TPSA contributions of N and O (as RDKit, without S and P)."""
    in3 = set()
    for r in m.rings:
        if len(r) == 3:
            in3.update(r)
    out = []
    for i, a in enumerate(m.atoms):
        if a.sym not in ("N", "O"):
            out.append(0.0)
            continue
        nS, nD, nT, nA = _bond_counts(m, i)
        nn, nh, chg, r3 = len(a.nbrs), a.h, a.charge, i in in3
        t = -1.0
        if a.sym == "N":
            if nn == 1:
                if nh == 0 and chg == 0 and nT == 1:
                    t = 23.79
                elif nh == 1 and chg == 0 and nD == 1:
                    t = 23.85
                elif nh == 2 and chg == 0 and nS == 1:
                    t = 26.02
                elif nh == 2 and chg == 1 and nD == 1:
                    t = 25.59
                elif nh == 3 and chg == 1 and nS == 1:
                    t = 27.64
            elif nn == 2:
                if nh == 0 and chg == 0 and nS == 1 and nD == 1:
                    t = 12.36
                elif nh == 0 and chg == 0 and nT == 1 and nD == 1:
                    t = 13.60
                elif nh == 1 and chg == 0 and nS == 2 and r3:
                    t = 21.94
                elif nh == 1 and chg == 0 and nS == 2:
                    t = 12.03
                elif nh == 0 and chg == 1 and nT == 1 and nS == 1:
                    t = 4.36
                elif nh == 1 and chg == 1 and nD == 1 and nS == 1:
                    t = 13.97
                elif nh == 2 and chg == 1 and nS == 2:
                    t = 16.61
                elif nh == 0 and chg == 0 and nA == 2:
                    t = 12.89
                elif nh == 1 and chg == 0 and nA == 2:
                    t = 15.79
                elif nh == 1 and chg == 1 and nA == 2:
                    t = 14.14
            elif nn == 3:
                if nh == 0 and chg == 0 and nS == 3 and r3:
                    t = 3.01
                elif nh == 0 and chg == 0 and nS == 3:
                    t = 3.24
                elif nh == 0 and chg == 0 and nS == 1 and nD == 2:
                    t = 11.68
                elif nh == 0 and chg == 1 and nS == 2 and nD == 1:
                    t = 3.01
                elif nh == 1 and chg == 1 and nS == 3:
                    t = 4.44
                elif nh == 0 and chg == 0 and nA == 3:
                    t = 4.41
                elif nh == 0 and chg == 0 and nS == 1 and nA == 2:
                    t = 4.93
                elif nh == 0 and chg == 0 and nD == 1 and nA == 2:
                    t = 8.39
                elif nh == 0 and chg == 1 and nA == 3:
                    t = 4.10
                elif nh == 0 and chg == 1 and nS == 1 and nA == 2:
                    t = 3.88
            elif nn == 4:
                if nh == 0 and nS == 4 and chg == 1:
                    t = 0.0
            if t < 0:
                t = max(0.0, 30.5 - nn * 8.2 + nh * 1.5)
        else:
            if nn == 1:
                if nh == 0 and chg == 0 and nD == 1:
                    t = 17.07
                elif nh == 1 and chg == 0 and nS == 1:
                    t = 20.23
                elif nh == 0 and chg == -1 and nS == 1:
                    t = 23.06
            elif nn == 2:
                if nh == 0 and chg == 0 and nS == 2 and r3:
                    t = 12.53
                elif nh == 0 and chg == 0 and nS == 2:
                    t = 9.23
                elif nh == 0 and chg == 0 and nA == 2:
                    t = 13.14
            if t < 0:
                t = max(0.0, 28.5 - nn * 8.6 + nh * 1.5)
        out.append(t)
    return out


_GASTEIGER = {
    ("H", "*"): (7.17, 6.24, -0.56), ("C", "sp3"): (7.98, 9.18, 1.88),
    ("C", "sp2"): (8.79, 9.32, 1.51), ("C", "sp"): (10.39, 9.45, 0.73),
    ("N", "sp3"): (11.54, 10.82, 1.36), ("N", "sp2"): (12.87, 11.15, 0.85),
    ("N", "sp"): (15.68, 11.7, -0.27), ("O", "sp3"): (14.18, 12.92, 1.39),
    ("O", "sp2"): (17.07, 13.79, 0.47), ("F", "sp3"): (14.66, 13.85, 2.31),
    ("Cl", "sp3"): (11.00, 9.69, 1.35), ("Br", "sp3"): (10.08, 8.47, 1.16),
    ("I", "sp3"): (9.9, 7.96, 0.96), ("S", "sp3"): (10.14, 9.13, 1.38),
    ("S", "so"): (10.14, 9.13, 1.38), ("S", "so2"): (12.00, 10.81, 1.20),
    ("S", "sp2"): (10.88, 9.49, 1.33), ("P", "sp3"): (8.90, 8.24, 0.96),
    ("P", "sp2"): (9.665, 8.530, 0.735), ("Si", "sp3"): (7.300, 6.567, 0.657),
    ("B", "sp3"): (5.980, 6.820, 1.605), ("B", "sp2"): (6.420, 6.807, 1.322),
}
IONXH = 20.02


def _hybrid(m, i):
    a = m.atoms[i]
    if a.sym in ("F", "Cl", "Br", "I"):
        return "sp3"
    zv = ELEMENTS[a.sym][3] - a.charge
    val = bond_sum(m, i)
    nlp = max(0, int(round((zv - val) / 2.0)))
    norb = len(a.nbrs) + a.h + nlp
    if nlp > 0:
        for j, kb in a.nbrs:
            if any(m.bonds[k][2] > 1 or m.bonds[k][3]
                   for _, k in m.atoms[j].nbrs if k != kb):
                norb -= 1
                break
    if norb >= 4:
        return "sp3"
    return "sp2" if norb == 3 else "sp"


def _conjugated_bond(m, k):
    i, j, o, ar = m.bonds[k]
    if o > 1 or ar:
        return True

    def multi(x):
        return any(m.bonds[kk][2] > 1 or m.bonds[kk][3] for _, kk in m.atoms[x].nbrs)

    def lp(x):
        a = m.atoms[x]
        return a.sym in ("N", "O", "S", "F", "Cl", "Br", "I") or (a.sym == "C" and a.charge < 0)

    return (multi(i) and (multi(j) or lp(j))) or (multi(j) and lp(i))


def gasteiger(m, niter=12):
    """PEOE charges of the heavy atoms and of the implicit hydrogens' total."""
    n = len(m.atoms)
    q = [0.0] * n
    # formal charge shared among equal elements across a conjugated path
    for i, a in enumerate(m.atoms):
        if a.charge != 0 and abs(q[i]) < 1e-12:
            formal, marks = float(a.charge), [i]
            for j, k1 in a.nbrs:
                if not _conjugated_bond(m, k1):
                    continue
                for y, k2 in m.atoms[j].nbrs:
                    if k2 != k1 and _conjugated_bond(m, k2) and m.atoms[y].Z == a.Z:
                        formal += m.atoms[y].charge
                        marks.append(y)
            for x in marks:
                q[x] = formal / len(marks)
    P = []
    ionx = []
    for i, a in enumerate(m.atoms):
        mode = _hybrid(m, i)
        key = (a.sym, mode)
        if key not in _GASTEIGER:
            key = (a.sym, "sp3")
        if key not in _GASTEIGER:
            raise SmilesError("no Gasteiger parameters for %s" % a.sym)
        p = _GASTEIGER[key]
        P.append(p)
        ionx.append(p[0] + p[1] + p[2])
    hp = _GASTEIGER[("H", "*")]
    hq = [0.0] * n
    damp = 0.5
    for _ in range(niter):
        en = [P[i][0] + q[i] * (P[i][1] + P[i][2] * q[i]) for i in range(n)]
        for i, a in enumerate(m.atoms):
            dq = 0.0
            for j, _ in a.nbrs:
                dx = en[j] - en[i]
                sgn = 0 if dx < 0 else 1
                dq += dx / (sgn * (ionx[i] - ionx[j]) + ionx[j])
            if a.h > 0:
                qh = hq[i] / a.h
                enh = hp[0] + qh * (hp[1] + hp[2] * qh)
                dx = enh - en[i]
                sgn = 0 if dx < 0 else 1
                dqh = dx / (sgn * (ionx[i] - IONXH) + IONXH)
                dq += a.h * dqh
                hq[i] -= a.h * dqh * damp
            q[i] += damp * dq
        damp *= 0.5
    return q, hq


def labute_contribs(m):
    """Approximate vdW surface (A^2) per heavy atom, hydrogens included in the
    atoms they sit on (the explicit-H total is added to the atom's own area)."""
    n = len(m.atoms)
    rad = [ELEMENTS[a.sym][2] for a in m.atoms]
    scale = {1: 0.1, 2: 0.2, 3: 0.3}
    V = [0.0] * n
    for i, j, o, ar in m.bonds:
        ri, rj = rad[i], rad[j]
        bij = ri + rj - (0.1 if ar else scale[o] if o in (2, 3) else 0.0)
        dij = min(max(abs(ri - rj), bij), ri + rj)
        V[i] += rj * rj - (ri - dij) ** 2 / dij
        V[j] += ri * ri - (rj - dij) ** 2 / dij
    rh = ELEMENTS["H"][2]
    hc = 0.0
    for i, a in enumerate(m.atoms):
        for _ in range(a.h):
            ri = rad[i]
            bij = ri + rh
            dij = min(max(abs(ri - rh), bij), ri + rh)
            V[i] += rh * rh - (ri - dij) ** 2 / dij
            hc += ri * ri - (rh - dij) ** 2 / dij
    area = [math.pi * rad[i] * (4 * rad[i] - V[i]) for i in range(n)]
    return area, hc


PEOE_BINS = [-.3, -.25, -.20, -.15, -.10, -.05, 0, .05, .10, .15, .20, .25, .30]
LOGP_BINS = [-0.4, -0.2, 0, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.6]
MR_BINS = [1.29, 1.82, 2.24, 2.45, 2.75, 3.05, 3.63, 3.8, 4.0]


def properties(m, niter=12):
    """SlogP, SMR, TPSA, Gasteiger charge sums and the VSA families."""
    d = {}
    contribs, hl, hm = crippen_contribs(m)
    d["SlogP"] = sum(c[0] for c in contribs) + hl
    d["SMR"] = sum(c[1] for c in contribs) + hm
    tp = tpsa_contribs(m)
    d["TPSA"] = sum(tp)
    q, hq = gasteiger(m, niter)
    area, hc = labute_contribs(m)
    d["LabuteASA"] = sum(area) + hc
    pos = [x for x in q if x > 0]
    neg = [x for x in q if x < 0]
    d["Q_PC+"] = sum(pos)
    d["Q_PC-"] = sum(neg)
    d["Q_RPC+"] = max(pos) / sum(pos) if pos else 0.0
    d["Q_RPC-"] = min(neg) / sum(neg) if neg else 0.0
    d["Q_max"] = max(q) if q else 0.0
    d["Q_min"] = min(q) if q else 0.0
    d["PEOE_VSA_POS"] = sum(a for a, x in zip(area, q) if x > 0)
    d["PEOE_VSA_NEG"] = sum(a for a, x in zip(area, q) if x < 0)
    d["PEOE_VSA_PPOS"] = sum(a for a, x in zip(area, q) if x > 0.2)
    d["PEOE_VSA_PNEG"] = sum(a for a, x in zip(area, q) if x < -0.2)
    d["PEOE_VSA_HYD"] = sum(a for a, x in zip(area, q) if abs(x) <= 0.2)
    tot = d["LabuteASA"]
    d["PEOE_VSA_FPOS"] = d["PEOE_VSA_POS"] / tot
    d["PEOE_VSA_FNEG"] = d["PEOE_VSA_NEG"] / tot
    for name, bins, vals in (("PEOE_VSA", PEOE_BINS, q),
                             ("SlogP_VSA", LOGP_BINS, [c[0] for c in contribs]),
                             ("SMR_VSA", MR_BINS, [c[1] for c in contribs])):
        acc = [0.0] * (len(bins) + 1)
        for a, v in zip(area, vals):
            acc[bisect.bisect_right(bins, v)] += a
        for k, v in enumerate(acc):
            d["%s%d" % (name, k + 1)] = v
    return d


def all_descriptors(smiles, groups=None, niter=12):
    """Every descriptor of a SMILES as an ordered dict-like list of pairs."""
    m = parse(smiles)
    out = {}
    out.update(counts(m))
    out.update(topology(m))
    out.update(properties(m, niter))
    out["lip_violation"] = ((out["weight"] > 500) + (out["SlogP"] > 5) +
                            (out["lip_don"] > 5) + (out["lip_acc"] > 10))
    return out


GROUPS = {
    "counts": ["weight", "a_count", "a_heavy", "a_nH", "a_nC", "a_nN", "a_nO", "a_nF",
               "a_nCl", "a_nBr", "a_nI", "a_nS", "a_nP", "a_nB", "a_nSi", "a_aro",
               "b_count", "b_heavy", "b_ar", "b_single", "b_double", "b_triple",
               "b_rotN", "b_rotR", "rings", "aro_rings", "chiral", "FCharge",
               "lip_don", "lip_acc", "lip_violation"],
    "topology": ["weinerPath", "weinerPol", "diameter", "radius", "petitjean", "zagreb1",
                 "zagreb2", "chi0", "chi0v", "chi1", "chi1v", "balabanJ", "Kier1",
                 "Kier2", "Kier3", "KierA1", "KierA2", "KierA3", "KierFlex", "ic0"],
    "models": ["SlogP", "SMR", "TPSA", "LabuteASA"],
    "charges": ["Q_PC+", "Q_PC-", "Q_RPC+", "Q_RPC-", "Q_max", "Q_min"],
    "vsa": (["PEOE_VSA_POS", "PEOE_VSA_NEG", "PEOE_VSA_PPOS", "PEOE_VSA_PNEG",
             "PEOE_VSA_HYD", "PEOE_VSA_FPOS", "PEOE_VSA_FNEG"] +
            ["PEOE_VSA%d" % i for i in range(1, 15)] +
            ["SlogP_VSA%d" % i for i in range(1, 13)] +
            ["SMR_VSA%d" % i for i in range(1, 11)]),
}
QM_NAMES = ["qm_E", "qm_HOMO", "qm_LUMO", "qm_gap", "qm_dipole", "qm_qmin", "qm_qmax"]
