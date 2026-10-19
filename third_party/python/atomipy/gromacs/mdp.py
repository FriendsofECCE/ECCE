"""GROMACS .mdp generation following MINFF conventions.

Templates mirror the MINFF lecture examples (mholmboe/atom, ATOM_scripts_lecture
/MINFF): Verlet + PME, rcoulomb = rvdw = 1.2 nm, ``periodic-molecules = yes``
(infinite clay sheets), and a ``define`` line selecting the angle-k variant,
ion model, flexible water, and position restraints.

Prototype simplification: thermostat / COM groups default to a single ``System``
group (no custom index file). Splitting into mineral vs water/ion groups is a
later (Phase-2) addition that needs an auto-generated ``.ndx``.
"""

# Defaults common to all dynamics stages (nm, ps, K, bar).
_COMMON = {
    "cutoff-scheme": "Verlet",
    "nstlist": "20",
    "rlist": "1.2",
    "coulombtype": "PME",
    "vdw-type": "Cut-off",
    "rcoulomb": "1.2",
    "rvdw": "1.2",
    "fourierspacing": "0.12",
    "pme-order": "4",
    "ewald-rtol": "1e-05",
    "pbc": "xyz",
    "periodic-molecules": "yes",
    "constraints": "none",
    "constraint_algorithm": "lincs",
    "lincs_order": "4",
    "lincs_iter": "1",
}


def build_defines(minff_variant="GMINFF_k500", ion_model=None, flexible=True, posres=None):
    """Assemble the ``define`` flag list, e.g. ['-DGMINFF_k500','-DOPC3_IOD_LM']."""
    flags = []
    if minff_variant:
        flags.append(f"-D{minff_variant}")
    if ion_model:
        flags.append(f"-D{ion_model}")
    if flexible:
        flags.append("-DFLEXIBLE")
    if posres:
        flags.append(f"-D{posres}")
    return flags


def fep_mdp_extra(couple_moltype, init_lambda_state, vdw_lambdas, coul_lambdas=None,
                  bonded_lambdas=None, sc_alpha=0.5, sc_power=1, sc_sigma=0.3,
                  couple_lambda0="vdw-q", couple_lambda1="none", couple_intramol="no",
                  nstdhdl=100, calc_lambda_neighbors=-1, dhdl_print_energy="total"):
    """Free-energy (alchemical) ``.mdp`` options for ONE λ window, as an ``extra`` dict
    for :func:`mdp` (``mdp(stage, extra=fep_mdp_extra(...))``).

    Decouples ``couple_moltype`` along the λ schedule. Run one window per state in
    ``0..len(vdw_lambdas)-1`` (each ``init_lambda_state`` a separate mdrun writing a
    ``dhdl.xvg``), then combine the windows with :func:`atomipy.gromacs.run_bar` to get ΔG.

    Parameters
    ----------
    couple_moltype : str
        Name of the ``[ moleculetype ]`` being coupled/decoupled (the solute/ligand).
    init_lambda_state : int
        Which λ window (index into the lambda vectors) this run computes.
    vdw_lambdas, coul_lambdas, bonded_lambdas : sequence of float
        The λ schedules (0→1). ``vdw_lambdas`` is required; the others default off.
    couple_lambda0 / couple_lambda1 : str
        Interactions present at λ=0 / λ=1 ('vdw-q', 'vdw', 'none').
    sc_alpha, sc_power, sc_sigma : float
        Soft-core parameters (avoid the λ→endpoint singularity). ``sc_alpha=0`` = linear.
    nstdhdl : int
        dH/dλ output frequency. ``calc_lambda_neighbors=-1`` writes all neighbours (MBAR-ready).
    """
    def _fmt(v):
        return " ".join(f"{float(x):g}" for x in v) if isinstance(v, (list, tuple)) else str(v)
    d = {
        "free-energy": "yes",
        "init-lambda-state": int(init_lambda_state),
        "couple-moltype": couple_moltype,
        "couple-lambda0": couple_lambda0,
        "couple-lambda1": couple_lambda1,
        "couple-intramol": couple_intramol,
        "vdw-lambdas": _fmt(vdw_lambdas),
        "sc-alpha": sc_alpha,
        "sc-power": sc_power,
        "sc-sigma": sc_sigma,
        "nstdhdl": int(nstdhdl),
        "calc-lambda-neighbors": int(calc_lambda_neighbors),
        "dhdl-print-energy": dhdl_print_energy,
        "separate-dhdl-file": "yes",
    }
    if coul_lambdas is not None:
        d["coul-lambdas"] = _fmt(coul_lambdas)
    if bonded_lambdas is not None:
        d["bonded-lambdas"] = _fmt(bonded_lambdas)
    return d


