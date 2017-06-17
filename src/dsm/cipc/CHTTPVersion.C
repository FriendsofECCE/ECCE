// CHTTPVersion.C
// Created: 05/19/2000
// Version: 05/19/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

#include "dsm/CHTTPVersion.H"

#include <cstdio>
#include <cstdlib>

// Constructor: CHTTPVersion ***********************************************

CHTTPVersion::CHTTPVersion(const string_type& v) throw(CHTTPVersion::ParseError)
  : major_(1), minor_(1)
// Pre: Assigned(v)
// Modifies:
// Post: Create a new instance from <v>
{
  if (!parse(v))
    throw CHTTPVersion::ParseError();
}

// CHTTPVersion::parse *****************************************************

bool CHTTPVersion::parse(const string_type& v)
// Pre: Assigned(v)
// Modifies:
// Post: Parse a result string into self and return true.
//       Return false on failure.
{
  using std::atoi;

  string_type::size_type  len  = v.length(),
                          spos = 0,
                          epos;
  if (!len)
    return false;

  // Skip HTTP/ if there
  spos = v.find("HTTP/");
  spos = (spos == string_type::npos ? 0 : spos + 5);

  // Find major version number
  epos = v.find(".", spos);
  if (epos == string_type::npos)
    return false;
  unsigned  m = atoi(v.substr(spos, epos - spos).c_str() );
  spos = epos + 1;
  if (spos >= len)
    return false;

  major_ = m;

  // Find minor version number
  epos = v.find(" ", spos);
  if (epos == string_type::npos)
    epos = len;
  minor_ = atoi(v.substr(spos, epos - spos).c_str() );

  return true;
}

// CHTTPVersion::toString **************************************************

CHTTPVersion::string_type
CHTTPVersion::toString(bool full) const
// Pre: Assigned(full)
// Modifies:
// Post: Return self as a string.
// If <full> is true, return HTTP/major.minor.
// Return major.minor otherwise
{
  using std::sprintf;

  const int   bsz = 16;
  char        buff[bsz];
  string_type result("");
  if (full)
    result += "HTTP/";
  sprintf(buff, "%d.%d", major_, minor_);
  result += buff;
  return result;
}

// end of CHTTPVersion.C *************************************************

