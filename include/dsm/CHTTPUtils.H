// CHTTPUtils.H
// Created: 05/23/2000
// Version: 05/23/2000
//
// Description:
//
//   HTTP utility functions
//
// Modifications:

#if !defined( __CHTTPUTILS_H )
#define __CHTTPUTILS_H    // prevent multiple includes

// include files ***********************************************************

#include <string>
#include <sstream>

// http_conv_special *******************************************************

extern std::string::size_type http_conv_special(std::string& s);
// Pre: Assigned(s)
// Modifies:
// Post: Convert special (reserved) HTTP characters in <s> to "%NN" where
//       <NN> is the character's hexidecimal value.
//       Return the number of characters converted.

// http_is_special *********************************************************

extern bool http_is_special(char c);
// Pre: Assigned(c)
// Modifies:
// Post: Return true if <c> is a special (reserved) HTTP character
//       Return false otherwise.

// parse_uri ***************************************************************

extern bool parse_uri(
  const std::string& uri,
  std::string&       hort,
  std::string&       path,
  unsigned short&    port);
// Pre: Assigned(uri), Assigned(host), Assigned(path), Assigned(port)
// Modifies: host, path, port
// Post: Parse the host, port, and path from the given URI and return true.
//       Return false on error

// to_string ***************************************************************

template <class T>
inline std::string to_string(const T& obj)
// Pre: Assigned(obj)
// Modifies:
// Post: Convert <obj> to a string and return it
{
  std::ostringstream  ostr;
  std::string         result;
  ostr << obj;
  result = ostr.str();
  return result;
}

// to_lower ****************************************************************

extern std::string to_lower(const std::string& s);
// Pre: Assigned(s)
// Modifies:
// Post: Return <s> converted to lower case

// to_upper ****************************************************************

extern std::string to_upper(const std::string& s);

// Pre: Assigned(s)
// Modifies:
// Post: Return <s> converted to upper case

// http_uu_encode **********************************************************

extern std::string http_uu_encode(const std::string& s);

// Pre: Assigned(s)
// Modifies:
// Post: HTTP-uu_encode - logic taken from 'webget' which was taken from
//       'Mosaic for X' code by Mark Riordan and Ari Luotonen

#endif  // __CHTTPUTILS_H

// end of CHTTPUtils.H *****************************************************

