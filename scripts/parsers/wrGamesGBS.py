"""GAMESS-US basis output; field widths and legacy element spellings are wire format."""

import sys
from rdStandardGBS import perl_number

ELEMENT_NAMES = {'Ac': 'Actinium',
 'Ag': 'Silver',
 'Al': 'Aluminum',
 'Am': 'Americium',
 'Ar': 'Argon',
 'As': 'Arsenic',
 'At': 'Astatine',
 'Au': 'Gold',
 'B': 'Boron',
 'Ba': 'Barium',
 'Be': 'Beryllium',
 'Bh': 'Bohrium',
 'Bi': 'Bismuth',
 'Bk': 'Berkelium',
 'Br': 'Bromine',
 'C': 'Carbon',
 'Ca': 'Calcium',
 'Cd': 'Cadmium',
 'Ce': 'Cerium',
 'Cf': 'Californium',
 'Cl': 'Chlorine',
 'Cm': 'Curium',
 'Co': 'Cobalt',
 'Cr': 'Chromium',
 'Cs': 'Cesium',
 'Cu': 'Copper',
 'Db': 'Dubnium',
 'Dy': 'Dysprosium',
 'Er': 'Erbium',
 'Es': 'Einsteinium',
 'Eu': 'Europium',
 'F': 'Flourine',
 'Fe': 'Iron',
 'Fm': 'Fermium',
 'Fr': 'Francium',
 'Ga': 'Gallium',
 'Gd': 'Gadolinium',
 'Ge': 'Germanium',
 'H': 'Hydrogen',
 'He': 'Helium',
 'Hf': 'Hafnium',
 'Hg': 'Mercury',
 'Ho': 'Holmium',
 'Hs': 'Hassium',
 'I': 'Iodine',
 'In': 'Indium',
 'Ir': 'Iridium',
 'K': 'Potassium',
 'Kr': 'Krypton',
 'La': 'Lanthanum',
 'Li': 'Lithium',
 'Lr': 'Lawrencium',
 'Lu': 'Lutetium',
 'Md': 'Mendelevium',
 'Mg': 'Magnesium',
 'Mn': 'Manganese',
 'Mo': 'MolyBdenum',
 'Mt': 'Meitnerium',
 'N': 'Nitrogen',
 'Na': 'Sodium',
 'Nb': 'Nioblum',
 'Nd': 'Neodymium',
 'Ne': 'Neon',
 'Ni': 'Nickel',
 'No': 'Nobelium',
 'Np': 'Neptunium',
 'O': 'Oxygen',
 'Os': 'Osmium',
 'P': 'Phosphorus',
 'Pa': 'Protactinium',
 'Pb': 'Lead',
 'Pd': 'Palladium',
 'Pm': 'Promethium',
 'Po': 'Polonium',
 'Pr': 'Praseodymium',
 'Pt': 'Platinum',
 'Pu': 'Plutonium',
 'Ra': 'Radium',
 'Rb': 'Rubidium',
 'Re': 'Rhenium',
 'Rf': 'Rutherfordium',
 'Rh': 'Rhodium',
 'Rn': 'Radon',
 'Ru': 'Ruthenium',
 'S': 'Sulfur',
 'Sb': 'Antimony',
 'Sc': 'Scandium',
 'Se': 'Selenium',
 'Sg': 'Seaborgium',
 'Si': 'Silicon',
 'Sm': 'Samarium',
 'Sn': 'Tin',
 'Sr': 'Strontium',
 'Ta': 'Tantalum',
 'Tb': 'Terbium',
 'Tc': 'Technetium',
 'Te': 'Tellurium',
 'Th': 'Thorium',
 'Ti': 'Titanium',
 'Tl': 'Thallium',
 'Tm': 'Thulium',
 'U': 'Uranium',
 'V': 'Vanadium',
 'W': 'Tungsten',
 'Xe': 'Xenon',
 'Y': 'Yttrium',
 'Yb': 'Ytterbium',
 'Zn': 'Zinc',
 'Zr': 'Zirconium'}


def wrGames(bs):
    out = sys.stdout.write
    for atom, orbitals in bs['gbs'].items():
        if atom in ('coordinants', 'polarization'):
            continue
        out(ELEMENT_NAMES.get(atom, '').upper() + '\n')
        for orbital, rows in orbitals:
            shell = 'L' if orbital == 'SP' else orbital
            column = 0
            while column < len(rows[0]) - 1:
                out('%2s%4d\n' % (shell, len(rows)))
                for i, row in enumerate(rows, 1):
                    out('%3d%19.8f%17.8f' %
                        (i, perl_number(row[0]), perl_number(row[column + 1])))
                    if shell == 'L':
                        out('%17.8f' % perl_number(row[column + 2] if column + 2 < len(row) else ''))
                    out('\n')
                column += 2 if shell == 'L' else 1
    shells = ('s', 'p', 'd', 'f', 'g', 'h')
    for atom, (nelec, components) in bs.get('ecp', {}).items():
        if atom == 'polarization':
            continue
        first = shells[len(components) - 1]
        out('%-7s%s %d    %d\n' %
            (atom.upper() + '-ECP', 'GEN', perl_number(nelec), len(components) - 1))
        for i, (_, primitives) in enumerate(components):
            shell = (first if i == 0 else shells[i - 1] + '-' + first) + ' potential'
            out(' %d      ----- %-13s   -----\n' % (len(primitives), shell))
            for prim in primitives:
                out('%16.8f  %s%18.8f\n' %
                    (perl_number(prim['Coefficient']), prim['Rexponent'],
                     perl_number(prim['GaussExponent'])))
        out('\n')
    out('$END\n')
