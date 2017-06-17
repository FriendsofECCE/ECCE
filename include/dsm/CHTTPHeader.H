// CHTTPHeader.H
// Created: 05/05/2000
// Version: 05/24/2000
//
// Description:
//
// Modifications:

#if !defined( __CHTTPHEADER_H )
#define __CHTTPHEADER_H   // prevent multiple includes

// include files ***********************************************************

#include <string>
#include <vector>

// class: CHTTPHeader ******************************************************

class CHTTPHeader
{
public :

  typedef std::string             string_type;
  typedef string_type::size_type  size_type;

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

    // Extension Headers
    // NOTE: Others can be used by 1. subclassing or 2. as strings
    COOKIE,
    FOR_METHOD,
    KEEP_ALIVE,
    SET_COOKIE,

    NUM_HEADERS     // number of types in header_type
  };

  struct ParseError {}; // Error parsing header

  // Constructors
  CHTTPHeader(void) : values_() {}
  CHTTPHeader(const string_type&) throw(CHTTPHeader::ParseError);
  CHTTPHeader(const CHTTPHeader& other) : values_(other.values_) {}

  // Destructor
  virtual ~CHTTPHeader(void) { clear(); }

  // Assignment operator
  CHTTPHeader& operator = (const CHTTPHeader& other)
  { copy(other); return *this; }

  // Clear current values
  virtual void clear(void);

  // Clear specified header
  virtual void clear(int key) { clear(headerToString(key)); }
  virtual void clear(const string_type& key);

  // Return the position of the specified header or size() if not found
  virtual size_type find(int key) const;
  virtual size_type find(const string_type& key) const;

  // Get/Set the specified header.
  // NOTE: In the case of duplicate keys, the first occurrence is returned.
  virtual const string_type& header(int key) const;
  virtual const string_type& header(const string_type& key) const;
  virtual void        header(
    int                key,
    const string_type& value);
  virtual void        header(
    const string_type& key,               // can be an extension header field
    const string_type& value,
    bool               validate = true);  // check if non-extension header

  // Return the string version of <header> or "" if not found
  virtual string_type headerToString(int header) const;

  // Get the header at the specified index and return true.
  // Return false if the specified index is invalid.
  virtual bool index(
    size_type    i,
    string_type& key,
    string_type& value) const;

  // Parse a header string into self and return true
  // Return false on failure.
  virtual bool parse(const string_type&);

  // Return the number of current headers
  virtual size_type size(void) const { return values_.size(); }

  // Return the header_type corresponding to <s> or -1 if not found
  virtual int stringToHeader(
    const string_type& s,
    bool               ignoreCase = true) const;

  // Return self as a string in proper HTTP format:
  // field-name ":" [ field-value ] <CRLF>
  virtual string_type toString(void) const;

protected :

  struct StringPair : public std::pair<string_type, string_type>
  {
    typedef std::pair<string_type, string_type> parent;
    StringPair(
      const string_type& f,
      const string_type& s = "") : parent(f, s) {}
      
    bool operator == (const StringPair&);
    bool operator != (const StringPair& other)
    { return !(operator == (other)); }
  };
  typedef std::vector<StringPair> string_vector;

  string_vector values_;  // current header values

  // Copy other to self
  virtual void copy(const CHTTPHeader& other);

private :

  // Extract the end position of the next header in <header>
  static bool extractHeader(
    const string_type& header,
    size_type&         endPos,
    size_type          startPos = 0);

  // Fix <s> so that each line ends with CRLF
  static string_type fixCRLF(const string_type& s);

  // Parse <key: value> from <header> and return true.
  // Return false on failure
  static bool parseHeaderLine(
    const string_type& header,
    string_type&       key,
    string_type&       value);
};

#endif  // __CHTTPHEADER_H

// end of CHTTPHeader.H ****************************************************

