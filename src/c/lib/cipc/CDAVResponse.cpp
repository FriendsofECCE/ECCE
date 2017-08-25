// CDAVResponse.C
// Created: 05/10/2000
// Version: 05/10/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

#include "dsm/CDAVResponse.H"

// CDAVResponse::isValidStatus *********************************************

bool CDAVResponse::isValidStatus(int s) const
// Pre: Assigned(s)
// Modifies:
// Post: Return true if <s> is a valid status code, false otherwise
{
  switch (s)
  {
    case MULTI_STATUS :
      return true;
    default :
      return parent::isValidStatus(s);
  }
}

// CDAVResponse::statusCodeToString ****************************************

CDAVResponse::string_type
CDAVResponse::statusCodeToString(int s) const
// Pre: Assigned(s)
// Modifies:
// Post: Return the string version of <s> or "" if not found
{
  switch (s)
  {
    case MULTI_STATUS :
      return "Multi-Status";
    default :
      return parent::statusCodeToString(s);
  }
}

// end of CDAVResponse.C *************************************************

