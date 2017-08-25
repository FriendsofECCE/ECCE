///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: Preferences.H
//
//
// CLASS SYNOPSIS:
//    This Class Provides an Interface to Preference Files of Key-Value Pairs
//
// EXPORTED TYPES:
//    Class Preferences
//
//      
// DESCRIPTION:
//	This class provides an interface to preference files that consist of
//	key/value pairs of information.  
//
//
///////////////////////////////////////////////////////////////////////////////
#ifndef PREFERENCES_HH
#define PREFERENCES_HH

#include <iostream>
using std::ostream;


#include <string>
#include <vector>
#include <map>
using std::string;
using std::vector;
using std::map;
using std::less;

#include "util/KeyValueReader.H"
#include "util/EcceMap.H"

class Preferences 
{

public:

  // Constructors
    Preferences(const string& pref_file, const bool& dataPrefFlag=false,
                int createMode=0644);

  // Virtual Destructor
    virtual ~Preferences(void);

  // Modifiers
    void caseSensitive(bool);
    void setBool(const string& key, const bool& value);
    void setInt(const string& key, const int& value);
    void setFloat(const string& key, const float& value);
    void setString(const string& key, const string& value);
    void setStringList(const string& key, const vector<string>& value);
    void setFloatList(const string& key, const vector<float>& value);
    void setBoolList(const string& key, const vector<bool>& value);
    void setIntList(const string& key, const vector<int>& value);

    void clear();
    void remove_entry(const string& key);

  // Accessors
    bool isValid() const;
    int  numPrefPairs() const;
    bool isDefined(const string& key) const;

    bool getBool(const string& key, bool& value) const;
    bool getInt(const string& key, int& value) const;
    bool getFloat(const string& key, float& value) const;
    bool getString(const string& key, string& value) const;
    bool getStringList(const string& key, vector<string>& value) const;
    bool getFloatList(const string& key, vector<float>& value) const;
    bool getBoolList(const string& key, vector<bool>& value) const;
    bool getIntList(const string& key, vector<int>& value) const;

  // Operators
    bool operator==(const Preferences& rhs) const;

  // Other
    vector<string> keys() const;
    bool saveFile();

    static Preferences * getGlobalPref(bool refresh = false);

protected:

    string patchKey(const string& str) const;
    bool load();
    bool create_pref_file(const string& pref_class, 
			       const string& pref_file,
			       string& filepath, int createMode);


private:

  // State
    bool p_caseSensitive;
    string  p_filename;
    EcceMap p_pref;

    void deletePrefs();
    static char find_delim_char(const vector<string>& strs);

    static Preferences * p_globalPref;

  // Friends
    friend ostream& operator<< (ostream& os, const Preferences&);

}; // Preferences

#endif /* PREFERENCES_HH */
