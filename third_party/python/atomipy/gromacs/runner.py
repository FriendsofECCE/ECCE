"""Local GROMACS runner: detect gmx, stage a run dir, run grompp/mdrun, stream log.

This is the engine layer for the local GROMACS path. The structure + topology
are produced by the usual atomipy writers (write_gro / write_gmx_top); this
module copies the ``min.ff`` force field into the run dir, writes the .mdp, and
drives ``gmx grompp``/``gmx mdrun`` as subprocesses whose merged stdout/stderr
is yielded line-by-line (for SSE streaming in the web app).
"""

import os
import re
import glob
import shutil
import subprocess
from pathlib import Path

from .mdp import mdp as _mdp_text, build_defines

# Canonical force-field dir shipped with atomipy: atomipy/ffparams/min.ff
_MINFF_SRC = Path(__file__).resolve().parent.parent / "ffparams" / "min.ff"


def write_freeze_ndx(workdir, n_frozen, n_total, *, group="frozen", ndx="index.ndx"):
    """Write a GROMACS index file for freezing the first ``n_frozen`` atoms.

    Used by the frozen "Dummy FF" mineral path: the framework atoms are written
    FIRST in the .gro/.top (see :func:`atomipy.write_dummy_system_top`), so they are
    the contiguous 1-based indices ``1..n_frozen``. The file holds two groups:

      * ``System`` — all atoms (so the .mdp's ``tc-grps = System`` still resolves
        when grompp is given this file with ``-n``);
      * ``group`` (default ``frozen``) — the framework, referenced by ``freezegrps``.

    Returns the ndx filename (relative to ``workdir``).
    """
    def _emit(name, idxs):
        out = [f"[ {name} ]"]
        for i in range(0, len(idxs), 15):
            out.append(" ".join(str(x) for x in idxs[i:i + 15]))
        return out
    lines = _emit("System", list(range(1, int(n_total) + 1)))
    lines += _emit(group, list(range(1, int(n_frozen) + 1)))
    (Path(workdir) / ndx).write_text("\n".join(lines) + "\n", encoding="utf-8")
    return ndx


def write_index_ndx(workdir, groups, *, ndx="index.ndx", n_total=None):
    """Write a GROMACS index file from ``{group_name: [1-based atom indices]}``.

    General version of :func:`write_freeze_ndx` — used to define the pull groups for
    umbrella sampling (e.g. two molecules whose COM distance is the reaction coordinate).
    If ``n_total`` is given, a ``[ System ]`` group of all atoms is written first. Returns
    the ndx filename (relative to ``workdir``).
    """
    def _emit(name, idxs):
        out = [f"[ {name} ]"]
        idxs = list(idxs)
        for i in range(0, len(idxs), 15):
            out.append(" ".join(str(int(x)) for x in idxs[i:i + 15]))
        return out
    lines = []
    if n_total is not None:
        lines += _emit("System", list(range(1, int(n_total) + 1)))
    for name, idxs in groups.items():
        lines += _emit(name, idxs)
    (Path(workdir) / ndx).write_text("\n".join(lines) + "\n", encoding="utf-8")
    return ndx


def group_indices(atoms, *, molid=None, resname=None, element=None, atom_type=None):
    """Return the 1-based atom indices in ``atoms`` matching ALL given criteria.

    Convenience for building pull groups from an atomipy atom list — e.g.
    ``group_indices(atoms, molid=1)`` (one molecule/slab) or
    ``group_indices(atoms, resname='MOL')`` (an adsorbate). Criteria left as ``None`` are
    ignored; ``molid``/``resname``/``element``/``atom_type`` may be a value or a set/list.
    """
    def _match(val, crit):
        if crit is None:
            return True
        if isinstance(crit, (set, list, tuple)):
            return val in crit
        return val == crit
    out = []
    for i, a in enumerate(atoms, start=1):
        el = a.get("element") or a.get("type")
        if (_match(a.get("molid"), molid) and _match(a.get("resname"), resname)
                and _match(el, element) and _match(a.get("type"), atom_type)):
            out.append(i)
    return out


