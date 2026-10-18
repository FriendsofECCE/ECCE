"""
Analysis module for atomipy - provides functions for structural analysis like RDF,
coordination numbers, and unwrapping coordinates.
"""

import copy
import numpy as np
from .distances import dist_matrix, get_neighbor_list
from .transform import wrap_coordinates, cartesian_to_fractional, fractional_to_cartesian, get_cell_vectors
from .cell_utils import normalize_box, Cell2Box_dim

def unwrap_coordinates(atoms, Box, molid=None):
    """
    Unwrap atom coordinates to fix molecules split across periodic boundaries.
    
    This function shifts atoms within the same molecule such that they are
    clustered together according to the minimum image convention relative
    to the first atom of the molecule.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries.
    Box : list of float
        Box dimensions (1x3, 1x6, or 1x9).
    molid : int or list of int, optional
        If provided, only unwrap molecules with these IDs.
        
    Returns
    -------
    list of dict
        Atoms with unwrapped coordinates.
    """
    if not atoms:
        return []
    
    # Identify molecules
    molids = np.array([a['molid'] for a in atoms])
    unique_molids = np.unique(molids)
    
    if molid is not None:
        if isinstance(molid, int):
            unique_molids = [molid]
        else:
            unique_molids = molid

    # Create a copy to avoid modifying originals
    unwrapped_atoms = [a.copy() for a in atoms]
    
    # We need to handle this molecule by molecule
    for mid in unique_molids:
        indices = np.where(molids == mid)[0]
        if len(indices) <= 1:
            continue
            
        # Use the first atom as reference
        ref_idx = indices[0]
        ref_pos = np.array([unwrapped_atoms[ref_idx]['x'], 
                           unwrapped_atoms[ref_idx]['y'], 
                           unwrapped_atoms[ref_idx]['z']])
        
        for idx in indices[1:]:
            pos = np.array([unwrapped_atoms[idx]['x'], 
                            unwrapped_atoms[idx]['y'], 
                            unwrapped_atoms[idx]['z']])
            
            # Find the shift needed for minimum image relative to ref_pos
            diff = pos - ref_pos
            # Use wrap_coordinates logic or similar minimum image shift
            # For simplicity, we assume we want the nearest image
            # We can use the logic from distances.py if we want code reuse
            # But here we just need to shift current atom's coordinates
            
            # TODO: Full triclinic support for unwrapping if Box is 1x9
            # For now, let's start with orthogonal support and expand
            if len(Box) == 3:
                lx, ly, lz = Box
                shift_x = -lx * np.round(diff[0] / lx)
                shift_y = -ly * np.round(diff[1] / ly)
                shift_z = -lz * np.round(diff[2] / lz)
                
                unwrapped_atoms[idx]['x'] += shift_x
                unwrapped_atoms[idx]['y'] += shift_y
                unwrapped_atoms[idx]['z'] += shift_z
            elif len(Box) == 9:
                # Triclinic unwrapping (Gromacs Box_dim format: lx ly lz 0 0 xy 0 xz yz)
                lx, ly, lz = Box[0], Box[1], Box[2]
                xy, xz, yz = Box[5], Box[7], Box[8]
                
                # Z direction
                sz = np.round(diff[2] / lz)
                unwrapped_atoms[idx]['z'] -= sz * lz
                unwrapped_atoms[idx]['y'] -= sz * yz
                unwrapped_atoms[idx]['x'] -= sz * xz
                
                # Recalculate diff for Y
                diff = np.array([unwrapped_atoms[idx]['x'], unwrapped_atoms[idx]['y'], unwrapped_atoms[idx]['z']]) - ref_pos
                sy = np.round(diff[1] / ly)
                unwrapped_atoms[idx]['y'] -= sy * ly
                unwrapped_atoms[idx]['x'] -= sy * xy
                
                # Recalculate diff for X
                diff = np.array([unwrapped_atoms[idx]['x'], unwrapped_atoms[idx]['y'], unwrapped_atoms[idx]['z']]) - ref_pos
                sx = np.round(diff[0] / lx)
                unwrapped_atoms[idx]['x'] -= sx * lx
                
    return unwrapped_atoms

