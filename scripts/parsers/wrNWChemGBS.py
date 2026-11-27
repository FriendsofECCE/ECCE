"""Emit NWChem library names or the stored exponents and coefficients."""

import re
import sys
from rdStandardGBS import perl_number


def wrNWChem(bs):
    write = sys.stdout.write
    explicit = any(re.search(pattern, name, re.I)
                   for atom, name in bs.get('name_gbs', {}).items()
                   if 'coordinants' not in atom
                   for pattern in (r'^\s*"\s*aug-cc-pwCV',
                                   r'^\s*"\s*(d-)?aug-.*V\(?[5-8][+Z]',
                                   r'^\s*"\s*d-aug-.*VQZ', r'-pcv[5-8]'))
    if 'name_gbs' not in bs or explicit:
        for part, label in (('gbs', 'ao'), ('dftXCfitting', 'xc'),
                            ('dftCDfitting', 'cd')):
            if part not in bs:
                continue
            gbs = dict(bs[part])
            coords = gbs.pop('coordinants', '')
            gbs.pop('polarization', None)
            write('basis "%s basis" %s print\n' % (label, coords))
            for atom in sorted(gbs):
                for shell, rows in gbs[atom]:
                    write('%s    %s\n' % (atom, shell))
                    for row in rows:
                        write(''.join('%19s' % value for value in row) + '\n')
            if gbs:
                write('END\n')
        if 'ecp' in bs:
            write('ECP\n')
            for atom in sorted(k for k in bs['ecp'] if k != 'polarization'):
                nelec, components = bs['ecp'][atom]
                write('%s nelec %s\n' % (atom, nelec))
                for shell, primitives in components:
                    once = True
                    for prim in primitives:
                        exponent = perl_number(prim['GaussExponent'])
                        coefficient = perl_number(prim['Coefficient'])
                        if exponent != 0 and coefficient != 0:
                            if once:
                                write(' %s %s\n' % (atom, shell))
                                once = False
                            write('    %s%20.8f%20.8f\n' %
                                  (prim['Rexponent'], exponent, coefficient))
            write('END\n')
    else:
        for part, label in (('name_gbs', 'ao'), ('name_dftXCfitting', 'xc'),
                            ('name_dftCDfitting', 'cd'), ('name_ecp', None)):
            if part not in bs:
                continue
            gbs = dict(bs[part])
            if label:
                write('basis "%s basis" %s print\n' %
                      (label, gbs.pop('coordinants', '')))
            else:
                write('ECP\n')
            for atom in sorted(gbs):
                write('  %s library %s\n' % (atom, gbs[atom]))
            if gbs:
                write('END\n')
