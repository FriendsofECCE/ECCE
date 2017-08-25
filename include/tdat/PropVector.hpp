///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropVector.H
//
//
// CLASS SYNOPSIS:
//    Upgrade of former DBVector class (object store free)
//    Uncertainties have been removed.
//
// EXPORTED TYPES:
//    Class PropVector
//
// SEE ALSO:
//    class TProperty, class PropFactory
//      
// DESCRIPTION:
//    PropVector contains an STL vector of values, a corresponding STL
//    vector of row labels, an overall rowLabel, and an overall valueLabel.
//    PropVector is write once, meaning all the values in the vector are
//    assigned at the same time.
//
// TODO
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PROPVECTOR_HH
#define PROPVECTOR_HH

#include <iostream>
  using std::istream;

#include "tdat/TProperty.H"


class PropVector : public TProperty
{

public:
  // You can only copy a valid property
        PropVector(const PropVector& propVector);  // copy constructor

  // Virtual Destructor
        virtual ~PropVector(void);

  // Operators (defined in parent class)

  // Analysis functions
	void histogram(vector<double> *x, vector<double> *y);
	vector<double> autocorrelate();

  // Accessors
        int    rows(void) const;                       // number of values
        double value(int row) const;                   // single value
        vector<double>& values(void) const;     // all values
        vector<string>* rowLabels(void) const;         // row headings
        string rowLabel(void) const;
        string valueLabel(void) const;
        virtual bool   canScalarize(void) const;   // virtual function
        virtual double scalarize(void) const;      // virtual function

  // Modifiers
        void setData(istream& istrm);               // set all values/labels
                                                       // from file
        void values(const vector<double>& values);  // set all values
 
        void rowLabels(const vector<string>* row_labels);
        void rowLabel(const string& newRowLabel);
        void valueLabel(const string& newValueLabel);
        virtual bool update(const string& msg);

protected:

 // Member Objects
    vector<double>* p_values;      // vector of values
    vector<string>* p_rowLabels;   // vector of row headers

 // Constructors (only PropFactory can create - ensures validity)
    PropVector(const string& type, const string& units);   // empty vector

private:

 // State
    string          p_rowLabel;    // overall row heading
    string          p_valueLabel;  // overall value heading
    
 // Friends:
    friend class PropFactory;

}; // PropVector

#endif /* PROPVECTOR_HH */
