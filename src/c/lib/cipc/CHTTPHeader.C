// CHTTPHeader.C
// Created: 05/05/2000
// Version: 05/24/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPHeader.H"
#include "dsm/CHTTPUtils.H"

#include <cctype>

// Constructor: CHTTPHeader ************************************************

CHTTPHeader::CHTTPHeader(const string_type& s) throw(CHTTPHeader::ParseError)
  : values_()
// Pre: Assigned(s)
// Modifies:
// Post: Create a new instance from <s>
{
  if (!parse(s))
    throw CHTTPHeader::ParseError();
}

// CHTTPHeader::clear ******************************************************

void CHTTPHeader::clear(void)
// Pre: Assigned()
// Modifies:
// Post: Clear current values
{
  values_.clear();
}

void CHTTPHeader::clear(const string_type& key)
{
  if (key.length())
  {
    StringPair  sp(key);
    for (string_vector::iterator i = values_.begin(); i != values_.end();)
    {
      if (*i == sp)
      {
        string_vector::iterator x = i;
        --i;
        values_.erase(x);
      }
      else
        ++i;
    }
  }
}

// CHTTPHeader::copy *******************************************************

void CHTTPHeader::copy(const CHTTPHeader& other)
// Pre: Assigned(other)
// Modifies: *this
// Post: Copy <other> to *this
{
  if (&other != this)
  {
    clear();
    values_ = other.values_;
  }
}

// CHTTPHeader::extractHeader **********************************************

bool CHTTPHeader::extractHeader(
  const string_type& hdr,
  size_type&         endPos,
  size_type          startPos)
// Pre: Assigned(hdr), Assigned(endPos), Assigned(startPos)
// Modifies: endPos
// Post: Extract the end position of the next header in <hdr>
{
  size_type len     = hdr.length(),
            currPos = startPos;

  if (!len || startPos >= len)
    return false;
  endPos = hdr.find(CRLF, currPos);
  while (endPos != string_type::npos &&
         (endPos + 2) < len &&
         (hdr[endPos + 2] == ' ' ||
          hdr[endPos + 2] == '\t') )
  {
    currPos = endPos + 2;
    endPos  = hdr.find(CRLF, currPos);
  }
  return endPos != string_type::npos;
}

// CHTTPHeader::find *******************************************************

CHTTPHeader::size_type
CHTTPHeader::find(int key) const
// Pre: Assigned(key)
// Modifies:
// Post: Return the position of the specified header or size() if not found
{
  return find(headerToString(key));
}

CHTTPHeader::size_type
CHTTPHeader::find(const string_type& key) const
// Pre: Assigned(key)
// Modifies:
// Post: Return the position of the specified header or size() if not found
{
  size_type   n = size();
  if (key.length())
  {
    StringPair  sp(key);
    for (size_type i = 0; i != n; ++i)
      if (sp == values_[i])
        return i;
  }
  return n;
}

// CHTTPHeader::fixCRLF ****************************************************

CHTTPHeader::string_type
CHTTPHeader::fixCRLF(const string_type& s)
// Pre: Assigned(s)
// Modifies:
// Post:  Fix <s> so that each line ends with CRLF
// NOTE: CRLF == "\r\n"
{
  string_type result(s);
  size_type   pos = 0;
  while ((pos = result.find("\n", pos)) != string_type::npos)
  {
    if (pos == 0 || result[pos - 1] != '\r')
    // lone newline char
      result.replace(pos, size_type(1), CRLF);
    pos += 2;
  }
  return result;
}

// CHTTPHeader::header *****************************************************

const CHTTPHeader::string_type&
CHTTPHeader::header(int key) const
// Pre: Assigned(key)
// Modifies:
// Post: Get the specified header
{
  return header(headerToString(key));
}

const CHTTPHeader::string_type&
CHTTPHeader::header(const string_type& key) const
// Pre: Assigned(key)
// Modifies:
// Post: Get the specified header
{
  static string_type  errStr = "";
  size_type           i      = find(key);

  if (i == size())
    return errStr;

  return values_[i].second;
}