def pull_mdp_extra(group1, group2, *, k=1000.0, rate=0.0, init=None, dim="N N Y",
                   geometry="distance", coord_type="umbrella", start=True,
                   pbc_ref_prev_step_com=True, group1_pbcatom=-1,
                   nstfout=50, nstxout=1000):
    """Pull-code ``.mdp`` options for ONE reaction coordinate (COM distance between two
    index groups), as an ``extra`` dict for :func:`mdp`.

    This is the engine piece for umbrella sampling: a steered-MD (SMD) pull generates
    configurations along the coordinate (``rate`` > 0), then a window per configuration
    is restrained in place (``rate = 0``) and the ``pullf.xvg`` forces are combined with
    :func:`atomipy.gromacs.run_wham` into a PMF.

    Parameters
    ----------
    group1, group2 : str
        Names of the two index groups whose COM distance is the reaction coordinate
        (must exist in the ``.ndx`` given to grompp — see
        :func:`atomipy.gromacs.write_index_ndx`).
    k : float
        Umbrella force constant (kJ/mol/nm²).
    rate : float
        Pull rate (nm/ps). ``0`` = umbrella restraint (hold in place); ``> 0`` = SMD pull.
    init : float or None
        Explicit reference COM distance (nm). ``None`` with ``start=True`` uses the
        starting structure's COM (the usual umbrella-window setup).
    dim : str
        Which Cartesian components the coordinate uses (e.g. ``"N N Y"`` = z only).
    geometry, coord_type : str
        Pull geometry (``distance``) and coordinate type (``umbrella``).
    nstfout, nstxout : int
        Output frequency for the pull force (``pullf.xvg``, needed by WHAM) and pull
        coordinate (``pullx.xvg``).
    """
    d = {
        "pull": "yes",
        "pull-ncoords": 1,
        "pull-ngroups": 2,
        "pull-group1-name": group1,
        "pull-group2-name": group2,
        "pull-coord1-type": coord_type,
        "pull-coord1-geometry": geometry,
        "pull-coord1-dim": dim,
        "pull-coord1-groups": "1 2",
        "pull-coord1-k": k,
        "pull-coord1-rate": rate,
        "pull-nstfout": int(nstfout),
        "pull-nstxout": int(nstxout),
    }
    if start:
        d["pull-coord1-start"] = "yes"
    if init is not None:
        d["pull-coord1-init"] = init
    if pbc_ref_prev_step_com:
        d["pull-pbc-ref-prev-step-com"] = "yes"
        d["pull-group1-pbcatom"] = int(group1_pbcatom)
    return d


def _render(pairs):
    return "\n".join(f"{k:<22}= {v}" for k, v in pairs) + "\n"


def _apply_extra(pairs, extra):
    """Merge ``extra`` {key: value} into the (key, value) ``pairs``, OVERRIDING an
    existing key in place (grompp rejects a doubly-defined parameter) and appending
    any new key. ``.mdp`` keys are case-insensitive and treat ``-``/``_`` as the same,
    so matching is normalised; the existing key's spelling is kept on override."""
    if not extra:
        return pairs
    def _norm(k):
        return str(k).strip().lower().replace("-", "_")
    ex = {_norm(k): (k, v) for k, v in extra.items()}
    out, used = [], set()
    for k, v in pairs:
        nk = _norm(k)
        if nk in ex:
            out.append((k, str(ex[nk][1]))); used.add(nk)   # override, keep original key spelling
        else:
            out.append((k, v))
    for nk, (ok, ov) in ex.items():
        if nk not in used:
            out.append((str(ok), str(ov)))                   # brand-new key
    return out


