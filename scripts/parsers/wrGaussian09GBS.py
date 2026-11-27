"""Write Gaussian 09 basis blocks using the standard reader wire model."""

import sys

from rdStandardGBS import perl_number


NameToBasis = {
    'sto-2g': 'sto-2g',
    'sto-3g': 'sto-3g',
    'sto-3g*': 'sto-3g*',
    '3-21g': '3-21g',
    '3-21+g': '3-21+g',
    '3-21g*': '3-21g*',
    '3-21+g*': '3-21+g*',
    '4-31g': '4-31g',
    '6-31g': '6-31g',
    '6-31++g': '6-31++g',
    '6-31++g**': '6-31++g**',
    '6-31g(3df,3pd)': '6-31G(3df,3pd)',
    '6-311g': '6-311G',
    '6-311++g': '6-311++G',
    '6-311++g(3df,3pd)': '6-311++G(3df,3pd)',
    'midi!': 'midix',
    'dz (dunning)': 'd95',
    'sv (dunning-hay)': 'd95v',
    'cc-pvdz': 'cc-pvdz',
    'cc-pvtz': 'cc-pvtz',
    'cc-pvqz': 'cc-pvqz',
    'cc-pv5z': 'cc-pv5z',
    'cc-pv6z': 'cc-pv6z',
    'aug-cc-pvdz': 'aug-cc-pvdz',
    'aug-cc-pvtz': 'aug-cc-pvtz',
    'aug-cc-pvqz': 'aug-cc-pvqz',
    'aug-cc-pv5z': 'aug-cc-pv5z',
    'aug-cc-pv6z': 'aug-cc-pv6z',
}


def library_name(value):
    value = value.lower()
    if value.startswith('"'):
        value = value[1:]
    if value.endswith('"'):
        value = value[:-1]
    return value


def writeGaussian09(bs):
    gbs = dict(bs['gbs'])
    gbs.pop('coordinants', None)
    gbs.pop('polarization', None)
    names = dict(bs.get('name_gbs', {}))
    if 'name_gbs' in bs and 'ecp' not in bs:
        name_coords = names.pop('coordinants', '')
        old_basis = ''
        use_route = True
        for atom in sorted(names):
            basis = library_name(names[atom])
            if (basis != old_basis and old_basis) or basis not in NameToBasis:
                use_route = False
            old_basis = basis
        if use_route:
            sys.stdout.write('useRouteCard %s %s\n' %
                             (NameToBasis.get(old_basis, ''), name_coords))
            return
    ecp = dict(bs.get('ecp', {}))
    ecp.pop('polarization', None)
    for atom in sorted(gbs):
        sys.stdout.write(' %s  0\n' % atom.upper())

        # The SP coefficient count persists across this centre's shells.
        counts = {'SP': 0}
        for orbital, rows in gbs[atom]:
            for row in rows:
                counts[orbital] = len(row) - 2
            count = counts.get(orbital, 0)
            if count >= 1 and counts['SP'] == 0:
                for column in range(1, count + 2):
                    nonzero = [row for row in rows
                               if perl_number(row[column] if column < len(row) else '') != 0]
                    sys.stdout.write(' %-2s%3d  1.00\n' % (orbital, len(nonzero)))
                    for row in nonzero:
                        sys.stdout.write('%17.8f%16.8f\n' %
                                         (perl_number(row[0]), perl_number(row[column])))
            else:
                sys.stdout.write(' %-2s%3d  1.00\n' % (orbital, len(rows)))
                for row in rows:
                    sys.stdout.write('%17.8f' % perl_number(row[0]))
                    for coefficient in row[1:]:
                        if perl_number(coefficient) != 0:
                            sys.stdout.write('%16.8f' % perl_number(coefficient))
                    sys.stdout.write('\n')
        sys.stdout.write(' ****\n')
    sys.stdout.write('\n')
    shells = ('s', 'p', 'd', 'f', 'g', 'h', 'i')
    for atom in sorted(ecp):
        electrons, components = ecp[atom]
        count = len(components) - 1
        first = shells[count] if 0 <= count < len(shells) else ''
        sys.stdout.write('%s  0\n%s-ECP   %d   %s\n' %
                         (atom.upper(), atom.upper(), count, electrons))
        for index, (_, primitives) in enumerate(components):
            shell = first if index == 0 else shells[index - 1] + '-' + first
            sys.stdout.write('%s potential\n %d\n' % (shell, len(primitives)))
            for primitive in primitives:
                sys.stdout.write('%s%18.8f%16.8f\n' %
                                 (primitive.get('Rexponent', ''),
                                  perl_number(primitive.get('GaussExponent')),
                                  perl_number(primitive.get('Coefficient'))))
