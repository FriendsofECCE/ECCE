////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropValue.H
//
//
// CLASS SYNOPSIS:
//    Upgrade of former DBValue class.  Removed uncertainties from all
//    subclasses.
//
// EXPORTED TYPES:
//    Class PropValue
//
// LOCAL TYPES:
//    //<place locally used classes,typedefs here>
//
// SEE ALSO:
//    class TProperty, class PropFactory      
//
// DESCRIPTION:
//    Derived from TProperty.  Represents a property that contains a single
//    value.       
//
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PROPVALUE_HH
#define PROPVALUE_HH

// system includes

// library includes

// application includes
   #include "tdat/TProperty.H"

class PropValue : public TProperty
{

public:
  // You can only copy a valid property
        PropValue(const PropValue& propValue); // copy constructor

  // Virtual Destructor
        virtual ~PropValue(void);

  // Operators (defined in parent class)

  // Accessors
        double value(void) const;
        string label(void) const;
        bool   canScalarize(void) const; // virtual
        double scalarize(void) const;    // virtual

  // Modifiers
        void value(const double& newValue);
        void label(const string& newLabel);
        bool update(const string& msg);

protected:

 
private:

  // Constructors (only PropFactory can create - ensures validity)
        PropValue(const string& type,          // creates empty PropValue
                  const string& units);

  // State
        string p_label;
        double p_value;

  // Friends
        friend class PropFactory;

}; // PropValue

#endif /* PROPVALUE_HH */


