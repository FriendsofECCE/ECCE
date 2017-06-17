// CIPCHTTP.H
// Created: 06/02/1998
// Version: 02/16/2000
//
// Description: 
//
//   HTTP constants, etc.
//
// Modifications: 
//

#if !defined( __CIPCHTTP_H )
#define __CIPCHTTP_H      // prevent multiple includes

// include files ***********************************************************

#include <string>

// constants ***************************************************************

extern const unsigned short HTTPDefPort;
extern const std::string    HTTPSpecialChars;

enum HTTPResult       // HTTP return status codes
{
  HTTPContinue = 100,
  HTTPSwitchingProtocols,
  HTTPOK = 200,
  HTTPCreated,
  HTTPAccepted,
  HTTPNonAuthoritativeInformation,
  HTTPNoContent,
  HTTPResetContent,
  HTTPPartialContent,
  HTTPMultipleChoices = 300,
  HTTPMovedPermanently,
  HTTPFound,
  HTTPSeeOther,
  HTTPNotModified,
  HTTPUseProxy,
  HTTPTemporaryRedirect,
  HTTPBadRequest = 400,
  HTTPUnauthorized,
  HTTPPaymentRequired,
  HTTPForbidden,
  HTTPNotFound,
  HTTPMethodNotAllowed,
  HTTPNotAcceptable,
  HTTPProxyAuthenticationRequired,
  HTTPRequestTimeout,
  HTTPConflict,
  HTTPGone,
  HTTPLengthRequired,
  HTTPPreconditionFailed,
  HTTPRequestEntityTooLarge,
  HTTPRequestURITooLarge,
  HTTPUnsupportedMediaType,
  HTTPRequestedRangeNotSatisfiable,
  HTTPExpectationFailed,
  HTTPInternalServerError = 500,
  HTTPNotImplemented,
  HTTPBadGateway,
  HTTPServiceUnavailable,
  HTTPGatewayTimeout,
  HTTPHTTPVersionNotSupported
};

// htconv_special **********************************************************

extern std::string::size_type htconv_special(std::string& s);

// Pre: Assigned(s)
// Modifies:
// Post: Convert special (reserved) HTTP characters in <s> to "%NN" where
//       <NN> is the character's hexidecimal value.
//       Return the number of characters converted.

// htis_special ************************************************************

extern bool htis_special(char c);
// Pre: Assigned(c)
// Modifies:
// Post: Return true if <c> is a special (reserved) HTTP character
//       Return false otherwise.

// htuu_encode *************************************************************

extern std::string htuu_encode(const std::string& s);
// Pre: Assigned(s)
// Modifies:
// Post: HTTP-uu_encode - logic taken from 'webget' which was taken from
//       'Mosaic for X' code by Mark Riordan and Ari Luotonen

#endif  // __CIPCHTTP_H

// end of CIPCHTTP.H *******************************************************

