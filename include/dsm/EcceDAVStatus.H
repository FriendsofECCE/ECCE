#ifndef ECCEDAVSTATUS_H
#define ECCEDAVSTATUS_H

#include <string>

using std::string;

/*
 * Small class to encapsulate http, dav, and Ecce status's for performing
 * http exchanges.
 */

class EcceDAVStatus {

public:

  enum _status { 
    CONTINUE = 100,
    SWITCHING_PROTOCOLS,

    // Success messages
    OK = 200,
    CREATED,
    ACCEPTED,
    NON_AUTHORITATIVE_INFORMATION,
    NO_CONTENT,
    RESET_CONTENT,
    PARTIAL_CONTENT,
    MULTISTATUS,


    MULTIPLE_CHOICES = 300,
    MOVED_PERMANENTLY,
    MOVED_TEMPORARILY,
    SEE_OTHER,
    NOT_MODIFIED,
    USE_PROXY,

    // http client-caused messages
    BAD_REQUEST = 400,
    UNAUTHORIZED,
    PAYMENT_REQUIRED,
    FORBIDDEN,
    NOT_FOUND,
    METHOD_NOT_ALLOWED,
    NOT_ACCEPTABLE,
    PROXY_AUTHENTICATION_REQUIRED,
    REQUEST_TIME_OUT,
    CONFLICT,
    GONE,
    LENGTH_REQUIRED,
    PRECONDITION_FAILED,
    REQUEST_ENTITY_TOO_LARGE,
    REQUEST_URI_TOO_LARGE,
    UNSUPPORTED_MEDIA_TYPE,
    UNPROCESSABLE_ENTRY = 422,
    LOCKED,
    FAILED_DEPENDENCY,

    // http server messages
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED_ON_SERVER,
    BAD_GATEWAY,
    SERVICE_UNAVAILABLE,
    GATEWAY_TIMEOUT,
    HTTP_VERSION_NOT_SUPPORT,  
    INSUFFICIENT_STORAGE = 507,

    // Additional Ecce Messages
    UNKNOWN = 600,
    NOT_IMPLEMENTED,
    UNABLE_TO_CONNECT,
    CANCEL,
    TOO_MANY_AUTH_TRIES,
    UNSUPPORTED_AUTH_SCHEME,
    UNABLE_TO_COMPLETE_REQUEST,
    UNABLE_TO_SEND_REQUEST,
    UNABLE_TO_GET_RESPONSE,
    INTERRUPTED
  };

  static string text(int key);
  static string edsiMessage(int key);
  static string edsiMessage(string& key);
  static bool   isOk(int key);

};

#endif
