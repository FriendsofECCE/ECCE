///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropTSTable.C
//
//
// DESIGN:
//  Contains a vector of tables (matrices).
//  Each matrix is represented as a 1-d vector of values, and is indexed
//  using:  index = row * numColumns + col
//
///////////////////////////////////////////////////////////////////////////////

#include <strstream>
using std::istrstream;

#include <stdlib.h>         // atoi

#include "util/ErrMsg.H"
#include "tdat/PropTSTable.H"


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty vector of tables
//
///////////////////////////////////////////////////////////////////////////////
PropTSTable::PropTSTable(const string& type,
                       const string& units) : TProperty(type, units)
{
  // rowLabel and columnLabel are empty strings from default string constructor

  p_values      = new vector< vector<double> >;   // create empty vector
  p_rowLabels   = 0;
  p_colLabels   = 0;
  p_tableLabels = 0;
  p_numRows     = 0;
  p_numColumns  = 0;
  p_classType   = TProperty::PROPTSTABLE;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropTSTable::PropTSTable(const PropTSTable& tsVecTable) : TProperty(tsVecTable)
{
   p_rowLabel    = tsVecTable.p_rowLabel;
   p_columnLabel = tsVecTable.p_columnLabel;
   p_tableLabel  = tsVecTable.p_tableLabel;
   p_numRows     = tsVecTable.p_numRows;
   p_numColumns  = tsVecTable.p_numColumns;

   if (tsVecTable.p_rowLabels != 0)
      p_rowLabels = new vector<string>((*tsVecTable.p_rowLabels));
   if (tsVecTable.p_colLabels != 0)
      p_colLabels = new vector<string>((*tsVecTable.p_colLabels));
   if (tsVecTable.p_tableLabels != 0)
      p_tableLabels = new vector<string>((*tsVecTable.p_tableLabels));

   p_values = new vector< vector<double> >((*tsVecTable.p_values));
}

 
// ---------- Virtual Destructor ------------
PropTSTable::~PropTSTable(void)
{
  delete p_values;
  if(p_rowLabels != 0)
     delete p_rowLabels;
  if(p_colLabels != 0)
     delete p_colLabels;
  if(p_tableLabels != 0)
     delete p_tableLabels;
}


// ---------- Operators ----------
   // defined in parent class


// ---------- Accessors ----------
int PropTSTable::rows(void) const
{
  return p_numRows;
}

int PropTSTable::columns(void) const
{
  return p_numColumns;
}

int PropTSTable::tables(void) const
{
  return p_values->size();
}

/////////////////////////////////////////
double PropTSTable::value(int table, int row, int col) const
{
  // Return the value for the given table/row/column index
  // Assumes index is within bounds of the vectors

  if (table >= p_values->size() || row >= p_numRows || col >= p_numColumns
      || table < 0 || row < 0 || col < 0)
    EE_RT_ASSERT(false, EE_WARNING,
                 "trying to access out-of-bounds index in PropTSTable");

  int index = row * p_numColumns + col;
  return (*p_values)[table][index];
}

/////////////////////////////////////////

const vector<double>& PropTSTable::values(int table) const
{
  return (*p_values)[table]; // return values for one table
}

/////////////////////////////////////////

vector<string>* PropTSTable::rowLabels() const
{
  return p_rowLabels; // return row headings

}

/////////////////////////////////////////

vector<string>* PropTSTable::colLabels() const
{
  return p_colLabels; // return column headings

}

/////////////////////////////////////////

vector<string>* PropTSTable::tableLabels() const
{
  return p_tableLabels; // return table headings

}

/////////////////////////////////////////

string PropTSTable::rowLabel(void) const
{
  return p_rowLabel; // label for all rows
}

/////////////////////////////////////////

string PropTSTable::columnLabel(void) const
{
  return p_columnLabel; // label for all columns
}

/////////////////////////////////////////

string PropTSTable::tableLabel(void) const
{
  return p_tableLabel; // label for all tables
}


// ------------ Modifiers ------------

///////////////////////////////////////////////////////////////////////////////
// Read list of values to insert from the ifstream
// 
// Formatted as:
//    Size:<numTables>,<numRows>,<numColumns>
//    <tablelabel>:<rowlabel>:<columnlabel>:<value>
//    ...
//    END
//
// This function resets all values in the vectors to those listed in
// the input stream.  Any previous values in the vectors are deleted.
// Labels vector is left empty (tablelable, rowlabel, and columnlabel
// are ignored as they are not needed if looping is used).
//
// Assumes that the # of values in the file = size
// ***Assumes that the values are listed in order.
///////////////////////////////////////////////////////////////////////////////
void PropTSTable::setData(istream& istrm)
{
   char line[256];
   string tmpString;
   int colonPos, comma1Pos, comma2Pos, numTables;
   vector<double> tableValues;

   istrm.getline(line,255);        // Read the size line
   tmpString = line;
   comma1Pos = tmpString.find_first_of(",");
   comma2Pos = tmpString.find_last_of(",");

   numTables    = atoi(tmpString.substr(5, comma1Pos-5).c_str());
   p_numRows    = atoi(tmpString.substr(
                   comma1Pos+1, comma2Pos-comma1Pos-1).c_str());  
   p_numColumns = atoi(tmpString.substr(comma2Pos+1).c_str());

   p_values->clear();        // remove any pre-existing values
   if (p_rowLabels != 0)
   {
     delete p_rowLabels;
     p_rowLabels = 0;
   }
   if (p_colLabels != 0)
   {
     delete p_colLabels;
     p_colLabels = 0;
   }
   if (p_tableLabels != 0)
   {
     delete p_tableLabels;
     p_tableLabels = 0;
   }

// Loop through the values, assigning to vector
   for (int j=0; j<numTables; j++)
   {
      tableValues.clear(); // clear temporary values

   // Add values for one table:
      for (int i=0; i<(p_numRows*p_numColumns); i++)
      {
        if (!istrm.getline(line,255));
          EE_RT_ASSERT(false, EE_FATAL,
                "size specified in file is greater than the number of values");

        tmpString = line;
        colonPos  = tmpString.find_last_of(":");
        tableValues.push_back(atof(tmpString.substr(colonPos+1).c_str()));
      }
      p_values->push_back(tableValues);
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Appends one table of values and (optional) labels to the back
//  of the table vector.
///////////////////////////////////////////////////////////////////////////////
void PropTSTable::appendTable(int rows, int cols,
                        const vector<double>& values)
{
  if (p_values->size() == 0)
  {
     p_numRows = rows;
     p_numColumns = cols;
  }

// make sure input vector matches its specified size
   if (values.size() != rows * cols) 
     EE_RT_ASSERT(false, EE_FATAL,
                  "input vector length does not match specified size");

// make sure the table being appended is the same size as the rest
// of the tables in the vector
   if (rows != p_numRows || cols != p_numColumns)
      EE_RT_ASSERT(false, EE_FATAL,
            "trying to append a table that is not the same size as "
            "the other tables in the vector");

   p_values->push_back(values);
}

///////////////////////////////////////////////////////////////////////////////
//
// Set all the row/column/table headings (labels) in the vector
//
///////////////////////////////////////////////////////////////////////////////
void PropTSTable::labels( const vector<string>* rowLabels,
                            const vector<string>* colLabels,
                            const vector<string>* tableLabels)
{
  if (rowLabels != 0)
    p_rowLabels = new vector<string>(*rowLabels);
  if (colLabels != 0)
    p_colLabels = new vector<string>(*colLabels);
  if (tableLabels != 0)
    p_tableLabels = new vector<string>(*tableLabels);
}

///////////////////////////////////////////////////////////////////////////////
//
// Clear all the tables in the vector
//
///////////////////////////////////////////////////////////////////////////////
void PropTSTable::clear(void)
{
  p_values->clear();
  if (p_rowLabels != 0) {
    delete p_rowLabels;
    p_rowLabels = 0;
  }
  if (p_colLabels != 0) {
    delete p_colLabels;
    p_colLabels = 0;
  }
  if (p_tableLabels != 0) {
    delete p_tableLabels;
    p_tableLabels = 0;
  }
}  

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the rows of the PropTSTable
//
///////////////////////////////////////////////////////////////////////////////
void PropTSTable::rowLabel(const string& newLabel)
{
   p_rowLabel = newLabel;
}
///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the columns of the PropTSTable
//
///////////////////////////////////////////////////////////////////////////////
void PropTSTable::columnLabel(const string& newLabel)
{
   p_columnLabel = newLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the tables  of the PropTSTable
//
///////////////////////////////////////////////////////////////////////////////
void PropTSTable::tableLabel(const string& newLabel)
{
   p_tableLabel = newLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Append a single table to the vector based on a tool talk message
// Columns delimited by space; rows delimited by CR.
// 
// At this time no error checking is performed to ensure that the length
// of the rows and columns are correct.
//
///////////////////////////////////////////////////////////////////////////////
bool PropTSTable::update(const string& msg)
{
   istrstream input(msg.c_str());
   char temp[100];
   vector<double> values;
   int numRows, numCols;
   double currentValue;
   numRows = 0;

   while(input.getline(temp,100)) {
     istrstream row(temp);
     numCols = 0;
     while (row >> currentValue) {
       values.push_back(currentValue);
       numCols++;
     }
     numRows++;
   }
   appendTable(numRows, numCols, values);
   return true;
}


