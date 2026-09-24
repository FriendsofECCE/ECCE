"""Verify the MOPAC Slater basis spec by numerical integration.

If the per-atom ordering, the (a,b,c) exponents or the r^d principal
quantum numbers are wrong, the molecular orbitals will not come out
normalised -- <psi_i|psi_i> is a sensitive test of all three at once,
and needs no 3-D viewer to run.
"""
import sys, math
import numpy as np

BOHR = 0.529177210903   # Angstrom per bohr

PQN = [0,1,1] + [2]*8 + [3]*8 + [4]*18 + [5]*18 + [6]*32
def npq(z):   return PQN[z] if z < len(PQN) else 0
def npqs(z):  return npq(z) + (1 if z in (10,18,36,54,86) else 0)
def npqp(z):  return npq(z) + (1 if z == 2 else 0)

#  (a, b, c) for the d set, in MOPAC graphf order.
DVALS = [(0,-2,0), (1,0,1), (-2,0,0), (0,1,1), (1,1,0)]

def shells_for_atom(z, zs, zp, zd):
    """(a, b, c, d, zeta) per basis function, in file order."""
    out = []
    if zs: out.append((0, 0, 0, npqs(z)-1, zs))
    if zp:
        for abc in ((1,0,0), (0,1,0), (0,0,1)):
            out.append((abc[0], abc[1], abc[2], npqp(z)-2, zp))
    if zd:
        for (a,b,c) in DVALS:
            out.append((a, b, c, npq(z)-3, zd))
    return out

def basis_value(f, dx, dy, dz):
    """psi at a displacement from the centre, in BOHR. Unnormalised."""
    a, b, c, d, zeta = f
    r = math.sqrt(dx*dx + dy*dy + dz*dz)
    if a == -2:    ang = 2*dz*dz - dx*dx - dy*dy
    elif b == -2:  ang = dx*dx - dy*dy
    else:          ang = (dx**a) * (dy**b) * (dz**c)
    return ang * (r**d) * math.exp(-zeta*r)

def parse_mgf(path):
    lines = open(path).read().splitlines()
    nat = int(lines[0].split()[0])
    atoms, zetas = [], []
    for i in range(1, 1+nat):
        t = lines[i].split()
        atoms.append((int(t[0]), float(t[1]), float(t[2]), float(t[3])))
    for i in range(1+nat, 1+2*nat):
        t = [float(x) for x in lines[i].split()]
        zetas.append((t[0], t[1], t[2]))
    mos, i = [], 1+2*nat
    while i < len(lines):
        if lines[i].strip().startswith("ORBITAL"):
            coeffs, i = [], i+1
            while i < len(lines):
                ln = lines[i]
                if not ln.strip() or ln.strip().startswith(("ORBITAL","Keywords","INVERSE")):
                    break
                import re
                got = re.findall(r'-?\d*\.\d+[DdEe][-+]?\d+', ln)
                if not got: break
                coeffs += [float(x.replace('D','e').replace('d','e')) for x in got]
                i += 1
            mos.append(coeffs)
            continue
        i += 1
    #  INVERSE_MATRIX: the lower triangle of the symmetric inverse
    #  square root of the overlap (Lowdin).  MOPAC's printed MO
    #  coefficients are in the ORTHONORMALISED basis -- sum(c^2) is
    #  exactly 1 for every orbital -- so they must be transformed back
    #  before being combined with real, non-orthogonal Slater functions.
    inv = None
    for j, ln in enumerate(lines):
        if "INVERSE_MATRIX" in ln:
            import re as _re
            nums, k = [], j+1
            while k < len(lines) and not lines[k].strip().startswith("Keywords"):
                nums += [float(x.replace('D','e').replace('d','e'))
                         for x in _re.findall(r'-?\d*\.\d+[DdEe][-+]?\d+', lines[k])]
                k += 1
            n = len(mos[0])
            inv, idx = [], 0
            for row in range(n):
                inv.append(nums[idx:idx+row+1]); idx += row + 1
            break
    return atoms, zetas, mos, inv

atoms, zetas, mos, inv = parse_mgf(sys.argv[1])
funcs, centres = [], []
for (z, x, y, zc), (zs, zp, zd) in zip(atoms, zetas):
    for f in shells_for_atom(z, zs, zp, zd):
        funcs.append(f)
        centres.append((x/BOHR, y/BOHR, zc/BOHR))   # .mgf coords are Angstrom
print("atoms=%d  basis functions=%d  MOs=%d  coeffs/MO=%d"
      % (len(atoms), len(funcs), len(mos), len(mos[0])))
if len(funcs) != len(mos[0]):
    print("MISMATCH: basis size != coefficient count"); sys.exit(1)

#  Grid in bohr around the molecule.
cx = [c[0] for c in centres]; cy = [c[1] for c in centres]; cz = [c[2] for c in centres]
pad, h = 8.0, 0.11
gx = np.arange(min(cx)-pad, max(cx)+pad, h)
gy = np.arange(min(cy)-pad, max(cy)+pad, h)
gz = np.arange(min(cz)-pad, max(cz)+pad, h)
X, Y, Z = np.meshgrid(gx, gy, gz, indexing='ij')
dV = h**3

# Evaluate each basis function on the grid, then normalise it.
vals = []
for f, (ox, oy, oz) in zip(funcs, centres):
    a, b, c, d, zeta = f
    DX, DY, DZ = X-ox, Y-oy, Z-oz
    R = np.sqrt(DX*DX + DY*DY + DZ*DZ)
    if a == -2:   ang = 2*DZ*DZ - DX*DX - DY*DY
    elif b == -2: ang = DX*DX - DY*DY
    else:         ang = (DX**a)*(DY**b)*(DZ**c)
    v = ang * (R**d) * np.exp(-zeta*R)
    n = math.sqrt((v*v).sum()*dV)
    vals.append(v/n if n > 0 else v)

if inv:
    def sinv(k, j):
        return inv[k][j] if k >= j else inv[j][k]
    n = len(mos[0])
    mos = [[sum(co[k]*sinv(k, j) for k in range(n)) for j in range(n)]
           for co in mos]
    print("applied INVERSE_MATRIX (Lowdin S^-1/2) to the coefficients")

print("\n  MO   <psi|psi>   (1.000 means the basis order and r^d are right)")
bad = 0
for i, co in enumerate(mos):
    psi = np.zeros_like(X)
    for cf, v in zip(co, vals):
        if cf: psi = psi + cf*v
    norm = (psi*psi).sum()*dV
    flag = "" if abs(norm-1.0) < 0.06 else "   <-- OFF"
    if flag: bad += 1
    print("  %3d   %8.4f%s" % (i+1, norm, flag))
print("\n%d of %d MOs normalised" % (len(mos)-bad, len(mos)))
