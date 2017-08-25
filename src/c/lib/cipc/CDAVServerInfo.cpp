// CDAVServerInfo.C
// Created: 05/10/2000
// Version: 05/23/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPUtils.H"
#include "dsm/CDAVServerInfo.H"
#include "dsm/CDAVRequest.H"

#include <cstdlib>

// CDAVServerInfo::getInfo *************************************************

void CDAVServerInfo::getInfo(const CDAVHeader& h)
// Pre: Assigned(h)
// Modifies:
// Post: Get server information from headers
{
  // Get HTTP info
  parent::getInfo(h);

  // Get DAV info
  string_type value;

  // Look for DAV
  value = to_upper(h.header(CDAVHeader::DAV) );
  if (value.length())
  {
    string_type::size_type  len = value.length(),
                            spos = 0,
                            epos;
    while (spos < len)
    {
      epos = value.find(",", spos);
      if (epos == string_type::npos)
        epos = len;
      int n = std::atoi(value.substr(spos, epos - spos).c_str() );
      switch (n)
      {
        case 1 :
          isDAVServer_ = true;
          break;
        case 2 :
          supportLocks_ = true;
          break;
        default :
          break;
      }
      spos = epos + 1;
    }
  }
}

// CDAVServerInfo::methodToInt *********************************************

int CDAVServerInfo::methodToInt(const string_type& m) const
// Pre: Assigned(m)
// Modifies:
// Post: If <m> is a valid method string, return it's numeric value.
//       Return -1 if invalid.
// NOTE: This version uses CDAVRequest::stringToMethod()
{
  return CDAVRequest().stringToMethod(m);
}

// end of CDAVServerInfo.C *************************************************

