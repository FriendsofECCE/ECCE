#include <iostream>
  using std::ends;
  using std::flush;

#include <strstream>
  using std::istrstream;
  using std::ostrstream;

#include <stdio.h>
#include <string.h>

#include "util/ErrMsg.H"
#include "tdat/GUIValues.H"


//-----------------------------------------------------------------------------
//                               Class GUIValue
//-----------------------------------------------------------------------------

void GUIValue::setValue(const string& value)
{
  p_value = value;
}

void GUIValue::setValue(int value)
{
  char buf[12];
  sprintf(buf,"%d",value);
  p_value = buf;
}

void GUIValue::setValue(double value)
{
  char buf[32];
  sprintf(buf,"%lf",value);
  p_value = buf;
}

string GUIValue::getValueAsString() const
{
  return p_value;
}

int GUIValue::getValueAsInt() const
{
  char *tail;
  return (int) strtol(p_value.c_str(),&tail,10);
}

double GUIValue::getValueAsDouble() const
{
  char *tail;
  return strtod(p_value.c_str(),&tail);
}







//-----------------------------------------------------------------------------
//                               Class GUIValues
//-----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Constructor
//////////////////////////////////////////////////////////////////////////////
GUIValues::GUIValues()
{
}

//////////////////////////////////////////////////////////////////////////////
// Copy Constructor
//////////////////////////////////////////////////////////////////////////////
GUIValues::GUIValues(const GUIValues& rhs)
{
  *this = rhs;
}

//////////////////////////////////////////////////////////////////////////////
// Destructor
//////////////////////////////////////////////////////////////////////////////
GUIValues::~GUIValues()
{
  clear();
}


//////////////////////////////////////////////////////////////////////////////
// operator=
//   Assignment with deep copy of map contents.
//////////////////////////////////////////////////////////////////////////////
GUIValues& GUIValues::operator=(const GUIValues& rhs)
{
  if (this != &rhs) {
    GUIValue* guival;
    GUIValues::const_iterator it;
    for (it=rhs.begin(); it!=rhs.end(); it++) {
      guival = new GUIValue(*(*it).second);
      GUIValues::value_type v((*it).first, guival);
      insert(v);
    }
  }
  return *this;
}

//////////////////////////////////////////////////////////////////////////////
// operator=
//   Assignment from string.  This clears any existing data and
//   effectively loads (parses) data from the string.  See load().
//////////////////////////////////////////////////////////////////////////////
GUIValues& GUIValues::operator=(const string& rhs)
{
  clear();
  istrstream buf(rhs.c_str());
  load(buf);
  return *this;
}

//////////////////////////////////////////////////////////////////////////////
// operator+=
//   Add new element(s) from string.  Does not clear any existing data but
//   is otherwise equivalent to operator=.  See load().
//////////////////////////////////////////////////////////////////////////////
GUIValues& GUIValues::operator+=(const string& rhs)
{
  istrstream buf(rhs.c_str());
  load(buf);
  return *this;
}

GUIValues& GUIValues::operator+=(const char* rhs)
{
  istrstream buf(rhs);
  load(buf);
  return *this;
}

bool GUIValues::append(const char* rhs)
{
  istrstream buf(rhs);
  return load(buf) > 0;
}

//////////////////////////////////////////////////////////////////////////////
// operator==
//   true if they are the same object.
//////////////////////////////////////////////////////////////////////////////
int GUIValues::operator==(const GUIValues& rhs) const
{
  return this==&rhs;
}

//////////////////////////////////////////////////////////////////////////////
// operator!=
//   true if they are not the same object.
//////////////////////////////////////////////////////////////////////////////
int GUIValues::operator!=(const GUIValues& rhs) const
{
  return this!=&rhs;
}

void GUIValues::clear()
{
  GUIValues::iterator it;
  for (it=begin(); it!=end(); it++) {
    // have to delete pointer
    delete (*it).second;
    erase(it);
  }
  map<const string,GUIValue*,less<string> >::clear();
}

void GUIValues::deletePrefix(const string& prefix)
{
  GUIValues::iterator it;

  for (it=begin(); it!=end(); it++)
    if ((*it).first.find(prefix) == 0) {
      delete (*it).second;
      erase(it);
    }
}


//////////////////////////////////////////////////////////////////////////////
// set()
//  Insert object into map.  If the key already exists, clean up the old
//  object.
//////////////////////////////////////////////////////////////////////////////
void GUIValues::set(const string& key, GUIValue *option)
{
  GUIValues::iterator it;
  it = find(key);
  if (it != end()) {
    // have to delete pointer
    delete (*it).second;
    erase(it);
  }

  GUIValues::value_type v(key, option);
  insert(v);
}


