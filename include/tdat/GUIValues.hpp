///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: GUIValues.H
//
//
// CLASS SYNOPSIS:
//    Class GUIValues contains data associated with gui detail options.
//
// DESCRIPTION:
//
///////////////////////////////////////////////////////////////////////////////
#ifndef GUIVALUES_HH
#define GUIVALUES_HH

#include <iostream>
using std::istream;
using std::ostream;

#include <string>
#include <map>

  using std::string;
  using std::map;
  using std::less;

/*
 * Structure to hold info about a single details option.  
 * The value is private so that we can later make a union of types
 * rather than having to translate in and out of string all the time
 * and not have to change the public interface.
 *
 * Default constructors and destructors are sufficient at this time.
 */
class GUIValue
{
public:
  string m_key;
  string m_type;
  string m_units;
  bool   m_sensitive;
  bool   m_write;

  void   setValue(const string& value);
  void   setValue(int value);
  void   setValue(double value);

  string getValueAsString() const;
  int    getValueAsInt() const;
  double getValueAsDouble() const;

private:
  string p_value;
};


class GUIValues : public map<const string, GUIValue*, less<string> >
{
public:
  GUIValues();
  GUIValues(const GUIValues& rhs);
  virtual ~GUIValues();

  GUIValues& operator=(const GUIValues& rhs);
  GUIValues& operator=(const string& data);
  GUIValues& operator+=(const string& data);
  GUIValues& operator+=(const char* data);
  int operator==(const GUIValues& rhs) const;
  int operator!=(const GUIValues& rhs) const;

  void set(const string& key, GUIValue* option);
  GUIValue *get(const string& key) const;
  bool append(const char* data);

  void clear();  // deletes and clears
  void deletePrefix(const string& prefix);  // deletes keys with prefix

  /*
   * Reads from stream in format
   *   key: value 1/0 1/0
   * where 1/0 means 1 or 0 (true or false).
   * the first is the sensitive flag, the second is the write flag.
   * returns how many were read.
   */
  int load(istream& inputsrc);

  // Dumps entire map with each entry as specified above.
  void dump(ostream& outputdest) const;

  // Dumps those key/values objects needed to generate an input file
  void dumpKeyVals(ostream& outputdest) const;

  // Searches for prefix in GUIValues keys.
  bool containsKeyPrefix(const string& prefix) const;

  string toString() const;

}; 


#endif 
