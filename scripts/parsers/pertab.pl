#!/usr/bin/env perl
################################################################################
# FILENAME:  pertab.pl
#
#
# USAGE:
#  executing &loadPT sets up the hashes %symbol_to_name %name_to_symbol
#  %symbol_to_number, and %number_to_symbol from the arrays @atomicSymbol
#  and @atomicName.
#
# PURPOSE:
#    Hashes and arrays for conversion among atomic symbols, atomic 
#    numbers, and atomic names. 
# 
################################################################################
#
sub loadPT{
  @atomicSymbol = 
      ("X","H","He",
       "Li","Be","B","C","N","O","F","Ne",
       "Na","Mg","Al","Si","P","S","Cl","Ar",
       "K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co","Ni","Cu","Zn",
       "Ga","Ge","As","Se","Br","Kr",
       "Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag","Cd",
       "In","Sn","Sb","Te","I","Xe",
       "Cs","Ba","La",
       "Ce","Pr","Nd","Pm","Sm","Eu","Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta",
       "W","Re","Os","Ir","Pt","Au","Hg","Tl","Pb","Bi","Po","At","Rn",
       "Fr","Ra","Ac",
       "Th","Pa","U","Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No","Lr",
       "Rf","Ha","Unh","Uns");

  @atomicName = 
      ( "Ghost", "Hydrogen", "Helium", "Lithium", "Beryllium", 
       "Boron", "Carbon", "Nitrogen", "Oxygen", "Flourine", 
       "Neon", "Sodium", "Magnesium", "Aluminum", "Silicon",
       "Phosphorus", "Sulfur", "Chlorine", "Argon", "Potassium",
       "Calcium", "Scandium", "Titanium", "Vanadium", "Chromium",
       "Manganese", "Iron", "Cobalt", "Nickel", "Copper",
       "Zinc", "Gallium", "Germanium", "Arsenic", "Selenium",
       "Bromine", "Krypton", "Rubidium", "Strontium", "Yttrium",
       "Zirconium", "Nioblum", "Molybdenum", "Technetium", "Ruthenium",
       "Rhodium", "Palladium", "Silver", "Cadmium", "Indium", "Tin",
       "Antimony", "Tellurium", "Iodine", "Xenon", "Cesium", "Barium",
       "Lanthanum", "Cerium", "Praseodymium", "Neodymium", "Promethium",
       "Samarium", "Europium", "Gadolinium", "Terbium", "Dysprosium",
       "Holmium", "Erbium", "Thulium", "Ytterbium", "Lutetium", "Hafnium",
       "Tantalum", "Tungsten", "Rhenium", "Osmium", "Iridium", "Platinum",
       "Gold", "Mercury", "Thallium", "Lead", "Bismuth", "Polonium",
       "Astatine", "Radon", "Francium", "Radium", "Actinium", "Thorium",
       "Protactinium", "Uranium", "Neptunium", "Plutonium", "Americium",
       "Curium", "Berkelium", "Californium", "Einsteinium", "Fermium",
       "Mendelevium", "Nobelium", "Lawrencium", "Rutherfordium", "Hahnium",
       "Unnihexium", "Unnioctium");

  $count = 0;
  foreach $symbol ( @atomicSymbol ) {
    $symbol_to_name{$symbol} = $atomicName[$count];
    $name_to_symbol{$atomicName[$count]} = $symbol;
    $symbol_to_number{$symbol} = $count;
    $number_to_symbol{$count} = $symbol;
    $count++;
  }
  $number_to_symbol{"-1"} = "X";

}

1;