def calculate_rdf(atoms, Box, rmax=15.0, dr=0.1, atom_types=None, pair_types=None, typeA=None, typeB=None, return_cn=False):
    """
    Calculate the Radial Distribution Function g(r) for specified atom pairs.

    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries.
    Box : list of float
        Box dimensions.
    rmax : float
        Maximum distance for RDF.
    dr : float
        Bin width for RDF.
    atom_types : list of str, optional
        Filter atoms by these types for both particles in the pair.
    pair_types : tuple of lists, optional
        (types_A, types_B) to compute RDF between elements of A and B.
    return_cn : bool
        If True, also return the running coordination number n(r) — the average
        number of B atoms within radius r of an A atom (cumulative integral of g(r)).

    Returns
    -------
    tuple
        (r, g_r) — or (r, g_r, n_r) when return_cn=True.
    """
    _empty = (np.array([]), np.array([]), np.array([])) if return_cn else (np.array([]), np.array([]))
    if not atoms:
        return _empty

    # Map typeA/typeB to pair_types if provided
    if typeA and typeB:
        pair_types = ([typeA] if isinstance(typeA, str) else typeA, 
                      [typeB] if isinstance(typeB, str) else typeB)
    elif typeA:
        atom_types = [typeA] if isinstance(typeA, str) else typeA

    if pair_types:
        types_a, types_b = pair_types
        indices_a = [i for i, a in enumerate(atoms) if a.get('type') in types_a]
        indices_b = [i for i, a in enumerate(atoms) if a.get('type') in types_b]
        is_cross = True
    elif atom_types:
        indices_a = [i for i, a in enumerate(atoms) if a.get('type') in atom_types]
        indices_b = indices_a
        is_cross = False
    else:
        indices_a = list(range(len(atoms)))
        indices_b = indices_a
        is_cross = False

    if not indices_a or not indices_b:
        return _empty

    # Use dist_matrix for all-to-all distances
    # For large systems, we should use cell_list for performance, but dist_matrix is easier for RDF
    # actually distances.py has get_neighbor_list which is better for large systems

    # dist_matrix uses single-image minimum-image distances, so g(r) is only valid out to
    # half the shortest box edge; beyond that periodic images are missed and the spherical
    # shell exceeds the box. Cap rmax to that limit (with a warning) rather than returning
    # silently-wrong large-r values.
    if len(Box) == 9:
        _edges = (Box[0], Box[1], Box[2])
    elif len(Box) == 3:
        _edges = (Box[0], Box[1], Box[2])
    else:
        from .cell_utils import Cell2Box_dim
        _bd = Cell2Box_dim(Box)
        _edges = (_bd[0], _bd[1], _bd[2])
    _rmax_valid = 0.5 * min(_edges)
    if rmax > _rmax_valid + 1e-9:
        import warnings
        warnings.warn(
            f"calculate_rdf: rmax={rmax:.3g} A exceeds half the shortest box edge "
            f"({_rmax_valid:.3g} A); capping to that limit (minimum-image g(r) is only "
            f"valid to L/2).", stacklevel=2)
        rmax = _rmax_valid

    # Construct histogram
    bins = np.arange(0, rmax + dr, dr)
    hist = np.zeros(len(bins) - 1)
    
    # Average density rho = N/V
    # Get box volume
    if len(Box) == 3:
        vol = np.prod(Box)
    elif len(Box) == 9:
        vol = Box[0] * Box[1] * Box[2]
    else:
        # Fallback for 1x6 Cell
        from .cell_utils import Cell2Box_dim
        bdim = Cell2Box_dim(Box)
        vol = bdim[0] * bdim[1] * bdim[2]
        
    rho = len(indices_b) / vol
    
    # Calculate distances
    # If system is small, use full matrix
    if len(atoms) < 5000:
        dists, _, _, _ = dist_matrix(atoms, Box)
        # Subset to indices
        subset_dists = dists[np.ix_(indices_a, indices_b)]
        
        # Flatten and filter
        valid_dists = subset_dists[(subset_dists > 1e-7) & (subset_dists <= rmax)]
        h, _ = np.histogram(valid_dists, bins=bins)
        hist += h
    else:
        # For large systems, use neighbor list to avoid memory overflow
        # get_neighbor_list returns (i, j, d, dx, dy, dz)
        # We might need to iterate over indices_a if it's too big
        # For now, let's assume dist_matrix is used as a baseline and we optimizes later
        dists, _, _, _ = dist_matrix(atoms, Box)
        subset_dists = dists[np.ix_(indices_a, indices_b)]
        valid_dists = subset_dists[(subset_dists > 1e-7) & (subset_dists <= rmax)]
        h, _ = np.histogram(valid_dists, bins=bins)
        hist += h

    # Normalize RDF
    r = bins[:-1] + dr/2
    # shell volume V(r) = 4/3 * pi * ((r+dr)^3 - r^3)
    shell_v = 4/3 * np.pi * (bins[1:]**3 - bins[:-1]**3)
    
    # g(r) = hist(r) / (N_a * rho * V(r))
    # where N_a is number of atoms in type A
    g_r = hist / (len(indices_a) * rho * shell_v)

    if return_cn:
        # Running coordination number: average number of B within r of an A atom.
        # Exact from the same pair histogram: cumulative pair count / N_a.
        n_r = np.cumsum(hist) / len(indices_a)
        return r, g_r, n_r

    return r, g_r

def density_profile(atoms, Box, axis='z', nbins=100, atom_types=None, mode='number'):
    """Compute a 1-D density profile along a box axis (averaged over the slab cross-section).

    Parameters
    ----------
    atoms : list of dict
        Atom dictionaries with 'x','y','z' (and 'type'/'element' for filtering/mass).
    Box : list of float
        Box dimensions (1x3, 1x6 Cell, or 1x9 Box_dim).
    axis : {'x','y','z'}
        Axis along which to bin.
    nbins : int
        Number of slabs.
    atom_types : list of str, optional
        Restrict to these atom names (matched against 'type' or 'element').
    mode : {'number','mass','charge'}
        'number' -> atoms/Å³, 'mass' -> g/cm³ (via atomic masses), 'charge' -> e/Å³.

    Returns
    -------
    (centers, density) : tuple of numpy.ndarray
        Slab centers (Å along the axis) and density values.
    """
    if not atoms:
        return np.array([]), np.array([])

    ai = {'x': 0, 'y': 1, 'z': 2}[axis]

    # Orthogonal box lengths (good enough for the slab cross-section area)
    if len(Box) >= 9:
        L = [Box[0], Box[1], Box[2]]
    elif len(Box) == 6:
        bd = Cell2Box_dim(Box)
        L = [bd[0], bd[1], bd[2]]
    elif len(Box) == 3:
        L = [Box[0], Box[1], Box[2]]
    else:
        return np.array([]), np.array([])

    Laxis = float(L[ai])
    if Laxis <= 0:
        return np.array([]), np.array([])
    area = 1.0
    for j in range(3):
        if j != ai:
            area *= float(L[j])

    if atom_types:
        sel = [a for a in atoms if a.get('type') in atom_types or a.get('element') in atom_types]
    else:
        sel = list(atoms)
    if not sel:
        return np.array([]), np.array([])

    coords = np.mod(np.array([a[axis] for a in sel], dtype=float), Laxis)
    edges = np.linspace(0.0, Laxis, nbins + 1)
    binw = Laxis / nbins

    if mode == 'mass':
        from .mass import mass as _mass
        from .element import element as _element
        mtab = _mass()
        _tmp = [dict(a) for a in sel]
        _element(_tmp)  # fill 'element' for atoms that lack it
        w = np.array([float(mtab.get(a.get('element'), 0.0)) for a in _tmp], dtype=float)
    elif mode == 'charge':
        w = np.array([float(a.get('charge', 0.0)) for a in sel], dtype=float)
    else:
        w = None

    hist, _ = np.histogram(coords, bins=edges, weights=w)
    slab_v = area * binw  # Å³
    dens = hist / slab_v
    if mode == 'mass':
        dens = dens * 1.66053906660  # amu/Å³ -> g/cm³
    centers = edges[:-1] + binw / 2.0
    return centers, dens


