// CHTTPResponse.H
// Created: 05/08/2000
// Version: 05/08/2000
//
// Description:
//
// Modifications:

#if !defined( __CHTTPRESPONSE_H )
#define __CHTTPRESPONSE_H   // prevent multiple includes

// include files ***********************************************************

#include "dsm/CHTTPVersion.H"
#include <string>

// class: CHTTPResponse ****************************************************

class CHTTPResponse
{
public :

  typedef std::string string_type;

  enum status_type
  {
    CONTINUE = 100,
    SWITCHING_PROTOCOLS,
    OK = 200,
    CREATED,
    ACCEPTED,
    NON_AUTHORITATIVE_INFORMATION,
    NO_CONTENT,
    RESET_CONTENT,
    PARTIAL_CONTENT,
    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY,
    FOUND,
    SEE_OTHER,
    NOT_MODIFIED,
    USE_PROXY,
    TEMPORARY_REDIRECT,
    BAD_REQUEST = 400,
    UNAUTHORIZED,
    PAYMENT_REQUIRED,
    FORBIDDEN,
    NOT_FOUND,
    METHOD_NOT_ALLOWED,
    NOT_ACCEPTABLE,
    PROXY_AUTHENTICATION_REQUIRED,
    REQUEST_TIMEOUT,
    CONFLICT,
    GONE,
    LENGTH_REQUIRED,
    PRECONDITION_FAILED,
    REQUEST_ENTITY_TOO_LARGE,
    REQUEST_URI_TOO_LARGE,
    UNSUPPORTED_MEDIA_TYPE,
    REQUESTED_RANGE_NOT_SATISFIABLE,
    EXPECTATION_FAILED,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED,
    BAD_GATEWAY,
    SERVICE_UNAVAILABLE,
    GATEWAY_TIMEOUT,
    VERSION_NOT_SUPPORTED
  };
  struct ParseError {}; // Error parsing header

  // Constructors
  CHTTPResponse(void);
  CHTTPResponse(const string_type&) throw(CHTTPResponse::ParseError);
  CHTTPResponse(const CHTTPResponse& other)
    : HTTPVersion_(other.HTTPVersion_), reasonPhrase_(other.reasonPhrase_),
      status_(other.status_) {}

  // Destructor
  virtual ~CHTTPResponse(void) {}

  // Assignment operator
  CHTTPResponse& operator = (const CHTTPResponse& other)
  {
    if (&other != this)
    {
      HTTPVersion_  = other.HTTPVersion_;
      reasonPhrase_ = other.reasonPhrase_;
      status_       = other.status_;
    }
    return *this;
  }

  // Get/Set the HTTP version (e.g. 1.1)
  virtual const CHTTPVersion& HTTPVersion(void) const { return HTTPVersion_; }
  virtual void                HTTPVersion(const CHTTPVersion& v)
  { HTTPVersion_ = v; }

  // Return true if <s> is a valid status code, false otherwise
  virtual bool isValidStatus(int s) const;

  // Parse a result string into self and return true.
  // Return false on failure.
  // NOTE: This doesn't check the reason phrase in the string.
  virtual bool parse(const string_type&);

  // Get the reason phrase for status()
  virtual string_type reasonPhrase(void) const { return reasonPhrase_; }
  virtual string_type reasonPhrase(int) const;

  // Return the status_type corresponding to <s> or -1 if not found
  // NOTE: The compare is on the reason phrase, for example:
  //       "Continue" instead of "100 Continue"
  virtual int reasonPhraseToStatus(
    const string_type& s,
    bool               ignoreCase = true) const;

  // Get/Set the status code
  // NOTE: The set method returns true if the status code was validated
  //       and set.
  virtual int  status(void) const { return status_; }
  virtual bool status(int);

  // Return the string version of <status> or "" if not found
  virtual string_type statusCodeToString(void) const
  { return statusCodeToString(status()); }
  virtual string_type statusCodeToString(int status) const;

  // Return self as a string in proper HTTP format:
  // HTTP-Version Status-Code Reason-Phrase CRLF
  virtual string_type toString(void) const;

protected :

  CHTTPVersion  HTTPVersion_;   // HTTP version
  string_type   reasonPhrase_;  // description of status_
  int           status_;        // status code
};

#endif  // __CHTTPRESPONSE_H

// end of CHTTPResponse.H **************************************************

