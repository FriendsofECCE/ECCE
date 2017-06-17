///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropTSVector.H
//
//
// CLASS SYNOPSIS:
//    Upgrade of former DBVector class (object store free)
//    Uncertainties have been removed.
//
// EXPORTED TYPES:
//    Class PropTSVector (i.e., time stepped vector)
//
// SEE ALSO:
//    class TProperty, class PropFactory
//      
// DESCRIPTION:
//    PropTSVector contains an STL vector of values, a corresponding STL
//    vector of row labels, an overall rowLabel, and an overall valueLabel.
//
//    PropTSVector has the same fuctionality as PropVector, except PropTSVector
//    is write many, meaning that values may be appended one at at time.
//
// TODO
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TSVECTOR_HH
#define TSVECTOR_HH

#include "tdat/PropVector.H"


class PropTSVector : public PropVector
{

public:
  // You can only copy a valid property
     PropTSVector(const PropTSVector& tsVector);  // copy constructor
 
  // Accessors
     bool canScalarize(void) const; // virtual
     double scalarize(void) const; // virtual

  // Modifiers
     void appendValue(const double& value);  // append one value to end
     void insertValues(const vector<double>& values);

     bool update(const string& msg);         // append one value from a tt msg

private:

  // Constructors (only PropFactory can create - ensures validity)
     PropTSVector(const string& type, const string& units);  // empty vector


 // Friends:
    friend class PropFactory;

}; // PropTSVector

#endif /* TSVECTOR_HH */
