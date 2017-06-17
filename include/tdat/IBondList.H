/////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: IBondList.H
//
//
// CLASS SYNOPSIS:
//      
// DESCRIPTION:
//  For now, the old BondIndices class.  Want to eliminate later.
//
/////////////////////////////////////////////////////////////////////////////

#ifndef IBONDLIST_HH
#define IBONDLIST_HH

#include "tdat/DisplayStyle.H"

class IBondList {
  public:
    IBondList(size_t numBonds);
    ~IBondList(void);

    size_t  mnumBonds;      // n bonds
    int *matomIndices;   // bond1(atom1, atom2), ..., bondn(atomn, atomm)
    double*  mbondOrders;    // bond1order, bond2order, ..., bondnorder
    DisplayStyle  *mstyle;            // style1, style2...
};

#endif