def _default_gmx():
    """Preferred default GROMACS when the user hasn't specified a path.

    Prefers a real GROMACS *install* (its ``GMXRC``) over a bare ``gmx`` on PATH —
    on macOS the PATH ``gmx`` is usually the Homebrew build, which the user may not
    want. Search order:

      1. ``$ATOMIPY_GMX_PATH`` (explicit override: gmx binary / GMXRC / install dir)
      2. common install locations, e.g. ``/usr/local/gromacs*/bin/GMXRC`` (latest)

    Returns a spec string for the existing install, or ``None`` to fall back to the
    PATH ``gmx`` (e.g. on Colab / the online server, where no such install exists).
    """
    override = os.environ.get("ATOMIPY_GMX_PATH") or os.environ.get("ATOMIPY_GMX")
    if override and Path(override).exists():
        return override
    patterns = (
        "/usr/local/gromacs*/bin/GMXRC",
        "/opt/gromacs*/bin/GMXRC",
        "/usr/local/gromacs/bin/GMXRC",
    )
    found = []
    for pat in patterns:
        found.extend(glob.glob(pat))
    if not found:
        return None

    def _ver_key(path):
        # Natural version order so gromacs-2024.2 > gromacs-2024-beta > unversioned.
        m = re.search(r"gromacs[-/]?([0-9]+(?:\.[0-9]+)*)", path)
        return tuple(int(x) for x in m.group(1).split(".")) if m else (0,)

    found.sort(key=_ver_key)      # prefer the highest version
    return found[-1]


def _resolve_gmx(spec):
    """Resolve a user GROMACS spec to (gmx_command, lib_dirs).

    ``spec`` may be: '' or 'gmx' (auto: a detected install, else PATH), a path to
    the gmx binary, a path to a GMXRC script, or a GROMACS install/bin directory.
    lib_dirs are the install's library folders, added to the loader path so a custom
    build finds its .so/.dylib.
    """
    if not spec or spec == "gmx":
        spec = _default_gmx()
        if not spec:
            return "gmx", []
        # fall through and resolve the detected install (GMXRC / dir / binary)
    p = Path(spec)
    if p.name.startswith("GMXRC"):          # .../bin/GMXRC  -> .../bin/gmx
        bindir = p.parent
        return str(bindir / "gmx"), _lib_dirs(bindir.parent)
    if p.is_dir():                          # install dir or bin dir
        if (p / "gmx").exists():
            return str(p / "gmx"), _lib_dirs(p.parent)
        if (p / "bin" / "gmx").exists():
            return str(p / "bin" / "gmx"), _lib_dirs(p)
        return str(p / "gmx"), _lib_dirs(p.parent)
    # a file (gmx binary) or a bare command name
    root = p.parent.parent if p.parent.name == "bin" else p.parent
    return spec, _lib_dirs(root)


def _lib_dirs(root):
    return [str(root / d) for d in ("lib", "lib64") if (root / d).is_dir()]


def _gmx_env(libs, base=None):
    """Copy the env and prepend the custom GROMACS lib dirs to the loader path."""
    env = dict(base if base is not None else os.environ)
    env.setdefault("GMX_MAXBACKUP", "-1")
    if libs:
        joined = os.pathsep.join(libs)
        for var in ("LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH"):
            env[var] = joined + (os.pathsep + env[var] if env.get(var) else "")
    return env


def detect_gmx(gmx="gmx"):
    """Return {'path','version','command','libs'} for a usable gmx, or None.

    ``gmx`` may be 'gmx' (PATH) or a custom binary/GMXRC/install-dir path.
    """
    cmd, libs = _resolve_gmx(gmx)
    path = shutil.which(cmd) or (cmd if os.path.isfile(cmd) and os.access(cmd, os.X_OK) else None)
    if not path:
        return None
    try:
        out = subprocess.run([path, "--version"], capture_output=True, text=True,
                             encoding="utf-8", errors="replace", timeout=30,
                             env=_gmx_env(libs))
        blob = (out.stdout or "") + (out.stderr or "")
        m = re.search(r"GROMACS version:?\s*(\S+)", blob)
        version = m.group(1) if m else "unknown"
    except Exception:
        version = "unknown"
    return {"path": path, "version": version, "command": gmx, "libs": libs}


