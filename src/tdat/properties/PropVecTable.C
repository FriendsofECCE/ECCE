///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropVecTable.C
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
#include "tdat/PropVecTable.H"


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty vector of tables
//
///////////////////////////////////////////////////////////////////////////////
PropVecTable::PropVecTable(const string& type,
                       const string& units) : TProperty(type, units)
{
  // rowLabel and columnLabel are empty strings from default string constructor

  p_values      = new vector< vector<double> >;   // create empty vector
  p_rowLabels   = 0;
  p_colLabels   = 0;
  p_tableLabels = 0;
  p_numRows     = 0;
  p_numColumns  = 0;
  p_classType   = TProperty::PROPVECTABLE;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropVecTable::PropVecTable(const PropVecTable& vecTable) : TProperty(vecTable)
{
   p_rowLabel    = vecTable.p_rowLabel;
   p_columnLabel = vecTable.p_columnLabel;
   p_tableLabel  = vecTable.p_tableLabel;
   p_numRows     = vecTable.p_numRows;
   p_numColumns  = vecTable.p_numColumns;

   if (vecTable.p_rowLabels != 0)
      p_rowLabels = new vector<string>((*vecTable.p_rowLabels));
   if (vecTable.p_colLabels != 0)
      p_colLabels = new vector<string>((*vecTable.p_colLabels));
   if (vecTable.p_tableLabels != 0)
      p_tableLabels = new vector<string>((*vecTable.p_tableLabels));

   p_values = new vector< vector<double> >((*vecTable.p_values));
}

 
// ---------- Virtual Destructor ------------
PropVecTable::~PropVecTable(void)
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
int PropVecTable::rows(void) const
{
  return p_numRows;
}

int PropVecTable::columns(void) const
{
  return p_numColumns;
}

int PropVecTable::tables(void) const
{
  return p_values->size();
}

/////////////////////////////////////////
double PropVecTable::value(int table, int row, int col) const
{
  // Return the value for the given table/row/column index
  // Assumes index is within bounds of the vectors

  if (table >= (int)p_values->size() || row >= p_numRows || col >= p_numColumns
      || table < 0 || row < 0 || col < 0) {
    // Same fall-through-after-warning bug shape fixed in PropTable::value()
    // -- EE_WARNING doesn't stop execution, so return early instead of
    // falling through to the unguarded access below.
    EE_RT_ASSERT(false, EE_WARNING,
          "trying to access out-of-bounds index in PropVecTable");
    return 0.0;
  }

  int index = row * p_numColumns + col;
  // p_numRows/p_numColumns describe the declared shape; setValues() accepts
  // whatever the stored document contains, so an individual table can be
  // shorter than rows*columns (see GitHub issue #27 and PropTSVecTable.C).
  // Check the real vector size too - otherwise this is a silent over-read.
  if (index >= (int)(*p_values)[table].size()) {
    EE_RT_ASSERT(false, EE_WARNING,
          "trying to access out-of-bounds index in PropVecTable (this table is "
          "shorter than the declared rows*columns)");
    return 0.0;
  }
  return (*p_values)[table][index];
}

/////////////////////////////////////////

const vector<double>& PropVecTable::values(int table) const
{
  static const vector<double> empty;
  if (p_values == 0 || table < 0 || table >= (int)p_values->size()) {
    EE_RT_ASSERT(false, EE_WARNING,
          "trying to access out-of-bounds table in PropVecTable");
    return empty;
  }
  return (*p_values)[table]; // return values for one table
}

/////////////////////////////////////////

vector<string>* PropVecTable::rowLabels() const
{
  return p_rowLabels; // return row headings

}

/////////////////////////////////////////

vector<string>* PropVecTable::colLabels() const
{
  return p_colLabels; // return column headings

}

/////////////////////////////////////////

vector<string>* PropVecTable::tableLabels() const
{
  return p_tableLabels; // return table headings

}

/////////////////////////////////////////

string PropVecTable::rowLabel(void) const
{
  return p_rowLabel; // label for all rows
}

/////////////////////////////////////////

string PropVecTable::columnLabel(void) const
{
  return p_columnLabel; // label for all columns
}

/////////////////////////////////////////

string PropVecTable::tableLabel(void) const
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
void PropVecTable::setData(istream& istrm)
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
        if (!istrm.getline(line,255))
          EE_RT_ASSERT(false, EE_FATAL,
                "size specified in file is greater than the number of values");

        tmpString = line;
        colonPos  = tmpString.find_last_of(":");
        tableValues.push_back(atof(tmpString.substr(colonPos+1).c_str()));
      }
      p_values->push_back(tableValues);
   }
}

