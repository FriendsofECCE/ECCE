// CHTTPDefs.C
// Created: 05/08/2000
// Version: 05/23/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

#include "dsm/CHTTPDefs.H"

using std::string;

// constants ***************************************************************

const unsigned short HTTPDefPort        = 80;
const string         HTTPDefContentType("application/x-www-form-urlencoded");
const string         HTTPDefFileTypes("text/html, text/xml, "
                                      "image/gif, image/x-xbitmap, "
                                      "image/jpeg, image/pjpeg, "
                                      "image/png, */*");
const string         HTTPDefUserAgent("Mozilla/5.25 Netscape/5.0 "
                                      "(Windows; U; NT4.0)");
const string         HTTPSpecialChars(" =?&;:/#");
const string         CRLF("\r\n");

// end of CHTTPDefs.C ****************************************************