def rdf_frames(frames, rmax=15.0, dr=0.1, typeA=None, typeB=None, atom_types=None, return_cn=False):
    """Ensemble-average g(r) over a list of (atoms, Box) frames (see calculate_rdf).

    With return_cn=True also returns the ensemble-averaged running coordination
    number n(r): (r, g_r, n_r).
    """
    accum = None
    accum_n = None
    r = None
    n = 0
    for atoms, Box in frames:
        res = calculate_rdf(atoms, Box, rmax=rmax, dr=dr,
                            typeA=typeA, typeB=typeB, atom_types=atom_types, return_cn=return_cn)
        gi = res[1]
        if gi is None or len(gi) == 0:
            continue
        if accum is None:
            accum = np.zeros_like(gi)
            r = res[0]
            if return_cn:
                accum_n = np.zeros_like(res[2])
        accum += gi
        if return_cn:
            accum_n += res[2]
        n += 1
    if n == 0:
        return (np.array([]), np.array([]), np.array([])) if return_cn else (np.array([]), np.array([]))
    if return_cn:
        return r, accum / n, accum_n / n
    return r, accum / n


def density_frames(frames, axis='z', nbins=100, atom_types=None, mode='number'):
    """Ensemble-average a density profile over a list of (atoms, Box) frames."""
    accum = None
    centers = None
    n = 0
    for atoms, Box in frames:
        c, d = density_profile(atoms, Box, axis=axis, nbins=nbins,
                               atom_types=atom_types, mode=mode)
        if d is None or len(d) == 0:
            continue
        if accum is None:
            accum = np.zeros_like(d)
            centers = c
        accum += d
        n += 1
    if n == 0:
        return np.array([]), np.array([])
    return centers, accum / n


def _box_lengths(Box):
    """Orthogonal box edge lengths [Lx, Ly, Lz] from a 1x3/1x6/1x9 box."""
    if len(Box) >= 9:
        return [float(Box[0]), float(Box[1]), float(Box[2])]
    if len(Box) == 6:
        bd = Cell2Box_dim(Box)
        return [float(bd[0]), float(bd[1]), float(bd[2])]
    if len(Box) == 3:
        return [float(Box[0]), float(Box[1]), float(Box[2])]
    return [0.0, 0.0, 0.0]


def _unwrap_trajectory(frames, idx):
    """'nojump' unwrap selected atoms across time (remove PBC jumps between frames).

    Returns an ndarray of shape (n_sel_atoms, n_frames, 3) of continuous coordinates.
    Assumes a constant atom ordering across frames (true for atomipy trajectories).
    """
    n = len(frames)
    m = len(idx)
    U = np.zeros((m, n, 3))
    atoms0 = frames[0][0]
    raw_prev = np.array([[atoms0[i]["x"], atoms0[i]["y"], atoms0[i]["z"]] for i in idx], dtype=float)
    U[:, 0, :] = raw_prev
    for t in range(1, n):
        atoms_t, box_t = frames[t]
        L = _box_lengths(box_t)
        raw = np.array([[atoms_t[i]["x"], atoms_t[i]["y"], atoms_t[i]["z"]] for i in idx], dtype=float)
        d = raw - raw_prev
        for c in range(3):
            if L[c] > 0:
                d[:, c] -= L[c] * np.round(d[:, c] / L[c])
        U[:, t, :] = U[:, t - 1, :] + d
        raw_prev = raw
    return U


def _select_indices(atoms, atom_types):
    if atom_types:
        return [i for i, a in enumerate(atoms) if a.get("type") in atom_types or a.get("element") in atom_types]
    return list(range(len(atoms)))


def _dim_components(dims):
    comp = [{"x": 0, "y": 1, "z": 2}[c] for c in str(dims).lower() if c in "xyz"]
    return comp or [0, 1, 2]


def msd(frames, atom_types=None, dims="xyz", origin_stride=1, dt=1.0, fit_lo=0.1, fit_hi=0.5):
    """Mean-square displacement with multiple time origins (restarts) + diffusion coefficient.

    Parameters
    ----------
    frames : list of (atoms, Box)
        Trajectory; coordinates are 'nojump'-unwrapped internally (PBC-aware).
    atom_types : list of str, optional
        Restrict to these atom names (matched against 'type' or 'element').
    dims : str
        Components to use: 'xyz' (3D isotropic), 'xy' (2D), 'z' (1D), etc.
    origin_stride : int
        Step between time origins (restarts). 1 = use every frame as an origin.
    dt : float
        Time between frames (ps); sets the lag axis and D units.
    fit_lo, fit_hi : float
        Fraction of the lag range used for the linear D fit (default middle 10–50%).

    Returns
    -------
    dict or None
        {'lags' (ps), 'msd' (Å²), 'D_A2_ps', 'D_cm2_s', 'D_1e9_m2_s', 'dim', 'n_atoms', 'n_frames'}
    """
    if not frames or len(frames) < 3:
        return None
    idx = _select_indices(frames[0][0], atom_types)
    if not idx:
        return None
    comp = _dim_components(dims)
    U = _unwrap_trajectory(frames, idx)[:, :, comp]  # (m, n, dim)
    n = U.shape[1]
    acc = np.zeros(n)
    cnt = np.zeros(n)
    stride = max(1, int(origin_stride))
    for t0 in range(0, n, stride):
        disp = U[:, t0:, :] - U[:, t0:t0 + 1, :]          # (m, n-t0, dim)
        sq = np.sum(disp * disp, axis=2)                   # (m, n-t0)
        acc[: n - t0] += np.mean(sq, axis=0)               # mean over atoms
        cnt[: n - t0] += 1
    msd_curve = acc / np.maximum(cnt, 1)
    lags = np.arange(n) * float(dt)
    lo = max(1, int(fit_lo * n))
    hi = max(lo + 2, int(fit_hi * n))
    hi = min(hi, n)
    dim = len(comp)
    slope = float(np.polyfit(lags[lo:hi], msd_curve[lo:hi], 1)[0]) if hi > lo + 1 else 0.0
    D = slope / (2.0 * dim)  # Å²/ps
    return {
        "lags": lags, "msd": msd_curve,
        "D_A2_ps": D, "D_cm2_s": D * 1e-4, "D_1e9_m2_s": D * 10.0,
        "dim": dim, "n_atoms": len(idx), "n_frames": n,
    }


