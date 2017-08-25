///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Fragment.C
//
//
// DESIGN:
//    This class was created from a previous structure because of the need
//    to sort objects of this type.  Particularly, the Formula class
//    needs to maintain a sorted vector of TagCountPair objects so
//    that comparisons are made easier.  In order to allow these
//    sorted vectors we had to provide an operator==() and operator<().
//
///////////////////////////////////////////////////////////////////////////////

// system includes
// general includes
// library includes
// application includes
#include "tdat/TTCPair.H"
#include "util/STLUtil.H"

TTagCountPair::TTagCountPair() {}
TTagCountPair::~TTagCountPair() {}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// OPERATORS
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Comparison Operators Based on the Tags and Counts 
//
///////////////////////////////////////////////////////////////////////////////
bool TTagCountPair::operator==(const TTagCountPair& tcpair) const
{ return (tag == tcpair.tag); }
bool TTagCountPair::operator!=(const TTagCountPair& tcpair) const
{ return (tag != tcpair.tag); }
bool TTagCountPair::operator<(const TTagCountPair& tcpair) const
{ return (tag < tcpair.tag); }

int TTagCountPair::compareTo(const TTagCountPair& tcpair) const
{
  if (tag != tcpair.tag) return (STLUtil::compareString(tag,tcpair.tag));
  return (count - tcpair.count);
}
