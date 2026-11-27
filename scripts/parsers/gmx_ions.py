"""Force-field ion names shared with the remote job's GmxIons.pm."""
import re

FAMILIES = [
    {'re': '^amber', 'ions': dict((el, el) for el in 'NA CL K CA MG ZN LI RB CS'.split())},
    {'re': '^oplsaa', 'ions': dict((el, el) for el in 'NA CL K CA MG LI RB CS F BR'.split())},
    {'re': '^charmm', 'ions': {'NA': 'SOD', 'CL': 'CLA', 'K': 'POT', 'CA': 'CAL',
                             'CS': 'CES', 'MG': 'MG'}},
    {'re': '^gromos54a7', 'ions': {'NA': 'NA', 'CL': 'CL'}},
    {'re': '^gromos', 'ions': dict((el, el) for el in 'NA CL CA MG ZN CU'.split())},
]



def ions(ff):
    ff = re.sub(r'\.ff$', '', ff)
    return next((f['ions'] for f in FAMILIES if re.search(f['re'], ff)), {})


def rewrite(text, ff):
    ion = ions(ff)
    lines = text.split('\n')
    count = {}
    for line in lines:
        if line.startswith(('ATOM  ', 'HETATM')):
            key = line[17:27]
            count[key] = count.get(key, 0) + 1
    notes = []
    for i, line in enumerate(lines):
        if not line.startswith(('ATOM  ', 'HETATM')) or count[line[17:27]] != 1:
            continue
        res, el = line[17:20].strip(), line[76:78].strip().upper()
        name = ion.get(el)
        if name is None or res not in ('', 'UNK', name):
            continue
        atom = '{:<4}'.format(name) if len(el) > 1 else ' {:<3}'.format(name)
        lines[i] = line[:12] + atom + line[16:17] + '{:<3}'.format(name) + line[20:]
        if res != name:
            notes.append('{}: residue {} -> {}'.format(el, res or '(none)', name))
    return '\n'.join(lines), notes