void PropVecTable::setValues(vector<vector<double> >* data,  unsigned long rows, unsigned long columns)
{
   // See PropTSVecTable::setValues() / GitHub issue #27: rows/columns come
   // from the stored document's attributes, the per-table vectors from its
   // data, and nothing guaranteed the two agreed.  Drop tables that cannot be
   // interpreted under the declared shape instead of leaving them as
   // out-of-bounds landmines for value().
   if (p_values != 0 && p_values != data) delete p_values;   // was leaked

   p_numRows = rows;
   p_numColumns = columns;
   p_values = data;

   if (p_values != 0 && p_numRows > 0 && p_numColumns > 0) {
      size_t expected = (size_t)p_numRows * (size_t)p_numColumns;
      size_t dropped = 0;
      vector< vector<double> >::iterator it = p_values->begin();
      while (it != p_values->end()) {
         if (it->size() != expected) {
            it = p_values->erase(it);   // NB: reassign, don't it++ after erase
            dropped++;
         } else {
            ++it;
         }
      }
      if (dropped > 0) {
         EE_RT_ASSERT(false, EE_WARNING,
                      "PropVecTable: discarded table(s) whose size does not match the "
                      "property's declared rows*columns");
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Appends one table of values and (optional) labels to the back
//  of the table vector.
///////////////////////////////////////////////////////////////////////////////
void PropVecTable::appendTable(int rows, int cols,
                        const vector<double>& values)
{
  if (p_values->size() == 0)
  {
     p_numRows = rows;
     p_numColumns = cols;
  }

// make sure input vector matches its specified size
   if (values.size() != (size_t)(rows * cols)) {
     // Was EE_FATAL (exit(1)): malformed parser output killed the whole
     // application instead of dropping one table.  Refuse the append instead.
     EE_RT_ASSERT(false, EE_WARNING,
                  "input vector length does not match specified size - "
                  "table not appended");
     return;
   }

// make sure the table being appended is the same size as the rest
// of the tables in the vector
   if (rows != p_numRows || cols != p_numColumns) {
     EE_RT_ASSERT(false, EE_WARNING,
           "trying to append a table that is not the same size as "
           "the other tables in the vector - table not appended");
     return;
   }

   p_values->push_back(values);

}

///////////////////////////////////////////////////////////////////////////////
//
// Set all the row/column/table headings (labels) in the vector
//
///////////////////////////////////////////////////////////////////////////////
void PropVecTable::labels( const vector<string>* rowLabels,
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
void PropVecTable::clear(void)
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
// Set the label of the rows of the PropVecTable
//
///////////////////////////////////////////////////////////////////////////////
void PropVecTable::rowLabel(const string& newLabel)
{
   p_rowLabel = newLabel;
}
///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the columns of the PropVecTable
//
///////////////////////////////////////////////////////////////////////////////
void PropVecTable::columnLabel(const string& newLabel)
{
   p_columnLabel = newLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the tables  of the PropVecTable
//
///////////////////////////////////////////////////////////////////////////////
void PropVecTable::tableLabel(const string& newLabel)
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
bool PropVecTable::update(const string& msg)
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


