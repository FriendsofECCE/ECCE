// CDAVHeader.C
// Created: 05/09/2000
// Version: 05/23/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPUtils.H"
#include "dsm/CDAVHeader.H"

// CDAVHeader::headerToString **********************************************

CDAVHeader::string_type
CDAVHeader::headerToString(int hdr) const
// Pre: Assigned(hdr)
// Modifies:
// Post: Return the string version of <header> or "" if not found
{
  switch (hdr)
  {
    case DAV :
      return "DAV";
    case DEPTH :
      return "Depth";
    case DESTINATION :
      return "Destination";
    case IF :
      return "If";
    case LOCK_TOKEN :
      return "Lock-Token";
    case OVERWRITE :
      return "Overwrite";
    case STATUS_URI :
      return "Status-URI";
    case TIMEOUT :
      return "Timeout";
    default :
      return parent::headerToString(hdr);
  }
}

// Return the header_type corresponding to <s> or -1 if not found
int CDAVHeader::stringToHeader(
  const string_type& s,
  bool               ignoreCase) const
// Pre: Assigned(s), Assigned(ignoreCase)
// Modifies:
// Post:
{
  int hdr = parent::stringToHeader(s, ignoreCase);
  if (hdr > -1 && hdr < parent::NUM_HEADERS)
    return hdr;

  string_type key;
  if (ignoreCase)
    key = to_upper(s);
  else
    key = s;

  for (int i = (int) DAV; i <= (int) TIMEOUT; ++i)
  {
    if ((ignoreCase && key == to_upper(headerToString((header_type) i) ) ) ||
        (key == headerToString((header_type) i) ) )
      return i;
  }
  return -1;
}

// end of CDAVHeader.C ***************************************************

