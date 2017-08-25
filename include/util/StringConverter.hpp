//////////////////////////////////////////////////////////////////////////////
// Description
//   Class for performing common conversions to and from strings.
//
//   Each method that converts from a string returns true if the conversion
//   was succesful.  If it wasn't successful, the return argument is not
//   modified.
//////////////////////////////////////////////////////////////////////////////
#ifndef STRINGCONVERTER
#define STRINGCONVERTER

#include <iostream>
  using std::istream;

#include <string>
  using std::string;
#include <vector>
  using std::vector;

class StringConverter 
{
  public:

    static bool isAllWhite(const string& str);

    //  Utility functions for converting a string to anything
    static bool toValue(const string& str, bool& value);
    static bool toValue(const string& str, int& value);
    static bool toValue(const string& str, long& value);
    static bool toValue(const string& str, size_t& value);
    //static bool toValue(const string& str, unsigned long& value);
    static bool toValue(const string& str, float& value);
    static bool toValue(const string& str, double& value);

    static bool toBoolean(const string& str, bool& value);
    static bool toInt(const string& str, int& value);
    static bool toLong(const string& str, long& value);
    static bool toSize_t(const string& str, size_t& value);
    static bool toFloat(const string& str, float& value);
    static bool toDouble(const string& str, double& value);

    static bool toUpper(const string& str, string& value);
    static bool toLower(const string& str, string& value); 
    static bool capitalize(const string& str, string& value); 
    static int compare(const string& str1, const string& str2,
                       bool caseSensitive=false); 

    // For list operations, if an error is encountered,
    // the list is cleared its wise to clear vector prior to 
    // calling these methods.
    static bool toIntList(const string& str,vector<int>& value,
                          const char *delim=":");
    static bool toDoubleList(const string& str,vector<double>& value,
                             const char *delim=":");
    static bool toBoolList(const string& str,vector<bool>& value,
                           const char *delim=":");
    static bool toStringList(const string& str,vector<string>& value,
                             const char *delim=":");

    // Range validation.  Basic rules are explained in the source
    // file.  This supercedes the overly complicated "Expression" class
    // that's proved to be more power than we needed to maintain.
    static bool validateRange(const bool& float_flag, const bool& hard_flag,
                              const string& value, const string& range,
                              string& message);

    // General use conversion of input stream to a string value.
    // Return value indicates whether the stream was valid and
    // contained any text;
    static bool streamToText(istream& is, string& text);

    //  Utility functions for converting primitive types to strings
    static string toString(bool val);
    static string toString(int val);
    static string toString(long val);
    static string toString(size_t val);
    //static string toString(unsigned long val);
    static string toString(float val);
    static string toString(double val);

    //  Utility functions for converting lists of primitives to strings
    static string toString(const vector<int>& value, const char *delim=":");
    static string toString(const vector<double>& value, const char *delim=":");
    static string toString(const vector<bool>& value, const char *delim=":");
    static string toString(const vector<string>& value, const char *delim=":");

    // Utility to handle file name extensions
    static string getFileNameExt(string fileName, bool toLower = false);
    static string getFileNameBase(string fileName);
};


#endif

