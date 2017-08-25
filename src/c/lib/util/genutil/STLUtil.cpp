#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>

#include <algorithm>
  using std::find;
  using std::replace;
  using std::transform;
#include <iterator>
  using std::istream_iterator;
#include <sstream>
  using std::istringstream;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

#include "util/StringTokenizer.H"
#include "util/STLUtil.H"


void STLUtil::toLower(string& str)
{
  transform(str.begin(),str.end(),str.begin(),tolower);
}


void STLUtil::toUpper(string& str)
{
  transform(str.begin(),str.end(),str.begin(),toupper);
}


string STLUtil::lower(const string& str)
{
  string ret = str;
  toLower(ret);
  return ret;
}


string STLUtil::upper(const string& str)
{
  string ret = str;
  toUpper(ret);
  return ret;
}


bool STLUtil::startsWith(const string& str, const string& key)
{
  if (key.size() > str.size()) return false;
  string beg = str.substr(0,key.size());
  return beg == key;
}


bool STLUtil::endsWith(const string& str, const string& key)
{
  bool ret = false;

  if (key.size() <= str.size()) {
    string end = str.substr(str.size()-key.size());
    ret = end == key;
  }

  return ret;
}


void STLUtil::substitute(string& str, const string& from, const string& to)
{
  // Be Careful: this will go into an infinite loop if the "to" string
  // contains the "from" string.  Therefore it is recommended that "from"
  // be a unique pattern that will never occur in "to" such as angle-bracket
  // delimited.

  for (size_t loc=str.find(from); loc!=string::npos; loc=str.find(from)) {
    str.replace(loc, from.size(), to);
  }
}


/**
 * Gets all characters after first occurence of key.
 * @return empty string if key is not found
 */
string STLUtil::afterFirst(const string& str, const string& key)
{
  size_t loc = str.find(key);
  if (loc == string::npos) {
    return "";
  } else {
    return str.substr(loc+1);
  }
}


/**
 * Gets all characters after last occurence of key.
 * @return whole string if key is not found
 */
string STLUtil::afterLast(const string& str, const string& key)
{
  size_t loc = str.rfind(key);
  if (loc == string::npos) {
    return str;
  } else {
    return str.substr(loc+1);
  }
}


/**
 * Gets all characters before first occurence of key.
 * @return whole string if key is not found
 */
string STLUtil::beforeFirst(const string& str, const string& key)
{
  size_t loc = str.find(key);
  if (loc == string::npos) {
    return str;
  } else {
    return str.substr(0,loc);
  }
}


/**
 * Gets all characters before last occurence of key.
 * @return empty string if key is not found
 */
string STLUtil::beforeLast(const string& str, const string& key)
{
  size_t loc = str.rfind(key);
  if (loc == string::npos) {
    return "";
  } else {
    return str.substr(0,loc);
  }
}


void STLUtil::stripLeadingWhiteSpace(string& str)
{
  str.replace(0,str.find_first_not_of(" \t\r\n"), "");
}


void STLUtil::stripTrailingWhiteSpace(string& str)
{
  str.replace(str.find_last_not_of(" \t\r\n")+1, string::npos, "");
}


void STLUtil::stripLeadingAndTrailingWhiteSpace(string& str)
{
  stripLeadingWhiteSpace(str);
  stripTrailingWhiteSpace(str);
}



/**
 *  Returns copy of supplied string with white space characters
 *  (' ', '\n', '\t', '\r') removed from both ends.
 *  This is much the same as stripLeadingAndTrailingWhiteSpace()
 *  except that it does not modify the supplied string.
 *  This allows it to be incorporated into more complex statements.
 */
string STLUtil::trim(const string& str)
{
  string dest = str;
  stripLeadingAndTrailingWhiteSpace(dest);
  return dest;
}



/**
 * Returns a vector of strings split up using standard whitespace from
 * the given string.
 */
vector<string> STLUtil::split(const string& str)
{
  istringstream is(str);
  return vector<string>(istream_iterator<string>(is),
                        istream_iterator<string>());
}



/**
 * Utility function to replace the compareTo method associated with
 * RWCStrings. This comparison ignores case.
 */
int STLUtil::compareString(const string& as, const string& bs)
{
  string a = lower(as);
  string b = lower(bs);
  return a.compare(b);
}


/**
 * Utility function to determine whether a string a contains a substring
 * b. This search ignores case.
 */
bool STLUtil::containsString(const string& as, const string& bs)
{
  string a = lower(as);
  string b = lower(bs);
  return a.find(b) != string::npos;
}


/**
 * Adds a new line to a string before "length" characters (in a white
 * space).  The user provides the "length" of the line requested.
 */
string STLUtil::insertNewLine(const string& str, const size_t& length)
{
  StringTokenizer next(str);   
  string token;
  string newString;
  size_t count = 0;
  while(!(token=next.next()).empty()){
    count += token.length();
    if (count > length) {
      newString += "\n" + token;
      count = 0;
    }
    else {
      if (newString.length() > 0) newString += " ";
      newString += token;
    }
  }
  return newString;
}


