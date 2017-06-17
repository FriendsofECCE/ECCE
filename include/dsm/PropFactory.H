///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropFactory.H
//
//
// CLASS SYNOPSIS:
//    PropFactory is used to:
//      1) Create valid properties.
//      2) Obtain property information that is constant for all properties
//         of the same type - this info is stored in a PropertyRef, with
//         one PropertyRef per property.
//     
// EXPORTED TYPES:
//    Class PropFactory
//
// LOCAL TYPES:
//    struct PropertyRef
//
// SEE ALSO:
//    classes TProperty, PropValue, PropVector, PropTable, PropVecTable,
//            PropVecString
//      
// DESCRIPTION:
//    PropFactory loads up a static vector of PropertyRefs from the
//    props file.  (It is now located on the DAV server in the /Ecce/v2.0
//    directory.)
//    
//    Static functions query the vector to find various property info
//    (such as classification, short_name, long_name, units, etc.).
//    This enables general data (which are constant for all properties of
//    the same type) to be stored only once, instead of repeating the same
//    info for every property.
// 
//    In several cases the property type (i.e., short_name) is used as the
//    lookup key.
//
//    Since the constant list of PropertyRefs is very small (around 130),
//    and since different lookup keys are used for different queries, a 
//    simple linear search was used for each of the static lookup functions.
//
//    PropFactory is also used to create a valid TProperty subclass given a
//    particular type (short_name) and units (which can be left blank).
//    PropFactory ensures that the created subproperty and its units are valid.
//
// EXAMPLES:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PROPFACTORY_HH
#define PROPFACTORY_HH

#include <iostream>
  using std::ostream;
#include <set>
  using std::set;
  using std::less;
#include <vector>
  using std::vector;

#include "tdat/TProperty.H"
#include "util/EcceException.H"



/*********************************************
  Declare Property_Ref struct
*********************************************/
struct Property_Ref
{
 // Data members:
    string        classification;       // Energy, Geometry, Vibration
    string        long_name;            // Full Description
    string        short_name;           // Internal Keyword Name
    string        units;                // Units
    string        dbclass;              // Full property class name
    TProperty::ClassType classType;     // TProperty subclass type as enum
    vector<string> index;               // Dimension index names
 
 // Accessors:   
    bool canGraph(void) const; 
    bool canLabel(void) const;
    int dimensions(void) const;
    string dimension(size_t index) const;
    string storagetype() const;
};

// Define Property_Ref comparison operator required by set objects:
// (by default set uses the < operator for comparison)
   bool operator<(const Property_Ref& ref1, const Property_Ref& ref2);

   ostream& operator<<(ostream& os, const Property_Ref& propref);

/**
 * Class to manage creation of ecce properties.
 *
 * 
 * All public methods may indirectly throw an EcceException if there are
 * problems accessing or parsing the properties database.
 */
class PropFactory
{
  public:

  // Constructor
     // Don't need one because all functions are static

  // Destructor
     ~PropFactory(void);  // don't really need one because no dynamic memory

  // Static utility functions:

     // Creates a valid TProperty subclass object
     static TProperty*   getProperty(const string& shortName);
 
     // Performs a lookup on the short_name and returns the Property_Ref info
     // associated with that type
     // returns NULL If invalid short_name entered
     static const Property_Ref* getPropRef(string short_name);

     // Returns an ordered list of all Property_Refs that match the given
     // classification (e.g., all Energy properties)
     // (set class is used because it automatically orders on insert)
     static set<Property_Ref, less<Property_Ref> > ofClassification(
                                         const string& classification);
    
     // Returns an ordered list of all Property_Refs that match the given
     // dbclass storage type (e.g. PropTSVector, PropValue, PropString)
     // (set class is used because it automatically orders on insert)
     static set<Property_Ref, less<Property_Ref> > ofStorageType(
            const string& storageType);

     // Returns a unique list of all known units:
     static set<string, less<string> > knownUnits(void);

     // Returns a list of all the all the property refs that match
     // the given primary Dimension (e.g., "Geometry Step")
     // only valid for properties with dimension >= 1 (i.e., no PropValue)
     static vector<Property_Ref> propsIndexedBy(const vector<string>& dims);
     static vector<Property_Ref> propsIndexedBy(const string& first,
            const string& second="", const string& third="");
     static set<Property_Ref, less<Property_Ref> > ofIndex(
            const vector<string>& dims);
     static set<Property_Ref, less<Property_Ref> > ofIndex(const string& first,
            const string& second="", const string& third="");

  private:

  // Methods:
     // Loads propRefList one time only from file:
     static void initialize(void) throw (EcceException);

  // State
     static vector<Property_Ref>  propRefList;
                                 // only one copy is instantiated
};

#endif // PROPFACTORY_HH
