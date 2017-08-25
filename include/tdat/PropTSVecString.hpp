///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropTSVecString.H
//
//
// CLASS SYNOPSIS:
//    Upgrade of former DBVecString class (object store free)
//
// EXPORTED TYPES:
//    Class PropTSVecString
//
// SEE ALSO:
//    class TProperty, class TSVector, class PropFactory
//      
// DESCRIPTION:
//    Just like TSVector only values are strings instead of doubles, and
//    there is no corresponding vector of labels.
//
// TODO
//
///////////////////////////////////////////////////////////////////////////////
#ifndef TSVECSTRING_HH
#define TSVECSTRING_HH

// system includes

// library includes

// application includes
   #include "tdat/TProperty.H"


class PropTSVecString : public TProperty
{

public:
  // Can only copy a valid property
        PropTSVecString(const PropTSVecString& tsVecString);  // copy constructor

  // Virtual Destructor
        virtual ~PropTSVecString(void);

  // Operators (defined in parent class)

  // Accessors
        int    rows(void) const;                   // number of values
        string value(int row) const;               // single value
        const  vector<string>& values(void) const; // all values
        string label(void) const;                  // overall label

  // Modifiers
        void values(const vector<string>& values);   // set all values
        void appendValue(const string& value);       // append one value to end
        void label(const string& newLabel);          // set overall label
        bool update (const string& msg);

protected:

   // Member Objects

   // Derived Class Accessors

private:

  // Constructors (only PropFactory can create properties - ensures validity)
        PropTSVecString(const string& type, const string& units);// empty vector

 // State 
    string          p_label;       // overall label
    vector<string>* p_values;      // vector of values
    
 // Friends:
    friend class PropFactory;
}; // PropTSVecString

#endif /* TSVECSTRING_HH */
