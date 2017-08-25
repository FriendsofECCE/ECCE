////////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropString.H
//
//
// CLASS SYNOPSIS:
//
// EXPORTED TYPES:
//    Class PropString
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
#ifndef PROPSTRING_HH
#define PROPSTRING_HH

#include "tdat/TProperty.H"

class PropString : public TProperty
{

public:
  // You can only copy a valid property
        PropString(const PropString& propValue); // copy constructor

  // Virtual Destructor
        virtual ~PropString(void);

  // Operators (defined in parent class)

  // Accessors
        string value(void) const;
        string label(void) const;

  // Modifiers
        void value(const string& newValue);
        void label(const string& newLabel);
        bool update(const string& msg);

protected:

 
private:

  // Constructors (only PropFactory can create - ensures validity)
        PropString(const string& type,          // creates empty PropString
                  const string& units);

  // State
        string p_label;
        string p_value;

  // Friends
        friend class PropFactory;

}; // PropString

#endif /* PROPSTRING_HH */


