"""Write the legacy Gaussian 98 basis block to stdout."""
import sys

from wrGaussian94GBS import write_explicit


ROUTE_NAMES = frozenset((
    'sto-2g',
    'sto-3g',
    'sto-3g*',
    '3-21g',
    '3-21+g',
    '3-21g*',
    '3-21+g*',
    '4-31g',
    '6-31g',
    '6-31++g',
    '6-31++g**',
    '6-31g(3df,3pd)',
    '6-311g',
    '6-311++g',
    '6-311++g(3df,3pd)',
    'midi!',
    'dz (dunning)',
    'sv (dunning-hay)',
    'cc-pvdz',
    'cc-pvtz',
    'cc-pvqz',
    'cc-pv5z',
    'cc-pv6z',
    'aug-cc-pvdz',
    'aug-cc-pvtz',
    'aug-cc-pvqz',
    'aug-cc-pv5z',
    'aug-cc-pv6z',
))


def writeGaussian98(bs):
    if 'name_gbs' in bs and 'ecp' not in bs:
        names = dict(bs['name_gbs'])
        coords = names.pop('coordinants', '')
        oldbasis = ''
        use_route = True
        for name in names.values():
            name = name.lower()
            if name.startswith('"'):
                name = name[1:]
            if name.endswith('"'):
                name = name[:-1]
            if name != oldbasis and oldbasis != '':
                use_route = False
            if name not in ROUTE_NAMES:
                use_route = False
            oldbasis = name
        if use_route:
            # The table is an eligibility test; the original spelling is emitted.
            sys.stdout.write('useRouteCard %s %s\n' % (oldbasis, coords))
            return
    write_explicit(bs, gaussian98=True)
