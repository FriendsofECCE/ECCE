///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropTSVecString.C
//
//
// DESIGN:
//   Each value in the vector is a string instead of a double
//
///////////////////////////////////////////////////////////////////////////////

// system includes
// general includes
// library includes

// application includes
   #include "util/ErrMsg.H"
   #include "tdat/PropTSVecString.H"


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty vector
//
///////////////////////////////////////////////////////////////////////////////
PropTSVecString::PropTSVecString(const string& type,
                           const string& units) : TProperty(type, units)
{
  // p_label is empty string from default string constructor

  p_values = new vector<string>; 
  p_classType = TProperty::PROPTSVECSTRING;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropTSVecString::PropTSVecString(const PropTSVecString& tsVecString)
               : TProperty(tsVecString)
{
   p_label   = tsVecString.p_label;
   p_values  = new vector<string>((*tsVecString.p_values));
}

 
// ---------- Virtual Destructor ------------
PropTSVecString::~PropTSVecString(void)
{
  delete p_values;
}


// ---------- Operators ----------
   // defined in parent class


// ---------- Accessors ----------
int PropTSVecString::rows(void) const
{
  return p_values->size();
}

/////////////////////////////////////////
string PropTSVecString::value(int row) const
{
  // Return the value for the given row index
  // Assumes index is within bounds of the vector

  if (row < 0 || row >= p_values->size())
    EE_RT_ASSERT(false, EE_WARNING,
                 "trying to access out-of-bounds index in PropTSVecString");

  return (*p_values)[row];
}

/////////////////////////////////////////

const vector<string>& PropTSVecString::values(void) const
{
  return *p_values;  // return a reference to p_values which cannot be changed
}

/////////////////////////////////////////

string PropTSVecString::label(void) const
{
  return p_label; // label for all values
}


// ------------ Modifiers ------------

///////////////////////////////////////////////////////////////////////////////
//
// Reset values given a vector of strings as data
//   - assume the size of the input vector is the desired size of PropTSVecString
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecString::values(const vector<string>& values)
{
  p_values->clear();        // remove any pre-existing values

  for (int i = 0; i < values.size(); i++)
     p_values->push_back(values[i]);
}

///////////////////////////////////////////////////////////////////////////////
//
// Append one value to the back of the vector
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecString::appendValue(const string& value)
{
  p_values->push_back(value);
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the overall label of the PropTSVecString
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecString::label(const string& newLabel)
{
   p_label = newLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Update the PropTSVecString based on a tt message
// Append one string to the back of the vector.
///////////////////////////////////////////////////////////////////////////////
bool PropTSVecString::update(const string& msg)
{
  appendValue(msg);
  return true;
}





