/*************************************************************************
 Class: EcceRegexp
 
 Summary:  This is a replacement class for RWCRegexp.  It lets the user
           make a regular expression, and then parse substrings based on
           that pattern.  Pattern matching is case sensitive.
**************************************************************************/

#include "util/EcceRegexp.H"
#include "util/ErrMsg.H"


/*************************************************************************
 Method :  Constructor
 Summary: 
*************************************************************************/
EcceRegexp::EcceRegexp(const string& pattern) { 

  *this = pattern;
}

/*************************************************************************
 Method :  operator =
 Summary: 
*************************************************************************/
EcceRegexp& EcceRegexp::operator=(const string& pattern) {

  p_pattern = pattern;
 
  // YOU MUST SET THE REG_NEWLINE FLAG OR THE *, $, AND ^ SPECIAL
  // CHARACTERS WON'T WORK RIGHT

  // build the regex_t structure
  p_error = regcomp(&p_regexp, p_pattern.c_str(), REG_EXTENDED |
		    REG_NEWLINE);
  return *this;
}

/*************************************************************************
 Method :  Copy Constructor
 Summary: 
*************************************************************************/
EcceRegexp::EcceRegexp(const EcceRegexp& regexp) { 

  *this = regexp.p_pattern;
}

/*************************************************************************
 Method :  Destructor
 Summary: 
*************************************************************************/
EcceRegexp::~EcceRegexp() {

  regfree (&p_regexp);
}


/*************************************************************************
 Method : parse
 Summary: Returns the first parsed substring that matches the pattern 
          represented by this regexp object.

          str = the string to search for the regexp
     startIdx = the character in str to start the search at (defaults to 0)
     foundIdx = the index where the substring matching the regex starts
      returns   the longest substring matching regex
*************************************************************************/
string EcceRegexp::match(const string& str,
			 int& foundIdx, 
			 int& substrLength,
			 int startIdx) 
{
  string ret;
  foundIdx = (int)string::npos;
  substrLength = 0;
  regmatch_t pmatch[1];   // only find 1 substr at a time
                          // (for the full pattern)
  int eflags = 0;
  
  if (startIdx > 0) {
    eflags = REG_NOTBOL;
  }
  
  // First check if regexp was compiled successfully:
  if (p_error != 0) {
    printError(p_error);

  } else {
    int status;
    if ((status = regexec(&p_regexp, (const char*)&str[startIdx], 1,
         pmatch, eflags)) == 0) {  
      substrLength = pmatch[0].rm_eo - pmatch[0].rm_so;
      ret = str.substr(pmatch[0].rm_so + startIdx, substrLength);
      foundIdx = pmatch[0].rm_so + startIdx; 

    } else if (status != REG_NOMATCH) {
      printError(status);
    }
  }
  return ret;
}


void EcceRegexp::printError(const int& errorCode) {

  char errMsg[1001];
  regerror(errorCode, &p_regexp, errMsg, 1000);
  EE_ASSERT(false, EE_WARNING, errMsg);

}
