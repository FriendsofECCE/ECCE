import openmm as mm
import openmm.app as app
from openmm import unit

def _normalize_defines(defines):
    """Accept either a dict or a list of names; return a dict."""
    if isinstance(defines, dict):
        return dict(defines)
    return {name: '' for name in defines}

def load_minff_into_openmm(
    top_path,
    gro_path,
    defines,
    include_dir=None,
    nonbonded_method=None,
    nonbonded_cutoff_nm=1.0,
    constraints=None,
    rigid_water=False,
    ewald_error_tolerance=5e-4,
    use_dispersion_correction=True,
):
    """
    Build an OpenMM Topology, System, and positions from an atomipy-generated
    GROMACS topology (.top) and coordinate (.gro) pair.

    This is the recommended OpenMM entry point for MINFF simulations. The
    function is a thin wrapper around openmm.app.GromacsTopFile and
    openmm.app.GromacsGroFile that handles the standard MINFF defaults:
    flexible bonds and angles on the mineral body, OPC3-style water at the
    user's discretion, Particle Mesh Ewald for electrostatics, and the
    Lorentz-Berthelot combination rule (MINFF's convention, also OpenMM's
    default).

    Parameters
    ----------
    top_path : str
        Path to a GROMACS .top file written by atomipy (e.g. via
        ap.write_gmx_top(atoms, Box=Box, file_path='kao.top')).
        Must be self-contained: include #include directives and #ifdef blocks
        for variant selection, but resolve to a complete topology once
        preprocessed.
    gro_path : str
        Path to the matching .gro coordinate file.
    defines : dict[str, str] or list[str]
        Preprocessor variables to activate, equivalent to the GROMACS .mdp
        directive `define = -DGMINFF_k500 -DOPC3_IOD_LM -DOPC3`. Either:
          - dict form: {'GMINFF_k500': '', 'OPC3_IOD_LM': '', 'OPC3': ''}
          - list form: ['GMINFF_k500', 'OPC3_IOD_LM', 'OPC3']  (auto-converted)
        The empty-string values are sufficient for plain #ifdef checks.
    include_dir : str or None
        Directory containing the MINFF .itp files referenced by `#include`
        directives in top_path. If None, OpenMM falls back to its default
        (typically /usr/local/gromacs/share/gromacs/top), which is almost
        certainly wrong for MINFF. Provide this explicitly in production.
    nonbonded_method : openmm.app constant, optional
        Defaults to app.PME. Other valid values: app.CutoffPeriodic,
        app.CutoffNonPeriodic, app.NoCutoff, app.Ewald, app.LJPME.
    nonbonded_cutoff_nm : float
        Real-space cutoff in nanometers (default 1.0).
    constraints : openmm.app constant or None
        Default None — MINFF requires *flexible* bonds and angles on the
        mineral body for the explicit angle terms to function. Do not set
        to AllBonds or HBonds for mineral simulations.
    rigid_water : bool
        Default False (flexible water). Set True if running with a rigid
        water model (TIP3P-rigid, SPC-rigid, OPC3-rigid) — OpenMM will then
        apply SETTLE constraints to water molecules automatically.
    ewald_error_tolerance : float
        PME tolerance. Default 5e-4 matches OpenMM's standard.
    use_dispersion_correction : bool
        Long-range LJ correction. Default True matches GROMACS default.

    Returns
    -------
    topology : openmm.app.Topology
    system : openmm.System
    positions : openmm.unit.Quantity (N, 3) array in nanometers
    """
    if nonbonded_method is None:
        nonbonded_method = app.PME
    defines_dict = _normalize_defines(defines)

    if gro_path.lower().endswith('.pdb'):
        coord_file = app.PDBFile(gro_path)
        positions = coord_file.positions
        box_vectors = coord_file.topology.getPeriodicBoxVectors()
    else:
        coord_file = app.GromacsGroFile(gro_path)
        positions = coord_file.positions
        box_vectors = coord_file.getPeriodicBoxVectors()
    water_o, water_h = 'OW_opc3', 'HW_opc3'
    for d in defines_dict:
        d_upper = d.upper()
        if 'SPCE' in d_upper:
            water_o, water_h = 'OW_spce', 'HW_spce'
            break
        elif 'SPC' in d_upper:
            water_o, water_h = 'OW_spc', 'HW_spc'
            break
        elif 'TIP3P' in d_upper:
            water_o, water_h = 'OW_tip3p', 'HW_tip3p'
            break
        elif 'TIP4P' in d_upper:
            water_o, water_h = 'OW_tip4p', 'HW_tip4p'
            break
        elif 'OPC3' in d_upper:
            water_o, water_h = 'OW_opc3', 'HW_opc3'
            break
        elif 'OPC' in d_upper:
            water_o, water_h = 'OW_opc', 'HW_opc'
            break

    water_bond_angles = """
[ bondtypes ]
OW_opc3 HW_opc3  1   0.097888  502416.0
HW_opc3 OW_opc3  1   0.097888  502416.0
OW_spce HW_spce  1   0.100000  502416.0
HW_spce OW_spce  1   0.100000  502416.0
OW_spc  HW_spc   1   0.100000  502416.0
HW_spc  OW_spc   1   0.100000  502416.0
OW_tip3p HW_tip3p 1  0.095720  502416.0
HW_tip3p OW_tip3p 1  0.095720  502416.0
OW_tip4p HW_tip4p 1  0.095720  502416.0
HW_tip4p OW_tip4p 1  0.095720  502416.0

[ angletypes ]
HW_opc3 OW_opc3 HW_opc3  1  109.47  628.02
HW_spce OW_spce HW_spce  1  109.47  628.02
HW_spc  OW_spc  HW_spc   1  109.47  628.02
HW_tip3p OW_tip3p HW_tip3p 1 104.52  628.02
HW_tip4p OW_tip4p HW_tip4p 1 104.52  628.02
"""

    is_clayff_2004 = any('CLAYFF_2004' in d.upper() for d in defines_dict)

    class TextFileWrapper:
        def __init__(self, f, is_ffbonded=False, is_clayff_2004=False):
            self.f = f
            self.is_ffbonded = is_ffbonded
            self.is_clayff_2004 = is_clayff_2004
            self.appended = False
        def read(self, *args, **kwargs):
            content = self.f.read(*args, **kwargs)
            if self.is_ffbonded and not self.appended:
                content += water_bond_angles
                self.appended = True
            return self._translate(content)
        def readline(self, *args, **kwargs):
            content = self.f.readline(*args, **kwargs)
            if self.is_ffbonded and not content and not self.appended:
                content = water_bond_angles
                self.appended = True
            return self._translate(content)
        def __iter__(self):
            for line in self.f:
                yield self._translate(line)
            if self.is_ffbonded and not self.appended:
                self.appended = True
                for line in water_bond_angles.strip().split('\n'):
                    yield line + '\n'
        def _translate(self, text):
            if not text:
                return text
            for w in [' ', '\t']:
                for e in [' ', '\t', '\n']:
                    text = text.replace(f'{w}Ow{e}', f'{w}{water_o}{e}')
                    text = text.replace(f'{w}Hw{e}', f'{w}{water_h}{e}')
                    # All ions are signed (ASCII +/-) in min.ff; defensively map any bare
                    # ion token a structure might still carry to the signed name so OpenMM
                    # matches min.ff. Whitespace-bounded, so 'Fe'/'Feo3'/'Cs' etc. are safe.
                    for _cat in ('Na', 'K', 'Li', 'Cs', 'Rb'):
                        text = text.replace(f'{w}{_cat}{e}', f'{w}{_cat}+{e}')
                    for _an in ('Cl', 'Br', 'F', 'I'):
                        text = text.replace(f'{w}{_an}{e}', f'{w}{_an}-{e}')
                    if self.is_clayff_2004:
                        text = text.replace(f'{w}Alo{e}', f'{w}ao{e}')
                        text = text.replace(f'{w}Sit{e}', f'{w}st{e}')
                        text = text.replace(f'{w}H{e}', f'{w}ho{e}')
                        text = text.replace(f'{w}Oh{e}', f'{w}oh{e}')
                        text = text.replace(f'{w}Ob{e}', f'{w}ob{e}')
                        text = text.replace(f'{w}Op{e}', f'{w}op{e}')
                        text = text.replace(f'{w}Mgo{e}', f'{w}mgo{e}')
                        text = text.replace(f'{w}Mgh{e}', f'{w}mgh{e}')
                        text = text.replace(f'{w}Cao{e}', f'{w}cao{e}')
                        text = text.replace(f'{w}Cah{e}', f'{w}cah{e}')
            return text
        def __getattr__(self, name):
            return getattr(self.f, name)
        def __enter__(self):
            return self
        def __exit__(self, exc_type, exc_val, exc_tb):
            self.f.close()

    import builtins
    original_open = builtins.open
    def utf8_open(file, *args, **kwargs):
        mode = kwargs.get('mode', args[0] if args else 'r')
        if 'b' not in mode:
            if 'encoding' not in kwargs:
                kwargs['encoding'] = 'utf-8'
            is_ffbonded = 'ffbonded.itp' in str(file)
            return TextFileWrapper(original_open(file, *args, **kwargs), is_ffbonded=is_ffbonded, is_clayff_2004=is_clayff_2004)
        return original_open(file, *args, **kwargs)

    builtins.open = utf8_open
    try:
        top = app.GromacsTopFile(
            top_path,
            periodicBoxVectors=box_vectors,
            includeDir=include_dir,
            defines=defines_dict,
        )
    finally:
        builtins.open = original_open
    system = top.createSystem(
        nonbondedMethod=nonbonded_method,
        nonbondedCutoff=nonbonded_cutoff_nm * unit.nanometer,
        constraints=constraints,
        rigidWater=rigid_water,
        ewaldErrorTolerance=ewald_error_tolerance,
        useDispersionCorrection=use_dispersion_correction,
    )
    return top.topology, system, positions
