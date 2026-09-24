"""Validate the whole Slater-to-Gaussian chain against MOPAC itself.

    tools/mopac/verify_expansion_vs_mopac.py <graphf file>

If the Gaussian expansion of MOPAC's Slater AOs is right, the overlap
matrix S it produces must reproduce the S^(-1/2) that MOPAC writes into
the GRAPHF file.  That is an independent reference computed by MOPAC
from the exact Slater functions, so agreeing with it validates the
exponents, the radial powers, the AO ordering and the units all at once.
"""
import numpy as np, math, re, sys
BOHR = 0.52917721092

# ---- Slater -> Gaussian (even-tempered, linear least squares) -----------
def expand(n, l, zeta, ng=10):
    r = np.linspace(1e-6, 60.0/zeta, 60000); w = r*r*(r[1]-r[0])
    target = r**(n-1)*np.exp(-zeta*r)
    best = (None, None, -1)
    for b in (1.8,2.0,2.2,2.5,3.0,3.5,4.0,5.0):
        for sh in np.arange(-3.0, 3.01, 0.25):
            al = zeta*zeta*b**sh/b**((ng-1)/2.0)*b**np.arange(ng)
            basis = np.array([r**l*np.exp(-a*r*r) for a in al])
            A = np.einsum('ir,jr,r->ij', basis, basis, w)
            v = np.einsum('ir,r,r->i', basis, target, w)
            try: c = np.linalg.solve(A+1e-12*np.trace(A)/ng*np.eye(ng), v)
            except np.linalg.LinAlgError: continue
            ap = c@basis
            ov = (ap*target*w).sum()/math.sqrt((ap*ap*w).sum()*(target*target*w).sum())
            if ov > best[2]: best = (al, c, ov)
    return best

# ---- analytic Cartesian Gaussian overlap --------------------------------
def df(k):
    """(k)!! with df(-1) = 1.

    Not scipy's factorial2: since 1.11 it returns 0 for -1, which is
    exactly the term every s-s overlap needs, so the whole matrix comes
    out with a zero diagonal and no eigendecomposition.
    """
    if k <= 0:
        return 1.0
    out = 1.0
    while k > 1:
        out *= k; k -= 2
    return out

def s1d(la, lb, pa, pb, g):
    """1-D overlap integral via the binomial/double-factorial sum."""
    tot = 0.0
    for i in range(la+1):
        for j in range(lb+1):
            if (i+j) % 2: continue
            tot += (math.comb(la,i)*math.comb(lb,j)
                    * df(i+j-1)
                    * pa**(la-i) * pb**(lb-j) / (2*g)**((i+j)//2))
    return tot

def prim_overlap(A, la, aa, B, lb, bb):
    g = aa+bb
    P = (aa*np.array(A)+bb*np.array(B))/g
    AB = np.array(A)-np.array(B)
    pre = math.exp(-aa*bb/g*AB.dot(AB))*(math.pi/g)**1.5
    out = pre
    for k in range(3):
        out *= s1d(la[k], lb[k], P[k]-A[k], P[k]-B[k], g)
    return out

class AO:
    def __init__(self, center, lxyz, alphas, coefs):
        self.c, self.l, self.a, self.d = center, lxyz, alphas, coefs
def overlap(u, v):
    t = 0.0
    for ai, di in zip(u.a, u.d):
        for bj, dj in zip(v.a, v.d):
            t += di*dj*prim_overlap(u.c, u.l, ai, v.c, v.l, bj)
    return t

# ---- read the GRAPHF file ----------------------------------------------
lines = open(sys.argv[1]).read().splitlines()
natom = int(lines[0].split()[0])
atoms = []
for i in range(1, 1+natom):
    f = lines[i].split()
    atoms.append((int(f[0]), [float(f[1])/BOHR, float(f[2])/BOHR, float(f[3])/BOHR]))
zetas = []
for i in range(1+natom, 1+2*natom):
    zetas.append([float(x) for x in lines[i].split()])

# principal quantum numbers: MOPAC's NPQ per element (s,p,d)
NPQ = {1:(1,2,3), 6:(2,2,3), 7:(2,2,3), 8:(2,2,3), 16:(3,3,3)}
P = [(1,0,0),(0,1,0),(0,0,1)]
aos, labels = [], []
for (z, xyz), zt in zip(atoms, zetas):
    ns, npp, nd = NPQ[z]
    if zt[0] > 0:
        al, c, _ = expand(ns, 0, zt[0]); aos.append(AO(xyz,(0,0,0),al,c)); labels.append("%d S"%z)
    if zt[1] > 0:
        for k,lab in enumerate(("Px","Py","Pz")):
            al, c, _ = expand(npp, 1, zt[1]); aos.append(AO(xyz,P[k],al,c)); labels.append("%d %s"%(z,lab))

n = len(aos)
S = np.array([[overlap(aos[i], aos[j]) for j in range(n)] for i in range(n)])
d = np.sqrt(np.diag(S)); S = S/np.outer(d,d)        # normalise each AO
ev, U = np.linalg.eigh(S)
Sinvhalf = U @ np.diag(ev**-0.5) @ U.T

# ---- MOPAC's own S^(-1/2) ----------------------------------------------
txt = open(sys.argv[1]).read()
blk = txt.split("INVERSE_MATRIX[%dx%d]=" % (n,n))[1].split("Keywords")[0]
vals = [float(x.replace('D','E')) for x in re.findall(r'-?\d\.\d+D[-+]\d+', blk)]
M = np.zeros((n,n)); k = 0
for i in range(n):
    for j in range(i+1):
        M[i,j] = M[j,i] = vals[k]; k += 1

print("AO order:", ", ".join(labels))
print("\n  max |S^-1/2(ours) - INVERSE_MATRIX(MOPAC)| = %.2e" % np.abs(Sinvhalf-M).max())
print("  largest matrix element                     = %.4f" % np.abs(M).max())
print("\n  ours vs MOPAC, the six largest entries:")
idx = np.dstack(np.unravel_index(np.argsort(-np.abs(M), axis=None), M.shape))[0][:6]
for i,j in idx:
    print("    [%d,%d]  %10.6f  %10.6f   diff %.2e" % (i,j,Sinvhalf[i,j],M[i,j],abs(Sinvhalf[i,j]-M[i,j])))
