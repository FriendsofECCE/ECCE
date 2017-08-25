// CHTTPClient.H
// Created: 06/03/1998
// Version: 02/29/2000
//
// Description: 
//
//   HTTP Client class.
//
// Modifications: 
//
//   10/19/1998 - K. Walker - Added namespace http_api.
//     Replaced RWCString with the standard library string class
//   10/21/1998 - K. Walker - Added proxies
//   03/25/1999 - K. Walker - Added "ipc" namespace
//   02/08/2000 - K. Walker - Added method_type, propertyString(),
//     otherMSG() and toupper() methods.
//   02/11/2000 - K. Walker - Added all HTTP methods to method_type.
//     Added methodToString(), stringToMethod(), namePassword(), password(),
//     userName() public methods.
//     Added createAuthStr() and stdHeader() protected methods.
//     Changed GETMsg() to HTTPMsg().
//   02/29/2000 - K. Walker - Added keepOpen() method

#if !defined( __CHTTPCLIENT_H )
#define __CHTTPCLIENT_H  // prevent multiple includes

// include files ***********************************************************

#include <iostream>
#include <sstream>
#include <map>
#include <string>
using std::string;

#include "dsm/CSocket.H"
#include "dsm/CCookie.H"
#include "dsm/CIPCHTTP.H"

using std::cout;
using std::ostream;

// header ******************************************************************

template <class T, class V>
inline void header(
  T&       inst,
  int      key,
  const V& value)
// Pre: Assigned(inst), Assigned(key), Assigned(value)
// Modifies:
// Post:
{
  std::ostringstream  ostr;
  ostr << value;
  // Karen is hacking again.  This line doesn't compile with g++
  //T::string_type  s = ostr.str();
  string  s = ostr.str();
  inst.header(key, s);
}

// class: CHTTPClient ******************************************************

class CHTTPClient
{
public :

  typedef std::string string_type;

  // We use a multi map to associate multiple cookies with a single path
  // <Key> is the cookie's path, <Data> is the cookie instance
  typedef std::multimap<string_type, CCookie>   cookie_list;
  typedef string_type::size_type                size_type;

  enum header_type
  {
    // General Headers
    CACHE_CONTROL,
    CONNECTION,
    DATE,
    PRAGMA,
    TRAILER,
    TRANSFER_ENCODING,
    UPGRADE,
    VIA,
    WARNING,

    // Request Headers
    ACCEPT,
    ACCEPT_CHARSET,
    ACCEPT_ENCODING,
    ACCEPT_LANGUAGE,
    AUTHORIZATION,
    EXPECT,
    FROM,
    HOST,
    IF_MATCH,
    IF_MODIFIED_SINCE,
    IF_NONE_MATCH,
    IF_RANGE,
    IF_UNMODIFIED_SINCE,
    MAX_FORWARDS,
    PROXY_AUTHORIZATION,
    RANGE,
    REFERER,
    TE,                   // Transfer encoding
    USER_AGENT,

    // Response Headers
    ACCEPT_RANGES,
    AGE,
    ETAG,
    LOCATION,
    PROXY_AUTHENTICATE,
    RETRY_AFTER,
    SERVER,
    VARY,
    WWW_AUTHENTICATE,

    // Entity Headers
    ALLOW,
    CONTENT_ENCODING,
    CONTENT_LANGUAGE,
    CONTENT_LENGTH,
    CONTENT_LOCATION,
    CONTENT_MD5,
    CONTENT_RANGE,
    CONTENT_TYPE,
    EXPIRES,
    LAST_MODIFIED,

    // Other Headers
    COOKIE,
    FOR_METHOD,
    SET_COOKIE,

    NUM_HEADERS     // number of types in header_type
  };

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
  CHTTPClient(
    const string_type& = "",            // host
    unsigned short     = HTTPDefPort);  // port
  CHTTPClient(
    const string_type&,                 // host
    const string_type&,                 // proxy
    unsigned short     = HTTPDefPort);  // port
  CHTTPClient(const CHTTPClient&);

  // Destructor
  virtual ~CHTTPClient(void);

  // Assignment operator
  CHTTPClient& operator = (const CHTTPClient&);

