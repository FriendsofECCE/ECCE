///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropVecString.H
//
//
// CLASS SYNOPSIS:
//    Upgrade of former DBVector class (object store free)
//    Uncertainties have been removed.
//
// EXPORTED TYPES:
//    Class PropVecString
//
// SEE ALSO:
//    class TProperty, class PropFactory
//      
// DESCRIPTION:
//    PropVecString contains an STL vector of values, a corresponding STL
//    vector of row labels, an overall rowLabel, and an overall valueLabel.
//    PropVecString is write once, meaning all the values in the vector are
//    assigned at the same time.
//
// TODO
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PROPVECSTRING_HH
#define PROPVECSTRING_HH

#include <iostream>
  using std::istream;

#include "tdat/TProperty.H"

class PropVecString : public TProperty
{

public:
  // You can only copy a valid property
        PropVecString(const PropVecString& propVecString);  // copy constructor

  // Virtual Destructor
        virtual ~PropVecString(void);

  // Operators (defined in parent class)

  // Accessors
        int    rows(void) const;                       // number of values
        string value(int row) const;                   // single value
        vector<string>& values(void) const;     // all values
        vector<string>* rowLabels(void) const;         // row headings
        string rowLabel(void) const;
        string valueLabel(void) const;

  // Modifiers
        void setData(istream& istrm);               // set all values/labels
                                                       // from file
        void values(const vector<string>& values);  // set all values
 
        void rowLabels(const vector<string>* row_labels);
        void rowLabel(const string& newRowLabel);
        void valueLabel(const string& newValueLabel);
        virtual bool update(const string& msg);

protected:

 // Member Objects
    vector<string>* p_values;      // vector of values
    vector<string>* p_rowLabels;   // vector of row headers

 // Constructors (only PropFactory can create - ensures validity)
    PropVecString(const string& type, const string& units);   // empty vector

private:

 // State
    string          p_rowLabel;    // overall row heading
    string          p_valueLabel;  // overall value heading
    
 // Friends:
    friend class PropFactory;

}; // PropVecString

#endif /* PROPVECSTRING_HH */
