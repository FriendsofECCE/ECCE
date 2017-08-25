///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropTSVecVector.C
//
//
// DESIGN:
//  Contains a vector of vectors (rows).
//
///////////////////////////////////////////////////////////////////////////////

#include <strstream>
using std::istrstream;

#include "tdat/PropTSVecVector.H"
#include "util/ErrMsg.H"

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty vector of vectors
//
///////////////////////////////////////////////////////////////////////////////
PropTSVecVector::PropTSVecVector(const string& type,
                       const string& units) : TProperty(type, units)
{
  // rowLabel and vectorLabel are empty strings from default string constructor

  p_values      = new vector< vector<double> >;   // create empty vector
  p_rowLabels   = 0;
  p_vecLabels   = 0;
  p_classType   = TProperty::PROPTSVECVECTOR;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropTSVecVector::PropTSVecVector(const PropTSVecVector& tsVecVector) 
                                               : TProperty(tsVecVector)
{
   p_rowLabel    = tsVecVector.p_rowLabel;
   p_vectorLabel = tsVecVector.p_vectorLabel;

   if (tsVecVector.p_rowLabels != 0)
      p_rowLabels = new vector<string>((*tsVecVector.p_rowLabels));
   if (tsVecVector.p_vecLabels != 0)
      p_vecLabels = new vector<string>((*tsVecVector.p_vecLabels));

   p_values = new vector< vector<double> >((*tsVecVector.p_values));
}

 
// ---------- Virtual Destructor ------------
PropTSVecVector::~PropTSVecVector(void)
{
  delete p_values;
  if(p_rowLabels != 0)
     delete p_rowLabels;
  if(p_vecLabels != 0)
     delete p_vecLabels;
}


// ---------- Operators ----------
   // defined in parent class


// ---------- Accessors ----------
int PropTSVecVector::rows(void) const
{
  if (p_values->size() != 0)
    return (*p_values)[0].size();
  else
    return 0;
}

int PropTSVecVector::vectors(void) const
{
  return p_values->size();
}

/////////////////////////////////////////
double PropTSVecVector::value(int vec, int row) const
{
  // Return the value for the given row/column index
  // Assumes index is within bounds of the vectors

  EE_ASSERT(vec < p_values->size() && (*p_values)[vec].size() < row,
            EE_FATAL,
            "trying to access out-of-bounds index in PropTSVecVector")

  return (*p_values)[vec][row];
}

/////////////////////////////////////////

const vector<double>& PropTSVecVector::values(int vec) const
{
  return (*p_values)[vec]; // return values for one vector
}

/////////////////////////////////////////

vector<string>* PropTSVecVector::rowLabels() const
{
  return p_rowLabels; // return row headings

}

/////////////////////////////////////////

vector<string>* PropTSVecVector::vecLabels() const
{
  return p_vecLabels; // return column headings

}

/////////////////////////////////////////

string PropTSVecVector::rowLabel(void) const
{
  return p_rowLabel; // overall row label for rows
}

/////////////////////////////////////////

string PropTSVecVector::vecLabel(void) const
{
  return p_vectorLabel; // overall label for vectors
}


/***************MODIFIERS*********************************/

///////////////////////////////////////////////////////////////////////////////
//
//  Appends one vector of values to the back of the vector.
///////////////////////////////////////////////////////////////////////////////
void PropTSVecVector::appendVector(const vector<double>& values)
{
// make sure the vector being appended is the same size as the rest
// of the vectors in the VecVector
   EE_ASSERT((p_values->size() == 0) ||
             (values.size() == (*p_values)[0].size()),
             EE_FATAL,
             "appending vector that is not the same size as the others")

   p_values->push_back(values);

}

///////////////////////////////////////////////////////////////////////////////
//
// Set all the row/vector headings (labels) in the vector
//
// Assumes that length of rowLabels will match numRows and
// length of vecLabels will match numVectors
///////////////////////////////////////////////////////////////////////////////
void PropTSVecVector::setLabels( const vector<string>* rowLabels,
                             const vector<string>* vecLabels)
{
  if (rowLabels != 0)
    p_rowLabels = new vector<string>(*rowLabels);
  if (vecLabels != 0)
    p_vecLabels = new vector<string>(*vecLabels);
}

///////////////////////////////////////////////////////////////////////////////
//
// Clear all the vectors in the vector
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecVector::clear(void)
{
  p_values->clear();
  if (p_rowLabels != 0) {
    delete p_rowLabels;
    p_rowLabels = 0;
  }
  if (p_vecLabels != 0) {
    delete p_vecLabels;
    p_vecLabels = 0;
  }
}  

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the rows of the PropTSVecVector
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecVector::rowLabel(const string& newLabel)
{
   p_rowLabel = newLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the vectors of the PropTSVecVector
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecVector::vecLabel(const string& newLabel)
{
   p_vectorLabel = newLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Update one vector based on a tool talk message
// Assumes the vector is a 1 column row vector
// Rows are delimited by CR
// 
///////////////////////////////////////////////////////////////////////////////
bool PropTSVecVector::update(const string& msg)
{
   istrstream input(msg.c_str());
   char temp[100];
   vector<double> values;

   while(input.getline(temp,100))
      values.push_back(atoi(temp));
   appendVector(values);  // this function checks to make sure rows
                          // are the correct length
   return true;
}