string STLUtil::stripKey(string& line)
{
  string result;
  size_t commentIndex = line.find(":");
  if (commentIndex != string::npos) {
    result = line.substr(0, commentIndex);
    line.erase(0, result.length()+1);
  }
  STLUtil::stripLeadingWhiteSpace(line);
  return result;
}


void STLUtil::removeCommentAndSpaces(string& line)
{
  size_t commentIndex = line.find("#");
  if (commentIndex != string::npos) {
    line = line.substr(0, commentIndex);
  }
  STLUtil::stripLeadingAndTrailingWhiteSpace(line);
}


static const double base = 10;

int STLUtil::stod(const string& string, double& value)
{
  double tmpvalue = 0.0;
  int i = 0;
  int sign = 1;
  int len = string.length();

  // skip white space??

  // Accept a plus or minus - this is optional
  if (i<len && string[i] == '+') 
    i++;
  else if (i < len && string[i] == '-')
  {
    i++;
    sign = -1;
  }

  // Now take digits
  while (i<len && isdigit(string[i]))
  {
    tmpvalue = tmpvalue * base + string[i] - '0';
    i++;
  }

  // Now take the decimal and trailing digits
  if (i<len && string[i] == '.')
  {
    double fractional_divisor = 1.0;
    i++;
    while (i<len && isdigit(string[i]))
    {
      fractional_divisor /= base;
      tmpvalue += (string[i] - '0')*fractional_divisor;
      i++;
    }
  }

  // skip white space??


  //
  // Final check for error conditions.  We don''t allow
  // . empty strings
  // . strings with extra characters at the end
  // . strings that don''t end in a digit unless the last char is
  //   the decimal point and the 2nd last char is a digit
  //

  int ret = -1;		// This is success
  if (len == 0 || i != len)
    ret = i;
  else if (!isdigit(string[len-1]))		// we don''t end in a digit
    if (len-2 >= 0 && isdigit(string[len-2]))
      value  = tmpvalue * sign;
    else
      ret = i;
  else 
    value  = tmpvalue * sign;
  
  return ret;
}


int STLUtil::stoi(const string& string, int& value)
{
  int tmpvalue = 0;
  int i = 0;
  int sign = 1;
  int len = string.length();

  // skip white space??

  // Accept a plus or minus - this is optional
  if (i<len && string[i] == '+') 
    i++;
  else if (i < len && string[i] == '-')
  {
    i++;
    sign = -1;
  }

  // Now take digits
  while (i<len && isdigit(string[i]))
  {
    tmpvalue = tmpvalue * (int) base + string[i] - '0';
    i++;
  }

  // skip white space??

  //
  // Final check for error conditions.  We don''t allow
  // . empty strings
  // . strings with extra characters at the end
  // . strings that don''t end in a digit
  //

  int ret = -1;		// This is success
  if (len == 0 || i != len)
    ret = i;
  else if (!isdigit(string[len-1]))		// we don''t end in a digit
    ret = i;
  else 
    value  = tmpvalue * sign;
  
  return ret;
}


int STLUtil::stoe(const string& string, double& value)
{
  double tmpvalue = 0.0;
  int i = 0;
  int sign = 1;
  int len = string.length();
  double expfactor = 1;

  // skip white space??

  // Accept a plus or minus - this is optional
  if (i<len && string[i] == '+') 
    i++;
  else if (i < len && string[i] == '-')
  {
    i++;
    sign = -1;
  }

  // Now take digits
  while (i<len && isdigit(string[i]))
  {
    tmpvalue = tmpvalue * base + string[i] - '0';
    i++;
  }

  // Now take the decimal and trailing digits
  if (i<len && string[i] == '.')
  {
    double fractional_divisor = 1.0;
    i++;
    while (i<len && isdigit(string[i]))
    {
      fractional_divisor /= base;
      tmpvalue += (string[i] - '0')*fractional_divisor;
      i++;
    }
  }

  // Now take the exponent +/- and digits
  if (i<len && (string[i] == 'E' || string[i] == 'e'))
  {
    bool neg=false;
    i++;
    if (i<len && string[i] == '+') 
      i++;
    else if (i < len && string[i] == '-')
    {
      i++;
      neg = true;
    }
    int exp=0;
    while (i<len && isdigit(string[i]))
    {
      exp = exp * (int)base + string[i] - '0';
      i++;
    }
    if (exp != 0)
    {
      expfactor = pow(base,exp);
      if (neg) expfactor = 1/expfactor;
    }
  }

  // skip white space??

  //
  // Final check for error conditions.  We don''t allow
  // . empty strings
  // . strings with extra characters at the end
  // . strings that don''t end in a digit unless the last char is
  //   the decimal point and the 2nd last char is a digit
  //

  int ret = -1;		// This is success
  if (len == 0 || i != len)
    ret = i;
  else if (!isdigit(string[len-1]))		// we don''t end in a digit
    if (string[len-1] != '.')
      ret = i;
    else if (len-2 >= 0 && isdigit(string[len-2]))
      value  = tmpvalue * sign *expfactor;
    else
      ret = i;
  else 
    value  = tmpvalue * sign *expfactor;
  
  return ret;
}
