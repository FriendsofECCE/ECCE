/**
 * @file 
 *
 *
 */
#ifndef STRINGTOKENIZER_HH
#define STRINGTOKENIZER_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

/**
 * Simple wrapper class that does non-descructive tokenizing of a
 * specified string.  It does this by making a copy and then letting
 * strtok destructivly tokenize that copy.
 */
class StringTokenizer 
{

   public:
      StringTokenizer(const string& str, const string& pattern=" \t");
      virtual ~StringTokenizer();

      vector<string> tokenize(const string& pattern);

      string next();
      string next(const string& pattern);
      bool hasMoreTokens();

   protected:

      StringTokenizer(const StringTokenizer& rhs);

      char *p_string;   // copy of string that we can parse destructively
      char *p_curToken; // last pointer;
      char *p_pattern;  // current match pattern
      char *p_end;      // end of string - needed for hasMoreTokens;
      bool  p_firstTime;


};

#endif
