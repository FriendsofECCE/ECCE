// CHTTPRequest.H
// Created: 05/05/2000
// Version: 05/11/2000
//
// Description:
//
//   Class CHTTPRequest represents a request to an HTTP server.
//
// Modifications:

#if !defined( __CHTTPREQUEST_H )
#define __CHTTPREQUEST_H    // prevent multiple includes

// include files ***********************************************************

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPVersion.H"
#include <string>

// class: CHTTPRequest *****************************************************

class CHTTPRequest
{
public :

#if defined( OLD_CPP_HEADERS )
  typedef string      string_type;
#else
  typedef std::string string_type;
#endif

  enum method_type
  {
    OPTIONS,      // request information about the communication options
                  // available on the request/response chain identified by
                  // the URI
    GET,          // retrieve whatever data is identified by the URI
    HEAD,         // return only HTTP headers and no document body
    POST,         // create a new object linked to the specified object
    PUT,          // specify that the data in the body section is to be
                  // stored under the supplied URL
    DELETE,       // request that the server delete the information
                  // corresponding to the given URL
    TRACE,        // invoke a remote, application-layer loop- back of the
                  // request message
    CONNECT,      // this specification reserves the method name CONNECT
                  // for use with a proxy that can dynamically switch to
                  // being a tunnel

    NUM_METHODS   // number of types in method_type
  };

  // Constructors
  CHTTPRequest(void)
    : HTTPVersion_(), URI_(""), method_(GET) {}
  CHTTPRequest(const CHTTPRequest& other)
    : HTTPVersion_(other.HTTPVersion_), URI_(other.URI_),
      method_(other.method_) {}

  // Destructor
  virtual ~CHTTPRequest(void) {}

  // Assignment operator
  CHTTPRequest& operator = (const CHTTPRequest& other)
  {
    if (&other != this)
    {
      HTTPVersion_ = other.HTTPVersion_;
      URI_         = other.URI_;
      method_      = other.method_;
    }
    return *this;
  }

  // Get/Set the HTTP version (e.g. 1.1)
  virtual const CHTTPVersion& HTTPVersion(void) const { return HTTPVersion_; }
  virtual void                HTTPVersion(const CHTTPVersion& v)
  { HTTPVersion_ = v; }

  // Get/Set the request method
  // NOTE: The set methods return true if the method was validated and set
  virtual int  method(void) const { return method_; }
  virtual bool method(int);
  virtual bool method(const string_type& s);

  // Return the string version of <method> or "" if not found
  virtual string_type methodToString(int method) const;

  // Return the method_type corresponding to <s> or -1 if not found
  virtual int stringToMethod(
    const string_type& s,
    bool               ignoreCase = true) const;

  // Return self as a string in proper HTTP format:
  // <method> <URI> HTTP/<HTTPVersion>CRLF
  virtual string_type toString(void) const;

  // Get/Set the request URI
  virtual const string_type& URI(void) const           { return URI_; }
  virtual void               URI(const string_type& s) { URI_ = s; }

protected :

  CHTTPVersion  HTTPVersion_; // HTTP version
  string_type   URI_;         // Request-URI
  int           method_;      // method for request
};

#endif  // __CHTTPREQUEST_H

// end of CHTTPRequest.H ***************************************************