def displacement_distribution(frames, atom_types=None, dims="xyz", lag=None, origin_stride=1, nbins=50):
    """Distribution of per-component displacements over a lag (van Hove self-part-like).

    Pools the signed displacement of each selected component (per `dims`) over all
    selected atoms and time origins at a fixed lag — Gaussian for normal diffusion.
    A Gaussian with the sample's mean/std is returned for overlay.

    Returns
    -------
    dict or None
        {'centers', 'pdf', 'gauss', 'mu', 'sigma', 'lag_frames', 'n_samples'}
    """
    if not frames or len(frames) < 3:
        return None
    idx = _select_indices(frames[0][0], atom_types)
    if not idx:
        return None
    comp = _dim_components(dims)
    U = _unwrap_trajectory(frames, idx)[:, :, comp]
    n = U.shape[1]
    if lag is None:
        lag = max(1, n // 2)
    lag = int(max(1, min(lag, n - 1)))
    stride = max(1, int(origin_stride))
    chunks = []
    for t0 in range(0, n - lag, stride):
        chunks.append((U[:, t0 + lag, :] - U[:, t0, :]).ravel())
    if not chunks:
        return None
    samples = np.concatenate(chunks)
    if samples.size == 0:
        return None
    rng = float(np.max(np.abs(samples))) or 1.0
    edges = np.linspace(-rng, rng, nbins + 1)
    pdf, _ = np.histogram(samples, bins=edges, density=True)
    centers = 0.5 * (edges[:-1] + edges[1:])
    mu = float(np.mean(samples))
    sigma = float(np.std(samples))
    if sigma > 0:
        gauss = np.exp(-((centers - mu) ** 2) / (2 * sigma ** 2)) / (sigma * np.sqrt(2 * np.pi))
    else:
        gauss = np.zeros_like(centers)
    return {"centers": centers, "pdf": pdf, "gauss": gauss, "mu": mu, "sigma": sigma,
            "lag_frames": lag, "n_samples": int(samples.size)}


def _fd_velocities(U, dt):
    """Central finite-difference velocities from unwrapped positions U (m, n, 3)."""
    n = U.shape[1]
    V = np.zeros_like(U)
    if n >= 3:
        V[:, 1:-1, :] = (U[:, 2:, :] - U[:, :-2, :]) / (2.0 * dt)
        V[:, 0, :] = (U[:, 1, :] - U[:, 0, :]) / dt
        V[:, -1, :] = (U[:, -1, :] - U[:, -2, :]) / dt
    elif n == 2:
        V[:, 0, :] = V[:, 1, :] = (U[:, 1, :] - U[:, 0, :]) / dt
    return V


def vacf(frames, atom_types=None, dt=1.0, origin_stride=1, max_lag=None, window=True):
    """Velocity autocorrelation function, power spectrum and Green-Kubo diffusion.

    NOTE: the trajectory stores POSITIONS only, so velocities are estimated by central
    finite difference of the (PBC-unwrapped) positions: v(t) = [r(t+dt) - r(t-dt)]/(2 dt).
    This is NOT the integrator's true velocity. Consequences:
      * the resolvable frequency range is capped at the Nyquist limit 1/(2 dt) — for a
        meaningful vibrational spectrum the trajectory must be saved every few fs;
      * the difference acts as a low-pass filter (amplitudes scale by sinc(w dt)), so
        high-frequency peaks are damped.
    The Green-Kubo diffusion (low-frequency / long-time) is robust; the power spectrum is
    only meaningful with fine output. For sharp spectra use true .trr velocities instead.

    Parameters
    ----------
    frames : list of (atoms, Box)
    atom_types : list of str, optional   Restrict to these atom names.
    dt : float                            Time between frames (ps).
    origin_stride : int                   Stride between time origins (restarts).
    max_lag : int, optional               Max VACF lag in frames (default n-1).
    window : bool                         Apply a decaying window before the FFT.

    Returns
    -------
    dict or None
        {'lags' (ps), 'vacf', 'vacf_norm', 'D_A2_ps','D_cm2_s','D_1e9_m2_s',
         'freq_thz','wavenumber_cm1','spectrum','nyquist_cm1','n_atoms','n_frames'}
    """
    if not frames or len(frames) < 4:
        return None
    idx = _select_indices(frames[0][0], atom_types)
    if not idx:
        return None
    dt = float(dt)
    U = _unwrap_trajectory(frames, idx)
    n = U.shape[1]
    V = _fd_velocities(U, dt)
    if max_lag is None:
        max_lag = n - 1
    max_lag = int(max(1, min(max_lag, n - 1)))
    C = np.zeros(max_lag + 1)
    cnt = np.zeros(max_lag + 1)
    stride = max(1, int(origin_stride))
    for t0 in range(0, n, stride):
        hi = min(n, t0 + max_lag + 1)
        d = np.einsum('ad,atd->at', V[:, t0, :], V[:, t0:hi, :])  # (m, hi-t0)
        s = d.mean(axis=0)
        C[: hi - t0] += s
        cnt[: hi - t0] += 1
    C = C / np.maximum(cnt, 1)
    lags = np.arange(max_lag + 1) * dt

    # Green-Kubo self-diffusion: D = (1/3) integral of the (3D) VACF over time.
    D = float(np.trapz(C, dx=dt) / 3.0)  # Å²/ps

    # Power spectrum (vibrational DOS) = FFT of the (windowed) VACF.
    L = len(C)
    if window and L > 1:
        w = 0.5 * (1.0 + np.cos(np.pi * np.arange(L) / (L - 1)))  # 1 at lag 0 -> 0 at max lag
        Cw = C * w
    else:
        Cw = C
    spec = np.abs(np.fft.rfft(Cw))
    freqs = np.fft.rfftfreq(L, d=dt)          # 1/ps == THz
    wn = freqs * 33.35641                      # cm^-1 (1 THz = 33.35641 cm^-1)
    spec_n = spec / spec.max() if spec.max() > 0 else spec
    nyq_cm1 = (1.0 / (2.0 * dt)) * 33.35641

    return {
        'lags': lags, 'vacf': C, 'vacf_norm': C / C[0] if C[0] != 0 else C,
        'D_A2_ps': D, 'D_cm2_s': D * 1e-4, 'D_1e9_m2_s': D * 10.0,
        'freq_thz': freqs, 'wavenumber_cm1': wn, 'spectrum': spec_n,
        'nyquist_cm1': nyq_cm1, 'n_atoms': len(idx), 'n_frames': n,
    }


def _mimg(d, L):
    """Minimum-image a displacement array d (...,3) for orthogonal box lengths L."""
    for c in range(3):
        if L[c] > 0:
            d[..., c] -= L[c] * np.round(d[..., c] / L[c])
    return d


def find_hbonds(atoms, Box, donor_types=None, acceptor_types=None,
                donor_resnames=None, acceptor_resnames=None,
                r_cut=3.5, angle_cut=30.0, exclude_same_molecule=True,
                dh_cut=1.3, elements=None):
    """Geometric hydrogen-bond detection for ONE frame (GROMACS gmx hbond convention).

    A hydrogen bond D-H...A requires D...A < r_cut AND the H-D...A angle (at the donor)
    <= angle_cut. Donors/acceptors are O/N/F by element, optionally restricted by atom
    name (donor_types / acceptor_types) AND/OR residue name (donor_resnames /
    acceptor_resnames). A filter left as None imposes no restriction on that dimension,
    so type=None and resname=None selects every O/N/F (the whole system). Each H is
    bonded to its nearest heavy O/N/F within dh_cut.

    Returns
    -------
    list of (donor_idx, h_idx, acceptor_idx)
    """
    if not atoms:
        return []
    if elements is None:
        from .element import element as _element
        ae = [dict(a) for a in atoms]
        _element(ae)
        elements = [a.get('element') for a in ae]
    # Normalize atomipy's water labels to base elements (Ow->O, Hw->H).
    elements = ['O' if e == 'Ow' else ('H' if e == 'Hw' else e) for e in elements]
    pos = np.array([[a['x'], a['y'], a['z']] for a in atoms], dtype=float)
    L = np.array(_box_lengths(Box), dtype=float)
    molid = [a.get('molid', i) for i, a in enumerate(atoms)]
    accept_el = {'O', 'N', 'F'}

    def _sel(i, types, resn):
        if types and atoms[i].get('type') not in types:
            return False
        if resn and atoms[i].get('resname') not in resn:
            return False
        return True

    acc_idx = [i for i in range(len(atoms))
               if elements[i] in accept_el and _sel(i, acceptor_types, acceptor_resnames)]
    don_ok = set(i for i in range(len(atoms))
                 if elements[i] in accept_el and _sel(i, donor_types, donor_resnames))
    H_idx = [i for i in range(len(atoms)) if elements[i] == 'H']
    heavy_all = [i for i in range(len(atoms)) if elements[i] in accept_el]
    if not acc_idx or not H_idx or not heavy_all:
        return []

    acc_arr = np.array(acc_idx)
    acc_pos = pos[acc_arr]
    heavy_arr = np.array(heavy_all)
    heavy_pos = pos[heavy_arr]

    hbonds = []
    for h in H_idx:
        r = np.sqrt((_mimg(heavy_pos - pos[h], L) ** 2).sum(1))
        j = int(np.argmin(r))
        if r[j] > dh_cut:
            continue
        D = int(heavy_arr[j])
        if D not in don_ok:
            continue
        rDA = np.sqrt((_mimg(acc_pos - pos[D], L) ** 2).sum(1))
        for ci in np.where(rDA < r_cut)[0]:
            A = int(acc_arr[ci])
            if A == D:
                continue
            if exclude_same_molecule and molid[A] == molid[D]:
                continue
            u = _mimg((pos[h] - pos[D]).reshape(1, 3).copy(), L)[0]
            v = _mimg((pos[A] - pos[D]).reshape(1, 3).copy(), L)[0]
            nu = np.linalg.norm(u); nv = np.linalg.norm(v)
            if nu == 0 or nv == 0:
                continue
            ang = np.degrees(np.arccos(np.clip(np.dot(u, v) / (nu * nv), -1.0, 1.0)))
            if ang <= angle_cut:
                hbonds.append((D, h, A))
    return hbonds


def hbonds_frames(frames, donor_types=None, acceptor_types=None,
                  donor_resnames=None, acceptor_resnames=None, r_cut=3.5,
                  angle_cut=30.0, exclude_same_molecule=True, max_count=8):
    """Hydrogen-bond analysis over a trajectory (or single structure).

    Returns total counts and the distribution of H-bonds per molecule (capturing
    single vs. multiple donor/acceptor participation). A molecule's count is the
    number of H-bonds it takes part in (as donor or acceptor).

    Returns
    -------
    dict or None
        {'n_frames', 'mean_total', 'mean_per_molecule', 'time_series',
         'dist_x' (#H-bonds), 'dist_y' (fraction of molecules)}
    """
    if not frames:
        return None
    from collections import defaultdict
    from .element import element as _element

    # Atom identity is constant across frames -> classify elements / capable molecules once.
    atoms0 = frames[0][0]
    ae = [dict(a) for a in atoms0]
    _element(ae)
    elements = ['O' if a.get('element') == 'Ow' else ('H' if a.get('element') == 'Hw' else a.get('element')) for a in ae]
    capable = set(atoms0[i].get('molid', i) for i in range(len(atoms0)) if elements[i] in ('O', 'N', 'F'))
    n_capable = max(len(capable), 1)

    dist = np.zeros(max_count + 1)
    time_series = []
    n = 0
    for atoms, Box in frames:
        hb = find_hbonds(atoms, Box, donor_types=donor_types, acceptor_types=acceptor_types,
                         donor_resnames=donor_resnames, acceptor_resnames=acceptor_resnames,
                         r_cut=r_cut, angle_cut=angle_cut, exclude_same_molecule=exclude_same_molecule,
                         elements=elements)
        time_series.append(len(hb))
        per_mol = defaultdict(int)
        for (D, _H, A) in hb:
            per_mol[atoms[D].get('molid')] += 1
            per_mol[atoms[A].get('molid')] += 1
        for m in capable:
            dist[min(per_mol.get(m, 0), max_count)] += 1
        n += 1
    if n == 0:
        return None
    dist = dist / (n * n_capable)
    return {
        'n_frames': n,
        'mean_total': float(np.mean(time_series)),
        'mean_per_molecule': float(np.sum(np.arange(max_count + 1) * dist)),
        'time_series': [int(x) for x in time_series],
        'dist_x': list(range(max_count + 1)),
        'dist_y': [float(x) for x in dist],
    }


def coordination_number(atoms, Box, cutoff=3.0, atom_types=None, neighbor_types=None, typeA=None, typeB=None):
    """
    Calculate the coordination number for each atom.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries.
    Box : list of float
        Box dimensions.
    cutoff : float, optional
        Distance cutoff for neighbors.
    atom_types : list of str, optional
        Filter target atoms.
    neighbor_types : list of str, optional
        Filter neighbor atoms.
        
    Returns
    -------
    list of int
        Coordination numbers for each atom in `atoms` (filtered or all).
    """
    # Map typeA/typeB to atom_types/neighbor_types if provided
    if typeA: atom_types = [typeA] if isinstance(typeA, str) else typeA
    if typeB: neighbor_types = [typeB] if isinstance(typeB, str) else typeB

    # Use neighbor list for performance
    i_idx, j_idx, d, _, _, _ = get_neighbor_list(atoms, Box, cutoff)
    
    cn = np.zeros(len(atoms), dtype=int)

    # get_neighbor_list returns an UNDIRECTED upper-triangle list: each pair (i, j) with
    # i < j appears once but represents two directed relations (i has neighbor j, and j
    # has neighbor i), so both endpoints normally get +1.
    if neighbor_types:
        neighbor_mask = np.array([a.get('type') in set(neighbor_types) for a in atoms])
        # Count j toward i's CN only when j is a neighbor_type, and i toward j's CN only
        # when i is a neighbor_type. This avoids crediting the neighbor atoms themselves
        # and avoids dropping pairs merely because of upper-triangle index ordering.
        count_arrays = (i_idx[neighbor_mask[j_idx]], j_idx[neighbor_mask[i_idx]])
    else:
        count_arrays = (i_idx, j_idx)

    for arr in count_arrays:
        unique, counts = np.unique(arr, return_counts=True)
        cn[unique] += counts

    if atom_types:
        target_indices = [i for i, a in enumerate(atoms) if a.get('type') in atom_types]
        return cn[target_indices].tolist()
        
    return cn.tolist()

def closest_atom(atoms, reference, Box=None):
    """
    Return the atom closest to a reference coordinate or atom.
    
    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries to search.
    reference : dict or list of float
        Either an atom dictionary or [x, y, z] coordinates.
    Box : list of float, optional
        Box dimensions for PBC distance.
        
    Returns
    -------
    dict
        The closest atom dictionary.
    """
    if not atoms:
        return None
        
    if isinstance(reference, dict):
        ref_pos = np.array([reference['x'], reference['y'], reference['z']])
    else:
        ref_pos = np.array(reference)
        
    positions = np.array([[a['x'], a['y'], a['z']] for a in atoms])
    diff = positions - ref_pos
    
    if Box is not None:
        # PBC correction
        if len(Box) == 3:
            diff -= np.round(diff / np.array(Box)) * np.array(Box)
        elif len(Box) == 9:
            # Simple orthogonal approximation for Box[0..2] if not handling full triclinic here
            # But let's use the logic from distances if possible or just implement orthogonal-lite
            lx, ly, lz = Box[0], Box[1], Box[2]
            diff[:, 0] -= lx * np.round(diff[:, 0] / lx)
            diff[:, 1] -= ly * np.round(diff[:, 1] / ly)
            diff[:, 2] -= lz * np.round(diff[:, 2] / lz)

    dists_sq = np.sum(diff**2, axis=1)
    min_idx = np.argmin(dists_sq)
    
    return atoms[min_idx]


def min_distances(atoms, Box, group_by='molid', n_pairs=10, cutoff=None):
    """
    Calculate the minimum distances between different molecules/residues.

    For each pair of distinct groups (molecules or residues), finds the
    single closest atom–atom contact, then returns the `n_pairs` shortest
    contacts sorted by distance.  Useful for detecting clashes after
    solvation or verifying inter-molecular separation.

    Parameters
    ----------
    atoms : list of dict
        List of atom dictionaries, each with at least 'x', 'y', 'z',
        'molid' (int), and 'resname' (str) keys.
    Box : list of float
        Box dimensions (1×3, 1×6, or 1×9).
    group_by : str
        'molid' (default) — group atoms by their molid integer.
        'resname' — group atoms by their residue name.
    n_pairs : int
        Maximum number of closest inter-group pairs to return (default 10).
    cutoff : float or None
        If given, only consider pairs whose minimum distance is ≤ cutoff Å.

    Returns
    -------
    list of dict
        Sorted list (ascending distance) of inter-group contacts.  Each
        entry has keys:
          'group_a'   – label of the first group
          'group_b'   – label of the second group
          'atom_a'    – index of the closest atom in group A
          'atom_b'    – index of the closest atom in group B
          'type_a'    – atom type of the closest atom in group A
          'type_b'    – atom type of the closest atom in group B
          'distance'  – minimum distance in Å (float, 2 d.p.)
    """
    if not atoms:
        return []

    # Determine box vectors for PBC
    if len(Box) == 3:
        lx, ly, lz = float(Box[0]), float(Box[1]), float(Box[2])
        box_vec = np.array([lx, ly, lz])
    elif len(Box) >= 9:
        lx, ly, lz = float(Box[0]), float(Box[1]), float(Box[2])
        box_vec = np.array([lx, ly, lz])
    elif len(Box) == 6:
        from .cell_utils import Cell2Box_dim
        bdim = Cell2Box_dim(Box)
        lx, ly, lz = float(bdim[0]), float(bdim[1]), float(bdim[2])
        box_vec = np.array([lx, ly, lz])
    else:
        lx, ly, lz = 1e9, 1e9, 1e9
        box_vec = None

    # Build groups
    key_func = (lambda a: a.get('molid', 0)) if group_by == 'molid' else (lambda a: a.get('resname', '?'))
    groups = {}
    for idx, atom in enumerate(atoms):
        key = key_func(atom)
        if key not in groups:
            groups[key] = []
        groups[key].append(idx)

    group_keys = list(groups.keys())
    n_groups = len(group_keys)
    if n_groups < 2:
        return []

    # Pre-build position array
    pos = np.array([[a['x'], a['y'], a['z']] for a in atoms], dtype=float)

    results = []
    for gi in range(n_groups):
        for gj in range(gi + 1, n_groups):
            key_a = group_keys[gi]
            key_b = group_keys[gj]
            idx_a = groups[key_a]
            idx_b = groups[key_b]

            pos_a = pos[idx_a]   # shape (Na, 3)
            pos_b = pos[idx_b]   # shape (Nb, 3)

            # Broadcast pairwise difference
            diff = pos_a[:, None, :] - pos_b[None, :, :]   # (Na, Nb, 3)

            # Minimum image convention (orthogonal approximation)
            if box_vec is not None:
                diff -= np.round(diff / box_vec) * box_vec

            dists = np.sqrt(np.sum(diff ** 2, axis=2))  # (Na, Nb)
            flat_min = np.argmin(dists)
            ia_local, ib_local = np.unravel_index(flat_min, dists.shape)
            d_min = float(dists[ia_local, ib_local])

            if cutoff is not None and d_min > cutoff:
                continue

            atom_a_idx = idx_a[ia_local]
            atom_b_idx = idx_b[ib_local]
            results.append({
                'group_a': key_a,
                'group_b': key_b,
                'atom_a': atom_a_idx,
                'atom_b': atom_b_idx,
                'type_a': atoms[atom_a_idx].get('type', '?'),
                'type_b': atoms[atom_b_idx].get('type', '?'),
                'distance': round(d_min, 2),
            })

    results.sort(key=lambda r: r['distance'])
    return results[:n_pairs]


def condense_supercell(atoms, Box, replicate, translate=None, pre_translate=None,
                       wrap=True, tol=0.05, merge_tol=None,
                       cif_file=None, title='Generated by atomipy'):
    """
    De-replicate a supercell into a unit cell with crystallographic displacement parameters.

    Accepts a single MD snapshot, a list of frames, or a path to a PDB/GRO
    trajectory file.  All frames are pooled before computing statistics, so
    ADPs reflect genuine thermal motion when a full trajectory is supplied.

    Parameters
    ----------
    atoms : list of dict | list of list of dict | str
        * **list of dict** — single supercell snapshot.
        * **list of list of dict** — multiple frames; ``Box`` is reused for
          every frame unless per-frame boxes are embedded (see below).
        * **str** — path to a ``.pdb`` or ``.gro`` trajectory file; the file
          is read with :func:`atomipy.import_conf.import_traj` and per-frame
          boxes are used automatically.
    Box : list
        Supercell dimensions: 1×3, 1×6 [a,b,c,α,β,γ], or 1×9 triclinic.
        Used as fallback when a frame carries no box information.  For a
        trajectory file this is used only if CRYST1/box lines are absent.
    replicate : list of int
        [nx, ny, nz] — replication factors used to build the supercell.
    translate : list of float, optional
        [dx, dy, dz] shift in **supercell** fractional coordinates applied
        after wrapping and before folding into the UC.  Moves the lattice
        origin, useful when a site of interest straddles a cell boundary.
        Default [0, 0, 0].
    pre_translate : list of float, optional
        Alias for ``translate``; ignored when ``translate`` is also given.
    wrap : bool, optional
        Wrap atoms drifted outside the periodic box before folding.
        Default True.
    tol : float, optional
        Grid spacing (UC fractional coords) for initial site clustering.
        Default 0.05.
    merge_tol : float or None, optional
        Cartesian distance (Å) below which two initial sites of the **same
        atom type** are merged into one.  Useful when noise or the 0/1
        boundary splits a single crystallographic site across two grid
        cells.  Default None (no merging).
    cif_file : str or None, optional
        Write the condensed UC to this CIF path after condensation,
        including U_iso, B_iso and anisotropic U_ij loops.  Default None.
    title : str, optional
        CIF data-block title.  Default 'Generated by atomipy'.

    Returns
    -------
    uc_atoms : list of dict
        One dict per unique crystallographic site with fields:

        ============  ==========================================================
        x, y, z       Mean Cartesian position in the UC (Å)
        xfrac/yfrac/zfrac  Mean fractional position
        occupancy     n_found / (n_frames · nx · ny · nz)
        U_iso         Isotropic ADP (Å²)
        B_iso         8π² · U_iso (Å²)
        U_11…U_23     Anisotropic ADP components for CIF (Å²)
        n_images      Total atom-images pooled onto this site
        ============  ==========================================================

    uc_Box : list
        UC Box_dim (1×9).
    uc_Cell : list
        UC cell parameters [a, b, c, α, β, γ].

    Notes
    -----
    **Processing order per frame:**

    1. Wrap into supercell box  (``wrap=True``)
    2. Translate in supercell fractional coords  (``translate``)
    3. Scale → UC fractional, wrap to [0, 1)
    4. Accumulate into site groups by ``(type, rounded UC frac)``

    **After all frames:**

    5. Optional: merge same-type sites within ``merge_tol`` Å
    6. Compute periodic mean, U_cart, U_iso, B_iso, U_ij per site

    **ADP formula:**

    .. code-block:: text

        U_cryst = M⁻¹ · U_cart · M⁻ᵀ          (dimensionless)
        U_ij (CIF) = a_i · a_j · U_cryst_ij    (Å²)

    where M has columns = UC cell vectors (Å), r_cart = M · r_frac.

    Examples
    --------
    Single snapshot::

        uc, Box, Cell = condense_supercell(atoms, Box, [2, 2, 1])

    PDB trajectory with CIF output::

        uc, Box, Cell = condense_supercell(
            'md_traj.pdb', Box, [4, 4, 2],
            wrap=True, merge_tol=0.5, cif_file='uc_adp.cif'
        )
    """
    # ------------------------------------------------------------------ #
    # 0. Normalise input → frames = list of (atoms_list, Box_or_None)      #
    # ------------------------------------------------------------------ #
    if isinstance(atoms, str):
        from . import import_conf as _ic
        frames = _ic.import_traj(atoms)          # list of (atoms, Box)
    elif atoms and isinstance(atoms[0], dict):
        frames = [(atoms, Box)]                  # single snapshot
    else:
        frames = [(frame, Box) for frame in atoms]  # list of frames

    n_frames = len(frames)
    if n_frames == 0:
        raise ValueError("No frames found in input.")

    # Handle pre_translate alias
    if translate is None:
        translate = pre_translate if pre_translate is not None else [0.0, 0.0, 0.0]
    translate = np.asarray(translate, dtype=float)

    nx, ny, nz = int(replicate[0]), int(replicate[1]), int(replicate[2])
    rep_vec = np.array([nx, ny, nz], dtype=float)

    # ------------------------------------------------------------------ #
    # 1. Cell parameters (from first frame with valid box, else fallback)  #
    # ------------------------------------------------------------------ #
    ref_box = next((b for _, b in frames if b is not None), Box)
    _, super_Cell = normalize_box(ref_box)
    if super_Cell is None:
        raise ValueError("No valid Box found in input or frames.")

    a_uc = super_Cell[0] / nx
    b_uc = super_Cell[1] / ny
    c_uc = super_Cell[2] / nz
    uc_Cell = [a_uc, b_uc, c_uc, super_Cell[3], super_Cell[4], super_Cell[5]]
    uc_Box_dim = Cell2Box_dim(uc_Cell)

    cell_vecs = get_cell_vectors(uc_Cell)   # rows are cell vectors
    M = cell_vecs.T                         # columns = a_vec, b_vec, c_vec
    M_inv = np.linalg.inv(M)

    # ------------------------------------------------------------------ #
    # 2. Accumulate UC fractional positions across all frames              #
    # ------------------------------------------------------------------ #
    n_grid = max(1, int(round(1.0 / tol)))

    def _site_key(atom, frac):
        label = atom.get('type') or atom.get('element') or atom.get('resname') or 'X'
        grid = tuple(round(f / tol) % n_grid for f in frac)
        return (label, grid)

    def _clean(arr):
        arr[np.abs(arr - 1.0) < 1e-9] = 0.0
        return arr

    groups = {}   # key → {'proto': atom, 'fracs': list of np.array(3)}

    for frame_atoms, frame_box in frames:
        fb = frame_box if frame_box is not None else ref_box
        _, f_cell = normalize_box(fb)
        if f_cell is None:
            f_cell = super_Cell

        fs: np.ndarray = np.asarray(
            cartesian_to_fractional(frame_atoms, Box=f_cell, add_to_atoms=False)
        )
        if wrap:
            fs = _clean(fs % 1.0)
        if np.any(translate != 0.0):
            fs = _clean((fs + translate) % 1.0)

        fuc = _clean((fs * rep_vec) % 1.0)

        for i, atom in enumerate(frame_atoms):
            key = _site_key(atom, fuc[i])
            if key not in groups:
                groups[key] = {'proto': atom, 'fracs': []}
            groups[key]['fracs'].append(fuc[i])

    # ------------------------------------------------------------------ #
    # 3. Optional: merge nearby same-type sites                            #
    # ------------------------------------------------------------------ #
    if merge_tol is not None:
        # Compute current mean position for every group
        def _mean_frac(fracs):
            arr = np.array(fracs)
            ref = arr[0].copy()
            d = arr - ref
            d -= np.round(d)
            return ((ref + d).mean(axis=0)) % 1.0

        changed = True
        while changed:
            changed = False
            keys = list(groups.keys())
            for i in range(len(keys)):
                if keys[i] not in groups:
                    continue
                for j in range(i + 1, len(keys)):
                    if keys[j] not in groups:
                        continue
                    # Only merge same atom type
                    if keys[i][0] != keys[j][0]:
                        continue
                    m1 = _mean_frac(groups[keys[i]]['fracs'])
                    m2 = _mean_frac(groups[keys[j]]['fracs'])
                    d = m1 - m2
                    d -= np.round(d)              # minimum image
                    dist_cart = float(np.linalg.norm(M @ d))
                    if dist_cart < merge_tol:
                        groups[keys[i]]['fracs'].extend(groups[keys[j]]['fracs'])
                        del groups[keys[j]]
                        changed = True
                        break
                if changed:
                    break

    # ------------------------------------------------------------------ #
    # 4. Compute crystallographic quantities for each site                 #
    # ------------------------------------------------------------------ #
    total_images = n_frames * nx * ny * nz
    cell_lengths = np.array([a_uc, b_uc, c_uc])
    uc_atoms = []

    for atom_idx, (key, grp) in enumerate(groups.items(), start=1):
        fracs = np.array(grp['fracs'])   # (n, 3)
        n = len(fracs)

        # Periodic mean
        ref = fracs[0].copy()
        d = fracs - ref
        d -= np.round(d)
        f_mean = ((ref + d).mean(axis=0)) % 1.0

        # Displacements from mean (minimum image) → Cartesian
        df = fracs - f_mean
        df -= np.round(df)
        dc = (M @ df.T).T                        # (n, 3) Å

        # Covariance in Cartesian (Å²)
        U_cart = (dc.T @ dc) / max(n, 1)
        U_iso  = float(np.trace(U_cart) / 3.0)
        B_iso  = 8.0 * np.pi**2 * U_iso

        # Crystallographic ADPs (Å²)
        U_cif = M_inv @ U_cart @ M_inv.T
        U_cif = U_cif * np.outer(cell_lengths, cell_lengths)

        r_cart = M @ f_mean

        out = copy.copy(grp['proto'])
        out['index']     = atom_idx
        out['xfrac']     = float(f_mean[0])
        out['yfrac']     = float(f_mean[1])
        out['zfrac']     = float(f_mean[2])
        out['x']         = float(r_cart[0])
        out['y']         = float(r_cart[1])
        out['z']         = float(r_cart[2])
        out['occupancy'] = n / total_images
        out['U_iso']     = U_iso
        out['B_iso']     = float(B_iso)
        out['U_11']      = float(U_cif[0, 0])
        out['U_22']      = float(U_cif[1, 1])
        out['U_33']      = float(U_cif[2, 2])
        out['U_12']      = float(U_cif[0, 1])
        out['U_13']      = float(U_cif[0, 2])
        out['U_23']      = float(U_cif[1, 2])
        out['n_images']  = n
        out['n_frames']  = n_frames
        uc_atoms.append(out)

    # ------------------------------------------------------------------ #
    # 5. Optionally write CIF                                              #
    # ------------------------------------------------------------------ #
    if cif_file is not None:
        from . import write_conf
        write_conf.cif(uc_atoms, uc_Cell, file_path=cif_file, title=title)

    return uc_atoms, uc_Box_dim, uc_Cell

