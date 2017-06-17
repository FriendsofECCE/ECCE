// CHTTPUtils.C
// Created: 05/23/2000
// Version: 05/23/2000
//
// Description:
//
//   HTTP utility functions
//
// Modifications:

// include files ***********************************************************

#include <sstream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <cstdlib>

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPUtils.H"

using std::string;

// http_conv_special *******************************************************

string::size_type http_conv_special(string& s)
// Pre: Assigned(s)
// Modifies:
// Post: Convert special (reserved) HTTP characters in <s> to "%NN" where
//       <NN> is the character's hexidecimal value.
//       Return the number of characters converted.
{
  string::size_type n = 0;

  for (string::size_type i = 0; i != HTTPSpecialChars.length(); ++i)
  {
    string::size_type pos = s.find(HTTPSpecialChars[i]);
    while (pos != string::npos)
    {
      std::ostringstream  ostr;
      using std::ends;
      using std::hex;
      using std::ios;

      ostr.setf(ios::uppercase);
      ostr << "%" << hex << ((int) HTTPSpecialChars[i]) << ends;
      s.replace(pos, 1, ostr.str());
      n++;
      pos = s.find(HTTPSpecialChars[i]);
    }
  }

  return n;
}

// http_is_special *********************************************************

bool http_is_special(char c)
// Pre: Assigned(c)
// Modifies:
// Post: Return true if <c> is a special (reserved) HTTP character
//       Return false otherwise.
{
  for (string::size_type i = 0; i != HTTPSpecialChars.length(); ++i)
    if (c == HTTPSpecialChars[i])
      return true;
  return false;
}

// parse_uri ***************************************************************

bool parse_uri(
  const string&   uri,
  string&         host,
  string&         path,
  unsigned short& port)
// Pre: Assigned(uri), Assigned(host), Assigned(path), Assigned(port)
// Modifies: host, path, port
// Post: Parse the host, port, and path from the given URI and return true.
//       Return false on error
{
  typedef string::size_type size_type;
  size_type len = uri.length();
  if (!len)
    return false;

  // Look for host and path
  size_type spos = 0,
            epos = uri.find("://", spos);
  if (epos != string::npos)
    spos = epos + 3;
  epos = uri.find("/", spos);
  if (epos == string::npos)
  {
    host = uri.substr(spos, len - spos);
    path = "/";
  }
  else
  {
    host = uri.substr(spos, epos - spos);
    spos = epos;
    epos = len;
    path = uri.substr(spos, epos - spos);
  }
  spos = host.find(":");
  if (spos == string::npos)
    port = HTTPDefPort;
  else
  {
    string temp;
    epos = host.length();
    temp = host.substr(spos + 1, epos - spos - 1);
    port = std::atoi(temp.c_str());
    host.erase(spos);
  }

  return true;
}

// to_lower ****************************************************************

string to_lower(const string& s)
// Pre: Assigned(s)
// Modifies:
// Post: Return <s> converted to lower case
{
  using std::transform;

  string  result(s);
  transform(result.begin(), result.end(), result.begin(), tolower);
  return result;
}

// to_upper ****************************************************************

string to_upper(const string& s)
// Pre: Assigned(s)
// Modifies:
// Post: Return <s> converted to upper case
{
  using std::transform;

  string  result(s);
  transform(result.begin(), result.end(), result.begin(), toupper);
  return result;
}

// http_uu_encode **********************************************************

string http_uu_encode(const string& s)
// Pre: Assigned(s)
// Modifies:
// Post: HTTP-uu_encode - logic taken from 'webget' which was taken from
//       'Mosaic for X' code by Mark Riordan and Ari Luotonen
{
  static const char *substr = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
                              "abcdefghijklmnopqrstuvwxyz" \
                              "0123456789+/";
  string  result,
          temp(s);

  // Append 2 nulls in case we need to round off an odd byte or two
  temp.append(2, '\0');

  while (temp.length() > 2)
  // From the next three input bytes,
  // construct four encoded output bytes
  {
    result.append(1, char(temp[0] >> 2));
    result.append(1, char(((temp[0] << 4) & 060) | ((temp[1] >> 4) & 017) ) );
    result.append(1, char(((temp[1] << 2) & 074) | ((temp[2] >> 6) & 003) ) );
    result.append(1, char(temp[2] & 077) );

    // Remove the 3 bytes we just used
    temp.erase(0, 3);
  }

  // <result> now contains indices to <substr> so map to actual characters
  for (string::size_type i = 0; i != result.length(); ++i)
    result[i] = substr[(int)result[i]];

  switch (temp.length())
  {
    case 2 :
      break;
    case 1 :
      result.replace(result.length() - 1, 1, "=");
      break;
    default :
      result.replace(result.length() - 2, 2, "==");
      break;
  }
  
  return result;
}

// end of CHTTPUtils.C ***************************************************

