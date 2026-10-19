"""Optional ctypes wrapper around GROMACS ``libxdrfile`` (v1.1.4) for .xtc/.trr I/O.

This mirrors what mxdrfile does for MATLAB (loadlibrary/calllib), but with Python's
``ctypes`` — so atomipy itself stays pure-Python: no C extension is compiled here, only
the small ``libxdrfile`` shared library needs to be present at runtime.

The library is looked up, in order, from:
  1. ``$ATOMIPY_XDRFILE`` (an explicit path to the shared library),
  2. ``ctypes.util.find_library("xdrfile")``,
  3. a few common SONAMEs (libxdrfile.so / .dylib / .dll).

If none is found, :func:`have_xdrfile` returns ``False`` and callers fall back (e.g. to
``gmx trjconv``). Build notes: see ``docs/libxdrfile.md``.

.xtc/.trr store coordinates and box vectors in **nanometres**; this module converts them
to **Ångström** (atomipy's unit) on the way out.
"""
import os
import sys
import glob
import ctypes
import ctypes.util

import numpy as np

# libxdrfile status codes (xdrfile.h)
_EXDR_OK = 0
_EXDR_EOF = 11

# C types: matrix = float[3][3], rvec = float[3]
_matrix = (ctypes.c_float * 3) * 3
_c_float_p = ctypes.POINTER(ctypes.c_float)

_LIB = None  # None = not yet tried; False = tried and unavailable; else the CDLL handle


def _candidate_libs():
    env = os.environ.get("ATOMIPY_XDRFILE")
    if env:
        yield env
    found = ctypes.util.find_library("xdrfile")
    if found:
        yield found
    # Conda/venv installs (e.g. conda-forge `xdrfile`) land in <prefix>/lib but that dir
    # isn't always on the loader path — glob it explicitly so the env-provided lib is found.
    prefixes = {sys.prefix, getattr(sys, "base_prefix", sys.prefix), os.environ.get("CONDA_PREFIX")}
    for base in filter(None, prefixes):
        for sub in ("lib", "lib64", os.path.join("Library", "bin")):
            for pat in ("libxdrfile*.so*", "libxdrfile*.dylib", "libxdrfile*.dll", "xdrfile*.dll"):
                for hit in sorted(glob.glob(os.path.join(base, sub, pat))):
                    yield hit
    for name in ("libxdrfile.so", "libxdrfile.so.4", "libxdrfile.so.2",
                 "libxdrfile.dylib", "libxdrfile.2.dylib",
                 "xdrfile.dll", "libxdrfile.dll"):
        yield name


def _configure(lib):
    """Declare argtypes/restypes (the ctypes analog of mxdrfile's fileheaders.m)."""
    lib.xdrfile_open.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
    lib.xdrfile_open.restype = ctypes.c_void_p
    lib.xdrfile_close.argtypes = [ctypes.c_void_p]
    lib.xdrfile_close.restype = ctypes.c_int

    lib.read_xtc_natoms.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int)]
    lib.read_xtc_natoms.restype = ctypes.c_int
    lib.read_xtc.argtypes = [ctypes.c_void_p, ctypes.c_int,
                             ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_float),
                             _matrix, _c_float_p, ctypes.POINTER(ctypes.c_float)]
    lib.read_xtc.restype = ctypes.c_int

    lib.read_trr_natoms.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int)]
    lib.read_trr_natoms.restype = ctypes.c_int
    lib.read_trr.argtypes = [ctypes.c_void_p, ctypes.c_int,
                             ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_float),
                             ctypes.POINTER(ctypes.c_float), _matrix,
                             _c_float_p, _c_float_p, _c_float_p]
    lib.read_trr.restype = ctypes.c_int


def load_libxdrfile():
    """Return the loaded ``libxdrfile`` CDLL, or ``None`` if it can't be found/loaded."""
    global _LIB
    if _LIB is not None:
        return _LIB or None
    for cand in _candidate_libs():
        try:
            lib = ctypes.CDLL(cand)
            # Probe a required symbol before committing to this handle.
            lib.read_xtc_natoms
            _configure(lib)
            _LIB = lib
            return lib
        except (OSError, AttributeError):
            continue
    _LIB = False
    return None