def _active_atomtypes(ffdir, defines):
    """Set of [atomtypes] names active in ffnonbonded.itp under the given defines.

    Honors #ifdef/#ifndef/#else/#endif so the result reflects which mineral
    parameter block (GMINFF_k* vs CLAYFF_EXT, ion sets, …) is selected. Returns
    None if ffnonbonded.itp is absent.
    """
    fn = Path(ffdir) / "ffnonbonded.itp"
    if not fn.exists():
        return None
    defset = {(d[2:] if d.startswith("-D") else d) for d in (defines or [])}
    active, in_atomtypes, stack = set(), False, []  # stack: (kind, name)

    def ok():
        for kind, name in stack:
            if kind == "ifdef" and name not in defset:
                return False
            if kind == "ifndef" and name in defset:
                return False
        return True

    for raw in fn.read_text(encoding="utf-8", errors="ignore").splitlines():
        s = raw.strip()
        if s.startswith("#ifdef"):
            stack.append(("ifdef", s.split()[1])); continue
        if s.startswith("#ifndef"):
            stack.append(("ifndef", s.split()[1])); continue
        if s.startswith("#else") and stack:
            k, n = stack[-1]; stack[-1] = ("ifndef" if k == "ifdef" else "ifdef", n); continue
        if s.startswith("#endif"):
            if stack: stack.pop()
            continue
        if s.startswith("["):
            in_atomtypes = s.lower().startswith("[ atomtypes")
            continue
        if in_atomtypes and s and not s.startswith(";") and ok():
            active.add(s.split()[0])
    return active


def _sanitize_minff(ffdir, defines=None):
    """Comment out ffbonded.itp lines that reference an undeclared atom type.

    The shipped min.ff has a few bonded/angle entries whose atom types aren't
    declared under every parameter set — e.g. generic water Ow/Hw (no model
    declares them) or Feo2/Feo3 (GMINFF-only; CLAYFF_EXT declares generic Feo).
    grompp fatally rejects these even when the types are unused. When ``defines``
    is given we strip lines referencing any type not active under those defines
    (define-aware); otherwise we fall back to the legacy Ow/Hw check. We patch
    only the *run-dir copy* — the canonical FF is untouched. Returns #lines cut.
    """
    fb = Path(ffdir) / "ffbonded.itp"
    if not fb.exists():
        return 0
    active = _active_atomtypes(ffdir, defines) if defines else None
    defset = {(d[2:] if d.startswith("-D") else d) for d in (defines or [])}
    out, n, stack = [], 0, []  # stack: (kind, name) for #ifdef/#ifndef

    def guards_ok():
        # Mirror grompp's preprocessor: only consider a line if its enclosing
        # #ifdef/#ifndef guards are satisfied by the active defines. So a properly
        # #ifdef-guarded orphan is left alone (grompp already skips it).
        for kind, name in stack:
            if kind == "ifdef" and name not in defset:
                return False
            if kind == "ifndef" and name in defset:
                return False
        return True

    for ln in fb.read_text(encoding="utf-8", errors="ignore").splitlines():
        s = ln.strip()
        if s.startswith("#ifdef"):
            stack.append(("ifdef", s.split()[1])); out.append(ln); continue
        if s.startswith("#ifndef"):
            stack.append(("ifndef", s.split()[1])); out.append(ln); continue
        if s.startswith("#else") and stack:
            k, nm = stack[-1]; stack[-1] = ("ifndef" if k == "ifdef" else "ifdef", nm); out.append(ln); continue
        if s.startswith("#endif"):
            if stack: stack.pop()
            out.append(ln); continue
        if s and not s.startswith(";") and not s.startswith("[") and not s.startswith("#") and guards_ok():
            toks = re.split(r"\s+", s)
            type_toks = []
            for t in toks:                       # leading non-integer tokens = atom types
                if re.fullmatch(r"-?\d+", t):
                    break
                type_toks.append(t)
            if type_toks:
                bad = (any(t not in active for t in type_toks) if active is not None
                       else ("Ow" in type_toks or "Hw" in type_toks))
                if bad:
                    out.append("; [atomipy-sanitized orphan type] " + ln)
                    n += 1
                    continue
        out.append(ln)
    if n:
        fb.write_text("\n".join(out) + "\n", encoding="utf-8")
    return n


def stage_run_dir(atoms, Box, workdir, *, write_top, write_gro,
                  minff_src=None, top_name="topol.top", gro_name="conf.gro",
                  sanitize=True):
    """Create the run dir: copy min.ff, write topology + coordinates.

    ``write_top`` / ``write_gro`` are passed in (the atomipy writers) to keep
    this module import-light and testable. ``write_top(atoms, Box, path)`` and
    ``write_gro(atoms, Box, path)`` are called.
    Returns {'workdir','top','gro','sanitized'}.
    """
    wd = Path(workdir)
    wd.mkdir(parents=True, exist_ok=True)
    src = Path(minff_src) if minff_src else _MINFF_SRC
    dst = wd / "min.ff"
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    sanitized = _sanitize_minff(dst) if sanitize else 0

    top_path = wd / top_name
    gro_path = wd / gro_name
    write_top(atoms, Box, str(top_path))
    write_gro(atoms, Box, str(gro_path))
    return {"workdir": str(wd), "top": str(top_path), "gro": str(gro_path), "sanitized": sanitized}


