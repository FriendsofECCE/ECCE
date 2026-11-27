"""ACES II basis output, including the legacy counter and wrapping rules."""

import sys
from rdStandardGBS import perl_number


def wracesII(bs):
    out = sys.stdout.write
    shells = ('S', 'P', 'D', 'F', 'G', 'H', 'I', 'J')
    for atom, orbitals in bs['gbs'].items():
        if atom in ('coordinants', 'polarization'):
            continue
        out('EMSL BASIS SET LIBRARY\n\n')
        shellctr, coeffctr = {}, {}
        sp_flag = False
        for orbital, rows in orbitals:
            if orbital == 'SP':
                sp_flag = True
                for shell in ('S', 'P'):
                    coeffctr[shell] = coeffctr.get(shell, 0) + 1
                    shellctr[shell] = shellctr.get(shell, 0) + 1
            else:
                coeffctr[orbital] = coeffctr.get(orbital, 0) + len(rows[0]) - 1
                shellctr[orbital] = shellctr.get(orbital, 0) + 1
        is_multi = any(value > 1 for value in shellctr.values())
        numshells = len(shellctr)
        out('%3d\n' % numshells)
        out(''.join('%5d' % i for i in range(numshells)) + '\n')
        for shell in shells:
            if shellctr.get(shell, 0) > 0 and sp_flag:
                out('%5d' % shellctr[shell])
            else:
                total = sum(len(rows[0]) - 1 for orbital, rows in orbitals
                            if shell in orbital)
                if total > 0:
                    out('%5d' % total)
        out('\n')
        for shell in shells:
            total = sum(len(rows) for orbital, rows in orbitals if shell in orbital)
            if total > 0:
                out('%5d' % total)
        out('\n')
        for shell in shells:
            return_ctr, local_shellctr = 0, 1
            if shellctr.get(shell, 0) > 0:
                out('\n')
            for orbital, rows in orbitals:
                if shell not in orbital:
                    continue
                for row in rows:
                    out('%14.6f' % perl_number(row[0]))
                    return_ctr += 1
                    if return_ctr == 5:
                        out('\n')
                        return_ctr = 0
                if local_shellctr == shellctr.get(shell, 0):
                    out('\n\n' if return_ctr != 0 else '\n')
                local_shellctr += 1
            counter = 0
            for orbital, rows in orbitals:
                if shell not in orbital:
                    continue
                num_coeff = 0
                for row in rows:
                    coefficients = row[1:]
                    num_coeff = len(coefficients)
                    if is_multi or sp_flag:
                        i = 0
                        while i < max(coeffctr.get(shell, 0), shellctr.get(shell, 0)):
                            if counter == i and len(coefficients) == 1:
                                out('%11.8f' % perl_number(coefficients[0]))
                            elif counter == i and shell == 'S' and orbital == 'SP':
                                out('%11.8f' % perl_number(coefficients[0]))
                            elif counter == i and shell == 'P' and orbital == 'SP':
                                out('%11.8f' % perl_number(coefficients[1]))
                            elif counter == i and len(coefficients) > 1:
                                for coefficient in coefficients:
                                    out('%11.8f' % perl_number(coefficient))
                                    i += 1
                                i -= 1
                            else:
                                out('%11.8f' % 0)
                            i += 1
                        out('\n')
                    else:
                        return_ctr = 0
                        for coefficient in coefficients:
                            out('%11.8f' % perl_number(coefficient))
                            return_ctr += 1
                            if return_ctr == 7:
                                out('\n')
                                return_ctr = 0
                            else:
                                out(' ')
                        if return_ctr != 0:
                            out('\n')
                if coeffctr.get(shell, 0) > 1 and orbital != 'SP':
                    counter += num_coeff
                else:
                    counter += 1
    ecp_shells = ('s', 'p', 'd', 'f', 'g', 'h')
    for atom, data in bs.get('ecp', {}).items():
        if atom == 'polarization':
            continue
        nelec, components = data
        first = ecp_shells[len(components) - 1]
        out('*\n%s:ECP-%s\n*\n' % (atom.upper(), nelec))
        out('    NCORE =  %s    LMAX =%d\n' % (nelec, len(components) - 1))
        for i, (_, primitives) in enumerate(components):
            out((first if i == 0 else ecp_shells[i - 1] + '-' + first) + '\n')
            for prim in primitives:
                out('%14.8f%5d%14.8f\n' %
                    (perl_number(prim['Coefficient']), perl_number(prim['Rexponent']),
                     perl_number(prim['GaussExponent'])))
    out('*\n')