//////////////////////////////////////////////////////////////////////////////
// get()
//  Returns the object specified by key or NULL if not found.  This is
//  the same as doing a find on the map - just a little easier to use.
//////////////////////////////////////////////////////////////////////////////
GUIValue *GUIValues::get(const string& key) const
{
  GUIValue *ret = NULL;
  GUIValues::const_iterator it;
  it = find(key);
  if (it != end()) {
    ret = (*it).second;
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// load()
//   Reads from stream in format
//     key: value 1/0 1/0
//        where 1/0 means 1 or 0 (true or false).
//     the first is the sensitive flag, the second is the write  flag.
//     returns how many were read.
//  Comment lines beginning with # are safely ignored.
//////////////////////////////////////////////////////////////////////////////
int GUIValues::load(istream& inputsrc)
{
  static const int MAXLINE = 120;
  int ret = 0;
  char buf[MAXLINE];
  char bufdup[MAXLINE];
  bool comment = false;
  char *cptr = NULL;
  GUIValue obj;
  while (inputsrc.getline(buf,MAXLINE-1)) {
    if (strlen(buf) == 0) continue; 

    // skip comment lines - can't use strtok
    comment = false;
    cptr = buf;
    while (cptr != '\0') {
      if (*cptr == ' ' ||  *cptr == '\t') {
        ;
      } else if (*cptr == '#') {
        comment = true;
        break;
      } else {
        break;
      }
      cptr++;
    }
    if (comment) continue;

    // Ok now process the line.
    // Used to use strtok but that didn't handle empty fields "||" so
    // now use strchr and keep track of my own pointers.
    cptr = buf;
    strcpy(bufdup, buf);

    char* nptr = strchr(cptr, '|');
    if (nptr == NULL) {
      string msg = "GUIValues: malformed line parsing key--1st field (";
      msg += bufdup;
      msg += ")";
      EE_RT_ASSERT(false, EE_WARNING, msg);
      continue;
    }
    *nptr = '\0';
    obj.m_key = cptr;

    cptr = nptr + 1;
    nptr = strchr(cptr, '|');
    if (nptr == NULL) {
      string msg = "GUIValues: malformed line parsing value--2nd field (";
      msg += bufdup;
      msg += ")";
      EE_RT_ASSERT(false, EE_WARNING, msg);
      continue;
    }
    *nptr = '\0';
    obj.setValue(cptr);

    cptr = nptr + 1;
    nptr = strchr(cptr, '|');
    if (nptr == NULL) {
      string msg = "GUIValues: malformed line parsing units--3rd field (";
      msg += bufdup;
      msg += ")";
      EE_RT_ASSERT(false, EE_WARNING, msg);
      continue;
    }
    *nptr = '\0';
    obj.m_units = cptr;

    cptr = nptr + 1;
    nptr = strchr(cptr, '|');
    if (nptr == NULL) {
      string msg = "GUIValues: malformed line parsing sensitivity-4th field (";
      msg += bufdup;
      msg += ")";
      EE_RT_ASSERT(false, EE_WARNING, msg);
      continue;
    }
    *nptr = '\0';
    obj.m_sensitive = (*cptr == '1' ? true : false);

    cptr = nptr + 1;
    nptr = strchr(cptr, '|');
    if (cptr == NULL) {
      string msg = "GUIValues: malformed line parsing write flag-5th field (";
      msg += bufdup;
      msg += ")";
      EE_RT_ASSERT(false, EE_WARNING, msg);
      continue;
    }
    if (nptr != NULL)
      *nptr = '\0';
    obj.m_write = (*cptr == '1' ? true : false);

    obj.m_type = "unknown";
    if (nptr != NULL) {
      cptr = nptr + 1;
      if (cptr != NULL)
        obj.m_type = cptr;
    }

    set(obj.m_key,new GUIValue(obj));
    ret++;
  }
  return ret;
}

string GUIValues::toString() const
{
  string ret;
  ostrstream os;
  dump(os);
  os << ends;
  ret = os.str();
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// containsKeyPrefix()
//   Determines if the given prefix exists for any of the keys.  Used to set
//   senstivity of "Details..." buttons in calc editors.
//////////////////////////////////////////////////////////////////////////////
bool GUIValues::containsKeyPrefix(const string& prefix) const
{
  GUIValues::const_iterator it;
  for (it=begin(); it!=end() && (*it).first.find(prefix)!=0; it++);

  return (it != end());
}

//////////////////////////////////////////////////////////////////////////////
// dump()
//   Dumps all keys to the stream.  See description of read for format.
//////////////////////////////////////////////////////////////////////////////
void GUIValues::dump(ostream& outputdest) const
{
  GUIValues::const_iterator it;
  GUIValue *obj;
  for (it=begin(); it!=end(); it++) {
    obj = (*it).second;
    outputdest << obj->m_key << "|";
    outputdest << obj->getValueAsString() << "|";
    outputdest << obj->m_units << "|";
    outputdest << obj->m_sensitive << "|";
    outputdest << obj->m_write << "|";
    outputdest << obj->m_type << endl << flush;
  }
}

//////////////////////////////////////////////////////////////////////////////
// dumpKeyVals()
//   Dumps those keys and values where the sensitive and write flags are true.
//   This is used to generate a code input file.
//////////////////////////////////////////////////////////////////////////////
void GUIValues::dumpKeyVals(ostream& outputdest) const
{
  GUIValues::const_iterator it;
  GUIValue *obj;
  for (it=begin(); it!=end(); it++) {
    obj = (*it).second;
    if (obj->m_sensitive && obj->m_write) {
      outputdest << obj->m_key << ": ";
      outputdest << obj->getValueAsString() << endl << flush;
    }
  }
}

