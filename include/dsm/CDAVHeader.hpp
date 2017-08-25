// CDAVHeader.H
// Created: 05/09/2000
// Version: 05/09/2000
//
// Description:
//
// Modifications:

#if !defined( __CDAVHEADER_H )
#define __CDAVHEADER_H    // prevent multiple includes

// include files ***********************************************************

#include "dsm/CHTTPHeader.H"

// class: CDAVHeader *******************************************************

class CDAVHeader : public CHTTPHeader
{
public :

  typedef CHTTPHeader parent;
  enum header_type
  {
    DAV = parent::NUM_HEADERS,
    DEPTH,
    DESTINATION,
    IF,
    LOCK_TOKEN,
    OVERWRITE,
    STATUS_URI,
    TIMEOUT,
    NUM_HEADERS = TIMEOUT - DAV + 1
  };

  // Constructors
  CDAVHeader(void) : parent() {}
  CDAVHeader(const string_type& s) throw(ParseError) : parent(s) {}
  CDAVHeader(const CDAVHeader& other) : parent(other) {}

  // Destructor
  virtual ~CDAVHeader(void) {}

  // Assignment operator
  CDAVHeader& operator = (const CDAVHeader& other)
  { parent::operator = (other); return *this; }

  // Return the string version of <header> or "" if not found
  virtual string_type headerToString(int header) const;

  // Return the header_type corresponding to <s> or -1 if not found
  virtual int stringToHeader(
    const string_type& s,
    bool               ignoreCase = true) const;
};

#endif  // __CDAVHEADER_H

// end of CDAVHeader.H *****************************************************

