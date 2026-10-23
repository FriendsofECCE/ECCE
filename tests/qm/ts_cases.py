"""HCN <-> HNC transition state, shared by ts_test.py and gen_ts_oracle.py."""
import math

# Atoms 1 C, 2 N, 3 H; a guess a few tenths of an angstrom from the saddle point.
GUESS = [("C", 0.0, 0.0, 0.0), ("N", 1.18, 0.0, 0.0), ("H", 0.80, 1.20, 0.0)]

CASES = {
    "sto3g": dict(basis="STO-3G", quick=True),
    "631gs": dict(basis="6-31G*", quick=False),
}


def internals(xyz):
    """C-N, C-H, N-H (angstrom) and the H-C-N angle (degrees)."""
    d = lambda a, b: math.dist(xyz[a], xyz[b])
    cn, ch, nh = d(0, 1), d(0, 2), d(1, 2)
    ang = math.degrees(math.acos((cn * cn + ch * ch - nh * nh) / (2 * cn * ch)))
    return [cn, ch, nh, ang]
