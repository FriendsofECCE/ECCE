///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: PropertyInterpreter.H
//
//
// CLASS SYNOPSIS:
//    Class PropertyInterpreter is a class used to define the interface for 
//    storing calculation properties.     
//
//
// LOCAL TYPES:
//
// SEE ALSO:
//
// DESCRIPTION:
//    The class converts formatted stream data into a property cache entry. 
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IPROPERTYINTERPRETER_H
#define _IPROPERTYINTERPRETER_H

#include <iostream>
  using std::istream;
  using std::ostream;
#include <string>
  using std::string;

#include "tdat/TProperty.H"

class PropertyDoc;
class Property_Ref;


class PropertyInterpreter {

  public:
    PropertyInterpreter();
    PropertyDoc* parse(istream& propStream, const string& key, string& log,  const unsigned long step = 0 );
  private:
    PropertyDoc*  processProperty(const Property_Ref* propref, istream& propStream, string& log, const unsigned long step = 0);
    string processUnits(istream &propStream, PropertyDoc* value);
    string processSize(istream &propStream, PropertyDoc* value);
    string processLabels(istream &propStream, PropertyDoc* value, const string& labelType);
    string processValues(istream& propStream, PropertyDoc* value); 
    string processVecTableValues(istream& propStream, PropertyDoc* value); 
    bool validatePropertyDoc( PropertyDoc* propdoc, string& os);

    bool p_ecceJobLogAll;         //flag set based on environment variable.
                                  //If true all messages are logged otherwise
                                  //only error, warning, failure will be logged.

};
#endif

