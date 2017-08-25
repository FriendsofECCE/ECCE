#include <stdlib.h>
#include <string.h>

#include "util/StringTokenizer.H"

StringTokenizer::StringTokenizer(const string& str, const string& pattern)
{
  p_string = strdup(str.c_str());
  p_pattern = strdup(pattern.c_str());
  p_curToken = 0;
  p_firstTime = true;

  // Set p_end to end of string for fast impl. of hasMoreTokens()
  p_end = p_string + strlen(p_string);
}

StringTokenizer::StringTokenizer(const StringTokenizer& rhs)
{
  p_string = strdup(rhs.p_string);
  p_pattern = strdup(rhs.p_pattern);

  // Set p_end to end of string for fast impl. of hasMoreTokens()
  p_end = p_string + strlen(p_string);

  // No nice way to handle this in copy constructor.
  p_firstTime = true;
  p_curToken = 0;  
}

StringTokenizer::~StringTokenizer()
{
  free(p_string);
  free(p_pattern);

  // Don't delete p_curToken since its a pointer to a string within  p_string.
}

/**
 * Returns a list of all the tokens in one easy step.
 * Safe to use if you've already called next.
 */
vector<string> StringTokenizer::tokenize(const string& pattern)
{
  vector<string> ret;
  string str;
  while (!(str=next(pattern)).empty()) {
    ret.push_back(str);
  }
  return ret;
}

/**
 * Get next token using the new pattern.  Make that the new default
 * pattern.
 */
string StringTokenizer::next()
{
  string ret;
  if (p_firstTime == true) {
    p_curToken = strtok(p_string,p_pattern);
  } else {
    p_curToken = strtok(0,p_pattern);
  }
  p_firstTime = false;
  if (p_curToken) ret = p_curToken;
  return ret;
}


/**
 * Get next token using the pattern set previously by other means.
 */
string StringTokenizer::next(const string& pattern)
{
  if (p_pattern) free(p_pattern);
  p_pattern = strdup(pattern.c_str());

  return next();
}

/**
 * Figures out if there are more tokens.
 * This method will return true if there is more data but there
 * aren't really tokens (ie data is all delimiters);
 * so you should always also check if next returns an empty string.
 * Couldn't think of a way around this that wasn't just ugly.
 * Uses p_end which is the address of the end of the original string.
 */
bool StringTokenizer::hasMoreTokens()
{
   bool ret = false;

   if (p_curToken) {
      if ((p_curToken + strlen(p_curToken) + 1) < p_end) ret = true;
   } else {
      if (p_firstTime) ret = true;
   }

   return ret;
}
