///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: LengthConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different length units.
//
// Design
//   All behavior except conversion setup, delegated to superclass.
//   Implements different energy conversion factors
///////////////////////////////////////////////////////////////////////////////
#ifndef LENGTHCONVERTER_HH
#define LENGTHCONVERTER_HH

#include "util/BaseUnitConverter.H"

class LengthConverter: public BaseUnitConverter
{
  public:

    LengthConverter();
    virtual ~LengthConverter();

  protected:

};

#endif
