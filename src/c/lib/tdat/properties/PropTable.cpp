///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropTable.C
//
//
// DESIGN:
//  Each value in the matrix has a corresponding lable in a separate matrix.
//  Each matrix is represented as a 1-d vector of values, and is indexed
//  using:  index = row * numColumns + col
//
///////////////////////////////////////////////////////////////////////////////

#include <strstream>
using std::istrstream;

#include <stdlib.h>         // atoi

#include "util/ErrMsg.H"
#include "tdat/PropTable.H"


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty table
//
///////////////////////////////////////////////////////////////////////////////
PropTable::PropTable(const string& type,
                       const string& units) : TProperty(type, units)
{
  // rowLabel and columnLabel are empty strings from default string constructor

  p_values     = new vector<double>;         // create empty vector
  p_rowLabels  = 0;
  p_colLabels  = 0;                          
  p_numRows    = 0;
  p_numColumns = 0;
  p_classType  = TProperty::PROPTABLE;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropTable::PropTable(const PropTable& propTable) : TProperty(propTable)
{
   p_rowLabel    = propTable.p_rowLabel;
   p_columnLabel = propTable.p_columnLabel;
   p_numRows     = propTable.p_numRows;
   p_numColumns  = propTable.p_numColumns;
   if (p_rowLabels != 0)
      p_rowLabels = new vector<string>((*propTable.p_rowLabels));
   else 
      p_rowLabels = 0;
   if (p_colLabels != 0)
      p_colLabels = new vector<string>((*propTable.p_colLabels));
   else
      p_colLabels = 0;
   p_values      = new vector<double>((*propTable.p_values));
}

 
// ---------- Virtual Destructor ------------
PropTable::~PropTable(void)
{
  delete p_values;
  if (p_rowLabels != 0)
     delete p_rowLabels;
  if (p_colLabels != 0)
     delete p_colLabels;
}


// ---------- Operators ----------
   // defined in parent class


// ---------- Accessors ----------
int PropTable::rows(void) const
{
  return p_numRows;
}

int PropTable::columns(void) const
{
  return p_numColumns;
}

/////////////////////////////////////////
double PropTable::value(int row, int col) const
{
  // Return the value for the given row/column index
  // Assumes index is within bounds of the vector

  if (row >= p_numRows || col >= p_numColumns || row < 0 || col < 0)
    EE_RT_ASSERT(false, EE_WARNING,
                 "trying to access out-of-bounds index in PropTable");

  int index = row * p_numColumns + col;
  return (*p_values)[index];
}

/////////////////////////////////////////

vector<double>& PropTable::values(void) const
{
  return *p_values;  
}

/////////////////////////////////////////

vector<string>* PropTable::rowLabels(void) const
{
  return p_rowLabels; // return row headings
}

/////////////////////////////////////////

vector<string>* PropTable::colLabels(void) const
{
  return p_colLabels; // return row headings

}
/////////////////////////////////////////

string PropTable::rowLabel(void) const
{
  return p_rowLabel; // label for all rows
}

/////////////////////////////////////////

string PropTable::columnLabel(void) const
{
  return p_columnLabel; // label for all values
}


// ------------ Modifiers ------------

///////////////////////////////////////////////////////////////////////////////
// Read list of values to insert from the ifstream
// 
// Formatted as:
//    Size:<numRows>,<numColumns>
//    <rowlabel>:<columnlabel>:<value>
//    ...
//    END
//
// This function resets all values in the vectors to those listed in
// the input stream.  Any previous values in the vectors are deleted.
// No labels are stored (<rowlabel> and <columnlabel> are ignored).
//
// Assumes that the # of values = size
// Assumes that the values are listed in order.
///////////////////////////////////////////////////////////////////////////////
void PropTable::values(istream& istrm)
{
   char line[256];
   string tmpString;
   int colonPos;  
   int commaPos;

   istrm.getline(line,255);        // Read the size line
   tmpString = line;
   commaPos = tmpString.find(",");
   
   p_numRows = atoi(tmpString.substr(5, commaPos-5).c_str());
   p_numColumns = atoi(tmpString.substr(commaPos+1).c_str());  

   p_values->clear();        // remove any pre-existing values
   if (p_rowLabels != 0)
      delete p_rowLabels;
   if (p_colLabels != 0)
      delete p_colLabels;

// Loop through the values, assigning to vector
   for (int i=0; i<(p_numRows*p_numColumns); i++)
   {
     if (!istrm.getline(line,255));
       EE_RT_ASSERT(false, EE_FATAL,
             "size specified in file is greater than the number of values");

     tmpString = line;
     colonPos  = tmpString.find_last_of(":");
     p_values->push_back(atof(tmpString.substr(colonPos+1).c_str()));
   }

}

///////////////////////////////////////////////////////////////////////////////
//
// Reset values given a vector of doubles as values
//  - assume the size of the vector = rows * cols
//
///////////////////////////////////////////////////////////////////////////////
void PropTable::values(const int& rows, const int& cols,
                        const vector<double>& values)
{
  if (values.size() != rows * cols)  // make sure dimensions are valid
     EE_RT_ASSERT(false, EE_FATAL,
                  "input vector length does not match rows*columns");
  
  delete p_values;         // clear preexisting values
  p_values = new vector<double>(values);

  p_numRows = rows;
  p_numColumns = cols;
}

///////////////////////////////////////////////////////////////////////////////
//
// Reset row and column headers (labels) given 2 vector of strings
//  - it is developer's responsibility to make sure the column labels
//    are the same size as numColumns and the row labels are the same
//    size as numRows
//
///////////////////////////////////////////////////////////////////////////////
void PropTable::labels(const vector<string>* row_labels,
                          const vector<string>* col_labels)
{ 
  if (p_rowLabels != 0) {
     delete p_rowLabels;
     p_rowLabels = 0;
  }
  if (p_colLabels != 0) {
     delete p_colLabels;
     p_colLabels = 0;
  }

  if (row_labels != 0)
     p_rowLabels = new vector<string>(*row_labels);
  if (col_labels != 0)
     p_colLabels = new vector<string>(*col_labels);
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the rows of the PropTable
//
///////////////////////////////////////////////////////////////////////////////
void PropTable::rowLabel(const string& newRowLabel)
{
   p_rowLabel = newRowLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the columns of the PropTable
//
///////////////////////////////////////////////////////////////////////////////
void PropTable::columnLabel(const string& newColumnLabel)
{
   p_columnLabel = newColumnLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Update the PropTable based on a tool talk message
// Clears out all existing values and rewrites the whole table.
//
// At this point no error checking is performed to ensure that the rows
// and columns are the correct length.
///////////////////////////////////////////////////////////////////////////////
bool PropTable::update(const string& msg)
{
   istrstream input(msg.c_str());
   char temp[100];
   vector<double> newValues;
   int numRows, numCols;
   double currentValue;
   numRows = 0;

   while(input.getline(temp,100)) {
     istrstream row(temp);
     numCols = 0;
     while (row >> currentValue) {
       newValues.push_back(currentValue);
       numCols++;
     }

     numRows++;
   }

   values(numRows, numCols, newValues);
   return true;
}




