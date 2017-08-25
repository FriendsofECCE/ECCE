// CHTTPResponse.C
// Created: 05/08/2000
// Version: 05/23/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPUtils.H"
#include "dsm/CHTTPResponse.H"

#include <sstream>
#include <cctype>
#include <cstdlib>

// Constructors: CHTTPResponse *********************************************

CHTTPResponse::CHTTPResponse(void)
  : HTTPVersion_(), reasonPhrase_(""), status_(OK)
// Pre:
// Modifies:
// Post: Create a new instance
{
  reasonPhrase_ = reasonPhrase(status_);
}

CHTTPResponse::CHTTPResponse(const string_type& s) throw(CHTTPResponse::ParseError)
  : HTTPVersion_(), reasonPhrase_(""), status_(OK)
// Pre: Assigned(s)
// Modifies:
// Post: Create a new instance from <s>
{
  reasonPhrase_ = reasonPhrase(status_);
  if (!parse(s))
    throw CHTTPResponse::ParseError();
}

// CHTTPResponse::isValidStatus ********************************************

bool CHTTPResponse::isValidStatus(int s) const
// Pre: Assigned(s)
// Modifies:
// Post: Return true if <s> is a valid status code, false otherwise
{
  switch (s)
  {
    case CONTINUE :
    case SWITCHING_PROTOCOLS :
    case OK :
    case CREATED :
    case ACCEPTED :
    case NON_AUTHORITATIVE_INFORMATION :
    case NO_CONTENT :
    case RESET_CONTENT :
    case PARTIAL_CONTENT :
    case MULTIPLE_CHOICES :
    case MOVED_PERMANENTLY :
    case FOUND :
    case SEE_OTHER :
    case NOT_MODIFIED :
    case USE_PROXY :
    case TEMPORARY_REDIRECT :
    case BAD_REQUEST :
    case UNAUTHORIZED :
    case PAYMENT_REQUIRED :
    case FORBIDDEN :
    case NOT_FOUND :
    case METHOD_NOT_ALLOWED :
    case NOT_ACCEPTABLE :
    case PROXY_AUTHENTICATION_REQUIRED :
    case REQUEST_TIMEOUT :
    case CONFLICT :
    case GONE :
    case LENGTH_REQUIRED :
    case PRECONDITION_FAILED :
    case REQUEST_ENTITY_TOO_LARGE :
    case REQUEST_URI_TOO_LARGE :
    case UNSUPPORTED_MEDIA_TYPE :
    case REQUESTED_RANGE_NOT_SATISFIABLE :
    case EXPECTATION_FAILED :
    case INTERNAL_SERVER_ERROR :
    case NOT_IMPLEMENTED :
    case BAD_GATEWAY :
    case SERVICE_UNAVAILABLE :
    case GATEWAY_TIMEOUT :
    case VERSION_NOT_SUPPORTED :
      return true;
    default :
      return false;
  }
}

// CHTTPResponse::parse ****************************************************

bool CHTTPResponse::parse(const string_type& s)
// Pre: Assigned(s)
// Modifies:
// Post: Parse a result string into self and return true.
//       Return false on failure.
// NOTE: This doesn't check the reason phrase in the string
{
  using std::atoi;
  using std::isspace;

//cout << "parse response " << s << endl;
  string_type::size_type  len  = s.length(),
                          spos = 0,
                          epos = s.rfind(CRLF);
//cout << "length " << len << endl;
  if (len < 12)
    return false;

  // If there was a CRLF, ignore it
  if (epos != string_type::npos)
    len = epos;

  // Find HTTP version
  spos = s.find("HTTP/", spos);
  if (spos == string_type::npos)
    return false;
  spos += 5;
  epos = s.find(" ", spos);
  if (epos == string_type::npos)
    return false;
  string_type verStr = s.substr(spos, epos - spos);

  // Find status code
  spos = epos;
  while (spos < len && isspace(s[spos]) )
    ++spos;
  epos = spos + 1;
  while (epos < len && !isspace(s[epos]) )
    ++epos;
  if (epos == len)
    return false;
  string_type statStr  = s.substr(spos, epos - spos);
  int         statCode = atoi(statStr.c_str());
  if (status(statCode)) // NOTE: This sets both status_ and reasonPhrase_
  // Try to set the HTTP version
    return HTTPVersion_.parse(verStr);
  else
    return false;
}

// CHTTPResponse::reasonPhrase *********************************************

