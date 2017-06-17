// CHTTPRequest.C
// Created: 05/05/2000
// Version: 05/23/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPUtils.H"
#include "dsm/CHTTPRequest.H"

// CHTTPRequest::method ****************************************************

bool CHTTPRequest::method(int m)
// Pre: Assigned(m)
// Modifies: method_
// Post: Set the request method
// NOTE: The set methods return true if the method was validated and set
{
  if (m < (int) OPTIONS || m >= (int) NUM_METHODS)
    return false;
  method_ = m;
  return true;
}

bool CHTTPRequest::method(const string_type& s)
// Pre: Assigned(s)
// Modifies: method_
// Post: Set the request method
// NOTE: The set methods return true if the method was validated and set
{
  return method(stringToMethod(s));
}

// CHTTPRequest::methodToString ********************************************

CHTTPRequest::string_type
CHTTPRequest::methodToString(int m) const
// Pre: Assigned(m)
// Modifies:
// Return the string version of <method> or "" if not found
{
  switch (m)
  {
    case OPTIONS :
      return "OPTIONS";
    case GET :
      return "GET";
    case HEAD :
      return "HEAD";
    case POST :
      return "POST";
    case PUT :
      return "PUT";
    case DELETE :
      return "DELETE";
    case TRACE :
      return "TRACE";
    case CONNECT :
      return "CONNECT";
    case NUM_METHODS :
    default :
      return "";
  }
}

// CHTTPRequest::stringToMethod ********************************************

int CHTTPRequest::stringToMethod(
    const string_type& s,
    bool               ignoreCase) const
// Pre: Assigned(s), Assigned(ignoreCase)
// Modifies:
// Post: Return the method_type corresponding to <s> or -1 if not found
{
  string_type key;
  if (ignoreCase)
    key = to_upper(s);
  else
    key = s;

  for (int i = 0; i != (int) NUM_METHODS; ++i)
  {
    if ((ignoreCase && key == to_upper(methodToString(i) ) ) ||
        (key == methodToString(i) ) )
      return i;
  }
  return -1;
}

// CHTTPRequest::toString **************************************************

CHTTPRequest::string_type
CHTTPRequest::toString(void) const
// Pre:
// Modifies:
// Post: Return self as a string in proper HTTP format:
//       field-name ":" [ field-value ] <CRLF>
{
  string_type result = methodToString(method());
  if (!result.length())
    return result;

  result += " ";
  result += URI();
  result += " ";
  result += HTTPVersion().toString();
  result += CRLF;

  return result;
}

// end of CHTTPRequest.C *************************************************

