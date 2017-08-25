///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: SpinMult.H
//
//
// CLASS SYNOPSIS:
//    Used to convert between spin mult enumerated values and text strings.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SPINMULT_H
#define SPINMULT_H

// library includes
   #include <string>

  using std::string;

class SpinMult { 

  public:

  // Enumeration designates spin multiplicity
     enum SpinMultEnum  
     { unknown=0, singlet, doublet, triplet, quartet, quintet, sextet, 
       septet,  octet,   nontet };

  // Enumeration utility functions
     static string toString(SpinMultEnum enumValue);
     static SpinMultEnum toSpinMult(const string& value);
};

#endif // SPINMULT_H