  // Relational operators
  bool operator == (const CHTTPClient& other) const
  { return compareTo(other) == 0; }
  bool operator != (const CHTTPClient& other) const
  { return compareTo(other) != 0; }
  bool operator <  (const CHTTPClient& other) const
  { return compareTo(other) < 0; }
  bool operator <= (const CHTTPClient& other) const
  { return compareTo(other) <= 0; }
  bool operator >  (const CHTTPClient& other) const
  { return compareTo(other) > 0; }
  bool operator >= (const CHTTPClient& other) const
  { return compareTo(other) >= 0; }

  // Clear cookies
  virtual void clearCookies(void);

  // Compare self to <other> on host():
  //   Return -1 if self is less than other
  //   Return  0 if self is equal to other
  //   Return  1 if self is greater than other
  virtual int compareTo(const CHTTPClient& other) const
  { return host().compare(other.host() ); }

  // Return the current cookies (keyed on CCookie::path())
  virtual const cookie_list& cookies(void) const { return cookiesByPath_; }
  virtual cookie_list&       cookies(void)       { return cookiesByPath_; }

  // Get the default Content-type
  static const string_type& defContentType(void) { return defContentType_; }

  // Get the default file types to accept from the server
  static const string_type& defFileTypes(void) { return defFileTypes_; }

  // Get the default HTTP Version "X.X"
  static const string_type& defHTTPVersion(void) { return defHTTPVersion_; }

  // Get the default User-Agent
  static const string_type& defUserAgent(void) { return defUserAgent_; }

  // Get/Set the specified header
  virtual const string_type& header(int key) const
  {
    static string_type  errStr = "";
    if (key > -1 && key < (int) NUM_HEADERS)
      return headers_[key];
    return errStr;
  }
  virtual void        header(
    int                key,
    const string_type& value)
  {
    if (key > -1 && key < (int) NUM_HEADERS)
      headers_[key] = value;
  }
  template <class T>
  void                header(
    int      key,
    const T& value)
  { header(*this, key, value); }
 
  // Return the string version of <header> or "" if not found
  virtual string_type headerToString(int header) const;

  // Get/Set the host.
  virtual const string_type& host(void) const
  { return headers_[(int) HOST]; }
  virtual void               host(const string_type& h)
  { headers_[(int) HOST] = h; }

  // Get/Set the HTTP version
  virtual const string_type& HTTPVersion(void) const
  { return HTTPVersion_; }
  virtual void               HTTPVersion(const string_type& v)
  { HTTPVersion_ = v; }

  // Get/Set if the communication socket to the server should be
  // kept open between calls to openURL()
  virtual bool keepOpen(void) const { return keepOpen_; }
  virtual void keepOpen(bool x)     { keepOpen_ = x; }

  // Return the string version of <method> or "" if not found
  virtual string_type methodToString(int method) const;

  // Open and retrieve the given URL and return true.
  // Return false on failure.
  // NOTE: Pass -1 for <method> to interpret the method from <methodStr>
  virtual bool openURL(
    const string_type& URL       = "/",
    ostream&           ostr      = cout,
    int                method    = GET,
    const string_type& methodStr = "",      // if <method> >= NUM_METHODS
    bool               ignoreCase = true);  // if <methodStr_> is used

  // Set the user name and password
  void namePassword(
    const string_type& n,
    const string_type& pw)
  {
    userName_ = n;
    password_ = pw;
    createAuthStr();
  }

  // Return the user password
  const string_type& password(void) const { return password_; }

  // Get/Set the port number
  virtual unsigned short port(void) const       { return port_; }
  virtual void           port(unsigned short p) { port_ = p; }

  // Get/Set the proxy
  virtual const string_type& proxy(void) const           { return proxy_; }
  virtual void               proxy(const string_type& s) { proxy_ = s; }

  // Return the header_type corresponding to <s> or -1 if not found
  virtual int stringToHeader(
    const string_type& s,
    bool               ignoreCase = true) const;

  // Return the method_type corresponding to <s> or -1 if not found
  virtual int stringToMethod(
    const string_type& s,
    bool               ignoreCase = true) const;

