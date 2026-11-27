"""Retired GAMESS-UK writer, preserving the explicit-only basis path."""

import sys
from rdStandardGBS import perl_number


def wrGAMESSUK(bs):
    out = sys.stdout.write
    # The library-name branch was unreachable: useExplicitBasis was always 1.
    for part in ('gbs', 'dftXCfitting', 'dftCDfitting'):
        gbs = {}
        if part == 'gbs' and part in bs:
            gbs = dict(bs[part])
            coords = gbs.pop('coordinants', None)
            gbs.pop('polarization', None)
            if coords == 'spherical':
                out('harmonic \n')
            out('basis nwchem \n')
        elif part == 'dftCDfitting' and part in bs:
            gbs = dict(bs[part])
            gbs.pop('coordinants', None)
            gbs.pop('polarization', None)
            out('dft jbas nwchem \n')
        for atom, orbitals in gbs.items():
            for orbital, rows in orbitals:
                out('%s    %s\n' % (atom, orbital))
                for row in rows:
                    out(''.join('%19s' % value for value in row) + '\n')
        if gbs:
            out('END\n')
    if 'ecp' in bs:
        out('PSEUDO ECP\n')
    for atom, data in bs.get('ecp', {}).items():
        if atom == 'polarization':
            continue
        nelec, components = data
        out('CARDS %s\n%d %s\n' % (atom, len(components) - 1, nelec))
        for shell, primitives in components:
            once = True
            for prim in primitives:
                if perl_number(prim['GaussExponent']) != 0 and perl_number(prim['Coefficient']) != 0:
                    if once:
                        # The count includes zero rows even though those rows are omitted.
                        out('# %s %s\n%d\n' % (atom, shell, len(primitives)))
                        once = False
                    out('    %s%20.8f%20.8f\n' %
                        (prim['Rexponent'], perl_number(prim['Coefficient']),
                         perl_number(prim['GaussExponent'])))
