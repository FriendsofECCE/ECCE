"""Emit the legacy MOLPRO_INT_LIB basis block without changing its numeric conventions."""

import re
import sys

from rdStandardGBS import perl_number


ELEMENT_NAMES = {
    'H': 'Hydrogen',
    'He': 'Helium',
    'Li': 'Lithium',
    'Be': 'Beryllium',
    'B': 'Boron',
    'C': 'Carbon',
    'N': 'Nitrogen',
    'O': 'Oxygen',
    'F': 'Flourine',
    'Ne': 'Neon',
    'Na': 'Sodium',
    'Mg': 'Magnesium',
    'Al': 'Aluminum',
    'Si': 'Silicon',
    'P': 'Phosphorus',
    'S': 'Sulfur',
    'Cl': 'Chlorine',
    'Ar': 'Argon',
    'K': 'Potassium',
    'Ca': 'Calcium',
    'Sc': 'Scandium',
    'Ti': 'Titanium',
    'V': 'Vanadium',
    'Cr': 'Chromium',
    'Mn': 'Manganese',
    'Fe': 'Iron',
    'Co': 'Cobalt',
    'Ni': 'Nickel',
    'Cu': 'Copper',
    'Zn': 'Zinc',
    'Ga': 'Gallium',
    'Ge': 'Germanium',
    'As': 'Arsenic',
    'Se': 'Selenium',
    'Br': 'Bromine',
    'Kr': 'Krypton',
    'Rb': 'Rubidium',
    'Sr': 'Strontium',
    'Y': 'Yttrium',
    'Zr': 'Zirconium',
    'Nb': 'Nioblum',
    'Mo': 'MolyBdenum',
    'Tc': 'Technetium',
    'Ru': 'Ruthenium',
    'Rh': 'Rhodium',
    'Pd': 'Palladium',
    'Ag': 'Silver',
    'Cd': 'Cadmium',
    'In': 'Indium',
    'Sn': 'Tin',
    'Sb': 'Antimony',
    'Te': 'Tellurium',
    'I': 'Iodine',
    'Xe': 'Xenon',
    'Cs': 'Cesium',
    'Ba': 'Barium',
    'La': 'Lanthanum',
    'Ce': 'Cerium',
    'Pr': 'Praseodymium',
    'Nd': 'Neodymium',
    'Pm': 'Promethium',
    'Sm': 'Samarium',
    'Eu': 'Europium',
    'Gd': 'Gadolinium',
    'Tb': 'Terbium',
    'Dy': 'Dysprosium',
    'Ho': 'Holmium',
    'Er': 'Erbium',
    'Tm': 'Thulium',
    'Yb': 'Ytterbium',
    'Lu': 'Lutetium',
    'Hf': 'Hafnium',
    'Ta': 'Tantalum',
    'W': 'Tungsten',
    'Re': 'Rhenium',
    'Os': 'Osmium',
    'Ir': 'Iridium',
    'Pt': 'Platinum',
    'Au': 'Gold',
    'Hg': 'Mercury',
    'Tl': 'Thallium',
    'Pb': 'Lead',
    'Bi': 'Bismuth',
    'Po': 'Polonium',
    'At': 'Astatine',
    'Rn': 'Radon',
    'Fr': 'Francium',
    'Ra': 'Radium',
    'Ac': 'Actinium',
    'Th': 'Thorium',
    'Pa': 'Protactinium',
    'U': 'Uranium',
    'Np': 'Neptunium',
    'Pu': 'Plutonium',
    'Am': 'Americium',
    'Cm': 'Curium',
    'Bk': 'Berkelium',
    'Cf': 'Californium',
    'Es': 'Einsteinium',
    'Fm': 'Fermium',
    'Md': 'Mendelevium',
    'No': 'Nobelium',
    'Lr': 'Lawrencium',
    'Rf': 'Rutherfordium',
    'Db': 'Dubnium',
    'Sg': 'Seaborgium',
    'Bh': 'Bohrium',
    'Hs': 'Hassium',
    'Mt': 'Meitnerium',
}


def _scientific(value):
    text = ('%17.9e' % (perl_number(value) * 10)).replace('e', 'E')
    return re.sub(r'(-*)(\d)(\.)(\d)(\d+)', r'\g<1>0\3\2\4\5', text, count=1)


def writemolpro_int_lib(bs):
    out = sys.stdout.write
    gbs = dict(bs['gbs'])
    gbs.pop('coordinants', None)
    gbs.pop('polarization', None)
    for atom in sorted(gbs):
        orbitals = gbs[atom]
        out('! %s\n' % ELEMENT_NAMES.get(atom, '').upper())
        counts = dict.fromkeys('SPDFGHI', 0)
        for orbital_type, rows in orbitals:
            if orbital_type == 'SP':
                counts['S'] += 1
                counts['P'] += 1
            else:
                counts[orbital_type] = counts.get(orbital_type, 0) + 1
        for shell in 'SPDFGHI':
            wrapped, started = 0, False
            for orbital_type, rows in orbitals:
                if shell.lower() in orbital_type.lower():
                    if not started:
                        out('  %s  %s\n' % (atom.upper(), shell.lower()))
                    started = True
                    for row in rows:
                        out(_scientific(row[0]))
                        wrapped += 1
                        if wrapped == 5:
                            out('\n')
                            wrapped = 0
            shell_count = 0
            for orbital_type, rows in orbitals:
                if shell not in orbital_type:
                    continue
                shell_count += 1
                num_coeff = (1 if orbital_type == 'SP' else
                             (len(rows[0]) - 1 if rows else 0))
                for index in range(num_coeff):
                    for row in rows:
                        coefficients = row[1:]
                        c = index
                        if orbital_type == 'SP' and shell in 'SP':
                            c = 0 if shell == 'S' else 1
                        value = coefficients[c] if c < len(coefficients) else ''
                        text = _scientific(value)
                        # Test the formatted value, as Perl does, before wrapping.
                        if perl_number(text) != 0:
                            out(text)
                            wrapped += 1
                        if wrapped == 5:
                            out('\n')
                            wrapped = 0
                if shell_count == counts.get(shell, 0) and wrapped != 0:
                    out('\n')
    ecp = dict(bs.get('ecp', {}))
    ecp.pop('polarization', None)
    shells = 'spdfgh'
    for atom in sorted(ecp):
        electrons, components = ecp[atom]
        count = len(components) - 1
        first_shell = shells[count] if -len(shells) <= count < len(shells) else ''
        out('ECP,atom #,%d,%d;\n' % (int(perl_number(electrons)), count))
        for index, (shell_type, primitives) in enumerate(components):
            shell_type = first_shell if index == 0 else (
                (shells[index - 1] if index <= len(shells) else '') + '-' + first_shell)
            shell_type += ' potential'
            out('%d; ! %-13s\n' % (len(primitives), shell_type))
            for primitive in primitives:
                out('%d,%8.8f,%8.8f;\n' % (
                    int(perl_number(primitive.get('Rexponent'))),
                    perl_number(primitive.get('GaussExponent')),
                    perl_number(primitive.get('Coefficient'))))
