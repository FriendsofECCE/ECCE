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
    ff = re.sub(r'\.ff$', '', ff, flags=re.ASCII)
    return next((f['ions'] for f in FAMILIES if re.search(f['re'], ff, re.ASCII)), {})


def rewrite(text, ff):
    ion = {el.encode('ascii'): name.encode('ascii') for el, name in ions(ff).items()}
    # PDB columns and case folding follow the original byte-oriented reader.
    lines = text.encode('utf-8', 'surrogateescape').split(b'\n')
    count = {}
    for line in lines:
        if line.startswith((b'ATOM  ', b'HETATM')):
            key = line[17:27]
            count[key] = count.get(key, 0) + 1
    notes = []
    for i, line in enumerate(lines):
        if not line.startswith((b'ATOM  ', b'HETATM')) or count[line[17:27]] != 1:
            continue
        res = line[17:20].strip(b" \t\n\r\f\v")
        el = line[76:78].strip(b" \t\n\r\f\v").upper()
        name = ion.get(el)
        if name is None or res not in (b'', b'UNK', name):
            continue
        atom = name.ljust(4) if len(el) > 1 else b' ' + name.ljust(3)
        lines[i] = line[:12] + atom + line[16:17] + name.ljust(3) + line[20:]
        if res != name:
            notes.append('{}: residue {} -> {}'.format(
                el.decode('ascii'), (res or b'(none)').decode('ascii'), name.decode('ascii')))
    return b'\n'.join(lines).decode('utf-8', 'surrogateescape'), notes
