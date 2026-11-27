"""Write ORCA names, segmented shells and indexed NewECP primitives."""

import re
import sys
from rdStandardGBS import perl_number


def library_name(name):
    return re.sub(r'"$', '', re.sub(r'^"', '', name.lower()))


def writeORCAShell(shell, rows, col):
    selected = [(row[0], row[col + 1]) for row in rows
                if len(row) > col + 1 and perl_number(row[col + 1]) != 0]
    if not selected:
        return
    sys.stdout.write(' %s %d\n' % (shell.upper(), len(selected)))
    for idx, (exponent, coefficient) in enumerate(selected, 1):
        sys.stdout.write(' %3d%17.8f%16.8f\n' %
                         (idx, perl_number(exponent), perl_number(coefficient)))


def writeORCA(bs):
    write = sys.stdout.write
    gbs = {k: v for k, v in bs['gbs'].items()
           if k not in ('coordinants', 'polarization')}
    names = dict(bs.get('name_gbs', {}))
    if 'name_gbs' in bs and 'ecp' not in bs:
        names.pop('coordinants', None)
        oldbasis = ''
        route = True
        for atom in sorted(names):
            name = library_name(names[atom])
            if name != oldbasis and oldbasis:
                route = False
            if name not in NameToBasis:
                route = False
            oldbasis = name
        if route:
            write('useRouteCard %s\n' % NameToBasis.get(oldbasis, ''))
            return
    ecp = {k: v for k, v in bs.get('ecp', {}).items() if k != 'polarization'}
    write('%basis\n')
    for atom in sorted(gbs):
        el = atom.lower().capitalize()
        name = library_name(names.get(atom, ''))
        if atom not in ecp and 'name_gbs' in bs and name in NameToBasis:
            write('NewGTO %s "%s" end\n' % (el, NameToBasis[name]))
            continue
        write('NewGTO %s\n' % el)
        for shell, rows in gbs[atom]:
            if shell == 'SP':
                writeORCAShell('S', rows, 0)
                writeORCAShell('P', rows, 1)
            else:
                for col in range(max([1] + [len(row) - 1 for row in rows])):
                    writeORCAShell(shell, rows, col)
        write('end\n')
    shells = ('s', 'p', 'd', 'f', 'g', 'h', 'i')
    for atom in sorted(ecp):
        nelec, components = ecp[atom]
        lmax = shells[len(components) - 1] if len(components) <= len(shells) else ''
        write('NewECP %s\n  N_core %s\n  lmax %s\n' %
              (atom.lower().capitalize(), nelec, lmax))
        for cntr, (shell, primitives) in enumerate(components):
            label = lmax if cntr == 0 else shells[cntr - 1]
            write('  %s %d\n' % (label, len(primitives)))
            for idx, prim in enumerate(primitives, 1):
                write('   %3d%17.8f%18.8f%4d\n' %
                      (idx, perl_number(prim['GaussExponent']),
                       perl_number(prim['Coefficient']), int(perl_number(prim['Rexponent']))))
        write('end\n')
    write('end\n')


