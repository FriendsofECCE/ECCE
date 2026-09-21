///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: PropTSVecTable.C
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
#include "tdat/PropTSVecTable.H"


// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//
// Initialize empty vector of tables
//
///////////////////////////////////////////////////////////////////////////////
PropTSVecTable::PropTSVecTable(const string& type,
                       const string& units) : TProperty(type, units)
{
  // rowLabel and columnLabel are empty strings from default string constructor

  p_values      = new vector< vector<double> >;   // create empty vector
  p_rowLabels   = 0;
  p_colLabels   = 0;
  p_tableLabels = 0;
  p_numRows     = 0;
  p_numColumns  = 0;
  p_classType   = TProperty::PROPTSVECTABLE;

}

///////////////////////////////////////////////////////////////////////////////
//
// Copy constructor
//
//////////////////////////////////////////////////////////////////////////////
PropTSVecTable::PropTSVecTable(const PropTSVecTable& tsVecTable) : TProperty(tsVecTable)
{
   p_rowLabel    = tsVecTable.p_rowLabel;
   p_columnLabel = tsVecTable.p_columnLabel;
   p_tableLabel  = tsVecTable.p_tableLabel;
   p_numRows     = tsVecTable.p_numRows;
   p_numColumns  = tsVecTable.p_numColumns;

   if (tsVecTable.p_rowLabels != 0)
      p_rowLabels = new vector<string>((*tsVecTable.p_rowLabels));
   else
      p_rowLabels = 0;
   
   if (tsVecTable.p_colLabels != 0)
      p_colLabels = new vector<string>((*tsVecTable.p_colLabels));
   else 
      p_colLabels = 0;
   
   if (tsVecTable.p_tableLabels != 0)
      p_tableLabels = new vector<string>((*tsVecTable.p_tableLabels));
   else
      p_tableLabels = 0;

   p_values = new vector< vector<double> >((*tsVecTable.p_values));
}

 
// ---------- Virtual Destructor ------------
PropTSVecTable::~PropTSVecTable(void)
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
int PropTSVecTable::rows(void) const
{
  return p_numRows;
}

int PropTSVecTable::columns(void) const
{
  return p_numColumns;
}

int PropTSVecTable::tables(void) const
{
  return p_values->size();
}

/////////////////////////////////////////
double PropTSVecTable::value(int table, int row, int col) const
{
  // Return the value for the given table/row/column index
  double ret = 0;

  if (p_values == 0) {
    EE_RT_ASSERT(false, EE_WARNING, "Cannot access value - table is "
		 "empty.");
  }

  else if (table >= (int)p_values->size() || row >= p_numRows ||
	   col >= p_numColumns || table < 0 || row < 0 || col < 0) {
    EE_RT_ASSERT(false, EE_WARNING,
                 "trying to access out-of-bounds index in PropTSVecTable");
  }
  else {
    int index = row * p_numColumns + col;
    // p_numRows/p_numColumns describe the *declared* shape of the series,
    // which setValues() takes from a single rows/columns attribute on the
    // stored property document.  Individual tables in the series are NOT
    // guaranteed to actually have p_numRows*p_numColumns entries -- only
    // appendTable() enforces that, and the DAV load path (PropertyTask::
    // getPropTSVecTable -> setValues) bypasses it entirely.  A parser that
    // emits steps of differing lengths under one property key therefore
    // produces a series where a row/col pair that passes the check above
    // still indexes past the end of this particular table's vector (see
    // GitHub issue #27: the Gaussian .desc files route "Input orientation",
    // "Z-Matrix orientation" and "Standard orientation" blocks -- which have
    // different atom counts when dummy centres are present -- into the one
    // GEOMTRACE key).  Validate against the real vector size too, otherwise
    // this is a silent heap over-read rather than a diagnosable warning.
    if (index >= (int)(*p_values)[table].size()) {
      EE_RT_ASSERT(false, EE_WARNING,
                   "trying to access out-of-bounds index in PropTSVecTable "
                   "(this table is shorter than the series' declared "
                   "rows*columns)");
    } else {
      ret = (*p_values)[table][index];
    }
  }
  return ret;
}

/////////////////////////////////////////

const vector<double>& PropTSVecTable::values(int table) const
{
  // Bounds-check: callers index this by a step number that comes from
  // elsewhere (playback sliders, a sibling property's step count, ...).
  static const vector<double> empty;
  if (p_values == 0 || table < 0 || table >= (int)p_values->size()) {
    EE_RT_ASSERT(false, EE_WARNING,
                 "trying to access out-of-bounds table in PropTSVecTable");
    return empty;
  }
  return (*p_values)[table]; // return values for one table
}

/////////////////////////////////////////

vector<string>* PropTSVecTable::rowLabels() const
{
  return p_rowLabels; // return row headings

}

/////////////////////////////////////////

vector<string>* PropTSVecTable::colLabels() const
{
  return p_colLabels; // return column headings

}

