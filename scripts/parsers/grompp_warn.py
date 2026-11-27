"""Classify grompp diagnostics for the launch dialog."""
import re

TABLE = [
    {
        'id': 'atom-names',
        're': 'non-matching atom names|atom name [0-9]+ in .* does not match',
        'why': ('the atom names of the structure and the topology differ; grompp takes the '
                "topology's. Harmless only when the atoms are in the same order, which grompp "
                'cannot check (a different count is a grompp error).'),
    },
    {
        'id': 'berendsen',
        're': 'Berendsen (thermostat|barostat) does not generate',
        'why': ('Berendsen coupling was chosen in the task; grompp warns about it whatever the '
                'system.'),
    },
    {
        'id': 'gromos-twin-range',
        're': 'GROMOS force fields have been parametrized with a physically incorrect',
        'why': 'any GROMOS force field draws this warning, whatever the system.',
    },
    {
        'id': 'net-charge',
        're': 'Ewald electrostatics in a system with net charge',
        'why': ('the net charge is below 1e-4, which is rounding, or the Prepare left it on '
                'purpose (Neutralise unticked: the topology carries ECCE-NETCHARGE-INTENDED).'),
        'charge': 1,
    },
]

CHARGE_LIMIT = 1e-4

MDPKEY = {
    'comm-grps': ['Run settings, groups', 0],
    'comm-mode': ['Dynamics tab, remove centre of mass motion', 0],
    'compressibility': ['Dynamics tab, compressibility', 0],
    'constraints': ['Constraints tab', 0],
    'continuation': ['Dynamics tab, continue', 0],
    'coulombtype': ['Interactions tab, PME / Cutoff only', 0],
    'cutoff-scheme': [None, 1],
    'define': ['Run settings, defines', 0],
    'emstep': ['Optimize tab, initial step', 0],
    'emtol': ['Optimize tab, maximum force', 0],
    'gen-temp': ['Dynamics tab, continue', 0],
    'gen-vel': ['Dynamics tab, continue', 0],
    'integrator': [None, 1],
    'nstcalcenergy': [None, 1],
    'nstcomm': ['Dynamics tab, remove centre of mass motion interval', 0],
    'nstenergy': ['Files tab, write energies interval', 0],
    'nsteps': ['Dynamics tab, equilibration and data steps (Optimize tab, maximum steps)', 0],
    'nstlist': ['Interactions tab, pair-list update interval', 0],
    'nstlog': ['Files tab, write energies interval', 0],
    'nstpcouple': ['Run settings, barostat interval', 0],
    'nstxout-compressed': ['Files tab, write trajectory interval', 0],
    'pcoupl': ['Dynamics tab, constant pressure', 0],
    'pcoupltype': ['Run settings, barostat coupling type', 0],
    'pme-order': ['Interactions tab, B-spline order', 0],
    'rcoulomb': ['Interactions tab, cutoff radius', 0],
    'ref-p': ['Dynamics tab, pressure', 0],
    'ref-t': ['Dynamics tab, temperature', 0],
    'rvdw': ['Interactions tab, cutoff radius', 0],
    'tau-p': ['Dynamics tab, pressure relaxation time', 0],
    'tau-t': ['Dynamics tab, temperature relaxation time', 0],
    'tc-grps': ['Run settings, groups', 0],
    'tcoupl': ['Dynamics tab, constant temperature', 0],
    'tinit': ['Dynamics tab, initial time', 0],
}

EXPLAIN = [
    'Ewald electrostatics in a system with net charge',
    'should be at least [0-9]+ times larger than nst[tp]couple',
    'constraints on all bonds',
    'plain Coulomb cut-off',
    'center of mass motion in the presence of position restraints',
]


def translate(text):
    own = set()
    for key in sorted(MDPKEY, key=lambda k: -len(k)):
        pattern = re.escape(key).replace(r'\-', '[-_]')
        pattern = r'(?<![\w-])' + pattern + r'(?![\w-])'
        name, internal = MDPKEY[key]
        if name is not None:
            text = re.sub(pattern + r'(?! \()', lambda m: name + ' (' + m[0] + ')', text,
                          flags=re.ASCII)
        if internal and re.search(pattern, text, re.ASCII):
            own.add(key)
    if own:
        text += ('\n\nECCE writes ' + ', '.join(sorted(own)) + ' itself, without a field in the editor, '
                 'so this is a fault in ECCE, not in the settings: please report it.')
    return text


def explain(w):
    for i, pattern in enumerate(EXPLAIN):
        if not re.search(pattern, w['text'], re.ASCII):
            continue
        if i == 0:
            lead = 'The system has a net charge.'
            if w.get('charge') is not None:
                q = format(float(w['charge']), '.3f').rstrip('0').rstrip('.')
                q = re.sub('^-', '\u2212', q)
                lead = 'Net charge ' + q + ' e.'
            return (lead + ' With PME, a net charge adds a uniform background charge that can '
                    'cause artefacts. Neutralise the system with counter-ions '
                    '(Prepare: Neutralise), optionally with added salt.')
        if i == 1:
            m = re.search(r'tau-([tp]) \(([0-9.eE+-]+)\) should be at least ([0-9]+) times larger '
                          r'than nst[tp]couple\*dt \(([0-9.eE+-]+)\)', w['text'], re.ASCII)
            if not m:
                continue
            k, tau, times, dt = m.groups()
            what, step = ('thermostat', 'nsttcouple') if k == 't' else ('barostat', 'nstpcouple')
            minimum = format(float(times) * float(dt), '.3g')
            return ('The {} coupling time tau-{} ({} ps) is too short for its coupling interval '
                    '({}*dt = {} ps). Raise tau-{} to {} ps or more in the task, or shorten {}.'
                    .format(what, k, tau, step, dt, k, minimum, step))
        return [
            'Every bond is constrained, but this force field was parametrised with constraints '
            'on bonds to hydrogen only. Choose h-bonds for constraints in the task, which is also faster.',
            'Plain Coulomb cut-off electrostatics can produce artefacts. Choose PME for '
            'coulombtype in the task.',
            'Removing centre-of-mass motion while atoms are position-restrained can cause '
            'artefacts; they are usually negligible when a macromolecule is restrained to '
            'equilibrate it. Set comm-mode to None in the task to avoid them.'
        ][i - 2]
    return None


def parse(out, intended=False):
    warnings, cur, charge = [], None, None
    for line in out.split('\n'):
        if re.match(r'^WARNING \d+( \[.*\])?:\s*$', line, re.ASCII):
            if cur is not None:
                warnings.append(cur)
            cur = dict(head=line, text='')
            continue
        if cur is not None and re.match(r'^\s+\S', line, re.ASCII):
            cur['text'] += (' ' if cur['text'] else '') + line.lstrip()
            continue
        if cur is not None:
            warnings.append(cur)
        cur = None
        m = re.search(r'System has non-zero total charge:\s*(\S+)', line, re.ASCII)
        if m:
            charge = m[1]
    if cur is not None:
        warnings.append(cur)
    for w in warnings:
        w.update(charge=charge, harmless=0)
        for entry in TABLE:
            if not re.search(entry['re'], w['text'], re.ASCII):
                continue
            if entry.get('charge') and not intended and not (
                    charge is not None and abs(float(charge)) < CHARGE_LIMIT):
                continue
            w.update(harmless=1, why=entry['why'])
            break
    return warnings
