////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropTSVecVector.H
//
//
// CLASS SYNOPSIS:
//    Contains a vector of (1-d) vectors.  PropTSVecVector is write many,
//    meaning that vectors (rows) ay be appended to the vector one at a time.
//
// EXPORTED TYPES:
//    Class PropTSVecVector
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
#ifndef TSVECVECTOR_HH
#define TSVECVECTOR_HH

#include "tdat/TProperty.H"

// Need to declare explicitly so this class can be used as template
// types inside a vector<...> declaration:
   //class vector<double>;

class PropTSVecVector : public TProperty
{
public:
  // Can only copy a valid property
        PropTSVecVector(const PropTSVecVector& tsVecVector);// copy constructor

  // Virtual Destructor
        virtual ~PropTSVecVector(void);

  // Operators (defined in parent class)

  // Accessors
        int    rows(void) const;                   // number of rows
        int    vectors(void) const;                // number of vectors (cols)
        double value(int vec, int row) const;      // single value
        const  vector<double>& values(int vec) const;   // one vector of values
        vector<string>* rowLabels() const;         // row headings
        vector<string>* vecLabels() const;         // vector headings
        string rowLabel(void) const;                    // overall row title
        string vecLabel(void) const;                    // overall vec title

  // Modifiers
        void appendVector(const vector<double>& values); 
        void setLabels( const vector<string>* rowLabels,
                        const vector<string>* vecLabels);

        void clear(void);                           // clear all vectors
        void rowLabel(const string& newLabel);
        void vecLabel(const string& newLabel);
        bool update(const string& msg); 

protected:

   // Member Objects

   // Derived Class Accessors

private:

  // Constructors (only PropFactory can create properties - ensures validity)
        PropTSVecVector(const string& type, const string& units);  // empty vector

  // State
     string                    p_rowLabel;
     string                    p_vectorLabel;
     vector< vector<double> >* p_values;      // the data
     vector<string>*           p_rowLabels;
     vector<string>*           p_vecLabels;

  // Friends:
     friend class PropFactory;

}; // TSVecTable

#endif /* TSVECTABLE_HH */

