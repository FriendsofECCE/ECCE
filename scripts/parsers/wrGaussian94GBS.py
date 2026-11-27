"""Write the legacy Gaussian 94 basis block to stdout."""
import contextlib
import io
import sys

from rdStandardGBS import perl_number, translator_main


def write_explicit(bs, gaussian98=False):
    gbs = dict(bs['gbs'])
    gbs.pop('coordinants', None)
    gbs.pop('polarization', None)
    for atom, orbitals in gbs.items():
        sys.stdout.write(' %s  0\n' % atom.upper())
        # The SP count persists between shells, affecting later contractions.
        counts = {'S': 0, 'SP': 0, 'P': 'D', '0': 'I'}
        for shell, rows in orbitals:
            for row in rows:
                counts[shell] = len(row) - 2
            if perl_number(str(counts.get(shell, 0))) >= 1 and counts['SP'] == 0:
                for column in range(counts[shell] + 1):
                    selected = [row for row in rows if
                                perl_number(row[column + 1] if
                                            column + 1 < len(row) else '') != 0]
                    sys.stdout.write(' %-2s%3d  1.00\n' % (shell, len(selected)))
                    for row in selected:
                        sys.stdout.write('%17.8f%16.8f\n' % (
                            perl_number(row[0]), perl_number(row[column + 1])))
            else:
                sys.stdout.write(' %-2s%3d  1.00\n' % (shell, len(rows)))
                for row in rows:
                    sys.stdout.write('%17.8f' % perl_number(row[0]))
                    for coefficient in row[1:]:
                        if perl_number(coefficient) != 0:
                            sys.stdout.write('%16.8f' % perl_number(coefficient))
                    sys.stdout.write('\n')
        sys.stdout.write(' ****\n')
    shells = ('s', 'p', 'd', 'f', 'g', 'h', 'i')
    ecp = dict(bs.get('ecp', {}))
    ecp.pop('polarization', None)
    if gaussian98:
        sys.stdout.write('\n')
    for atom, (electrons, components) in ecp.items():
        count = len(components) - 1
        first = shells[count] if 0 <= count < len(shells) else ''
        atom = atom.upper()
        if not gaussian98:
            sys.stdout.write('\n')
        sys.stdout.write('%s  0\n%s-ECP   %s   %s\n' % (
            atom, atom, count, electrons))
        for index, (_, primitives) in enumerate(components):
            shell = first if index == 0 else (
                (shells[index - 1] if index <= len(shells) else '') + '-' + first)
            sys.stdout.write('%s potential\n %s\n' % (shell, len(primitives)))
            for primitive in primitives:
                # ECP zero suppression uses string equality, unlike GBS rows.
                if primitive['Coefficient'] != '0':
                    sys.stdout.write('%s%18.8f%16.8f\n' % (
                        primitive['Rexponent'],
                        perl_number(primitive['GaussExponent']),
                        perl_number(primitive['Coefficient'])))
        if not gaussian98:
            sys.stdout.write('\n')


def writeGaussian94(bs):
    write_explicit(bs)


def legacy_main(writer, version):
    # Short help uses an alias; Getopt::Std's long help uses the filename.
    diagnostics = io.StringIO()
    with contextlib.redirect_stderr(diagnostics):
        status = translator_main(writer, 'gaus' + version, '22972')
    sys.stderr.write(diagnostics.getvalue().replace(
        'Usage: std2gaus' + version + ' [',
        'Usage: std2Gaussian-' + version + ' ['))
    return status
