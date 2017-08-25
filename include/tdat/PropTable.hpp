////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropTable.H
//
//
// CLASS SYNOPSIS:  class for 2d data of doubles
//                  Updated version of DBTable
//
// EXPORTED TYPES:
//    Class PropTable
//
// DESCRIPTION:
//   This class holds 2D double data.  Data is indexed by
//   indices (not row/column label which has been removed).
//   PropTable is write-once, meaning all values in the table
//   are assigned at one time.
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PROPTABLE_HH
#define PROPTABLE_HH

#include <iostream>
  using std::istream;

#include "tdat/TProperty.H"

class PropTable : public TProperty
{

public:
  // Can only copy a valid property
       PropTable(const PropTable& propTable);  // copy constructor

  // Virtual Destructor
        virtual ~PropTable(void);

  // Operators (defined in parent class)

  // Accessors
        int    rows(void) const;                      // number of rows
        int columns(void) const;                      // number of cols
        double value(int row, int col) const;         // single value
        vector<double>& values(void) const;    // all values
        vector<string>* rowLabels(void) const;        // row headings
        vector<string>* colLabels(void) const;        // column headings
        string rowLabel(void) const;               
        string columnLabel(void) const;

  // Modifiers
        void values(istream& istrm);                    // set all values
                                                        // from file
        void values(const int& rows, const int& cols,
                       const vector<double>& values);
        
        void labels(const vector<string>* row_labels,
                       const vector<string>* col_labels);

        void rowLabel(const string& newRowLabel);
        void columnLabel(const string& newColumnLabel);
        bool update(const string& msg);

protected:

 // Constructors (only PropFactory can construct a property - ensures validity)
      PropTable(const string& type, const string& units); // empty

  // Member Objects
      vector<double>* p_values;      // vector of values

  // Derived Class Accessors

private:

 // State
      int p_numRows;                 // number rows in vector
      int p_numColumns;              // number of columns in vector
      string          p_rowLabel;    // overall row heading
      string          p_columnLabel; // overall column heading
      vector<string>* p_rowLabels;   // row headings
      vector<string>* p_colLabels;   // column headings
    
 // Friends
      friend class PropFactory;

}; // PropTable



#endif /* PROPTABLE_HH */

