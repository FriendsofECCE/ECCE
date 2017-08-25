// CDAVResponse.H
// Created: 05/10/2000
// Version: 05/10/2000
//
// Description:
//
// Modifications:

#if !defined( __CDAVRESPONSE_H )
#define __CDAVRESPONSE_H    // prevent multiple includes

// include files ***********************************************************

#include "dsm/CHTTPResponse.H"

// class: CDAVResponse *****************************************************

class CDAVResponse : public CHTTPResponse
{
public :

  typedef CHTTPResponse parent;
  enum status_type
  {
    MULTI_STATUS = 207
  };

  // Constructors
  CDAVResponse(void) : parent() {}
  CDAVResponse(const CDAVResponse& other) : parent(other) {}

  // Destructor
  virtual ~CDAVResponse(void) {}

  // Assignment operator
  CDAVResponse& operator = (const CDAVResponse& other)
  { parent::operator = (other); return *this; }

  // Return true if <s> is a valid status code, false otherwise
  virtual bool isValidStatus(int s) const;

  // Return the string version of <status> or "" if not found
  virtual string_type statusCodeToString(void) const
  { return parent::statusCodeToString(); }
  virtual string_type statusCodeToString(int status) const;
};

#endif  // __CDAVRESPONSE_H

// end of CDAVResponse.H ***************************************************

