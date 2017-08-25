#include <stdlib.h> // atoi

#include "dsm/EcceDAVStatus.H"

/**
 * Determines if the http code is success or not.
 * I excluded 1xx messages because I don't think our code handles
 * them.  Basically 200-299 means ok.
 */
bool EcceDAVStatus::isOk(int key)
{
   return (key >= 200 && key <= 299);
}

string EcceDAVStatus::edsiMessage(int key)
{
  string ret;
  switch (key) {
    case  CONTINUE:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case SWITCHING_PROTOCOLS:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  OK:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  CREATED:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  ACCEPTED:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case NON_AUTHORITATIVE_INFORMATION:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case NO_CONTENT:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case RESET_CONTENT:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  PARTIAL_CONTENT:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case MULTISTATUS:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  MULTIPLE_CHOICES:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  MOVED_PERMANENTLY:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  MOVED_TEMPORARILY:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  SEE_OTHER:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  NOT_MODIFIED:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case  USE_PROXY:
      ret = "SERVER_REQUEST_ACKNOWLEDGEMENT";
      break;
    case BAD_REQUEST:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case UNAUTHORIZED:
      ret = "NOT_AUTHENTICATION";
      break;
    case PAYMENT_REQUIRED:
      ret = "SERVICE_DENIAL";
      break;
    case FORBIDDEN:
      ret = "NOT_PRIVLEDGES";
      break;
    case NOT_FOUND:
      ret = "NOT_EXIST";
      break;
    case METHOD_NOT_ALLOWED:
      ret = "NOT_PRIVLEDGES";
      break;
    case NOT_ACCEPTABLE:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case PROXY_AUTHENTICATION_REQUIRED:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case REQUEST_TIME_OUT:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case CONFLICT:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case GONE:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case LENGTH_REQUIRED:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case PRECONDITION_FAILED:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case REQUEST_ENTITY_TOO_LARGE:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case REQUEST_URI_TOO_LARGE:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case UNSUPPORTED_MEDIA_TYPE:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case UNPROCESSABLE_ENTRY:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case LOCKED:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case FAILED_DEPENDENCY:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case INTERNAL_SERVER_ERROR:
      ret = "SERVER_ERROR";
      break;
    case NOT_IMPLEMENTED_ON_SERVER:
      ret = "NOT_IMPLEMENTED";
      break;
    case BAD_GATEWAY:
      ret = "SERVER_ERROR";
      break;
    case SERVICE_UNAVAILABLE:
      ret = "SERVER_ERROR";
      break;
    case GATEWAY_TIMEOUT:
      ret = "SERVER_ERROR";
      break;
    case HTTP_VERSION_NOT_SUPPORT:
      ret = "NOT_IMPLEMENTED";
      break;
    case INSUFFICIENT_STORAGE:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case CANCEL:
      ret = "CANCELED";
      break;
    case NOT_IMPLEMENTED:
      ret = "NOT_IMPLEMENTED";
      break;
    case UNABLE_TO_CONNECT:
      ret = "SERVER_NOT_FOUND";
      break;
    case UNABLE_TO_SEND_REQUEST:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case UNABLE_TO_GET_RESPONSE:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case TOO_MANY_AUTH_TRIES:
      ret = "TOO_MANY_RETRIES";
      break;
    case UNSUPPORTED_AUTH_SCHEME:
      ret = "NOT_IMPLEMENTED";
      break;
    case UNABLE_TO_COMPLETE_REQUEST:
      ret = "UNABLE_TO_COMPLETE_REQUEST";
      break;
    case INTERRUPTED:
      ret = "INTERRUPTED";
      break;
    default:
      ret = "UNKNOWN";
      break;
  }
  return ret;
}

string EcceDAVStatus::edsiMessage(string& key)
{
  return edsiMessage(atoi(key.c_str()));
}

