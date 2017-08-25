///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: EcceMap.H
//
//
// CLASS SYNOPSIS:
//    Holds a map of key/value pairs for GUI parameters. 
// 
//
// DESCRIPTION:
//    EcceMap is an easy way to convert from a string of keys/values to
//    a map of key/value pairs.
//    
///////////////////////////////////////////////////////////////////////////////

#ifndef ECCEMAP_H
#define ECCEMAP_H

// Library includes:
   #include <map>
   #include <string>

  using std::map;
  using std::string;
  using std::less;
  using std::pair;

// typedef for EcceMap iterator
   typedef map<string, string, less<string> >::const_iterator EcceMapIterator; 

// typedef for STL pair class used in EcceMap
   typedef map<string, string, less<string> >::value_type EcceMapPair;    

class EcceMap : public map<string, string, less<string> > { 

  public:
 
    // Constructors:
       EcceMap();
       EcceMap(const EcceMap& ecceMap);
       static EcceMap *load(const string& filename, char delim=':');

    // Parse a string into key/value pairs that are used to initialize
    // the map:
       EcceMap& operator= (const string& data);

    // Convert the map to a string of key/value pairs using the assigned
    // separator
       string toString() const;

    // Assign a token separator (default value is ':')
       void setSeparator(char sep);

    // Functions for compatibility with RWValHashMap
       bool findValue(const string& key, string& value) const;
       bool contains(const string& key) const;
       bool insert(string& key, string& value);

    // Here are 2 ways to add a key/value pair to the map:
        // ecceMapObject.insert(key,value);
        // ecceMapObject[key] = value;

  private:
    char separator;  // character that separates tokens
};

#endif // ECCEMAP_H

