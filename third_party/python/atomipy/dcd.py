"""Pure-Python reader for DCD trajectories (CHARMM / NAMD / OpenMM) — zero dependencies.

DCD is a Fortran-unformatted binary: every record is ``[int32 n][n payload bytes][int32 n]``.
Layout:

    header : 'CORD' + 20 int32 (icntrl)
    title  : NTITLE + NTITLE*80 chars
    natom  : one int32
    then per frame: an optional unit-cell record (6 float64) when the crystal flag
    (icntrl[10]) is set, followed by X, Y, Z coordinate records (natom float32 each).

Coordinates and cell lengths are already in Ångström (unlike .xtc/.trr which are nm).
Only the common case is supported (no fixed atoms, no 4th dimension); those raise a clear
error so the caller can fall back to another backend.
"""
import struct

import numpy as np


def _read_record(data, off, endian):
    (n,) = struct.unpack(endian + "i", data[off:off + 4]); off += 4
    payload = data[off:off + n]; off += n
    (n2,) = struct.unpack(endian + "i", data[off:off + 4]); off += 4
    if n != n2 or len(payload) != n:
        raise ValueError("corrupt DCD record (mismatched Fortran markers)")
    return payload, off


def _cell_to_box(doubles):
    """CHARMM cell record [A, f(gamma), B, f(beta), f(alpha), C] -> atomipy Box (Å).

    The angle slots are cos(angle) (|.| <= 1, e.g. OpenMM/CHARMM c36) or the angle in
    degrees (older CHARMM); handle both. Returns [lx,ly,lz] for an orthogonal cell, else
    the triclinic box-dim [lx,ly,lz,xy,xz,yz].
    """
    A, s1, B, s2, s3, C = (float(x) for x in doubles)

    def cosval(v):
        return v if -1.0 <= v <= 1.0 else float(np.cos(np.radians(v)))

    cg, cb, ca = cosval(s1), cosval(s2), cosval(s3)
    if abs(ca) < 1e-6 and abs(cb) < 1e-6 and abs(cg) < 1e-6:
        return [A, B, C]
    lx = A
    xy = B * cg
    ly = float(np.sqrt(max(0.0, B * B - xy * xy)))
    xz = C * cb
    yz = (B * C * ca - xy * xz) / ly if ly else 0.0
    lz = float(np.sqrt(max(0.0, C * C - xz * xz - yz * yz)))
    return [lx, ly, lz, xy, xz, yz]


def read_dcd_frames(path, stride=1, start=0, stop=None):
    """Yield ``(coords_A, box_A)`` per frame of a DCD file.

    ``coords_A`` is an (natoms, 3) float32 array in Ångström; ``box_A`` is a Box list in
    Ångström (or None if the file has no unit cell). Raises ValueError on an unsupported
    or non-DCD file.
    """
    with open(path, "rb") as fh:
        data = fh.read()
    if len(data) < 4:
        raise ValueError("not a DCD file (too short)")
    endian = "<" if struct.unpack("<i", data[:4])[0] == 84 else ">"
    if struct.unpack(endian + "i", data[:4])[0] != 84:
        raise ValueError("not a DCD file (bad header record marker)")

    off = 0
    hdr, off = _read_record(data, off, endian)
    if hdr[:4] != b"CORD":
        raise ValueError("not a DCD file (missing CORD magic)")
    icntrl = struct.unpack(endian + "20i", hdr[4:84])
    n_fixed = icntrl[8]
    has_cell = icntrl[10] != 0
    four_dim = icntrl[11] != 0
    if n_fixed:
        raise ValueError("DCD with fixed atoms is not supported")
    if four_dim:
        raise ValueError("4D DCD is not supported")

    _title, off = _read_record(data, off, endian)          # title block (ignored)
    natom_rec, off = _read_record(data, off, endian)
    natom = struct.unpack(endian + "i", natom_rec[:4])[0]
    fdt = np.dtype(endian + "f4")

    stride = max(1, int(stride))
    idx = 0
    while off < len(data):
        box = None
        try:
            if has_cell:
                cell, off = _read_record(data, off, endian)
                if len(cell) >= 48:
                    box = _cell_to_box(struct.unpack(endian + "6d", cell[:48]))
            xr, off = _read_record(data, off, endian)
            yr, off = _read_record(data, off, endian)
            zr, off = _read_record(data, off, endian)
        except (struct.error, ValueError):
            break  # trailing/partial data -> stop cleanly
        if idx >= start and (stop is None or idx < stop) and (idx - start) % stride == 0:
            x = np.frombuffer(xr, dtype=fdt, count=natom)
            y = np.frombuffer(yr, dtype=fdt, count=natom)
            z = np.frombuffer(zr, dtype=fdt, count=natom)
            coords = np.ascontiguousarray(np.stack([x, y, z], axis=1), dtype=np.float32)
            yield coords, box
        idx += 1
        if stop is not None and idx >= stop:
            break
