# Two-letter chemical element symbols (lowercased). Used to catch genuine elements like
# Hf/Hg/He/Kr/Os *before* the single-letter fallbacks below would collapse them to H/K/O.
# Deliberately EXCLUDES force-field type codes that equal a symbol but mean something else:
#   'ho' = CLAYFF/MINFF hydroxyl hydrogen  (must stay H, not Holmium)
#   'at' = tetrahedral-aluminium code      (must not become Astatine)
#   'sc' = already mapped to a Si variant above
# Matching is on the FULL token (exact equality), so multi-char FF names ('ob', 'oh',
# 'st', 'ao', 'nap'...) are unaffected and fall through to the prefix rules.
_TWO_LETTER_ELEMENTS = frozenset({
    'he', 'li', 'be', 'ne', 'na', 'mg', 'al', 'si', 'cl', 'ar', 'ca', 'ti', 'cr', 'mn',
    'fe', 'co', 'ni', 'cu', 'zn', 'ga', 'ge', 'as', 'se', 'br', 'kr', 'rb', 'sr', 'zr',
    'nb', 'mo', 'tc', 'ru', 'rh', 'pd', 'ag', 'cd', 'in', 'sn', 'sb', 'te', 'xe', 'cs',
    'ba', 'la', 'ce', 'pr', 'nd', 'pm', 'sm', 'eu', 'gd', 'tb', 'dy', 'er', 'tm', 'yb',
    'lu', 'hf', 'ta', 're', 'os', 'ir', 'pt', 'au', 'hg', 'tl', 'pb', 'bi', 'po', 'rn',
    'fr', 'ra', 'ac', 'th', 'pa', 'np', 'pu', 'am', 'cm', 'bk', 'cf', 'es', 'fm', 'md',
    'no', 'lr',
})


def element(atoms):
    """Guess the chemical element for multiple atom entries and update atom types.
    
    Uses explicit prefix matching on the 'type' field to determine the element,
    similar to the MATLAB implementation. If 'type' is not available, falls back to
    'atname' or 'resname'. Supports special cases like 'Ale', 'Alt', 'Fee', 'Fet', 'Ow', 'Hw'.
    
    Also updates the 'type' field to match the element name (e.g., 'SOD' -> 'Na'),
    which standardizes atom names for output files.
    
    Args:
       atoms: A list of atom dictionaries, each with at least one of 'type', 'atname', or 'resname' keys.
             Can also accept a single atom dictionary.

    Returns:
       The atoms list with updated 'element' and 'type' fields in each atom dictionary.
       If a single atom was provided, returns that atom with these fields updated.
    """
    # Handle the case where a single atom is provided
    single_atom_input = False
    if not isinstance(atoms, list):
        atoms = [atoms]
        single_atom_input = True
    
    # Process each atom in the list
    for atom in atoms:
        # If 'element' is already provided and non-empty (e.g., from PDB cols 77-78),
        # trust it, standardize capitalization, and update type to match.
        current_element = atom.get('element')
        if isinstance(current_element, str) and current_element.strip():
            # Standardize to capitalized form (e.g., 'al' -> 'Al', 'AL' -> 'Al')
            atom['element'] = current_element.strip().capitalize()
            atom['type'] = atom['element']
            continue

        # First try to get the atom type, falling back to atname or resname
        atomtype = atom.get('type', '')
        if atomtype is None:
            atomtype = ''
        else:
            atomtype = atomtype.strip()
            
        if not atomtype and 'atname' in atom:
            atname = atom.get('atname', '')
            if atname is not None:
                atomtype = atname.strip()
                
        if not atomtype and 'name' in atom:  # PDB files often use 'name' instead of 'atname'
            name = atom.get('name', '')
            if name is not None:
                atomtype = name.strip()
                
        if not atomtype and 'resname' in atom:
            resname = atom.get('resname', '')
            if resname is not None:
                atomtype = resname.strip()
        
        # Convert to string and ensure case-insensitive comparison
        atomtype_lower = atomtype.lower()
        
        # Use a series of explicit prefix checks similar to the MATLAB implementation
        if atomtype_lower.startswith('si'):
            atom['element'] = 'Si'
        elif atomtype_lower.startswith('sc'):
            atom['element'] = 'Si'  # Special case: SC -> Si
        elif atomtype_lower.startswith('ale'):
            atom['element'] = 'Al'
        elif atomtype_lower.startswith('al'):
            atom['element'] = 'Al'
        elif atomtype_lower.startswith('mg'):
            atom['element'] = 'Mg'
        elif atomtype_lower.startswith('ca'):
            atom['element'] = 'Ca'
        elif atomtype_lower.startswith('fee'):
            atom['element'] = 'Fe'
        elif atomtype_lower.startswith('fet'):
            atom['element'] = 'Fe'
        elif atomtype_lower.startswith('fe'):
            atom['element'] = 'Fe'
        elif atomtype_lower in _TWO_LETTER_ELEMENTS:
            # Exact two-letter element symbol (He, Hf, Hg, Kr, Os, ...) — takes precedence
            # over the single-letter fallbacks below so it isn't collapsed to H/K/O/F.
            atom['element'] = atomtype_lower.capitalize()
        elif atomtype_lower.startswith('f'):
            atom['element'] = 'F'
        elif atomtype_lower.startswith('li'):
            atom['element'] = 'Li'
        elif atomtype_lower.startswith('ow'):
            atom['element'] = 'Ow'
        elif atomtype_lower.startswith('hw'):
            atom['element'] = 'Hw'
        elif atomtype_lower.startswith('o'):
            atom['element'] = 'O'
        elif atomtype_lower.startswith('h'):
            atom['element'] = 'H'
        elif atomtype_lower.startswith('ti'):
            atom['element'] = 'Ti'
        elif atomtype_lower.startswith('sod'):
            atom['element'] = 'Na'
        elif atomtype_lower.startswith('pot'):
            atom['element'] = 'K'
        elif atomtype_lower.startswith('cal'):
            atom['element'] = 'Ca'
        elif atomtype_lower.startswith('cla'):
            atom['element'] = 'Cl'
        elif atomtype_lower.startswith('oh2'):
            atom['element'] = 'Ow'
        elif atomtype_lower.startswith('na'):
            atom['element'] = 'Na'
        elif atomtype_lower.startswith('k'):
            atom['element'] = 'K'
        elif atomtype_lower.startswith('cl'):
            atom['element'] = 'Cl'
        else:
            # If no match found, use the original atom type as the element
            atom['element'] = atomtype
        
        # Also update type to match the element name
        atom['type'] = atom['element']
    
    # Return the input in the same format it was provided
    return atoms[0] if single_atom_input else atoms
