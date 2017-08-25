// CHTTPDefs.H
// Created: 05/08/2000
// Version: 05/23/2000
//
// Description:
//
//   General use HTTP constants and typedefs
//
// Modifications:

#if !defined( __CHTTPDEFS_H )
#define __CHTTPDEFS_H   // prevent multiple includes

// include files ***********************************************************

#include <string>

// constants ***************************************************************

extern const unsigned short HTTPDefPort;  // 80
#if defined( OLD_CPP_HEADERS )
extern const      string    HTTPDefContentType;
extern const      string    HTTPDefFileTypes;
extern const      string    HTTPDefUserAgent;
extern const      string    HTTPSpecialChars;
extern const      string    CRLF;
#else
extern const std::string    HTTPDefContentType;
extern const std::string    HTTPDefFileTypes;
extern const std::string    HTTPDefUserAgent;
extern const std::string    HTTPSpecialChars;
extern const std::string    CRLF;
#endif

// typedefs ****************************************************************

// callback state and type for transfer of message bodies

enum http_body_tx_state
{
  HTTP_BODY_TX_INIT,  // Start a new transfer
  HTTP_BODY_TX_CONT,  // Continuing a transfer
  HTTP_BODY_TX_DONE,  // Transfer complete
  HTTP_BODY_TX_ERROR  // Transfer error
};

typedef int (*http_body_tx_callback)(
  http_body_tx_state state,       // transfer state
  void *             buffer,      // transfer buffer
  unsigned           bsz,         // transfer buffer capacity
  void *             clientData); // opaque data for client
// Pre: Assigned(state), Assigned(buffer),
//      Assigned(bsz), Assigned(clientData)
// Modifies: buffer
// Post: If state is HTTP_BODY_TX_CONT, read data into <buffer> and
//       return the number of bytes to transfer next.
//       Return -1 on error, 0 on completion of transferring the body.
//       The return value is ignored for chunked transfer encoding.
//
//       A state of HTTP_BODY_TX_INIT indicates that transfer of the body
//       should begin on the next call (state is HTTP_BODY_TX_CONT).
//       Return the number of bytes to transfer next.
//       The return value is ignored for chunked transfer encoding.
//
//       A state of HTTP_BODY_TX_DONE indicates that transfer of the body
//       has completed successfully.
//       The return value is not used.
//
//       A state of HTTP_BODY_TX_ERROR indicates that there was an error
//       transfering the body.
//       The error message is placed in <buffer> and it's length in <bsz>
//       The return value is not used.

typedef bool (*http_name_password_callback)(
#if defined( OLD_CPP_HEADERS )
  string&      userName,
  string&      password,
#else
  std::string& userName,
  std::string& password,
#endif
  void *       clientData);   // opaque data for client
// Pre: Assigned(userName), Assigned(password), Assigned(clientData)
// Modifies: userName, password
// Post: Get the user's user name and password, copy them into
//       <userName, password> and return true.
//       Return false on failure.

#endif  // __CHTTPDEFS_H

// end of CHTTPDefs.H ******************************************************

