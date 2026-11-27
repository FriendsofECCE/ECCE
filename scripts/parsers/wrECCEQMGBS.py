"""Write every stored shell so edited bases need no engine-side library."""

import re
import sys
from rdStandardGBS import perl_number


def writeECCEQMShell(el, shell, rows, col):
    selected = [(row[0], row[col + 1]) for row in rows
                if len(row) > col + 1 and perl_number(row[col + 1]) != 0]
    if selected:
        sys.stdout.write('shell %s %s\n' % (el, shell))
        for exponent, coefficient in selected:
            sys.stdout.write('  %.10g %.10g\n' %
                             (perl_number(exponent), perl_number(coefficient)))


def writeECCEQM(bs):
    gbs = {k: v for k, v in bs['gbs'].items()
           if k not in ('coordinants', 'polarization')}
    names = {k: v for k, v in bs.get('name_gbs', {}).items() if k != 'coordinants'}
    elements = sorted(k for k in bs.get('ecp', {}) if k != 'polarization')
    if elements:
        raise ValueError('ECCE-QM has no effective core potentials; the basis set needs one for: '
                         + ' '.join(elements)
                         + '. Choose a basis set that covers every element without one.')
    label = ''
    for atom in sorted(names):
        name = re.sub(r'"$', '', re.sub(r'^"', '', names[atom]))
        if not label:
            label = name
        elif label != name:
            label = 'custom'
            break
    sys.stdout.write('basis %s\nbasis_data\n' % (label or 'custom'))
    for atom in sorted(gbs):
        if not re.match(r'^[A-Za-z]{1,2}$', atom):
            raise ValueError("ECCE-QM takes one basis set per element; '%s' is a per-atom basis set." % atom)
        el = atom.lower().capitalize()
        for shell, rows in gbs[atom]:
            shell = shell.upper()
            if shell == 'SP':
                writeECCEQMShell(el, 'S', rows, 0)
                writeECCEQMShell(el, 'P', rows, 1)
            else:
                for col in range(max([1] + [len(row) - 1 for row in rows])):
                    writeECCEQMShell(el, shell, rows, col)
    sys.stdout.write('end\n')