def mdp(stage, *, defines=None, nsteps=None, dt=0.001, temperature=298.0,
        pressure=1.0, nstxtc=1000, nstenergy=100, nstlog=100,
        emtol=1000.0, emstep=0.01, gen_vel=None, continuation=None,
        freeze_group=None, freeze_dim="Y Y Y", extra=None):
    """Return .mdp text for a stage: 'em', 'nvt', 'npt', or 'md'.

    Parameters
    ----------
    stage : str
        'em' (steepest-descent minimization), 'nvt', 'npt', or 'md'.
    defines : list of str, optional
        ``define`` flags (see :func:`build_defines`). Default GMINFF_k500+flexible.
    nsteps : int, optional
        Step count (stage defaults: em 5000, nvt/npt 20000, md 50000).
    dt : float
        Timestep in ps (default 0.001 = 1 fs).
    temperature, pressure : float
        Reference T (K) and P (bar) for coupling.
    freeze_group : str, optional
        Index-group name to hold rigid via ``freezegrps`` (all dimensions by
        default). Used for the frozen "Dummy FF" mineral — a bond-free framework
        that would otherwise drift apart with no bonded terms and no mass=0. The
        group must exist in the ``.ndx`` passed to grompp (see
        :func:`atomipy.gromacs.runner.write_freeze_ndx`). Works for EM and dynamics.
    freeze_dim : str
        The ``freezedim`` value (default ``"Y Y Y"`` — frozen in x, y and z).
    extra : dict, optional
        Arbitrary ``{key: value}`` ``.mdp`` options appended after the standard block
        (e.g. ``{'ref_t': 350}``, ``{'tau_t': 0.5}``, or free-energy fields like
        ``{'free-energy': 'yes', 'init-lambda-state': 3}``). Values are written
        verbatim (``str(value)``), so a vector like ``'0.0 0.5 1.0'`` works. Enables
        parameter sweeps over any ``.mdp`` field. A key given here overrides the same
        key emitted above it (grompp honours the last occurrence).
    """
    if defines is None:
        defines = build_defines()
    define_line = " ".join(defines)
    pairs = [("define", define_line)]
    # freezegrps holds the named group rigid — the .mdp counterpart of OpenMM's
    # setParticleMass(i, 0). Appended to whichever stage's pairs are returned below.
    freeze_pairs = ([("freezegrps", freeze_group), ("freezedim", freeze_dim)]
                    if freeze_group else [])

    if stage == "em":
        pairs += [
            ("integrator", "steep"),
            ("nsteps", str(nsteps if nsteps is not None else 5000)),
            ("emtol", str(emtol)),
            ("emstep", str(emstep)),
            # steepest-descent writes the minimization trajectory to .trr (it ignores
            # nstxout-compressed/.xtc), so set nstxout to capture the EM progression.
            ("nstxout", str(nstxtc)),
            ("nstxout-compressed", str(nstxtc)),
        ]
        pairs += list(_COMMON.items())
        pairs += [("DispCorr", "No")]
        pairs += freeze_pairs
        pairs = _apply_extra(pairs, extra)
        return _render(pairs)

    # dynamics stages
    pairs += [
        ("integrator", "md"),
        ("nsteps", str(nsteps if nsteps is not None else (50000 if stage == "md" else 20000))),
        ("dt", str(dt)),
    ]
    # Center-of-mass motion removal. GROMACS COM removal is mass-weighted and a
    # frozen atom keeps its real mass, so removing the whole-system COM velocity
    # would translate the frozen framework (it inherits -v_com). OpenMM's mass=0
    # excludes frozen atoms from the COM entirely, so to match it we disable COM
    # removal when freezing — the frozen wall then defines the reference frame and
    # stays exactly put.
    if freeze_group:
        pairs += [("comm-mode", "None")]
    else:
        pairs += [("nstcomm", "100"), ("comm-mode", "Linear")]
    pairs += [
        ("nstxout-compressed", str(nstxtc)),
        ("nstenergy", str(nstenergy)),
        ("nstlog", str(nstlog)),
        ("continuation", continuation if continuation is not None else ("yes" if stage != "nvt" else "no")),
    ]
    pairs += list(_COMMON.items())
    # temperature coupling (single group for the prototype)
    pairs += [
        ("tcoupl", "V-rescale"),
        ("tc-grps", "System"),
        ("tau_t", "1.0"),
        ("ref_t", str(temperature)),
    ]
    # pressure coupling
    if stage == "npt":
        pairs += [
            ("pcoupl", "C-rescale"),
            ("pcoupltype", "semiisotropic"),
            ("tau_p", "2.0"),
            ("ref_p", f"{pressure} {pressure}"),
            ("compressibility", "4.5e-5 4.5e-5"),
            ("refcoord-scaling", "all"),
        ]
    else:
        pairs += [("pcoupl", "no")]
    # velocity generation: on for a fresh NVT start
    gv = gen_vel if gen_vel is not None else (stage == "nvt")
    if gv:
        pairs += [("gen_vel", "yes"), ("gen_temp", str(temperature)), ("gen_seed", "-1")]
    pairs += [("DispCorr", "EnerPres")]
    pairs += freeze_pairs
    pairs = _apply_extra(pairs, extra)
    return _render(pairs)