void CHTTPHeader::header(
  int                key,
  const string_type& value)
// Pre: Assigned(key), Assigned(value)
// Modifies:
// Post: Set the specified header
{
  header(headerToString(key), value, false);
}

void CHTTPHeader::header(
  const string_type& key,     // can be an extension header field name
  const string_type& value,
  bool               validate)
// Pre: Assigned(key), Assigned(value), Assigned(validate)
// Modifies:
// Post: Set the specified header
{
  if (!key.length() ||
      (validate && stringToHeader(key) < 0) )
    return;

  StringPair              sp(key);
  string_vector::iterator i;
  for (i = values_.begin(); i != values_.end() && *i != sp; ++i) {}
  if (i == values_.end())
    values_.push_back(StringPair(key, value) );
  else
    (*i).second = value;
}

// CHTTPHeader::headerToString *********************************************

CHTTPHeader::string_type
CHTTPHeader::headerToString(int header) const
// Pre: Assigned()
// Modifies:
// Post: Return the string version of <header> or "" if not found
{
  switch (header)
  {
    case CACHE_CONTROL :
      return "Cache-Control";
    case CONNECTION :
      return "Connection";
    case DATE :
      return "Date";
    case PRAGMA :
      return "Pragma";
    case TRAILER :
      return "Trailer";
    case TRANSFER_ENCODING :
      return "Transfer-Encoding";
    case UPGRADE :
      return "Upgrade";
    case VIA :
      return "Via";
    case WARNING :
      return "Warning";

    case ACCEPT :
      return "Accept";
    case ACCEPT_CHARSET :
      return "Accept-Charset";
    case ACCEPT_ENCODING :
      return "Accept-Encoding";
    case ACCEPT_LANGUAGE :
      return "Accept-Language";
    case AUTHORIZATION :
      return "Authorization";
    case EXPECT :
      return "Expect";
    case FROM :
      return "From";
    case HOST :
      return "Host";
    case IF_MATCH :
      return "If-Match";
    case IF_MODIFIED_SINCE :
      return "If-Modified-Since";
    case IF_NONE_MATCH :
      return "If-None-Match";
    case IF_RANGE :
      return "If-Range";
    case IF_UNMODIFIED_SINCE :
      return "If-Unmodified-Since";
    case MAX_FORWARDS :
      return "Max-Forwards";
    case PROXY_AUTHORIZATION :
      return "Proxy-Authorization";
    case RANGE :
      return "Range";
    case REFERER :
      return "Referer";
    case TE :
      return "TE";
    case USER_AGENT :
      return "User-Agent";

    case ACCEPT_RANGES :
      return "Accept-Ranges";
    case AGE :
      return "Age";
    case ETAG :
      return "ETag";
    case LOCATION :
      return "Location";
    case PROXY_AUTHENTICATE :
      return "Proxy-Authenticate";
    case RETRY_AFTER :
      return "Retry-After";
    case SERVER :
      return "Server";
    case VARY :
      return "Vary";
    case WWW_AUTHENTICATE :
      return "WWW-Authenticate";

    case ALLOW :
      return "Allow";
    case CONTENT_ENCODING :
      return "Content-Encoding";
    case CONTENT_LANGUAGE :
      return "Content-Language";
    case CONTENT_LENGTH :
      return "Content-Length";
    case CONTENT_LOCATION :
      return "Content-Location";
    case CONTENT_MD5 :
      return "Content-MD5";
    case CONTENT_RANGE :
      return "Content-Range";
    case CONTENT_TYPE :
      return "Content-Type";
    case EXPIRES :
      return "Expires";
    case LAST_MODIFIED :
      return "Last-Modified";

    case COOKIE :
      return "Cookie";
    case FOR_METHOD :
      return "For-Method";
    case KEEP_ALIVE :
      return "Keep-Alive";
    case SET_COOKIE :
      return "Set-Cookie";
    case NUM_HEADERS :
    default :
      return "";
  }
}