/////////////////////////////////////////

vector<string>* PropTSVecTable::tableLabels() const
{
  return p_tableLabels; // return table headings

}

/////////////////////////////////////////

string PropTSVecTable::rowLabel(void) const
{
  return p_rowLabel; // label for all rows
}

/////////////////////////////////////////

string PropTSVecTable::columnLabel(void) const
{
  return p_columnLabel; // label for all columns
}

/////////////////////////////////////////

string PropTSVecTable::tableLabel(void) const
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
void PropTSVecTable::setData(istream& istrm)
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
          EE_RT_ASSERT(false, EE_WARNING,
                "size specified in file is greater than the number of values");

        tmpString = line;
        colonPos  = tmpString.find_last_of(":");
        tableValues.push_back(atof(tmpString.substr(colonPos+1).c_str()));
      }
      p_values->push_back(tableValues);
   }
}
void PropTSVecTable::setValues(vector<vector<double> >* data,  unsigned long rows, unsigned long columns)
{
   // This is the path the DAV load takes (PropertyTask::getPropTSVecTable):
   // rows/columns come from a single pair of attributes on the property
   // document -- which putTSProp only ever writes from the *first* step --
   // while data holds one vector per <step> element, each of whatever length
   // that step's parse script happened to emit.  Nothing here used to check
   // that the two agreed, so a series with non-uniform steps was accepted
   // silently and every subsequent value() call on a short step read past the
   // end of its vector (GitHub issue #27).  Drop the steps that cannot be
   // interpreted under the declared shape rather than keeping them as
   // landmines; the accessors bounds-check as a backstop.
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
                      "PropTSVecTable: discarded step(s) whose size does not "
                      "match the property's declared rows*columns");
      }
   }
}
///////////////////////////////////////////////////////////////////////////////
//
//  Appends one table of values and (optional) labels to the back
//  of the table vector.
///////////////////////////////////////////////////////////////////////////////
void PropTSVecTable::appendTable(int rows, int cols,
                        const vector<double>& values)
{
  if (p_values->size() == 0)
  {
     p_numRows = rows;
     p_numColumns = cols;
  }

// make sure input vector matches its specified size.
// This used to warn and then push the mismatched vector anyway -- EE_WARNING
// only logs, it does not stop execution, so the malformed table still entered
// the series and every later value() call on it read past its end.  Same
// fall-through-after-warning shape fixed across this file family in f8e3be6.
   if (values.size() != (size_t)(rows * cols)) {
     EE_RT_ASSERT(false, EE_WARNING,
                  "input vector length does not match specified size - "
                  "table not appended");
     return;
   }

// make sure the table being appended is the same size as the rest
// of the tables in the vector.
// Previously EE_FATAL, i.e. exit(1): a code whose output happens to yield a
// differently shaped step (see issue #27) killed the whole application rather
// than dropping the step.  Refuse the append and warn instead; the storage
// layer (PropertyTask::updatePropTSVecTable) already drops such steps.
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
void PropTSVecTable::labels( const vector<string>* rowLabels,
                            const vector<string>* colLabels,
                            const vector<string>* tableLabels)
{
  if (rowLabels != 0)
    p_rowLabels = new vector<string>(*rowLabels);
  else 
     p_rowLabels = 0;
  
  if (colLabels != 0)
    p_colLabels = new vector<string>(*colLabels);
  else
     p_colLabels = 0;

  if (tableLabels != 0)
    p_tableLabels = new vector<string>(*tableLabels);
  else
    p_tableLabels = 0; 
}

///////////////////////////////////////////////////////////////////////////////
//
// Clear all the tables in the vector
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecTable::clear(void)
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
// Set the label of the rows of the PropTSVecTable
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecTable::rowLabel(const string& newLabel)
{
   p_rowLabel = newLabel;
}
///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the columns of the PropTSVecTable
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecTable::columnLabel(const string& newLabel)
{
   p_columnLabel = newLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set the label of the tables  of the PropTSVecTable
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecTable::tableLabel(const string& newLabel)
{
   p_tableLabel = newLabel;
}

///////////////////////////////////////////////////////////////////////////////
//
// Set a single value in one of the tables
//
///////////////////////////////////////////////////////////////////////////////
void PropTSVecTable::value(double newValue, int table, int row, int col)
{
  // Check array bounds:
  if(table >= 0 && table < p_values->size())
  {
    if (row >= 0 && row < p_numRows)
    {
      if (col >= 0 && col < p_numColumns)
      {
	int index = row * p_numColumns + col;
	(*p_values)[table][index] = newValue;
      }
      else
	EE_RT_ASSERT (0, EE_WARNING, "Column index out of range.");
    }
    else
      EE_RT_ASSERT (0, EE_WARNING, "Row index out of range.");
  }
  else      
    EE_RT_ASSERT (0, EE_WARNING, "Table index out of range.");
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
bool PropTSVecTable::update(const string& msg)
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