  // Get/Set whether or not to use cookies.
  virtual bool useCookies(void) const { return useCookies_; }
  virtual void useCookies(bool n)     { useCookies_ = n; }

  // Get/Set whether or not to use HTTPS.
  // NOTE: This is only used if the proxy is set
  virtual bool useHTTPS(void) const { return useHTTPS_; }
  virtual void useHTTPS(bool n)     { useHTTPS_ = n; }

  // Return the user name
  const string_type& userName(void) const { return userName_; }

protected :

  static const string_type  defContentType_;
  static const string_type  defFileTypes_;
  static const string_type  defHTTPVersion_;
  static const string_type  defUserAgent_;

  string_type         headers_[NUM_HEADERS];  // all headers
  string_type         HTTPVersion_,
                      password_,
                      proxy_,
                      userName_;
  bool                keepOpen_,              // keep connection open?
                      useHTTPS_,              // used with proxy
                      useCookies_;
  cookie_list         cookiesByName_,         // key on CCookie::name()
                      cookiesByPath_;         // key on CCookie::path()
  ipc::CSocket::port_type   port_;
  ipc::CClientSocket        *client_;

  // Add <cookie> to <cookiesByName_> and <cookiesByPath_> and return true.
  // Return false if not added.
  // Cookies with the same path and name/value overwrite each other, with
  // the latest instance replacing the older.
  virtual bool addCookie(const CCookie& cookie);

  // Copy values in the headers array
  virtual void copyHeaders(const CHTTPClient& other);

  // Create <authStr_>
  void createAuthStr(void);

  // Create a method string from <method> and <URL> based on self's current
  // settings.
  virtual string_type HTTPMsg(
    const string_type& URL,
    method_type        method);

  // Set default values in the headers array
  virtual void initHeaders(void);

  // Create a string for <URL> from the method specified by <method> and/or
  // <methodStr>.
  // NOTE: This version does nothing.  It is up to a subclass to
  //       decide what <method>/<methodStr> is.
  virtual string_type otherMsg(
    const string_type& URL,
    int                method,
    const string_type& methodStr  = "",
    bool               ignoreCase = true) // if <methodStr_> is used
  { return ""; }

  // Process an incoming buffer from the server.
  // If the buffer is not HTTP header information, insert it into <ostr>
  // Return true on success, false otherwise.
  bool process(
    const char * buff,
    size_type    msgLen,
    ostream&     ostr);

  // Process HTTP header messages
  // Return true to continue processing server data, false otherwise
  // NOTE: processOther() is called for unrecognized headers
  virtual bool processHeader(
    int                header,
    const string_type& value) { return true; }
  virtual bool processOther(const string_type& msg) { return true; }
  virtual bool processSetCookie(const string_type& msg);
  virtual bool processStatus(const string_type& msg) { return true; }

  // Return a property string from <name, value>
  static string_type propertyString(
    const string_type& name,
    const string_type& value);

  // Return the standard header fields
  virtual string_type stdHeader(const string_type& URL) const;

  // Return <s> converted to lower case
  static string_type tolower(const string_type& s);

  // Return <s> converted to upper case
  static string_type toupper(const string_type& s);

private :

  enum
  {
    init,                 // new connection (init + [1..4]: looking for "HTTP")
    inHeader = init + 5,  // in HTTP header
    eoh1,                 // found "\n"
    eoh2,                 // found "\n\r"
    endHeader,            // found the end of the header
    inBody                // in entity body
  };

  int         state_;     // parsing state
  string_type header_;    // header buffer

  // Extract the end position of the next header in <header>
  static bool extractHeader(
    const string_type& header,
    size_type&         endPos,
    size_type          startPos = 0);

  // Return the position of <c> in <s>.
  // Return string::npos if not found
  static size_type index(
    const char * s,
    char         c,
    size_type    sLen,
    size_type    from = 0);

  // Parse the HTTP header
  bool parseHeader(void);

  // Process a line of the HTTP header
  bool processHeaderLine(const string_type&);
};

#endif  // __CHTTPCLIENT_H

// end of CHTTPClient.H ****************************************************

