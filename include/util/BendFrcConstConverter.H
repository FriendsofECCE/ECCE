///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: BendFrcConstConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different angle force units.
//
// Design
//   All behavior except conversion setup, delegated to superclass.
//   Implements different energy conversion factors
///////////////////////////////////////////////////////////////////////////////
#ifndef BENDFRCCONSTCONVERTER_HH
#define BENDFRCCONSTCONVERTER_HH

#include "util/BaseUnitConverter.H"

class BendFrcConstConverter: public BaseUnitConverter
{
  public:
    BendFrcConstConverter();
    virtual ~BendFrcConstConverter();

  private:

};
#endif   
