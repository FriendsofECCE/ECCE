"""GROMACS .mdp generation following MINFF conventions.

Templates mirror the MINFF lecture examples (mholmboe/atom, ATOM_scripts_lecture
/MINFF): Verlet + PME, rcoulomb = rvdw = 1.2 nm, ``periodic-molecules = yes``
(infinite clay sheets), and a ``define`` line selecting the angle-k variant,
ion model, flexible water, and position restraints.

Prototype simplification: thermostat / COM groups default to a single ``System``
group (no custom index file). Splitting into mineral vs water/ion groups is a
later (Phase-2) addition that needs an auto-generated ``.ndx``.
"""

from ..minff_defines import minff_defines

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


def build_defines(minff_variant="MINFF_k500", ion_model=None, flexible=True, posres=None,
                  mineral=None):
    """Assemble the ``define`` flag list, e.g. ['-DMINFF_k500','-DOPC3_IOD_LM'].

    The general MINFF parameters are selected by the angle force constant alone
    (``minff_variant='MINFF_k500'``). The tailored (TMINFF) parameters need the mineral
    as well, which is given with ``mineral``::

        build_defines(mineral="Montmorillonite")
        # ['-DMontmorillonite', '-DMINFF_k500', '-DFLEXIBLE']

    The pre-v1.0 spellings ``'GMINFF_k500'`` and ``'Montmorillonite_k500'`` are still
    accepted for ``minff_variant`` and turned into the new defines, with a
    ``FutureWarning`` (see :func:`atomipy.minff_defines.minff_defines`).
    """
    flags = [f"-D{d}" for d in minff_defines(minff_variant, mineral)]
    if ion_model:
        flags.append(f"-D{ion_model}")
    if flexible:
        flags.append("-DFLEXIBLE")
    if posres:
        flags.append(f"-D{posres}")
    return flags


def _render(pairs):
    return "\n".join(f"{k:<22}= {v}" for k, v in pairs) + "\n"


def mdp(stage, *, defines=None, nsteps=None, dt=0.001, temperature=298.0,
        pressure=1.0, nstxtc=1000, nstenergy=100, nstlog=100,
        emtol=1000.0, emstep=0.01, gen_vel=None, continuation=None,
        freeze_group=None, freeze_dim="Y Y Y"):
    """Return .mdp text for a stage: 'em', 'nvt', 'npt', or 'md'.

    Parameters
    ----------
    stage : str
        'em' (steepest-descent minimization), 'nvt', 'npt', or 'md'.
    defines : list of str, optional
        ``define`` flags (see :func:`build_defines`). Default MINFF_k500+flexible.
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
    return _render(pairs)