# Only verified library aliases may bypass the stored primitives.
NameToBasis = {
    'cc-pv6z': 'cc-pV6Z',
    'aug-cc-pv6z': 'aug-cc-pV6Z',
    '6-311++g(2d,2p)': '6-311++G(2d,2p)',
    '6-311g(2df,2pd)': '6-311G(2df,2pd)',
    'pc-1': 'pc-1',
    'aug-pc-1': 'aug-pc-1',
    'cc-pvdz-dk': 'cc-pVDZ-DK',
    '6-31+g*': '6-31+G*',
    '6-31++g**': '6-31++G**',
    '6-311g*': '6-311G*',
    '6-311+g*': '6-311+G*',
    '6-311++g**': '6-311++G**',
    '6-311++g(3df,3pd)': '6-311++G(3df,3pd)',
    'aug-cc-pv5z': 'aug-cc-pV5Z',
    'def2-svp(p)': 'def2-SV(P)',
    'def2-tzvpp': 'def2-TZVPP',
    'def2-qzvpp': 'def2-QZVPP',
    'def2-mtzvp': 'def2-mTZVP',
    'def2-mtzvpp': 'def2-mTZVPP',
    'vdzp': 'vDZP',
    'ma-def2-svp': 'ma-def2-SVP',
    '3-21gsp': '3-21GSP',
    '4-22gsp': '4-22GSP',
    'ahlrichs tzv': 'TZV',
    'ahlrichs vdz': 'SV',
    'ahlrichs pvdz': 'SVP',
    'iglo-ii': 'IGLO-II',
    'iglo-iii': 'IGLO-III',
    'midi (huzinaga)': 'MIDI',
    'mini (huzinaga)': 'MINI',
    'ugbs': 'UGBS',
    'aug-cc-pcvdz': 'aug-cc-pCVDZ',
    'aug-cc-pcvqz': 'aug-cc-pCVQZ',
    'aug-cc-pcvtz': 'aug-cc-pCVTZ',
    'aug-cc-pwcvdz': 'aug-cc-pwCVDZ',
    'aug-cc-pwcvqz': 'aug-cc-pwCVQZ',
    'aug-cc-pwcvtz': 'aug-cc-pwCVTZ',
    'aug-pc-0': 'aug-pc-0',
    'aug-pc-2': 'aug-pc-2',
    'aug-pc-3': 'aug-pc-3',
    'aug-pcsseg-0': 'aug-pcSseg-0',
    'aug-pcsseg-1': 'aug-pcSseg-1',
    'aug-pcsseg-2': 'aug-pcSseg-2',
    'aug-pcsseg-3': 'aug-pcSseg-3',
    'aug-pcseg-0': 'aug-pcseg-0',
    'aug-pcseg-1': 'aug-pcseg-1',
    'aug-pcseg-2': 'aug-pcseg-2',
    'aug-pcseg-3': 'aug-pcseg-3',
    'cc-pcvdz': 'cc-pCVDZ',
    'cc-pcvqz': 'cc-pCVQZ',
    'cc-pcvtz': 'cc-pCVTZ',
    'cc-pvqz-dk': 'cc-pVQZ-DK',
    'cc-pvtz-dk': 'cc-pVTZ-DK',
    'cc-pwcvdz': 'cc-pwCVDZ',
    'cc-pwcvqz': 'cc-pwCVQZ',
    'cc-pwcvtz': 'cc-pwCVTZ',
    'pc-2': 'pc-2',
    'pc-3': 'pc-3',
    'pc-4': 'pc-4',
    'pcsseg-0': 'pcSseg-0',
    'pcsseg-1': 'pcSseg-1',
    'pcsseg-2': 'pcSseg-2',
    'pcsseg-3': 'pcSseg-3',
    'pcsseg-4': 'pcSseg-4',
    'pcseg-1': 'pcseg-1',
    'pcseg-2': 'pcseg-2',
    'pcseg-3': 'pcseg-3',
    'pcseg-4': 'pcseg-4',
    'sto-3g': 'STO-3G',
    '3-21g': '3-21G',
    '6-31g': '6-31G',
    '6-31g*': '6-31G*',
    '6-31g**': '6-31G**',
    '6-311g': '6-311G',
    '6-311g**': '6-311G**',
    'cc-pvdz': 'cc-pVDZ',
    'cc-pvtz': 'cc-pVTZ',
    'cc-pvqz': 'cc-pVQZ',
    'cc-pv5z': 'cc-pV5Z',
    'aug-cc-pvdz': 'aug-cc-pVDZ',
    'aug-cc-pvtz': 'aug-cc-pVTZ',
    'aug-cc-pvqz': 'aug-cc-pVQZ',
    'def2-svp': 'def2-SVP',
    'def2-tzvp': 'def2-TZVP',
    'def2-qzvp': 'def2-QZVP',
    'def2-svpd': 'def2-SVPD',
    'def2-tzvpd': 'def2-TZVPD',
    'def2-tzvppd': 'def2-TZVPPD',
    'def2-qzvpd': 'def2-QZVPD',
    'def2-qzvppd': 'def2-QZVPPD',
}
