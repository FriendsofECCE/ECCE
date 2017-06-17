// CDAVClient.H
// Created: 02/08/2000
// Version: 02/29/2000
//
// Description: 
//
// Modifications: 

#if !defined( __CDAVCLIENT_H )
#define __CDAVCLIENT_H  // prevent multiple includes

// include files ***********************************************************

#include "dsm/CHTTPClient.H"

// class: CDAVClient *******************************************************

class CDAVClient : public CHTTPClient
{
public :

  typedef CHTTPClient           parent;

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
    UNKNOWN     // unknown method
  };

  // Constructors
  CDAVClient(
    const string_type& h = "",                    // host
    unsigned short     p = HTTPDefPort)           // port
    : parent(h, p) { initDAVHeaders(); }
  CDAVClient(
    const string_type& h,                         // host
    const string_type& px,                        // proxy
    unsigned short     p = HTTPDefPort)           // port
    : parent(h, px, p) { initDAVHeaders(); }
  CDAVClient(const CDAVClient& other)
    : parent(other) { copyDAVHeaders(other); }

  // Destructor
  virtual ~CDAVClient(void) {}

  // Assignment operator
  CDAVClient& operator = (const CDAVClient& other)
  {
    if (&other != this)
    {
      parent::operator = (other);
      copyDAVHeaders(other);
    }
    return *this;
  }

  // Get/Set the specified header
  virtual const string_type& header(int key) const;
  virtual void        header(
    int                key,
    const string_type& value);
  template <class T>
  void                header(
    int      key,
    const T& value)
  { header(*this, key, value); }

  // Return the string version of <header> or "" if not found
  virtual string_type headerToString(int header) const;

  // Return the string version of <method> or "" if not found
  virtual string_type methodToString(int method) const;

  // Return the header_type corresponding to <s> or -1 if not found
  virtual int stringToHeader(
    const string_type& s,
    bool               ignoreCase = true) const;

  // Return the method_type corresponding to <s> or -1 if not found
  virtual int stringToMethod(
    const string_type& s,
    bool               ignoreCase = true) const;

protected :

  string_type davHeaders_[NUM_HEADERS];

  // Copy values in the davHeaders_ array
  virtual void copyDAVHeaders(const CDAVClient& other);

  // Create a method string from <method> and <URL> based on self's current
  // settings.
  virtual string_type DAVMsg(
    const string_type& URL,
    method_type        method);

  // Set default values in the davHeaders_ array
  virtual void initDAVHeaders(void);

  // Create a <methodStr> method string from <URL>.
  // Return "" if <methodStr> is not a valid DAV method
  virtual string_type otherMsg(
    const string_type& URL,
    int                method,
    const string_type& methodStr  = "",
    bool               ignoreCase = true);

  // Return the standard header fields
  virtual string_type stdHeader(const string_type& URL) const;
};

#endif  // __CDAVCLIENT_H

// end of CDAVClient.H *****************************************************

