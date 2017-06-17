///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: EcceRegexp.H
//
//
// CLASS SYNOPSIS:
//  Takes the place of the RWCRegexp class.  Compiles regular expressions
//  and lets you extract substrings based on them.  Pattern matching is
//  case sensitive.
///////////////////////////////////////////////////////////////////////////////

#ifndef ECCEREGEXP_H
#define ECCEREGEXP_H

// System includes:
   #include <sys/types.h>
   #include <regex.h>

// Library includes:
   #include <string>

  using std::string;


class EcceRegexp { 

  public:
 
    // Constructors:
       EcceRegexp(const string& pattern);
       EcceRegexp(const EcceRegexp& regexp);
       ~EcceRegexp();
  
    // Methods:
       EcceRegexp& operator=(const string& pattern);

       // Finds the first substring in string that matches the regexp pattern
       string match(const string& str,
		    int& foundIdx,
		    int& substrLength,
		    int startIdx=0);
       
  private:

    // Methods
       void printError(const int& errorCode);

    // Data:
       regex_t    p_regexp;
       string     p_pattern;
       int        p_error;

};

#endif // ECCEREGEXP_H

