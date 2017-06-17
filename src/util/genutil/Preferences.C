///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: Preferences.C
//
//
// DESIGN:
//
// LIMITATIONS:
//	. Does not support exponential numbers (e.g., 3.2e4) 
//
///////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fstream>
using std::ifstream;
using std::ofstream;

#include <algorithm>

#include "util/ErrMsg.H"
#include "util/SDirectory.H"
#include "util/Preferences.H"
#include "util/PreferenceLabels.H"
#include "util/Ecce.H"
#include "util/SFile.H"
#include "util/StringConverter.H"


Preferences * Preferences::p_globalPref = NULL;



// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//	Loads the Preference File Located by a Directory and File
//      The File will be Created if it Doesn't Exist
//
//  Implementation
//      The directory identifies the overall class of preferences
//      (like ECCE), and the name identifies the actual file.  
//	Thus, if dir="ECCE" and name="mypref", then the
//	actual preference file is expected to be under the home directory in
//	~/.ECCE/mypref
//
//	If the directory and/or file does not exist, they will be automatically
//	created.
//
//      Modified to allow the directory to be input as a full path
//      specification.  This is to allow the behavior of the two constructors
//      to be consistent.
///////////////////////////////////////////////////////////////////////////////
Preferences::Preferences(
  const string& pref_file,
  const bool& dataPrefFlag,
  int createMode
)
{
  string pref_dir;
  string pref_name;

  // This must be false for it to work.  To support case sensitive keys,
  // need to add another map or something similar.
  p_caseSensitive = false;

  if (dataPrefFlag) {
    if (pref_file[0] != '/') {
      pref_dir = Ecce::ecceDataPrefPath();
      pref_name = pref_file;
    }
    else {
      size_t pos = pref_file.find_last_of('/');
      if (pos == 0)
        pref_dir = '/';
      else
        pref_dir = pref_file.substr(0, pos);
      pref_name = pref_file.substr(pos+1, pref_file.length()-pos-1);
    }
  } else {
    pref_dir = Ecce::realUserPrefPath();
    pref_name = pref_file;
  }

  if (p_pref.size() == 0 && 
      create_pref_file(pref_dir, pref_name, p_filename, createMode))
    load();
}

// ---------- Virtual Destructor ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//	Deallocate Dynamically Allocated Local Memory
//
//  Implementation
//	Since the value is a void*, we need to iterate through the
//	dictionary and free each value and key ourselves.  The
//	clearAndDestroy call can't possibly correctly delete void*.
//
///////////////////////////////////////////////////////////////////////////////
Preferences::~Preferences(void)
{
  deletePrefs();
}

void 
Preferences::deletePrefs(void)
{
  p_pref.clear();
}