string EcceDAVStatus::text(int key) 
{
  string ret;

  switch (key) {

    case CONTINUE:
      ret = "  ECCE can continue.";
      break;
    case  SWITCHING_PROTOCOLS:
      ret = "  Request to change protocols has been accepted.";
      break;
    case  OK:
      ret = "  Request succeeded.";
      break;
    case  CREATED:
      ret = "  Created successfully.";
      break;
    case  ACCEPTED:
      ret = "  Request accepted, but was not completed.";
      break;
    case  NON_AUTHORITATIVE_INFORMATION:
      ret = "  Meta information presented by the ECCE did not originate from the server.";
      break;
    case  NO_CONTENT:
      ret = "  Request succeeded, but there was information to return.";
      break;
    case  RESET_CONTENT:
      ret = "  ECCE should refresh data.";
      break;
    case  PARTIAL_CONTENT:
      ret = "  Request for partial resource extraction succeeded.";
      break;
    case  MULTISTATUS:
      ret = "  Multi-resource request succeeded.";
      break;
    case  MULTIPLE_CHOICES:
      ret = "  Requested resource corresponds to a set of representations .";
      break;
    case  MOVED_PERMANENTLY:
      ret = "  Requested succeeded.";
      break;
    case  MOVED_TEMPORARILY:
      ret = "  Resource has been temporarily moved to another location.";
      break;
    case  SEE_OTHER:
      ret = "  The response of the request can be found in another URI.";
      break;
    case  NOT_MODIFIED:
      ret = "  Resource was available, and not modified.";
      break;
    case  USE_PROXY:
      ret = "  The requested resource must be accessed via a proxy.";
      break;

    case BAD_REQUEST:
      ret = "  ECCE sent a request to the server that was syntactically incorrect .";
      break;
    case UNAUTHORIZED:
      ret = "  HTTP authentication required.";
      break;
    case  PAYMENT_REQUIRED:
      ret = "  Services are denied until payment is filled.";
      break;

    case FORBIDDEN:
      ret = "  Server understood request but refused to fulfill it.";
      break;

    case NOT_FOUND:
      ret = "  Resource was not found.";
      break;
    case METHOD_NOT_ALLOWED:
      ret = "  Request type is not allowed on this resource.";
      break;
    case NOT_ACCEPTABLE:
      ret = "  Request criteria not accepted by server.";
      break;
    case PROXY_AUTHENTICATION_REQUIRED:
      ret = "  ECCE must first authenticate itself with a proxy."; 
      break;
    case REQUEST_TIME_OUT:
      ret = "  ECCE did not produce request within the time that the server was prepared to wait.";
      break;
    case CONFLICT:
      ret = "  A conflict with the current state of resource caused failure)";
      break;
    case GONE:
      ret = "  Resource is no longer available at the server.";
      break;
    case LENGTH_REQUIRED:
      ret = "  ECCE request incomplete.";
      break;
    case PRECONDITION_FAILED:
      ret = "  ECCE cannot overwrite existing resource.";
      break;
    case REQUEST_ENTITY_TOO_LARGE:
      ret = "  Server was unable or unwilling to process request.";
      break;
    case REQUEST_URI_TOO_LARGE:
      ret = "  Server was unable or unwilling to process lengthy request URI.";
      break;
    case UNSUPPORTED_MEDIA_TYPE:
      ret = "  Server refused request because the request format is not supported.";
      break;
    case  UNPROCESSABLE_ENTRY:
      ret = "  Server was unable to decifer format of ECCE request.";
      break;
    case  LOCKED:
      ret = "  Resource is locked, so the request is rejected..";
      break;
    case  FAILED_DEPENDENCY:
      ret = "  Server was unable to fulfill request because of dependencies.";
      break;

    case INTERNAL_SERVER_ERROR:
      ret = "  An internal server error occurred.";
      break;
    case NOT_IMPLEMENTED_ON_SERVER:
      ret = "  Server does not support the functionality needed to fulfill request.";
      break;
    case BAD_GATEWAY:
      ret = "  Server received an invalid response from server it consulted when acting as a proxy or gateway.";
      break;
    case SERVICE_UNAVAILABLE:
      ret = "  Server is temporarily overloaded and is unable to fulfill request.";
      break;
    case GATEWAY_TIMEOUT:
      ret = "  Server did not receive a timely response from upstream server while acting as a gateway.";
      break;
    case HTTP_VERSION_NOT_SUPPORT:
      ret = "  Server does not support HTTP version sent in request.";
      break;
    case INSUFFICIENT_STORAGE:
      ret = "  The resource did not have sufficient space to record the state of the resource after the request was completed.";
      break;
    case CANCEL:
      ret = "  Request canceled by user.";
      break;
    case NOT_IMPLEMENTED:
      ret = "  Not implemented in Ecce.";
      break;
    case UNABLE_TO_CONNECT:
      ret = "  ECCE unable to reach server.";
      break;
    case TOO_MANY_AUTH_TRIES:
      ret = "  Too many failed authentications.";
      break;
    case UNSUPPORTED_AUTH_SCHEME:
      ret = "  Unsupported Authentication Scheme.";
      break;
    case UNABLE_TO_COMPLETE_REQUEST:
      ret = "  Unable to send or receive data to complete request.";
      break;
    case INTERRUPTED:
      ret = "  Request interrupted by user.";
      break;
    default:
      ret = "  ECCE received status message code but is unknown to Ecce.";
      break;
  }
  return ret;
}
