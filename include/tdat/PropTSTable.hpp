////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropTSTable.H
//
//
// CLASS SYNOPSIS:
//    Contains a vector of (2-d) tables.  PropTSTable is write many,
//    meaning that tables may be appended to the vector one at a time.
//
// EXPORTED TYPES:
//    Class PropTSTable
//
// LOCAL TYPES:
//
// SEE ALSO:
//    PropTable, PropVector, TProperty, PropFactory
//  
// DESCRIPTION:
//
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TSTABLE_HH
#define TSTABLE_HH

#include <iostream>
  using std::istream;

#include <vector>
#include <string>
  using std::vector;
  using std::string;

#include "tdat/TProperty.H"

// Need to declare explicitly so this class can be used as template
// type inside a vector<...> declaration:
   //class vector<double>; 

class PropTSTable : public TProperty
{
   public:
      // Can only copy a valid property
      PropTSTable(const PropTSTable& tsVecTable);  // copy constructor

      // Virtual Destructor
      virtual ~PropTSTable(void);

      // Operators (defined in parent class)

      // Accessors
      int    rows(void) const;                   // number of rows
      int    columns(void) const;                // number of columns
      int    tables(void) const;                 // number of tables
      double value(int table, int row,           // single value
            int col) const;  
      const vector<double>& values(int table) const; // one table of values
      vector<string>* rowLabels() const;         // row headings
      vector<string>* colLabels() const;         // column headings
      vector<string>* tableLabels() const;       // table headings
      string rowLabel(void) const;               // overall row title
      string columnLabel(void) const;            // overall column title
      string tableLabel(void) const;             // overall table title

      // Modifiers
      void setData(istream& istrm);               // set all values from file
      void appendTable(int rows, int cols,        // add one table to vector
            const vector<double>& values);

      void labels( const vector<string>* rowLabels,
            const vector<string>* colLabels,
            const vector<string>* tableLabels);

      void clear(void);                           // clear all tables
      void rowLabel(const string& newLabel);
      void columnLabel(const string& newLabel);
      void tableLabel(const string& newLabel);
      bool update (const string& msg);

   protected:

      // Member Objects

      // Derived Class Accessors

   private:

      // Constructors (only PropFactory can create properties - ensures validity)
      PropTSTable(const string& type, const string& units);// empty vector

      // State
      string                    p_tableLabel;
      string                    p_rowLabel;
      string                    p_columnLabel;
      vector< vector<double> >* p_values;      // the data
      vector<string>*           p_rowLabels;
      vector<string>*           p_colLabels;
      vector<string>*           p_tableLabels;  
      int                       p_numRows;    // number of rows in each table
      int                       p_numColumns; // number of columns in each table

      // Friends:
      friend class PropFactory;

}; // PropTSTable

#endif /* TSTABLE_HH */