// ------------ Accessors ------------
///////////////////////////////////////////////////////////////////////////////
//
//  Description
//	Returns TRUE if the Preferences Have Successfully Been Initialized
//
///////////////////////////////////////////////////////////////////////////////
bool 
Preferences::isValid() const
{ 
  return p_pref.size() > 0; 
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//	Returns the number of Preference key-value pairs that are in the
//      hash dictionary, 0 is returned if the hash dictionary is NULL.
//
///////////////////////////////////////////////////////////////////////////////
int 
Preferences::numPrefPairs() const
{
  return p_pref.size();
}

///////////////////////////////////////////////////////////////////////////////
// Transform the key to upper case if we are working in case insensitive
// mode.
///////////////////////////////////////////////////////////////////////////////
string Preferences::patchKey(const string& key) const
{
  string ret = key;
  if (p_caseSensitive == false) {
    transform(key.begin(),key.end(),ret.begin(),toupper);
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//	Returns TRUE if the Specified Key is Defined Within These Preferences
//	The Search is Case-Insensitive
//
///////////////////////////////////////////////////////////////////////////////
bool 
Preferences::isDefined(const string& key) const
{
  bool ret = false;

  string stmp = patchKey(key);

  if (p_pref.find(stmp) != p_pref.end()) ret = true;
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    All of the Preference::get_xxx() Functions Return a Value for
//    the Specified Key
//    If the Requested Value Type (xxx) Doesn not Match the Value Type,
//    FALSE is Returned and the By-Reference Parameter is not Touched
//
///////////////////////////////////////////////////////////////////////////////
bool 
Preferences::getBool(const string& key, bool& value) const
{
  bool retval = false;

  string stmp = patchKey(key);

  string strvalue;
  if (p_pref.findValue(stmp,strvalue)) {
     // Now have to convert to bool
    retval = StringConverter::toBoolean(strvalue,value);
  }


  return retval;
}


bool 
Preferences::getInt(const string& key, int& value) const
{
  bool retval = false;

  string stmp = patchKey(key);

  string strvalue;
  if (p_pref.findValue(stmp,strvalue)) {
     // Now have to convert 
    retval = StringConverter::toInt(strvalue,value);
  }
  return retval;

}

bool 
Preferences::getFloat(const string& key, float& value) const
{
  bool retval = false;
  string stmp = patchKey(key);

  string strvalue;
  if (p_pref.findValue(stmp,strvalue)) {
     // Now have to convert 
    double dvalue;
    retval = StringConverter::toDouble(strvalue,dvalue);
    if (retval) value = dvalue;
  }
  return retval;
}

bool 
Preferences::getString(const string& key, string& value) const
{
  bool retval = false;
  string stmp = patchKey(key);

  string strvalue;
  if (p_pref.findValue(stmp,strvalue)) {
    retval = true;
    // Strip of tokenizer char if it exists
    if (strvalue.size() >= 2 && strvalue[0] == '\\')
      value = strvalue.substr(2);
    else 
      value = strvalue;
  }
  return retval;
}

bool
Preferences::getStringList(const string& key, vector<string>& value) const
{
  bool retval = false;
  string stmp = patchKey(key);

  string strvalue;
  if (p_pref.findValue(stmp,strvalue)) {
    // Now have to convert 
    string tokenizer = " \t";
    if (strvalue.size() >= 2 && strvalue[0] == '\\') {
      tokenizer = strvalue.substr(1,1);
      strvalue = strvalue.substr(2);
    }

    // strtok modifies string so gotta do this
    char *cstr = strdup(strvalue.c_str());

    char *token = strtok(cstr,tokenizer.c_str());

    if (token) {
      retval = true;
      value.push_back(token);

      while ((token = strtok(NULL,tokenizer.c_str())) != NULL) {
        value.push_back(token);
      }
    }
    free(cstr);
  }
  return retval;
}

bool
Preferences::getFloatList(const string& key, vector<float>& value) const
{
  bool retval = false;
  string stmp = patchKey(key);

  string strvalue;
  if (p_pref.findValue(stmp,strvalue)) {
    // Now have to convert 
    vector<double> dvalue;
    retval = StringConverter::toDoubleList(strvalue,dvalue," ");
    if (retval) {
      vector<double>::const_iterator it = dvalue.begin();
      while (it != dvalue.end()) {
        value.push_back((*it));
        it++;
      }
    }
  }
  return retval;
}

bool
Preferences::getIntList(const string& key, vector<int>& value) const
{
  bool retval = false;
  string stmp = patchKey(key);

  string strvalue;
  if (p_pref.findValue(stmp,strvalue)) {
     // Now have to convert 
    retval = StringConverter::toIntList(strvalue,value," ");
  }
  return retval;
}

bool
Preferences::getBoolList(const string& key, vector<bool>& value) const
{
  bool retval = false;
  string stmp = patchKey(key);

  string strvalue;
  if (p_pref.findValue(stmp,strvalue)) {
     // Now have to convert 
    retval = StringConverter::toBoolList(strvalue,value," ");
  }
  return retval;
}



///////////////////////////////////////////////////////////////////////////////
//  Description
//    These Modifiers Set the Value of the Specified Key
//    No checks are done to see if its already in use.
//
///////////////////////////////////////////////////////////////////////////////
void 
Preferences::setInt(const string& key, const int& value)
{
  char buf[32];
  sprintf(buf,"%d",value);
  
  p_pref[patchKey(key)] = buf;

}

void 
Preferences::setBool(const string& key, const bool& value)
{
  string strvalue = "true";
  if (!value) strvalue = "false";
  p_pref[patchKey(key)] = strvalue;
}

void 
Preferences::setFloat(const string& key, const float& value)
{
  char buf[32];
  sprintf(buf,"%f",value);
  
  p_pref[patchKey(key)] = buf;
}

void 
Preferences::setString(const string& key, const string& value)
{
  p_pref[patchKey(key)] = value;
}

void 
Preferences::setIntList(const string& key, const vector<int>& value)
{
  string strvalue;
  char buf[32];
  int numItems = value.size();
  for (int idx=0; idx<numItems; idx++) {
    sprintf(buf,"%d",value[idx]);
    strvalue.append(buf);
    strvalue.append(" ");
  }
  p_pref[patchKey(key)] = strvalue;
}

void
Preferences::setBoolList(const string& key, const vector<bool>& value)
{
  string strvalue;
  int numItems = value.size();
  for (int idx=0; idx<numItems; idx++) {
    if (value[idx])
      strvalue.append("true");
    else
      strvalue.append("false");
    strvalue.append(" ");
  }
  p_pref[patchKey(key)] = strvalue;
}

void
Preferences::setFloatList(const string& key, const vector<float>& value)
{
  string strvalue;
  char buf[32];
  int numItems = value.size();
  for (int idx=0; idx<numItems; idx++) {
    sprintf(buf,"%f",value[idx]);
    strvalue.append(buf);
    strvalue.append(" ");
  }
  p_pref[patchKey(key)] = strvalue;
}

void
Preferences::setStringList(const string& key, const vector<string>& value)
{
  string strvalue;
  char delim = ' ';
  delim = find_delim_char(value);

  strvalue = '\\';
  strvalue += delim;

  int numItems = value.size();
  for (int idx=0; idx<numItems; idx++) {
    strvalue.append(value[idx]);
    strvalue += delim;
  }
  if (strvalue.size() > 0) strvalue.erase(strvalue.end()-1);
  p_pref[patchKey(key)] = strvalue;

}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Clear All Preferences
//
///////////////////////////////////////////////////////////////////////////////
void 
Preferences::clear(void)
{
  p_pref.clear();
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Removes the specified key/value pair from the dictionary.
//
//  Implementation
//    Remove the specified entry and delete the key and value.
///////////////////////////////////////////////////////////////////////////////
void Preferences::remove_entry(const string& lookupKey)
{
  string stmp = patchKey(lookupKey);

  EcceMap::iterator it = p_pref.find(stmp);
  if (it != p_pref.end()) {
    p_pref.erase(it);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Identity-Based Object Equality
//
///////////////////////////////////////////////////////////////////////////////
bool 
Preferences::operator==(const Preferences& rhs) const
{ 
  return (this == &rhs); 
}



// --------------------------
// Protected Member Functions
// --------------------------


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Attempt to Locates the Preferences File and Create it if Non-Existent
//    Success Status and the Complete Pathname are Returned
//
///////////////////////////////////////////////////////////////////////////////
bool 
Preferences::create_pref_file(
  const string& pref_dir,
  const string& pref_name,
  string& filepath,
  int createMode
)
{
  bool retval = true;

  filepath = pref_dir;
  
  ErrMsg errs;
  SDirectory directory(filepath, 0700);

  // We don't want to pass this warning along - doesn't matter.
  errs.flush("EE_FILE_EXISTS");

  if (directory.exists() && !directory.is_dir()) {
    // It already existed as something other than a directory - not good
    retval = false;
    errs.message("EE_NOT_DIR", EE_FATAL, WHERE, filepath.c_str());
  } else {
    filepath += "/";
    filepath += pref_name;	// should now be "~/.classdir/filename"
    SFile file(filepath, createMode);
    if (createMode==0 && !file.exists()) {
      EE_RT_ASSERT((createMode>0),EE_WARNING,filepath+" is required to exist!");
      deletePrefs();  // causes to be invalid
    } else
      // We don't want to pass this warning along - doesn't matter.
      errs.flush("EE_FILE_EXISTS");
  }
  return retval;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//	Read the Contents of the Current File into the Hash Table.
//
//  Implemenation
//      The name of the current file is expected to be in p_filename.
//	If an error occurs, it puts a message on the error queue and returns
//	FALSE
//
///////////////////////////////////////////////////////////////////////////////
bool 
Preferences::load()
{
   SFile file(p_filename);
   if (!file.exists()) 
     // file does not yet exist.  not an error
     return true;

   // Open the file 
   ifstream is(file.path(true).c_str());
   if (!is.good()) {
     ErrMsg().message("EE_FILE_ACCESS", EE_WARNING, WHERE, p_filename.c_str());
     return false;
   }

   bool retval=true;	// Return at end only to do close()

   // The rest of the file should be key/value pairs that can
   // be handled by the HashParser class.  Comments are disallowed 
   // because come codes use # which is our comment character for
   // character string and the parser lops this off which breaks
   // things.
      KeyValueReader reader(is);
      reader.stripTrailingComments(false);
      string key,value;

      while (reader.getpair(key,value)) {
        p_pref[patchKey(key)] = value;
      }
     is.close();
     return retval;
}


vector<string>
Preferences::keys() const
{
  vector<string> result;
  EcceMapIterator it = p_pref.begin();
  while (++it != p_pref.end()) {
    result.push_back((*it).first);
  }
  return result;
}      

///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Checkpoint the Current Preference Values to a File.  Returns TRUE
//    if the file was written.  If False is returned, an EE_MISC is
//    also placed on the queue.
//
//    This function also dumps out an ECCE header comment.
//
///////////////////////////////////////////////////////////////////////////////
bool 
Preferences::saveFile()
{
  ofstream os(p_filename.c_str());

  // make sure it opened
  if (!os) {
    ErrMsg().message("EE_FILE_ACCESS",EE_WARNING,WHERE,p_filename.c_str());
    return false;
  }

  // Write comment header
  os << "# ECCE Preference File  ";
  os << "#\n";
  os << "# Created by ECCE version " << Ecce::ecceVersion() << endl;
  os << "#\n";
  os << "#### EDIT WITH CAUTION ### - File automatically generated." << endl;

  // save it
  os << *this;

  // close it
  os.close();

  return true;
}


///////////////////////////////////////////////////////////////////////////////
//
//  Description
//	Search the Given String List for a Character not Appearing in any
//
//  Implementation
//      Individual String (for USe as a Delimeter)
//	any where in the list.  The goal is to find a character that can
//	serve as a delimiter in the preference file.
//	If it can't find a printable ASCII character that is not used in
//	the strings, it puts a message on the error queue and returns
//	a comma as the delimiter.
//
///////////////////////////////////////////////////////////////////////////////
char 
Preferences::find_delim_char(const vector<string> &strings)
{
  string   str;	// temp place holder
  static char preferred[] = {',', '!', '|', '@', '#', '$', '%', '^', '&', '*'};
  bool   found;
  unsigned char retval;
  size_t i, j;

  for (i=0; i < sizeof(preferred); i++)
  {
    found = false;
    // see if this delimiter is in any of the strings
    for (j=0; j < strings.size() && !found; j++)
    {
      str = strings[j];
      if ((int)str.find(preferred[i]) >= 0)
        // can't use this one
        found = true;
    }

    if (!found)
      // happiness - no stings have this delimiter
      return preferred[i];
  }

  // if we got here, it means that the string contains all of the "preferred"
  // delimiters, so simply start looping through the ASCII characters and
  // find one.
  for (retval = 32; retval < 127; retval++)
  {
    found = false;
    // see if this delimiter is in any of the strings
    for (j=0; j < strings.size() && !found; j++)
    {
      str = strings[j];
      if ((int)str.find(retval) >= 0)
        // can't use this one
        found = true;
    }

    if (!found)
      return (char)retval;
  }

  // if we got here, then the string has every single printable ascii character
  // which means that there are no good delimiters.
  ErrMsg().message("EE_MISC", EE_WARNING, WHERE, "Unable to find a delimiter");
  // return a comma for lack of anything better
  return ',';
}



///////////////////////////////////////////////////////////////////////////////
//
//  Description
//    Print a Formatted Representation of the Preferences
//
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const Preferences& pref)
{
  EcceMapIterator it = pref.p_pref.begin();

  while (it != pref.p_pref.end())
  {
     os << (*it).first << ":	";
     os << (*it).second << "\n";
     it++;
  }
  os << endl;

  return os;
}


Preferences * Preferences::getGlobalPref(bool refresh)
{
  if (refresh) {
    if (p_globalPref != NULL) {
      delete p_globalPref;
    }
    p_globalPref = new Preferences(PrefLabels::GLOBALPREFFILE);
  }
  else if (p_globalPref == NULL)
    p_globalPref = new Preferences(PrefLabels::GLOBALPREFFILE);
  return p_globalPref;
}