CHTTPResponse::string_type
CHTTPResponse::reasonPhrase(int s) const
// Pre: Assigned(s)
// Modifies:
// Post: Return the string version of <status> or "" if not found
{
  switch (s)
  {
    case CONTINUE :
      return "Continue";
    case SWITCHING_PROTOCOLS :
      return "Switching Protocols";
    case OK :
      return "OK";
    case CREATED :
      return "Created";
    case ACCEPTED :
      return "Accepted";
    case NON_AUTHORITATIVE_INFORMATION :
      return "Non-Authoritative Information";
    case NO_CONTENT :
      return "No Content";
    case RESET_CONTENT :
      return "Reset Content";
    case PARTIAL_CONTENT :
      return "Partial Content";
    case MULTIPLE_CHOICES :
      return "Multiple Choices";
    case MOVED_PERMANENTLY :
      return "Moved Permanently";
    case FOUND :
      return "Found";
    case SEE_OTHER :
      return "See Other";
    case NOT_MODIFIED :
      return "Not Modified";
    case USE_PROXY :
      return "Use Proxy";
    case TEMPORARY_REDIRECT :
      return "Temporary Redirect";
    case BAD_REQUEST :
      return "Bad Request";
    case UNAUTHORIZED :
      return "Unauthorized";
    case PAYMENT_REQUIRED :
      return "Payment Required";
    case FORBIDDEN :
      return "Forbidden";
    case NOT_FOUND :
      return "Not Found";
    case METHOD_NOT_ALLOWED :
      return "Method Not Allowed";
    case NOT_ACCEPTABLE :
      return "Not Acceptable";
    case PROXY_AUTHENTICATION_REQUIRED :
      return "Proxy Authentication Required";
    case REQUEST_TIMEOUT :
      return "Request Time-out";
    case CONFLICT :
      return "Conflict";
    case GONE :
      return "Gone";
    case LENGTH_REQUIRED :
      return "Length Required";
    case PRECONDITION_FAILED :
      return "Precondition Failed";
    case REQUEST_ENTITY_TOO_LARGE :
      return "Request Entity Too Large";
    case REQUEST_URI_TOO_LARGE :
      return "Request-URI Too Large";
    case UNSUPPORTED_MEDIA_TYPE :
      return "Unsupported Media Type";
    case REQUESTED_RANGE_NOT_SATISFIABLE :
      return "Requested Range Not Satisfiable";
    case EXPECTATION_FAILED :
      return "Expectation Failed";
    case INTERNAL_SERVER_ERROR :
      return "Internal Server Error";
    case NOT_IMPLEMENTED :
      return "Not Implemented";
    case BAD_GATEWAY :
      return "Bad Gateway";
    case SERVICE_UNAVAILABLE :
      return "Service Unavailable";
    case GATEWAY_TIMEOUT :
      return "Gateway Timeout";
    case VERSION_NOT_SUPPORTED :
      return "HTTP Version Not Supported";
    default :
      return "";
  }
}

// CHTTPResponse::reasonPhraseToStatus *************************************

int CHTTPResponse::reasonPhraseToStatus(
  const string_type& s,
  bool               ignoreCase) const
// Pre: Assigned(s), Assigned(ignoreCase)
// Modifies:
// Post: Return the status_type corresponding to <s> or -1 if not found
// NOTE: The compare is on the reason phrase, for example:
//       "Continue" instead of "100 Continue"
{
  static int  statusCodes[] = {
    CONTINUE, SWITCHING_PROTOCOLS, OK, CREATED, ACCEPTED,
    NON_AUTHORITATIVE_INFORMATION, NO_CONTENT, RESET_CONTENT,
    PARTIAL_CONTENT, MULTIPLE_CHOICES, MOVED_PERMANENTLY, FOUND,
    SEE_OTHER, NOT_MODIFIED, USE_PROXY, TEMPORARY_REDIRECT, BAD_REQUEST,
    UNAUTHORIZED, PAYMENT_REQUIRED, FORBIDDEN, NOT_FOUND,
    METHOD_NOT_ALLOWED, NOT_ACCEPTABLE, PROXY_AUTHENTICATION_REQUIRED,
    REQUEST_TIMEOUT, CONFLICT, GONE, LENGTH_REQUIRED, PRECONDITION_FAILED,
    REQUEST_ENTITY_TOO_LARGE, REQUEST_URI_TOO_LARGE, UNSUPPORTED_MEDIA_TYPE,
    REQUESTED_RANGE_NOT_SATISFIABLE, EXPECTATION_FAILED,
    INTERNAL_SERVER_ERROR, NOT_IMPLEMENTED, BAD_GATEWAY,
    SERVICE_UNAVAILABLE, GATEWAY_TIMEOUT, VERSION_NOT_SUPPORTED
  };
  static int  nCodes = sizeof(statusCodes) / sizeof(statusCodes[0]);
  string_type key;
  if (ignoreCase)
    key = to_upper(s);
  else
    key = s;

  for (int i = 0; i != nCodes; ++i)
  {
    if ((ignoreCase && key == to_upper(reasonPhrase(statusCodes[i]) ) ) ||
        (key == reasonPhrase(statusCodes[i]) ) )
      return statusCodes[i];
  }
  return -1;
}

// CHTTPResponse::status ***************************************************

bool CHTTPResponse::status(int s)
// Pre: Assigned(s)
// Modifies:
// Post: Set the status code
//       Return true if the status code was validated and set.
{
  if (!isValidStatus(s))
    return false;
  status_       = s;
  reasonPhrase_ = reasonPhrase(s);

  return true;
}

// CHTTPResponse::statusCodeToString ***************************************

CHTTPResponse::string_type
CHTTPResponse::statusCodeToString(int s) const
// Pre: Assigned(s)
// Modifies:
// Post: Return the string version of <s> or "" if not found
{
  if (!isValidStatus(s))
    return "";

  std::ostringstream  ostr;
  ostr << s;
  string_type result = ostr.str();

  return result;
}

// CHTTPResponse::toString *************************************************

CHTTPResponse::string_type
CHTTPResponse::toString(void) const
// Pre:
// Modifies:
// Post: Return self as a string in proper HTTP format:
//         HTTP-Version Status-Code Reason-Phrase CRLF
{
  string_type result("");
  result += HTTPVersion().toString();
  result += " ";
  result += statusCodeToString();
  result += " ";
  result += reasonPhrase();
  result += CRLF;

  return result;
}

// end of CHTTPResponse.C ************************************************