def stage_minff(workdir, *, minff_src=None, sanitize=True, defines=None):
    """Copy min.ff into an existing run dir (which already has top/gro/organic itps).

    Lighter than stage_run_dir: only the force field. Use when the topology and
    coordinates were already written into ``workdir`` (e.g. by the web-module
    codegen). ``defines`` (the active FF defines, e.g. ['GMINFF_k500','OPC3'] or
    ['CLAYFF_EXT', …]) makes sanitization define-aware. Returns #lines sanitized.
    """
    wd = Path(workdir)
    src = Path(minff_src) if minff_src else _MINFF_SRC
    dst = wd / "min.ff"
    if dst.exists():
        shutil.rmtree(dst)
    shutil.copytree(src, dst)
    return _sanitize_minff(dst, defines=defines) if sanitize else 0


def run_local_gmx(workdir, top, gro, stages, *, defines=None, gmx="gmx",
                  minff_src=None, do_stage_minff=True, san_defines=None,
                  on_line=print, **mdp_kwargs):
    """High-level local-GROMACS run for codegen: stage min.ff, run stages, log.

    The .top/.gro (and any organic .itp) must already exist in ``workdir``.
    Copies min.ff in, runs the stage pipeline chaining each .gro forward, and
    calls ``on_line(str)`` for every log line (so a plain print() streams to SSE).
    Returns the list of per-stage status dicts.
    """
    if do_stage_minff:
        n = stage_minff(workdir, minff_src=minff_src, defines=(san_defines if san_defines is not None else defines))
        if n:
            on_line(f"[atomipy] sanitized {n} orphan FF type line(s) in run-dir min.ff")
    statuses = []
    for item in run_pipeline(workdir, stages, gro, defines=defines, gmx=gmx, top=top, **mdp_kwargs):
        if isinstance(item, dict):
            statuses.append(item)
        else:
            on_line(item)
    return statuses


