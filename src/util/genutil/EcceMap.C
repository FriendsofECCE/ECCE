/*************************************************************************
 Class: EcceMap
 
 Summary:  Provides an easy way to switch between a map of key/value
           pairs and a single formatted string of all pairs.
           The string provides an easy way to save and read the data to
           a file or to a DAV meta data property.
**************************************************************************/
#include <iostream>
#include <strstream>
using std::istrstream;

#include "util/EcceMap.H"
#include "util/KeyValueReader.H"


// PUBLIC MEMBER FUNCTIONS

/*************************************************************************
 Method :  Constructor
 Summary: 
*************************************************************************/
EcceMap::EcceMap()
{
  separator = ':';
}

/*************************************************************************
 Method :  Copy Constructor
 Summary: 
*************************************************************************/
EcceMap::EcceMap(const EcceMap& ecceMap) : 
  map<string, string, less<string> >(ecceMap)
{
  separator = ecceMap.separator;
}

/*************************************************************************
 Method : Contains 
 Summary: Returns true if key is in the map.
*************************************************************************/
bool EcceMap::contains(const string& key) const
{
  bool ret = false;
  EcceMap::const_iterator it = find(key);
  
  if (it != end()) {
    ret = true;
  }
  
  return ret;
}

/*************************************************************************
 Method : findValue
 Summary: Returns true if key was found, and assigns value to value
          parameter.
*************************************************************************/
bool EcceMap::findValue(const string& key, string& value) const
{
  bool ret = false;
  EcceMap::const_iterator it = find(key);
  
  if (it != end()) {
    ret = true;
    value = (*it).second;
  }
  
  return ret;
}

/*************************************************************************
 Method : insert
 Summary: Returns true if key was inserted into the map
*************************************************************************/
bool EcceMap::insert(string& key, string& value)
{ 
  bool ret;
  pair<EcceMap::iterator, bool> valPair = 
    map<string, string, less<string> >::insert( EcceMapPair(key,value) );

  ret = valPair.second;
  return ret;
}

/*************************************************************************
 Method:  setSeparator
 Summary: Changes the token separator value.
*************************************************************************/
void EcceMap::setSeparator(char sep)
{
  separator = sep;
}

/*************************************************************************
 Method:  operator=
 Summary: Parses a string into key value pairs for the map.
          Uses KeyValueReader class to parse the values.
          Each key/value pair must be on a separate line.
*************************************************************************/
EcceMap& EcceMap::operator=(const string& data)
{
  // Make an istrstream out of the string, then use its streambuf to
  // initialize the KeyValueReader

  string key, value;
  istrstream buf(data.c_str());
  istream input(buf.rdbuf());
  KeyValueReader reader(input);

  reader.setSeparator(separator);  

  while (reader.getpair(key, value))
    map<string, string, less<string> >::insert( EcceMapPair(key, value));
  return *this;
}

/**
 * Load a map from a file using delim to tokenize.
 */
EcceMap *EcceMap::load(const string& filename,char delim)
{
  EcceMap *ret = new EcceMap();
  KeyValueReader reader(filename);

  reader.setSeparator(delim);  

  string key, value;
  while (reader.getpair(key, value))
    ret->insert( key, value);
  return ret;
}

/*************************************************************************
 Method:  toString
 Summary: Converts the map into a string of key/value pairs where each
          key/value pair is on a separate line and the keys are separated
          from the values by the specified separator (':' is default).  
          For example:

          key:value
          key:value
          .
          .
          .
*************************************************************************/
string EcceMap::toString() const
{
  string mapString;
  map<string, string, less<string> >::const_iterator mIt;

  for (mIt = begin(); mIt != end(); mIt++)
    mapString += (*mIt).first + separator + (*mIt).second + '\n';

  return mapString;

}

