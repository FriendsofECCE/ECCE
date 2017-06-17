///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: TrsnFrcConstConverter.H
//
//
// CLASS SYNOPSIS: Class for converting between different torsion force
//                 constant units.
//
// Design
//   Implements different angle conversion factors
///////////////////////////////////////////////////////////////////////////////
#ifndef TRSNFRCCONSTCONVERTER_HH
#define TRSNFRCCONSTCONVERTER_HH

#include "util/BaseUnitConverter.H"

class TrsnFrcConstConverter: public BaseUnitConverter
{
  public:
    TrsnFrcConstConverter();
    virtual ~TrsnFrcConstConverter();
};

#endif
