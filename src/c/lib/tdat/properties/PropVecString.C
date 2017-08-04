///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropVecString.C
//
//
// DESIGN:
//   Each row in the vector has a corresponding label.
//   PropVecString is write-once.
//
///////////////////////////////////////////////////////////////////////////////

#include <strstream>
using std::istrstream;

#include <stdlib.h>         // atoi

#include "util/ErrMsg.H"
#include "tdat/PropVecString.H"


// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty vector
//
///////////////////////////////////////////////////////////////////////////////
PropVecString::PropVecString(const string& type,
                       const string& units) : TProperty(type, units)
{
  // rowLabel and valueLabel are empty strings from default string constructor

  p_values    = new vector<string>; 
  p_rowLabels = 0;                 
  p_classType = TProperty::PROPVECSTRING;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropVecString::PropVecString(const PropVecString& propVector) : TProperty(propVector)
{
   p_rowLabel   = propVector.p_rowLabel;
   p_valueLabel = propVector.p_valueLabel;
   if (propVector.p_rowLabels != 0)
      p_rowLabels  = new vector<string>(*(propVector.p_rowLabels));
   else
      p_rowLabels   = 0;
   p_values     = new vector<string>(*(propVector.p_values));
}

 
// ---------- Virtual Destructor ------------
PropVecString::~PropVecString(void)
{
  delete p_values;
  if (p_rowLabels != 0)
     delete p_rowLabels;
}


// ---------- Operators ----------
   // defined in parent class


// ---------- Accessors ----------
int PropVecString::rows(void) const
{
  return p_values->size();
}

/////////////////////////////////////////
string PropVecString::value(int row) const
{
  // Return the value for the given row index
  // Assumes index is within bounds of the vector

  if (row < 0 || row >= p_values->size())
    EE_RT_ASSERT(false, EE_WARNING,
          "trying to access out-of-bounds index in PropVecString");

  return (*p_values)[row];
}

/////////////////////////////////////////

vector<string>& PropVecString::values(void) const
{
  return *p_values;  // return a reference to _values which cannot be changed
}

/////////////////////////////////////////

vector<string>* PropVecString::rowLabels(void) const
{
  return p_rowLabels; // return all row labels (one for each value)
}

/////////////////////////////////////////

string PropVecString::rowLabel(void) const
{
  return p_rowLabel; // label for all rows
}

/////////////////////////////////////////

string PropVecString::valueLabel(void) const
{
  return p_valueLabel; // label for all values
}


// ------------ Modifiers ------------

///////////////////////////////////////////////////////////////////////////////
// Read list of values and corresponding labels to insert from the ifstream
//
//    Formatted as:
//       Size:xx
//       <rowlabel>:<value>
//       ...
//       END
//
// This function resets all values in the rowLabels and value vectors to those
// listed in the input stream.  Any previous values in the vectors are deleted.
//
// Assumes that the # of values = size
///////////////////////////////////////////////////////////////////////////////
void PropVecString::setData(istream& istrm)
{
   char line[256];
   string tmpString;
   int colonPos;  

   istrm.getline(line,255);        // Read the size line
   int numRows = atoi(line+5);     // Remove the "Size:", get size as number

   p_values->clear();               // remove any pre-existing values
   if (p_rowLabels != 0)
     delete p_rowLabels;
   p_rowLabels = new vector<string>;

// Loop through the values, assigning to vector
   for (int i=0; i<numRows; i++)
   {
     if (!istrm.getline(line,255));
       EE_RT_ASSERT(false, EE_FATAL,
             "size specified in file is greater than the number of values");

     tmpString = line;
     colonPos  = tmpString.find(":");
     p_values->push_back(tmpString.substr(colonPos+1).c_str()); // value
     p_rowLabels->push_back(tmpString.substr(0,colonPos));            // label
   }

}

///////////////////////////////////////////////////////////////////////////////
//
// Reset values given a vector of strings as data
//  - assume the size of the array is the desired size of vector
//
///////////////////////////////////////////////////////////////////////////////
void PropVecString::values(const vector<string>& values)
{

  delete p_values;        // remove any pre-existing values
  p_values = new vector<string>(values);
}

///////////////////////////////////////////////////////////////////////////////
//
// Reset row labels given a vector of strings as data
//  - assume the size of the labels array is the same size as the values array
//    (it is up to developer to verify)
//
///////////////////////////////////////////////////////////////////////////////
void PropVecString::rowLabels(const vector<string>* labels)
{
  if (p_rowLabels != 0) {
     delete p_rowLabels;
     p_rowLabels = 0;
  }
  if (labels != 0)
    p_rowLabels = new vector<string>(*labels);
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the rows of the PropVecString
//
///////////////////////////////////////////////////////////////////////////////
void PropVecString::rowLabel(const string& newRowLabel)
{
   p_rowLabel = newRowLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the values of the PropVecString
//
///////////////////////////////////////////////////////////////////////////////
void PropVecString::valueLabel(const string& newValueLabel)
{
   p_valueLabel = newValueLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Reset all the values of the PropVecString based on data from tt message
// One value per row.  Each row is delimited by CR
//
///////////////////////////////////////////////////////////////////////////////
bool PropVecString::update(const string& msg)
{
   istrstream input(msg.c_str());
   char temp[100];
   if (p_values != 0)
     p_values->clear();

   while(input.getline(temp,100))
      p_values->push_back(temp);
   return true;
}




