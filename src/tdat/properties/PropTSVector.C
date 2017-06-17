///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropTSVector.C
//
//
// DESIGN:
//          Adds an append capability to the PropVector class.
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h> 

#include "tdat/PropTSVector.H"


// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty PropTSVector
//
///////////////////////////////////////////////////////////////////////////////
PropTSVector::PropTSVector(const string& type,
                   const string& units) : PropVector(type, units)
{            
  p_classType = TProperty::PROPTSVECTOR;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropTSVector::PropTSVector(const PropTSVector& tsVector) : PropVector(tsVector)
{

}

// ---------- Accessors ----------

/////////////////////////////////////////
bool PropTSVector::canScalarize(void) const
{
  // All PropTSVectors can be scalarized because
  // they are stepped
  
  return true;
}

/////////////////////////////////////////
double PropTSVector::scalarize(void) const
{
  // returns the last value in the vector
  // assumes this will only be called if canScalarize is true

  double ret = 0.0;
  if (p_values->size() > 0)
    ret = (*p_values)[p_values->size()-1];

  return ret;
}

// ------------ Modifiers ------------

///////////////////////////////////////////////////////////////////////////////
//
// Append one value to the back of the vector
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVector::appendValue(const double& value)
{
  p_values->push_back(value);
}

void PropTSVector::insertValues(const vector<double>& values)
{
  p_values->clear();
  *p_values = values;
}


bool PropTSVector::update(const string& msg)
{
  p_values->push_back(atoi(msg.c_str()));
  return true;
}
