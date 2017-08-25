// CDAVRequest.H
// Created: 05/09/2000
// Version: 05/09/2000
//
// Description:
//
// Modifications:

#if !defined( __CDAVREQUEST_H )
#define __CDAVREQUEST_H   // prevent multiple includes

// include files ***********************************************************

#include "dsm/CHTTPRequest.H"

// class: CDAVRequest ******************************************************

class CDAVRequest : public CHTTPRequest
{
public :

  typedef CHTTPRequest  parent;
  enum method_type
  {
    PROPFIND = parent::NUM_METHODS,
                // retrieve properties defined on the resource identified
                // by the Request-URI
    PROPPATCH,  // process instructions specified in the request body to
                // set and/or remove properties defined on the resource
                // identified by the Request-URI
    MKCOL,      // create a new collection
    COPY,       // create a duplicate of the source resource, identified by
                // the Request-URI, in the destination resource, identified
                // by the URI in the Destination header
    MOVE,       // the logical equivalent of a copy (COPY), followed by
                // consistency maintenance processing, followed by a delete
                // of the source, where all three actions are performed
                // atomically
    LOCK,       // take out a lock of any access type
    UNLOCK,     // remove the lock identified by the lock token in the
                // Lock-Token request header from the Request-URI, and all
                // other resources included in the lock
    NUM_METHODS = UNLOCK - PROPFIND + 1
  };

  // Constructors
  CDAVRequest(void) : parent() {}
  CDAVRequest(const CDAVRequest& other) : parent(other) {}

  // Destructor
  virtual ~CDAVRequest(void) {}

  // Assignment operator
  CDAVRequest& operator = (const CDAVRequest& other)
  { parent::operator = (other); return *this; }

  // Get/Set the request method
  // NOTE: The set methods return true if the method was validated and set
  virtual int  method(void) const { return parent::method(); }
  virtual bool method(int);
  virtual bool method(const string_type& s) { return parent::method(s); }

  // Return the string version of <method> or "" if not found
  virtual string_type methodToString(int method) const;

  // Return the method_type corresponding to <s> or -1 if not found
  virtual int stringToMethod(
    const string_type& s,
    bool               ignoreCase = true) const;
};

#endif  // __CDAVREQUEST_H

// end of CDAVRequest.H ****************************************************

