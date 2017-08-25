// CDAVRequest.C
// Created: 05/09/2000
// Version: 05/23/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

//#include "CHTTPDefs.h"
#include "dsm/CHTTPUtils.H"
#include "dsm/CDAVRequest.H"

// CDAVRequest::method *****************************************************

bool CDAVRequest::method(int m)
// Pre: Assigned(m)
// Modifies:
// Post: Set the request method.
//       Return true if the method was validated and set
{
  if (m < (int) PROPFIND)
    return parent::method(m);
  else if (m > (int) UNLOCK)
    return false;
  method_ = m;
  return true;
}

// CDAVRequest::methodToString *********************************************

CDAVRequest::string_type
CDAVRequest::methodToString(int m) const
// Pre: Assigned(m)
// Modifies:
// Post: Return the string version of <m> or "" if not found
{
  switch (m)
  {
    case PROPFIND :
      return "PROPFIND";
    case PROPPATCH :
      return "PROPPATCH";
    case MKCOL :
      return "MKCOL";
    case COPY :
      return "COPY";
    case MOVE :
      return "MOVE";
    case LOCK :
      return "LOCK";
    case UNLOCK :
      return "UNLOCK";
    default :
      return parent::methodToString(m);
  }
}

// CDAVRequest::stringToMethod *********************************************

int CDAVRequest::stringToMethod(
  const string_type& s,
  bool               ignoreCase) const
// Pre: Assigned(s), Assigned(ignoreCase)
// Modifies:
// Post:  Return the method_type corresponding to <s> or -1 if not found
{
  int m = parent::stringToMethod(s, ignoreCase);
  if (m > -1 && m < parent::NUM_METHODS)
    return m;

  string_type key;
  if (ignoreCase)
    key = to_upper(s);
  else
    key = s;

  for (int i = (int) PROPFIND; i <= (int) UNLOCK; ++i)
  {
    if ((ignoreCase && key == to_upper(methodToString(i) ) ) ||
        (key == methodToString(i) ) )
      return i;
  }
  return -1;
}

// end of CDAVRequest.C ****************************************************