// CHTTPHeader::index ******************************************************

bool CHTTPHeader::index(
  size_type    i,
  string_type& key,
  string_type& value) const
// Pre: Assigned(i), Assigned(key). Assigned(value)
// Modifies: key, value
// Post: Get the header at the specified index and return true.
//       Return false if the specified index is invalid.
{
  if (i >= size())
    return false;

  key   = values_[i].first;
  value = values_[i].second;

  return true;
}

// CHTTPHeader::parse ******************************************************

bool CHTTPHeader::parse(const string_type& s)
// Pre: Assigned(s)
// Modifies: *this
// Post: Parse a header string into self and return true
//       Return false on failure.
{
  bool        result = true;
  string_type hdr    = fixCRLF(s);
  size_type   len    = hdr.length();
  if (!len)
    return false;

  string_vector tmpValues;
  size_type     currPos = 0,
                nextPos;
  while (result && extractHeader(hdr, nextPos, currPos) )
  {
    string_type line(hdr.substr(currPos, nextPos - currPos)),
                key,
                value;
    result = parseHeaderLine(line, key, value);
    if (result)
      tmpValues.push_back(StringPair(key, value) );
    currPos = nextPos + 1;
  }
  if (result)
  {
    values_.clear();
    values_ = tmpValues;
  }
  return result;
}

// CHTTPHeader::parseHeaderLine ********************************************

bool CHTTPHeader::parseHeaderLine(
  const string_type& hdr,
  string_type&       key,
  string_type&       value)
// Pre: Assigned(hdr), Assigned(key), Assigned(value)
// Modifies:
// Post: Parse <key: value> from <header> and return true.
//       Return false on failure
{
  using std::isspace;

  size_type len = hdr.length();
  if (!len)
    return false;
  size_type spos = 0,
            epos;
  while (spos < len && isspace(hdr[spos]))
    ++spos;
  if (spos == len)
    return false;
  epos = hdr.find(":", spos);
  if (epos == string_type::npos)
    return false;
  key = hdr.substr(spos, epos - spos);
  spos = epos + 1;
  while (spos < len && isspace(hdr[spos]))
    ++spos;
  if (spos >= len)
  {
    value = "";
    return true;
  }
  epos = hdr.rfind(CRLF);
  if (epos == string_type::npos || epos != len - 2)
    epos = len;
  value = hdr.substr(spos, epos - spos);

  return true;
}

// CHTTPHeader::stringToHeader *********************************************

int CHTTPHeader::stringToHeader(
  const string_type& s,
  bool               ignoreCase) const
// Pre: Assigned(s), Assigned(ignoreCase)
// Modifies:
// Post: Return the header_type corresponding to <s> or -1 if not found
{
  string_type key;
  if (ignoreCase)
    key = to_upper(s);
  else
    key = s;

  for (int i = 0; i != (int) NUM_HEADERS; ++i)
  {
    if ((ignoreCase && key == to_upper(headerToString(i) ) ) ||
        (key == headerToString(i) ) )
      return i;
  }
  return -1;
}

// CHTTPHeader::toString ***************************************************

CHTTPHeader::string_type
CHTTPHeader::toString(void) const
// Pre:
// Modifies:
// Post: Return self as a string in proper HTTP format:
//         field-name ":" [ field-value ] <CRLF>
{
  string_type result("");
  for (string_vector::const_iterator i = values_.begin();
       i != values_.end(); ++i)
  {
    result += (*i).first;
    result += ": ";
    result += (*i).second;
    result += CRLF;
  }
  return result;
}

// CHTTPHeader::StringPair::operator == ************************************

bool
CHTTPHeader::StringPair::operator == (const CHTTPHeader::StringPair& other)
{
  return to_upper(first) == to_upper(other.first);
}

// end of CHTTPHeader.C **************************************************

