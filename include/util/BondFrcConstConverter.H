///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: BondFrcConstConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different torsion force
//                 constant units.
//
// Design
//   Implements different angle conversion factors
///////////////////////////////////////////////////////////////////////////////
#ifndef BONDFRCCONSTCONVERTER_HH
#define BONDFRCCONSTCONVERTER_HH

#include "util/BaseUnitConverter.H"

class BondFrcConstConverter: public BaseUnitConverter
{
  public:
    BondFrcConstConverter();
    virtual ~BondFrcConstConverter();
};

#endif
