import numpy as np


def place(a, b, c, bond, angle, tors):
    angle, tors = np.radians(angle), np.radians(tors)
    bc = c - b
    bc /= np.linalg.norm(bc)
    n = np.cross(b - a, bc)
    n /= np.linalg.norm(n)
    m = np.cross(n, bc)
    d = np.array([-bond * np.cos(angle), bond * np.sin(angle) * np.cos(tors),
                  bond * np.sin(angle) * np.sin(tors)])
    return c + d[0] * bc + d[1] * m + d[2] * n


seq = ["ALA", "GLY", "SER", "ALA"]
phi, psi, om = -120., 130., 180.
CA = np.array([0., 0., 0.])
C = np.array([1.52, 0., 0.])
N = place(C + np.array([0, 0, 1.]), C, CA, 1.46, 111., -60.)
atoms = []
for i, r in enumerate(seq):
    if i > 0:
        N = place(Np, CAp, Cp, 1.33, 116., psi)
        CA = place(CAp, Cp, N, 1.46, 122., om)
        C = place(Cp, N, CA, 1.52, 111., phi)
    res = [("N", N), ("CA", CA), ("C", C)]
    if r != "GLY":
        res.append(("CB", place(C, N, CA, 1.53, 110., 122.)))
    if r == "SER":
        res.append(("OG", place(N, CA, res[3][1], 1.42, 110., 60.)))
    res.append(("O", place(N, CA, C, 1.23, 121., psi + 180.)))
    if i == len(seq) - 1:
        res.append(("OXT", place(N, CA, C, 1.25, 117., psi)))
    atoms.append((r, res))
    Np, CAp, Cp = N, CA, C
k = 1
with open("pep.pdb", "w") as f:
    for i, (r, res) in enumerate(atoms):
        for nm, x in res:
            f.write("ATOM  %5d  %-3s %3s A%4d    %8.3f%8.3f%8.3f  1.00  0.00\n"
                    % (k, nm, r, i + 1, x[0], x[1], x[2]))
            k += 1
    f.write("END\n")
