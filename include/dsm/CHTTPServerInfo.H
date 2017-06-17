// CHTTPServerInfo.H
// Created: 05/10/2000
// Version: 05/10/2000
//
// Description:
//
//   Class CHTTPServerInfo can be used to hold basic server information
//   from a set of headers returned from a server request
//
// Modifications:

#if !defined( __CHTTPSERVERINFO_H )
#define __CHTTPSERVERINFO_H   // prevent multiple includes

// include files ***********************************************************

#include "dsm/CHTTPHeader.H"

// class: CHTTPServerInfo **************************************************

class CHTTPServerInfo
{
public :

  typedef std::string       string_type;
  typedef std::vector<int>  methods_vector;
  enum auth_type
  {
    AUTH_NONE,    // no authorization required
    AUTH_BASIC,   // basic authorization
    AUTH_DIGEST   // digest authorization
  };

  // Constructors
  CHTTPServerInfo(void);
  CHTTPServerInfo(const CHTTPHeader&);
  CHTTPServerInfo(const CHTTPServerInfo&);

  // Destructor
  virtual ~CHTTPServerInfo(void) { clear(); }

  // Assignment operator
  CHTTPServerInfo& operator = (const CHTTPServerInfo&);

  // Get the authorization type
  virtual int authType(void) const { return auth_; }
  virtual int authType(string_type& realm) const
  { realm = realm_; return auth_; }

  // Check if using chunked Transfer-Encoding
  virtual bool chunkedTE(void) const { return chunkedTE_; }

  // Clear current values
  virtual void clear(void);

  // Get server information from headers
  virtual void getInfo(const CHTTPHeader&);

  // Check if using Keep-Alive
  virtual bool keepAlive(void) const { return keepAlive_; }
  virtual bool keepAlive(
    int& timeout,       // connection timeout
    int& maxReq) const  // max requests per connection
  { timeout = timeout_; maxReq = max_; return keepAlive_; }

  // Get the methods supported
  virtual const methods_vector& methods(void) const { return methods_; }

  // Get the value from the "Server" header
  virtual const string_type& server(void) const { return server_; }

protected :

  bool            chunkedTE_,   // using chunked Transfer-Encoding
                  keepAlive_;   // using Keep-Alive?
  int             auth_,        // authorization type
                  timeout_,     // Keep-Alive timeout
                  max_;         // max requests per connection
  string_type     realm_,       // realm needing authorization
                  server_;      // value from the Server header
  methods_vector  methods_;     // methods supported from Allow header

  // If <m> is a valid method string, return it's numeric value.
  // Return -1 if invalid.
  // NOTE: This version uses CHTTPRequest::stringToMethod()
  virtual int methodToInt(const string_type& m) const;
};

#endif  // __CHTTPSERVERINFO_H

// end of CHTTPServerInfo.H ************************************************

