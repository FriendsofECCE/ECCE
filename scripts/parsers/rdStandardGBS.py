"""Read ECCE's standard basis dump, retaining the Perl reader's wire model.

readStandardBS(stream=None) returns a dict. ``gbs`` is always present;
``ecp``, ``dftXCfitting``, ``dftCDfitting`` and their ``name_`` counterparts
are present only when populated, as is ``name_gbs``. Basis dictionaries map
centres to [[shell, [[exponent, coefficient, ...], ...]], ...], with a
``coordinants`` string. Name dictionaries map centres to quoted library
names (and also carry ``coordinants``, except name_ecp). ECP dictionaries
map centres to [nelec, [[shell, [primitive, ...]], ...]], where each primitive
has Rexponent, GaussExponent and Coefficient keys. All numbers stay strings.
"""

import re
import sys


# Perl read the dump as bytes, so \s, \d and \w are ASCII there: a no-break
# space or a non-ASCII digit or letter must not match here either (#1331).
_WS = ' \t\n\r\f\v'


def perl_number(value):
    # Perl accepts a numeric prefix, including E but not Fortran D exponents.
    match = re.match(r'\s*([+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?)',
                     value or '', re.ASCII)
    return float(match.group(1)) if match else 0.0


def readStandardBS(stream=None):
    if stream is None:
        # Perl handled the dump as bytes: latin-1 keeps one char per byte, so
        # field widths and stray bytes come out as Perl wrote them (#1331).
        sys.stdin.reconfigure(encoding="latin-1")
        stream = sys.stdin
    bs = {'gbs': {}}
    numeric = {kind: {} for kind in ('orbital', 'exchange', 'charge', 'ecp')}
    names = {kind: {} for kind in numeric}
    reading_name = reading_numeric = False
    kind = center = orbital = coords = shell = nelec = ''
    contractions, orbitals, primitives, components = [], [], [], []
    for raw in stream:
        if re.match(r'^\s*NameBasis', raw, re.ASCII):
            reading_name = True
        if re.match(r'^\s*EndNameBasis', raw, re.ASCII):
            reading_name = False
        if re.match(r'^\s*NumericalBasis', raw, re.ASCII):
            reading_numeric = True
        if re.match(r'^\s*EndNumericalBasis', raw, re.ASCII):
            reading_numeric = False
        line = re.sub(r'[#\!].*$', '', raw).rstrip(_WS)
        if not line:
            continue
        header = re.match(r'^\s*basis "(\w\w) basis" (\w+)(\s+print)?', line, re.ASCII)
        if not reading_name and reading_numeric:
            if header:
                coords = header.group(2)
                kind = {'ao': 'orbital', 'xc': 'exchange', 'cd': 'charge'}.get(
                    header.group(1), kind)
            atom = re.match(r'^(\w+)\s+(\w+)$', line, re.ASCII)
            if atom and kind != 'ecp':
                if contractions:
                    orbitals.append([orbital, contractions])
                    contractions = []
                    if center != atom.group(1):
                        if kind in numeric:
                            numeric[kind][center] = orbitals
                        orbitals = []
                center, orbital = atom.groups()
            if re.match(r'^\s*-?[\d.]+\s+-?[\d.]+', line, re.ASCII):
                row = [f for f in re.split(r'[ \t\n\r\f\v]+', line) if f]
                if kind != 'ecp':
                    contractions.append(row)
                else:
                    primitives.append(dict(zip(
                        ('Rexponent', 'GaussExponent', 'Coefficient'), row[:3])))
            if re.search('END', line, re.I | re.ASCII) and kind != 'ecp':
                orbitals.append([orbital, contractions])
                contractions = []
                if kind in numeric:
                    numeric[kind]['coordinants'] = coords
                    numeric[kind][center] = orbitals
                orbitals = []
            if re.match(r'^\s*ECP', line, re.I | re.ASCII):
                kind = 'ecp'
            atom = re.match(r'^\s*(\w+)\s+nelec\s+(\d+)$', line, re.ASCII)
            if kind == 'ecp' and atom:
                if primitives:
                    components.append([shell, primitives])
                    primitives = []
                if components:
                    numeric['ecp'][center] = [nelec, components]
                    components = []
                center, nelec = atom.groups()
            component = re.match(r'^\s*\w+\s+(\w+)$', line, re.ASCII)
            if kind == 'ecp' and component:
                if primitives:
                    components.append([shell, primitives])
                    primitives = []
                shell = component.group(1)
            if re.search('END', line, re.I | re.ASCII) and kind == 'ecp':
                # A lone ECP component is not flushed by the Perl reader.
                if components:
                    components.append([shell, primitives])
                    primitives = []
                    numeric['ecp'][center] = [nelec, components]
                    components = []
        elif reading_name and not reading_numeric:
            if header:
                coords = header.group(2)
                new_kind = {'ao': 'orbital', 'xc': 'exchange', 'cd': 'charge'}.get(
                    header.group(1))
                if new_kind:
                    kind = new_kind
                    names[kind]['coordinants'] = coords
            atom = re.match(r'^\s*(\w+)\s+library\s+(".+")$', line, re.I | re.ASCII)
            if atom:
                center, basis_name = atom.groups()
                if kind in names:
                    names[kind][center] = basis_name
            if re.match(r'^\s*ECP', line, re.I | re.ASCII):
                kind = 'ecp'
    for kind, key in (('orbital', 'gbs'), ('exchange', 'dftXCfitting'),
                      ('charge', 'dftCDfitting'), ('ecp', 'ecp')):
        if numeric[kind]:
            bs[key] = numeric[kind]
        if names[kind]:
            bs['name_' + key] = names[kind]
    return bs


def translator_main(writer, code, revision):
    """Getopt::Std's -h/-v interface, including ignored unknown options."""
    help_option = version = False
    args = sys.argv[1:]
    while args and args[0].startswith('-') and args[0] != '-':
        arg = args.pop(0)
        if arg == '--':
            break
        if arg in ('--help', '--version'):
            # Keep Getopt::Std's compatibility banner as legacy text; these
            # options continue translating, unlike the documented -h/-v.
            sys.stderr.write(
                '%s version [unknown] calling Getopt::Std::getopts (version 1.14 [paranoid]),\n'
                'running under Perl version 5.40.1.\n' % sys.argv[0])
            if arg == '--help':
                sys.stderr.write(
                    '\nUsage: std2%s [-OPTIONS [-MORE_OPTIONS]] [--] [PROGRAM_ARG1 ...]\n'
                    '\nThe following single-character options are accepted:\n'
                    '\tBoolean (without arguments): -h -v\n'
                    '\nOptions may be merged together.  -- stops processing of options.\n' % code)
            sys.stderr.write(
                '  [Now continuing due to backward compatibility and excessive paranoia.\n'
                "   See 'perldoc Getopt::Std' about $Getopt::Std::STANDARD_HELP_VERSION.]\n")
            continue
        for option in arg[1:]:
            if option == 'h':
                help_option = True
            elif option == 'v':
                version = True
            else:
                sys.stderr.write('Unknown option: %s\n' % option)
    if help_option:
        sys.stdout.write('Usage: std2%s\nOptions:    -h        this help\n'
                         '            -v        version\n' % code)
    elif version:
        sys.stdout.write('Revision: %s \n' % revision)
    else:
        try:
            if hasattr(sys.stdout, 'reconfigure'):  # not a redirect_stdout buffer
                sys.stdout.reconfigure(encoding="latin-1")
            writer(readStandardBS())
        except ValueError as error:
            sys.stderr.write(str(error) + '\n')
            return 255
    return 0
