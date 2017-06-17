#ifndef STLUTIL_H
#define STLUTIL_H

#include <algorithm>
  using std::find;
#include <string>
  using std::string;
#include <vector>
  using std::vector;


/**
 * static helper functions for strings.
 * STLUtil is probably a bad name for this class.
 */
class STLUtil 
{
public:
  static void toLower(string& str);
  static void toUpper(string& str);
  static string lower(const string& str);
  static string upper(const string& str);
  static bool startsWith(const string& str, const string& key);
  static bool endsWith(const string& str, const string& key);
  static void substitute(string& str, const string& from, const string& to);
  static string afterFirst(const string& str, const string& key);
  static string afterLast(const string& str, const string& key);
  static string beforeFirst(const string& str, const string& key);
  static string beforeLast(const string& str, const string& key);
  static void stripLeadingWhiteSpace(string& str);
  static void stripTrailingWhiteSpace(string& str);
  static void stripLeadingAndTrailingWhiteSpace(string& str);
  static string trim(const string& str);
  static vector<string> split(const string& str);
  static int compareString(const string& as, const string& bs);
  static bool containsString(const string& as, const string& bs);
  static string insertNewLine(const string& str, const size_t& length);
  static string stripKey(string& line);
  static void removeCommentAndSpaces(string& line);
  static int stod(const string& string, double& value);
  static int stoe(const string& string, double& value);
  static int stoi(const string& string, int& value);
};

#endif // STLUTIL_H
