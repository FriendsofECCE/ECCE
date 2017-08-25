///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TRunType.H
//
//
// CLASS SYNOPSIS:
//    TRunType is an instance of a computational experiments "run-type"
//    or "what is being done by the calculation".
//
// EXPORTED TYPES:
//    Class TRunType TRunTypeExpr
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//	A given instance represents a single run-type for a computation.
//	There are also static members that can be used to validate
//	run-type names.  The valid list of run-types is loaded in from
//	a text file.
//
//	Each runtype has a corresponding gui name which can be any
//	string suitable for GUIs.
//
// EXAMPLES:
//     See test program testTRunType.C
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TRUNTYPE_HH
#define TRUNTYPE_HH

#include <iostream>
  using std::ostream;
#include <string>
  using std::string;


class TRunType
{

public:

  // Constructors
    TRunType();
    TRunType(const char *name, const bool& noSpherical);
    TRunType(const string& name, const bool& noSpherical);
    TRunType(const TRunType& rhs);

  // Virtual Destructor
    virtual ~TRunType(void);

  // Operators
     TRunType& operator=(const TRunType&);
     bool operator==(const TRunType&) const;
     bool operator!=(const TRunType&) const;

  // Modifiers
     void name(const string& new_name);
     void noSpherical(const bool& new_noSpherical);

  // Accessors
     string name(void) const;
     bool   noSpherical(void) const;

protected:


    bool validate_and_assign(const string& runtype, const bool& noSpherical);

private:

  // State
    string p_name;
    bool   p_noSpherical;

    friend ostream& operator<<(ostream& os, const TRunType& rhs);


}; // TRunType


#endif /* TRUNTYPE_HH */
