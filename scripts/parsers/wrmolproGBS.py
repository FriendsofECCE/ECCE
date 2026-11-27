"""Emit the legacy MOLPRO basis block without changing its numeric conventions."""

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
    'Unq': 'Unnilquaduim',
    'Unp': 'Unnilpentium',
    'Unh': 'Unnihexium',
    'Uns': 'Unnioctium',
}


def _scientific(value, pattern):
    # The legacy mantissa shift keeps the exponent after multiplying by ten.
    text = (pattern % (perl_number(value) * 10)).replace('e', 'E')
    return re.sub(r'(-*)(\d)(\.)(\d)(\d+)', r'\g<1>0\3\2\4\5', text, count=1)


def writemolpro(bs):
    out = sys.stdout.write
    gbs = dict(bs['gbs'])
    gbs.pop('coordinants', None)
    gbs.pop('polarization', None)
    out('basis={\n')
    for atom in sorted(gbs):
        orbitals = gbs[atom]
        out('! %s\n' % ELEMENT_NAMES.get(atom, '').upper())
        for shell in 'SPDFGHIJ':
            exponents = []
            all_coefficients = []
            shell_exists = False
            for orbital_type, rows in orbitals:
                columns = []
                if shell.lower() in orbital_type.lower():
                    shell_exists = True
                    exponents.extend(row[0] for row in rows)
                    index, num_coeff = 0, 1
                    while index <= num_coeff:
                        column = []
                        for row in rows:
                            coefficients = row[1:]
                            if orbital_type.lower() == 'sp' and shell in 'SP':
                                c = 0 if shell == 'S' else 1
                                column.append(coefficients[c] if c < len(coefficients) else '')
                                num_coeff = 0
                            else:
                                column.append(coefficients[index] if index < len(coefficients) else '')
                                num_coeff = len(coefficients) - 1
                        columns.append(column)
                        index += 1
                all_coefficients.append(columns)
            if shell_exists:
                out('%s,%s ' % (shell.lower(), atom.upper()))
            for exponent in exponents:
                out(_scientific(exponent, ',%14.8e'))
                # Numeric equality also breaks a line at earlier duplicate values.
                if perl_number(exponent) == perl_number(exponents[-1]):
                    out('\n')
            first, last = 1, 0
            for columns in all_coefficients:
                for column_index, column in enumerate(columns):
                    if column_index:
                        first, last = 1, 0
                    found = False
                    for coefficient in column:
                        number = perl_number(coefficient)
                        if not found and number == 0:
                            first += 1
                            last += 1
                        elif found and number == 0:
                            break
                        elif number != 0 and not found:
                            last += 1
                            if len(column) == 1:
                                first = last
                        elif number != 0:
                            last += 1
                        if number != 0:
                            found = True
                    out('c,%d.%d' % (first, last))
                    for coefficient in column:
                        if perl_number(coefficient) != 0:
                            out(_scientific(coefficient, ',%15.8e'))
                    if column:
                        out('\n')
                if last != 0:
                    first = last + 1
    out('}\n')