def have_xdrfile():
    """True if a usable ``libxdrfile`` is available."""
    return load_libxdrfile() is not None


def _box_to_list(box):
    """Convert a libxdrfile ``matrix`` (nm) to an atomipy Box list (Å).

    Returns ``[lx, ly, lz]`` for an orthogonal cell, else the triclinic box-dim form
    ``[lx, ly, lz, xy, xz, yz]`` (GROMACS lower-triangular convention).
    """
    m = [[float(box[i][j]) * 10.0 for j in range(3)] for i in range(3)]
    lx, ly, lz = m[0][0], m[1][1], m[2][2]
    xy, xz, yz = m[1][0], m[2][0], m[2][1]
    if abs(xy) < 1e-6 and abs(xz) < 1e-6 and abs(yz) < 1e-6:
        return [lx, ly, lz]
    return [lx, ly, lz, xy, xz, yz]


def _frame_wanted(idx, start, stop, stride):
    if idx < start:
        return False
    if stop is not None and idx >= stop:
        return False
    return (idx - start) % stride == 0


def read_xtc_frames(path, stride=1, start=0, stop=None):
    """Yield ``(coords_A, box_A)`` per frame of an .xtc file.

    ``coords_A`` is an (natoms, 3) float32 NumPy array in Ångström; ``box_A`` is a Box
    list in Ångström. Raises ``RuntimeError`` if the file can't be opened/read.
    """
    lib = load_libxdrfile()
    if lib is None:
        raise RuntimeError("libxdrfile is not available")
    natoms = ctypes.c_int()
    if lib.read_xtc_natoms(path.encode(), ctypes.byref(natoms)) != _EXDR_OK:
        raise RuntimeError(f"read_xtc_natoms failed for {path}")
    n = natoms.value
    xd = lib.xdrfile_open(path.encode(), b"r")
    if not xd:
        raise RuntimeError(f"could not open {path}")
    step, time, prec = ctypes.c_int(), ctypes.c_float(), ctypes.c_float()
    box = _matrix()
    x = np.zeros((n, 3), dtype=np.float32)
    xptr = x.ctypes.data_as(_c_float_p)
    idx = 0
    try:
        while True:
            rc = lib.read_xtc(xd, n, ctypes.byref(step), ctypes.byref(time), box, xptr, ctypes.byref(prec))
            if rc != _EXDR_OK:
                break  # EOF or read error -> stop
            if _frame_wanted(idx, start, stop, stride):
                yield x.copy() * 10.0, _box_to_list(box)
            idx += 1
            if stop is not None and idx >= stop:
                break
    finally:
        lib.xdrfile_close(xd)


def read_trr_frames(path, stride=1, start=0, stop=None):
    """Yield ``(coords_A, box_A)`` per frame of a .trr file (coordinates only).

    .trr also stores velocities/forces; those buffers are read but not returned here.
    """
    lib = load_libxdrfile()
    if lib is None:
        raise RuntimeError("libxdrfile is not available")
    natoms = ctypes.c_int()
    if lib.read_trr_natoms(path.encode(), ctypes.byref(natoms)) != _EXDR_OK:
        raise RuntimeError(f"read_trr_natoms failed for {path}")
    n = natoms.value
    xd = lib.xdrfile_open(path.encode(), b"r")
    if not xd:
        raise RuntimeError(f"could not open {path}")
    step = ctypes.c_int()
    time, lam = ctypes.c_float(), ctypes.c_float()
    box = _matrix()
    x = np.zeros((n, 3), dtype=np.float32)
    v = np.zeros((n, 3), dtype=np.float32)
    f = np.zeros((n, 3), dtype=np.float32)
    xp = x.ctypes.data_as(_c_float_p)
    vp = v.ctypes.data_as(_c_float_p)
    fp = f.ctypes.data_as(_c_float_p)
    idx = 0
    try:
        while True:
            rc = lib.read_trr(xd, n, ctypes.byref(step), ctypes.byref(time),
                              ctypes.byref(lam), box, xp, vp, fp)
            if rc != _EXDR_OK:
                break
            if _frame_wanted(idx, start, stop, stride):
                yield x.copy() * 10.0, _box_to_list(box)
            idx += 1
            if stop is not None and idx >= stop:
                break
    finally:
        lib.xdrfile_close(xd)