def trjconv_to_pdb(workdir, *, tpr, xtc, out, gmx="gmx", pbc="atom", skip=1,
                   group="System", on_line=None):
    """Convert a GROMACS trajectory (.xtc/.trr) to a multi-frame PDB via trjconv.

    Browser viewers (3Dmol/JSmol) can't read binary GROMACS trajectories, so we
    convert to a multi-MODEL PDB — the same format the OpenMM path produces.
    ``group`` is the trjconv output group (fed on stdin), e.g. 'System' or
    'non-Water'. Returns the output path, or None if the .xtc is missing/empty or
    trjconv fails (caller can fall back to a single final frame).
    """
    wd = Path(workdir)
    if not (wd / xtc).exists() or (wd / xtc).stat().st_size == 0:
        return None
    gmx, _libs = _resolve_gmx(gmx)
    env = _gmx_env(_libs)
    cmd = [gmx, "trjconv", "-s", tpr, "-f", xtc, "-o", out, "-pbc", pbc]
    if skip and int(skip) > 1:
        cmd += ["-skip", str(int(skip))]
    proc = subprocess.run(
        cmd, cwd=str(wd), env=env, input=f"{group}\n",
        capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    if on_line:
        for ln in (proc.stdout + proc.stderr).splitlines():
            on_line(ln)
    if proc.returncode == 0 and (wd / out).exists():
        _postprocess_traj_pdb(wd / out)
        return str(wd / out)
    return None


def trjconv(workdir, *, tpr, src, out, gmx="gmx", pbc="mol", skip=1, group="System", on_line=None):
    """Convert a GROMACS trajectory to another format via ``gmx trjconv``.

    The output format follows the ``out`` extension (.xtc/.trr/.gro/.pdb). Unlike
    ``trjconv_to_pdb`` this does NO PDB post-processing, so it's safe for binary outputs
    (xtc/trr). Returns the output path, or None if the source is missing/empty or trjconv
    fails.
    """
    wd = Path(workdir)
    if not (wd / src).exists() or (wd / src).stat().st_size == 0:
        return None
    gmxcmd, _libs = _resolve_gmx(gmx)
    env = _gmx_env(_libs)
    cmd = [gmxcmd, "trjconv", "-s", tpr, "-f", src, "-o", out, "-pbc", pbc]
    if skip and int(skip) > 1:
        cmd += ["-skip", str(int(skip))]
    proc = subprocess.run(
        cmd, cwd=str(wd), env=env, input=f"{group}\n",
        capture_output=True, text=True, encoding="utf-8", errors="replace",
    )
    if on_line:
        for ln in (proc.stdout + proc.stderr).splitlines():
            on_line(ln)
    if proc.returncode == 0 and (wd / out).exists():
        return str(wd / out)
    return None


def _postprocess_traj_pdb(path):
    """Make a trjconv PDB viewer-friendly: fill the element column (cols 77-78)
    from the atom name, and put CRYST1 right after each MODEL so 3Dmol/JSmol
    associate the box with every frame.

    trjconv leaves cols 77-78 blank (it doesn't know Sit->Si, Alo->Al, ...) and
    writes CRYST1 *before* each MODEL (outside the frame block) — both break
    per-frame box rendering and element coloring in the browser viewers.
    """
    try:
        from ..element import element as _element   # the function, from the submodule
    except Exception:
        _element = None
    cache = {}

    # atomipy uses pseudo-elements Ow/Hw for water; the PDB element column wants
    # the real chemical symbol.
    _real = {"Ow": "O", "Hw": "H"}

    def elem_for(name):
        n = (name or "").strip()
        if n not in cache:
            el = ""
            if _element is not None:
                try:
                    el = (_element([{"type": n}])[0].get("element") or "").strip()
                except Exception:
                    el = ""
            el = _real.get(el, el)
            cache[n] = el[:2]
        return cache[n]

    p = Path(path)
    out, last_cryst = [], None
    for ln in p.read_text(encoding="utf-8", errors="replace").splitlines():
        if ln.startswith("CRYST1"):
            last_cryst = ln          # hold; re-emit right after the next MODEL
            continue
        if ln.startswith("MODEL"):
            out.append(ln)
            if last_cryst:
                out.append(last_cryst)
            continue
        if ln.startswith(("ATOM", "HETATM")):
            el = elem_for(ln[12:16])
            if el:
                if len(ln) < 78:
                    ln = ln.ljust(78)
                ln = ln[:76] + el.rjust(2) + ln[78:]
        out.append(ln)
    p.write_text("\n".join(out) + "\n", encoding="utf-8")


def _parse_xvg(path):
    """Parse a GROMACS .xvg into {'time': [...], 'series': [{'name','values'}, ...]}."""
    names = []
    rows = []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("@"):
                m = re.match(r'@\s+s(\d+)\s+legend\s+"(.*)"', line)
                if m:
                    names.append(m.group(2))
                continue
            if not line.strip() or line.startswith(("#", "&")):
                continue
            parts = line.split()
            try:
                rows.append([float(p) for p in parts])
            except ValueError:
                continue
    if not rows:
        return None
    cols = list(zip(*rows))
    time = list(cols[0])
    series = []
    for i, nm in enumerate(names):
        col = i + 1
        if col < len(cols):
            series.append({"name": nm, "values": list(cols[col])})
    return {"time": time, "series": series}


def energy_timeseries(workdir, edr, *, terms=None, gmx="gmx", out="energy.xvg", on_line=None):
    """Run ``gmx energy`` on an .edr and return a thermodynamic time-series.

    Parameters
    ----------
    workdir, edr : str
        Working directory and .edr filename (relative to it).
    terms : list of str, optional
        GROMACS energy term names to extract. Terms not present in the .edr are
        silently skipped by gmx (e.g. EM .edr has only Potential).
    Returns
    -------
    dict or None
        {'time': [...], 'series': [{'name': 'Potential (kJ/mol)', 'values': [...]}, ...]}
    """
    if terms is None:
        terms = ["Potential", "Kinetic-En.", "Total-Energy", "Temperature",
                 "Pressure", "Volume", "Density"]
    gmx_cmd, libs = _resolve_gmx(gmx)
    env = _gmx_env(libs)
    wd = Path(workdir)
    sel = "\n".join(terms) + "\n\n"
    cmd = [gmx_cmd, "energy", "-f", edr, "-o", out]
    if on_line:
        on_line("$ echo <terms> | " + " ".join(cmd))
    proc = subprocess.run(cmd, cwd=str(wd), input=sel, capture_output=True,
                          text=True, encoding="utf-8", errors="replace", env=env)
    if on_line:
        for ln in (proc.stdout + "\n" + proc.stderr).splitlines():
            on_line(ln)
    xvg = wd / out
    if proc.returncode != 0 or not xvg.exists():
        return None
    return _parse_xvg(xvg)


def run_bar(workdir, dhdl_files, *, out="bar.xvg", oi="barint.xvg", begin=None,
            gmx="gmx", on_line=None):
    """Run ``gmx bar`` on per-window ``dhdl.xvg`` files and parse the total ΔG.

    Combines the free-energy windows of an alchemical (FEP) sweep — one ``dhdl.xvg``
    per ``init-lambda-state`` (see :func:`atomipy.gromacs.fep_mdp_extra`) — with the
    Bennett Acceptance Ratio estimator built into GROMACS. Needs at least two files.

    Returns ``None`` on failure, else::

        {'dG': float, 'dG_err': float, 'unit': 'kJ/mol',
         'intervals': [{'from','to','dG','err'}, ...], 'bar_xvg': path, 'stdout': str}
    """
    wd = Path(workdir)
    files = [f for f in dhdl_files if (wd / f).exists()]
    if len(files) < 2:
        return None
    gmxcmd, libs = _resolve_gmx(gmx)
    env = _gmx_env(libs)
    cmd = [gmxcmd, "bar", "-f", *files, "-o", out, "-oi", oi]
    if begin is not None:
        cmd += ["-b", str(begin)]
    if on_line:
        on_line("$ " + " ".join(cmd))
    proc = subprocess.run(cmd, cwd=str(wd), capture_output=True, text=True,
                          encoding="utf-8", errors="replace", env=env)
    blob = (proc.stdout or "") + "\n" + (proc.stderr or "")
    if on_line:
        for ln in blob.splitlines():
            on_line(ln)
    if proc.returncode != 0:
        return None
    # gmx bar prints per-interval and a 'total' line, e.g.:
    #   lam_A - lam_B,   DG  1.234 +/- 0.056
    #   total  0 - 1,    DG  5.678 +/- 0.123
    rx = re.compile(r"([-\d.eE+]+)\s*-\s*([-\d.eE+]+)\s*,\s*DG\s+([-\d.eE+]+)\s*\+/-\s*([-\d.eE+]+)")
    intervals, total = [], None
    for ln in blob.splitlines():
        m = rx.search(ln)
        if not m:
            continue
        row = {"from": m.group(1), "to": m.group(2),
               "dG": float(m.group(3)), "err": float(m.group(4))}
        if ln.strip().lower().startswith("total"):
            total = row
        else:
            intervals.append(row)
    if total is None and intervals:   # fall back: sum the intervals (errors in quadrature)
        total = {"dG": sum(r["dG"] for r in intervals),
                 "err": sum(r["err"] ** 2 for r in intervals) ** 0.5}
    if total is None:
        return None
    return {"dG": total["dG"], "dG_err": total.get("err"), "unit": "kJ/mol",
            "intervals": intervals, "bar_xvg": str(wd / out), "stdout": blob}


def _sample_frames_by_spacing(distances, spacing):
    """Pick frame indices spaced ~``spacing`` apart along ``distances`` (a monotonic-ish
    reaction coordinate). Greedy: start at frame 0, then repeatedly jump to the frame
    closest to current + spacing. Mirrors Lemkul/Harms' setupUmbrella sampleDistances."""
    cur, sel = 0, [0]
    n = len(distances)
    while cur < n:
        target = distances[cur] + spacing
        rest = distances[cur:]
        nxt = min(range(len(rest)), key=lambda j: abs(target - rest[j])) + cur
        if nxt == cur:
            break
        sel.append(nxt)
        cur = nxt
    return sel


def _parse_xvg_xy(path):
    """Parse a 2-column .xvg into [(x, y), ...] (skips @/# header lines)."""
    rows = []
    for ln in Path(path).read_text(encoding="utf-8", errors="replace").splitlines():
        s = ln.strip()
        if not s or s.startswith(("#", "@", "&")):
            continue
        parts = s.split()
        try:
            rows.append((float(parts[0]), float(parts[1])))
        except (ValueError, IndexError):
            continue
    return rows


def extract_umbrella_windows(workdir, pull_tpr, pull_traj, ndx, group1, group2, *,
                             spacing, gmx="gmx", out_prefix="umbrella_conf", on_line=None):
    """Turn an SMD pull trajectory into umbrella windows spaced ~``spacing`` nm apart.

    Runs ``gmx distance`` over ``pull_traj`` to get the COM(``group1``,``group2``) distance
    per frame, selects a subset of frames spaced ~``spacing`` along that coordinate, and
    dumps each selected frame's structure with ``gmx trjconv -dump``. Returns
    ``[(frame_gro_filename, com_distance_nm), ...]`` — one per umbrella window — or ``[]``.
    """
    wd = Path(workdir)
    gmxcmd, libs = _resolve_gmx(gmx)
    env = _gmx_env(libs)

    def _run(cmd, stdin=None):
        p = subprocess.run(cmd, cwd=str(wd), env=env, input=stdin, capture_output=True,
                           text=True, encoding="utf-8", errors="replace")
        if on_line:
            for ln in (p.stdout + p.stderr).splitlines():
                on_line(ln)
        return p

    sel = f'com of group "{group1}" plus com of group "{group2}"'
    p = _run([gmxcmd, "distance", "-s", pull_tpr, "-f", pull_traj, "-n", ndx,
              "-select", sel, "-oall", "umbrella_alldist.xvg"])
    if p.returncode != 0 or not (wd / "umbrella_alldist.xvg").exists():
        return []
    table = _parse_xvg_xy(wd / "umbrella_alldist.xvg")   # [(time, distance), ...]
    if len(table) < 2:
        return []
    chosen = _sample_frames_by_spacing([d for _, d in table], spacing)
    windows = []
    for wi, fi in enumerate(chosen):
        t, d = table[fi]
        gro = f"{out_prefix}{wi}.gro"
        _run([gmxcmd, "trjconv", "-s", pull_tpr, "-f", pull_traj, "-dump", str(t), "-o", gro],
             stdin="0\n")   # group 0 = System (all atoms)
        if (wd / gro).exists():
            windows.append((gro, d))
    return windows


def run_wham(workdir, tpr_files, pullf_files, *, out="profile.xvg", hist="histo.xvg",
             unit="kJ", begin=None, bootstrap=None, gmx="gmx", on_line=None):
    """Run ``gmx wham`` on the umbrella windows and parse the PMF profile.

    ``tpr_files`` and ``pullf_files`` are equal-length lists (one per window) of the
    window ``.tpr`` and ``pullf.xvg`` filenames. Returns ``None`` on failure, else::

        {'pmf': [(xi, G), ...], 'dG': float, 'xi_min': float, 'xi_max': float,
         'unit': 'kJ/mol', 'profile_xvg': path, 'histo_xvg': path, 'stdout': str}

    ``dG`` is the PMF range (max − min) — for a dissociation profile that is the well
    depth relative to the unbound plateau. Set ``bootstrap`` (e.g. 100) for error bars.
    """
    wd = Path(workdir)
    if len(tpr_files) < 2 or len(pullf_files) < 2:
        return None
    gmxcmd, libs = _resolve_gmx(gmx)
    env = _gmx_env(libs)
    (wd / "wham_tpr.dat").write_text("\n".join(tpr_files) + "\n", encoding="utf-8")
    (wd / "wham_pullf.dat").write_text("\n".join(pullf_files) + "\n", encoding="utf-8")
    cmd = [gmxcmd, "wham", "-it", "wham_tpr.dat", "-if", "wham_pullf.dat",
           "-o", out, "-hist", hist, "-unit", unit]
    if begin is not None:
        cmd += ["-b", str(begin)]
    if bootstrap:
        cmd += ["-nBootstrap", str(int(bootstrap)), "-bsres", "wham_bsres.xvg"]
    if on_line:
        on_line("$ " + " ".join(cmd))
    proc = subprocess.run(cmd, cwd=str(wd), env=env, capture_output=True, text=True,
                          encoding="utf-8", errors="replace")
    blob = (proc.stdout or "") + "\n" + (proc.stderr or "")
    if on_line:
        for ln in blob.splitlines():
            on_line(ln)
    if proc.returncode != 0 or not (wd / out).exists():
        return None
    pmf = _parse_xvg_xy(wd / out)
    if not pmf:
        return None
    gs = [g for _, g in pmf if g == g]   # drop NaNs
    dG = (max(gs) - min(gs)) if gs else None
    unit_label = {"kJ": "kJ/mol", "kCal": "kcal/mol", "kT": "kT"}.get(unit, unit)
    return {"pmf": pmf, "dG": dG, "xi_min": pmf[0][0], "xi_max": pmf[-1][0],
            "unit": unit_label, "profile_xvg": str(wd / out),
            "histo_xvg": str(wd / hist), "stdout": blob}


def _stream(cmd, cwd, env):
    """Run cmd, yielding merged stdout/stderr lines; finally yield a status dict."""
    proc = subprocess.Popen(
        cmd, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, encoding="utf-8", errors="replace", bufsize=1,
    )
    for line in proc.stdout:
        yield line.rstrip("\n")
    proc.wait()
    yield {"returncode": proc.returncode}


def run_stage(workdir, stage, struct_in, *, defines=None, gmx="gmx", maxwarn=2,
              restraint=None, top="topol.top", ntmpi=1, ntomp=None, mdp_text=None,
              ndx=None, **mdp_kwargs):
    """Generate the .mdp, run grompp then mdrun for one stage; yield log lines.

    ``ndx`` is an optional index file (relative to ``workdir``) passed to grompp
    with ``-n`` — required when the .mdp references a custom group (e.g. the
    ``freezegrps`` group for a frozen Dummy FF mineral). Any remaining
    ``mdp_kwargs`` (including ``freeze_group``) flow to :func:`mdp`.

    Yields str log lines, then a final dict {'stage','returncode','gro'} where
    'gro' is the output coordinate file (``{stage}.gro``) if mdrun succeeded.
    """
    wd = Path(workdir)
    gmx, _libs = _resolve_gmx(gmx)
    env = _gmx_env(_libs)

    mdp_path = wd / f"{stage}.mdp"
    if mdp_text is not None and str(mdp_text).strip():
        mdp_path.write_text(str(mdp_text), encoding="utf-8")   # user-supplied .mdp, verbatim
    else:
        mdp_path.write_text(_mdp_text(stage, defines=defines, **mdp_kwargs), encoding="utf-8")

    grompp = [gmx, "grompp", "-f", f"{stage}.mdp", "-c", struct_in, "-p", top,
              "-o", f"{stage}.tpr", "-maxwarn", str(maxwarn)]
    if restraint:
        grompp += ["-r", restraint]
    if ndx:
        grompp += ["-n", ndx]
    rc = 0
    yield f"$ {' '.join(grompp)}"
    for item in _stream(grompp, str(wd), env):
        if isinstance(item, dict):
            rc = item["returncode"]
        else:
            yield item
    if rc != 0:
        yield {"stage": stage, "returncode": rc, "gro": None, "step": "grompp"}
        return

    # -ntmpi 1: single thread-MPI rank (no domain decomposition). Robust for the
    # small / periodic-molecule systems typical here — auto DD often fails to split
    # a clay sheet that spans the cell across many ranks. OpenMP still parallelizes.
    # -v: verbose step progress to stderr (merged into the stream) so the run panel
    # shows live progress instead of going silent during the mdrun compute.
    mdrun = [gmx, "mdrun", "-deffnm", stage, "-v"]
    if ntmpi:
        mdrun += ["-ntmpi", str(int(ntmpi))]
    if ntomp:
        mdrun += ["-ntomp", str(int(ntomp))]
    yield f"$ {' '.join(mdrun)}"
    for item in _stream(mdrun, str(wd), env):
        if isinstance(item, dict):
            rc = item["returncode"]
        else:
            yield item
    gro = str(wd / f"{stage}.gro") if rc == 0 and (wd / f"{stage}.gro").exists() else None
    yield {"stage": stage, "returncode": rc, "gro": gro, "step": "mdrun"}


def run_pipeline(workdir, stages, struct_in, *, defines=None, gmx="gmx", **mdp_kwargs):
    """Run a sequence of stages, chaining each stage's output .gro to the next.

    ``stages`` is a list of stage names (e.g. ['em','nvt','npt','md']). Yields
    log lines and per-stage status dicts. Stops if any stage fails.
    """
    current = struct_in
    for stage in stages:
        last = None
        for item in run_stage(workdir, stage, current, defines=defines, gmx=gmx, **mdp_kwargs):
            if isinstance(item, dict):
                last = item
            yield item
        if not last or last.get("returncode", 1) != 0 or not last.get("gro"):
            return
        current = os.path.basename(last["gro"])
