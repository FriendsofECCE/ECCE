#!/usr/bin/env python3
"""compareGrids.py a.bin b.bin : max abs and max relative difference of two double grids."""
import os
import sys
import array


def load(p):
    a = array.array('d')
    with open(p, 'rb') as f:
        a.fromfile(f, os.path.getsize(p) // 8)
    return a


a, b = load(sys.argv[1]), load(sys.argv[2])
ma = max(abs(x - y) for x, y in zip(a, b))
mr = max(abs(x - y) / max(abs(x), 1e-3) for x, y in zip(a, b))
print("points %d  max|diff| %.3e  max rel(floor 1e-3) %.3e  range %.4f..%.4f"
      % (len(a), ma, mr, min(a), max(a)))
