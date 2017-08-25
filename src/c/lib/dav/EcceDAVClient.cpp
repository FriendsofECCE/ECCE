#include <stdio.h> // sprintf

#include <iostream>
  using std::cout;
  using std::cerr;
  using std::ends;
  using std::endl;
  using std::ios;

#include <strstream>
  using std::ostrstream;
  using std::istrstream;
  using std::streambuf;

#include <stdlib.h>

// Control whether or not to use persistent connections.  My tests have
// shown these to be slower than re-establishing them each time.
//#define persistentconnections

#include "util/Ecce.H" 
#include "util/ProgressEvent.H" 
#include "util/UrlEncoder.H" 
#include "util/AuthEvent.H" 

#include "dsm/EcceDAVClient.H" 
#include "dsm/DavDebug.H" 
#include "dsm/EcceDAVStatus.H" 


// The following macros control logging output.  Bodies has a special
// once since it can get absurdly large for moving big documents.
static bool gdebug = getenv("ECCE_DAV_DEBUG") != 0;
//static bool GDEBUGBODYTOO = getenv("ECCE_DAV_DEBUG_BODY");

// If you leave this, an in memory buffer of the data is created so
// large files will eat all your memory.
static bool gdebugbodytoo = false;

//
// Statics
// p_gAuthStr provides a way to reuse the last authentication rather than
// having to do fancy searching of the cache
//
string EcceDAVClient::p_gAuthStr;

// Callback data structure for getBodyCallback()
//
// The data member is where data chunks are deposited while they are
// read.  By default its an in-memory stream.  However, some methods
// replace it with a stream of their own for moving data straight into
// files.  A little ugly but requires minimal changes.
struct GetBodyCBData
{
  typedef unsigned long size_type;
  bool            m_chunked;        // chunked transfer encoding?
  size_type       m_contentLength;  // known content length?
  int             m_bytesLeft;
  int             m_totalBytes;
  ostream         *m_data;         // oststream to hold body
  // Constructor
  GetBodyCBData(void) : m_chunked(false), m_contentLength(0), m_bytesLeft(0),
                        m_totalBytes(0), m_data(0)
      { m_data = new ostrstream; }
    // Set to use chunked transfer encoding
    void setChunked(void) {
        m_chunked = true; 
        m_contentLength = 0;
    }
    // Set to use a known content length
    void setContentLength(size_type n) {
      m_chunked = false;
      m_contentLength = n;
    }
    // Destructor
    ~GetBodyCBData(void) {
       if (m_data != 0) delete m_data;
    }
};






/**
 * Constructor.
 */
EcceDAVClient::EcceDAVClient(void) 
{ 
   p_lastAuthEvent = 0;
   p_authListener = 0;
   p_progressListener = 0;
   p_progressEvent = new ProgressEvent;
}


/**
 * Constructor.
 */
EcceDAVClient::EcceDAVClient(const EcceURL uri) 
{ 
   url(uri);
   p_lastAuthEvent = 0;
   p_authListener = 0;
   p_progressListener = 0;
   p_progressEvent = new ProgressEvent;
}


/**
 * Destructor.
 */
EcceDAVClient::~EcceDAVClient(void) 
{ 
   if (!p_client.deadConnection()) {
      p_client.disconnect();
   }
   if (p_lastAuthEvent) delete p_lastAuthEvent;
   delete p_progressEvent;

}


/**
 *  Return the current url context.
 */
EcceURL EcceDAVClient::url() const 
{
  return p_url; 
}



/**
 * Set the current context to the specified url.
 * If changing servers, clear cached password.
 */
void EcceDAVClient::url(const string& url) 
{
   if (p_url.toString().size() != 0) {
      EcceURL newurl(url);
      if ((p_url.getHost() != newurl.getHost()) ||
            (p_url.getPort() != newurl.getPort())) {
         p_gAuthStr = "";
      }
   }
   p_url = url;
}


/**
 * Set the current context to the specified url.
 * If changing servers, clear cached password.
 */
void EcceDAVClient::url(const EcceURL& url)   
{
   if (p_url.toString().size() != 0) {
      if ((p_url.getHost() != url.getHost()) ||
            (p_url.getPort() != url.getPort())) {
         p_gAuthStr = "";
      }
   }
   p_url = url; 
}


/**
 * Adds listener whose responsiblity it is to provide authorization
 * information as needed.  
 * Currently only one listener is supported at a time.
 */
void EcceDAVClient::addListener(AuthEventListener *l)
{ 
  p_authListener = l;
}


////////////////////////////////////////////////////////////////////////////////
// removeListener()
//   Remove listener if it has been added.  Equality based on object identity.
////////////////////////////////////////////////////////////////////////////////
void EcceDAVClient::removeListener(AuthEventListener *l)
{
  if (p_authListener == l) p_authListener = 0;
}

void EcceDAVClient::addProgressEventListener(ProgressEventListener *l)
{ 
  p_progressListener = l;
}
void EcceDAVClient::removeProgressEventListener(ProgressEventListener *l)
{
  if (p_progressListener == l) p_progressListener = 0;
}


////////////////////////////////////////////////////////////////////////////////
// sendRequest()
//   Send header and request.  Log output.
////////////////////////////////////////////////////////////////////////////////
bool EcceDAVClient::sendRequest(const CDAVRequest& request, const CDAVHeader& header)
{
  bool ret;
  DavDebug debug;
  string dump;
  if (gdebug) {
    dump = request.toString();
    dump += header.toString();
    debug.startTimer();
  }

  ret = p_client.sendRequest(request, header);

  if (gdebug) {
    debug.dump(dump.c_str(),"Request Header");
    debug.endTimer("Elapsed time for SendRequest (ms): ");
  }
  return ret;
}

void EcceDAVClient::notifyProgress()
{
  if (p_progressListener) {
    p_progressListener->progressNotice(*p_progressEvent);
  }
}
////////////////////////////////////////////////////////////////////////////////
// getResponse()
//   gets server response and adds to debug log if enabled.
////////////////////////////////////////////////////////////////////////////////
bool EcceDAVClient::getResponse(CDAVResponse& response, 
    CDAVHeader& header, int sec, int msec)
{
  bool ret = true;
  DavDebug debug;
  if (gdebug) {
    debug.startTimer();
  }

  p_progressEvent->m_interval = sec;
  p_progressEvent->m_msg = "Waiting for response...";

  do {
    p_progressEvent->m_tick++;
    notifyProgress();
    ret = p_client.getResponse(response, header, sec, msec);
  } while (!p_client.deadConnection() && !ret && !p_progressEvent->m_stop);

  if (gdebug) {
    string dump;
    if (ret == false) {
      dump += "**WARNING** http layer getResponse() returned false\n";
    }
    dump += response.toString();
    dump += header.toString();
    debug.dump(dump.c_str(),"Response Header");
    debug.endTimer("Elapsed time for GetResponse (ms): ");
  }

  // Discovered that this was not set anywhere so set it here
  if (ret)  p_header = header.toString();

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
// getHeaderText()
//   Returns the header portion of the DAV response.
////////////////////////////////////////////////////////////////////////////////
string EcceDAVClient::getResultsText()
{
  return p_results;
}

////////////////////////////////////////////////////////////////////////////////
// getHeaderText()
//   Returns the header portion of the DAV response.
////////////////////////////////////////////////////////////////////////////////
string EcceDAVClient::getHeaderText()
{
  return p_header;
}

////////////////////////////////////////////////////////////////////////////////
// getStatusText()
//   Returns the status message portion of the DAV response.
//   The integer returned from the DAV operation is part of this status.
////////////////////////////////////////////////////////////////////////////////
string EcceDAVClient::getStatusText()
{
  return p_status;
}

////////////////////////////////////////////////////////////////////////////////
// processStatus()
//   pulls the server code out of a response.  May in the future also
//   interpret the response and add our own codes.
//
//   Expecting to find strings like:
//       HTTP/1.1 401 Unauthorized^M
//   But also have the following:
//       HTTP/1.1 Multi-Status ^M
//   Eric says the above used to include #207 so we'll add it in.
////////////////////////////////////////////////////////////////////////////////
int EcceDAVClient::processStatus(const string& msg)
{
  int ret = EcceDAVStatus::UNKNOWN;
  int firstSpace = msg.find_first_of(' ');

  // Add assert in case we start passing in unprocessable strings
  // Can't use this until header file is RW-free
  //EE_ASSERT((firstSpace != string::npos),EE_WARNING,msg);

  firstSpace++;
  string tmp = msg.substr(firstSpace);
  if (tmp.find("Multi-Status") == 0) {
    ret = 207;
  } else {
    ret = atoi(tmp.c_str());
  }
  return ret;
}


int EcceDAVClient::options(void) 
{
  return execute(CHTTPRequest::OPTIONS);
}

int EcceDAVClient::trace(void) 
{
  return execute(CHTTPRequest::TRACE);
}

int EcceDAVClient::head(const bool& newUser) 
{
  return execute(CHTTPRequest::HEAD, 0, 0, false, 0, 0, 0, newUser);
}

int EcceDAVClient::get_document(void) 
{
  return execute(CHTTPRequest::GET);
}

int EcceDAVClient::get_document(ostream& dest) 
{
  return execute(CHTTPRequest::GET,(istream*)0, &dest);
}


int EcceDAVClient::get_document_subset(int start, int length, ostream& dest)
{
  execute(CHTTPRequest::HEAD);
  return execute(CHTTPRequest::GET,(istream*)0, &dest, true, start, length);
}


int EcceDAVClient::create_collection() 
{
  return execute(CDAVRequest::MKCOL);
}

int EcceDAVClient::delete_document() 
{
  return execute(CDAVRequest::DELETE);
}

///////////////////////////////////////////////////////////////////////////////
// put_document
//  This version of put_document is designed to handle both memory and
//  file streams including large files.  We do a HEAD first so that we
//  have a small unit of transfer in which to find out if we need
//  authentication.  If we do the PUT right away on a large document, the
//  entire thing gets transfered before we find out that we have to
//  supply authentication info an try again.
//
//  TODO Given time it would be useful to check locks and permissions
//  also prior to trying to move a big file.
///////////////////////////////////////////////////////////////////////////////
int EcceDAVClient::put_document(istream *putStream) 
{
  execute(CHTTPRequest::HEAD);
  return execute(CHTTPRequest::PUT,putStream);
}

int EcceDAVClient::put_document(const char* putStream) 
{
  int status = 0;
  execute(CHTTPRequest::HEAD);
  istrstream *data = NULL;
  if (putStream && strlen(putStream) > 0) {
     data = new istrstream(putStream);
     status =  execute(CHTTPRequest::PUT,data);
     delete data;
  }
  return status;
}

int EcceDAVClient::append_document(istream *putStream, int bytesToOverwrite)
{
  execute(CHTTPRequest::HEAD);
  return execute(CHTTPRequest::PUT,putStream, (ostream*)0,true,0,0,bytesToOverwrite);
}

int EcceDAVClient::append_document(const char* putStream, int bytesToOverwrite)
{
  int status = 0;
  execute(CHTTPRequest::HEAD);
  istrstream *data = NULL;
  if (putStream && strlen(putStream) > 0) {

     data = new istrstream(putStream);
     status = execute(CHTTPRequest::PUT,data, (ostream*)0,true,0,0,bytesToOverwrite);
     delete data;
  }
  return status;
}

//TODO
int EcceDAVClient::lock(const string& request) 
{
  p_header = p_results = "";
  p_status = EcceDAVStatus::text(EcceDAVStatus::NOT_IMPLEMENTED) + " LOCK";
  return EcceDAVStatus::NOT_IMPLEMENTED;
}

//TODO
int EcceDAVClient::unlock(const string& LockToken) 
{
  p_header = p_results = "";
  p_status = EcceDAVStatus::text(EcceDAVStatus::NOT_IMPLEMENTED) + " UNLOCK";
  return EcceDAVStatus::NOT_IMPLEMENTED;
}    

/**
 * Implements POST method.
 * Caller should construct URL in the normal syntax for POST request.
 * Example:
 * http://localhost:8080/Ecce/cgi-bin/script?key1=value1&key2=value2
 */
int EcceDAVClient::doPost(ostream& data) 
{

  EcceURL currurl = this->url();
  string paramstring = currurl.paramsAsString();
  istrstream params(paramstring.c_str());
  return execute(CDAVRequest::POST,&params, &data);
}

int EcceDAVClient::get_properties(const string& getRequest) 
{
  istrstream data(getRequest.c_str());
 return execute(CDAVRequest::PROPFIND,&data);
}


int EcceDAVClient::put_properties(const string& putRequest) 
{
  istrstream data(putRequest.c_str());
  return execute(CDAVRequest::PROPPATCH, &data);
}

int EcceDAVClient::copy(const EcceURL& TargetURL) 
{
  string tstr = TargetURL.toString();
  istrstream data(tstr.c_str());
  return execute(CDAVRequest::COPY, &data);
}

int EcceDAVClient::move(const EcceURL& TargetURL) 
{
  string tstr = TargetURL.toString();
  istrstream data(tstr.c_str());
  return execute(CDAVRequest::MOVE,&data);
}



int 
EcceDAVClient::execute
(
 int method, 
 istream* putdata,            // body to send over - can be null
 ostream* returndata,   // body that comes back - can be null
 bool content_range, 
 int start, 
 int length,
 int bytesToOverwrite,       // for appending and overwriting specified
                             // number of bytes from the end
 const bool& newUser
) 
{
  int ret = EcceDAVStatus::UNABLE_TO_COMPLETE_REQUEST;

  const int       maxTries   = 3; // how many attempts
  int             numTries   = 0; // attempt number
  string          proxy;

  // Reset these internal variables.
  p_status = p_header = p_results = "";

  EcceURL cururl = UrlEncoder::encode(this->url());

  string  host = cururl.getHost();
  unsigned short port = cururl.getPort();
  string path = cururl.getPath();

  bool done = false;

  // Clear this since we have a new operation
  // Could probably do before returning too
  if (p_lastAuthEvent) {
     delete p_lastAuthEvent;
     p_lastAuthEvent = 0;
  }

  // Initialize progress event
  p_progressEvent->m_tick = 1;
  p_progressEvent->m_msg = "";
  p_progressEvent->m_stop = false;

  // try up to maxTries times to send the request
  while (numTries < maxTries && !done && !p_progressEvent->m_stop)
  {
    numTries++;

    // Set up request header with authorization:
    CDAVRequest     request;
    CDAVResponse    davresponse;
    CDAVHeader      header;
    string          status;

    if (putdata) {
      putdata->clear();
      putdata->seekg(0);
    }

    // The request uri should not include parameters and should not include 
    // the server info unless a proxy is in use.
    string requesturi = path;
    if (proxy.length() > 0) {
      // TODO: handle proxy ports specifically
      port = 800; 
      requesturi = cururl.toString(false);
    }

    createRequest(method, request, header, requesturi, putdata);
    
    // If we have an existing authorization string start out by using it.
    if (p_gAuthStr.length()) {
      header.header(CDAVHeader::AUTHORIZATION, p_gAuthStr);
    }

    // Used for PUT requests that want to append to a document.
    // This is dependent on a successful HEAD request which sets
    // p_length = document "Content-Length"
    if ((method == CHTTPRequest::PUT) && (content_range)) {
      setPutContentRangeHeader(header, bytesToOverwrite);
    }

    if ((method == CHTTPRequest::GET) && (content_range)) {
      setGetContentRangeHeader(header, start, length);
    }



    // Try to connect to the server (return on error):
    // (We have to reconnect every time we send a request because
    // we are not using persistent connections.)
#ifdef persistentconnections
    if (p_client.deadConnection())
#else
    if (true) 
#endif
    {
      p_progressEvent->m_msg = "Connecting to " + host;
      notifyProgress();
      if (!p_client.connect(host, port, "")) {
        ret = EcceDAVStatus::UNABLE_TO_CONNECT;
        p_status = EcceDAVStatus::text(ret)  + host ;
        break;
      }
    }

    // Try to send the request:
    p_progressEvent->m_msg = "Sending request";
    p_progressEvent->m_tick++;
    notifyProgress();
    if (!sendRequest(request,header)) { // error sending, retry request
      ret = EcceDAVStatus::UNABLE_TO_COMPLETE_REQUEST;
      p_status = EcceDAVStatus::text(ret) + 
        request.toString() + header.toString(); 
      p_client.disconnect();
      continue;
    }

    // Now listen for a response...:

    // Note: putBody() only tries to send body if needed
    if (!putBody(p_client, method, header, putdata)) {
      ret = EcceDAVStatus::UNABLE_TO_COMPLETE_REQUEST;
      p_status = EcceDAVStatus::text(ret);
      p_client.disconnect();

    } else {
      // Use our header instance for the response headers.
      // We could use a separate instance for request/response headers
      // if we wanted to.
      header.clear();

      // If resonse fails, check to see if the server went down.
      // If server is still up, then wait until the transaction is completed.
#if 0000
      // Tried to drop the select timeout to 0 (turning it into a poll) to
      // fix an NFS problem, but it made no difference.  But, it can be done
      // if it turns out to be useful.  GDB 12/21/2009
      if (!getResponse(davresponse, header, 0)) {
#else
      if (!getResponse(davresponse, header, 1)) {
#endif
        ret = EcceDAVStatus::UNABLE_TO_GET_RESPONSE;
        p_status = EcceDAVStatus::text(ret)  + host ;
        p_client.disconnect();

      } else   {
        // Got a response
        // Check to see if authorization is required and request must
        // be re-sent:
        switch (checkAuth(davresponse, header, newUser))  {
          case AUTHOK:
            // Some hack for partial puts?
            if (CHTTPRequest::HEAD) {
              p_length = header.header(CHTTPHeader::CONTENT_LENGTH);
              if (p_length.empty())
                p_length = "0";
            }
            
            
            ret = processStatus(davresponse.toString());
            p_status = davresponse.toString();

            // Note: getBody() only tries to get body if needed
            if (!getBody(p_client, method, davresponse, header,returndata)) {
              ret = EcceDAVStatus::UNABLE_TO_COMPLETE_REQUEST;
              p_status = EcceDAVStatus::text(ret);
            } else {
              done = true;
            }
            notifyAuthAccepted();
#if 111111
            // GDB 4/26/02 Hack to support Apache2
            // GDB 12/5/02 Only need to retry once--check for trailing slash already
            if (ret == 301) {
              if (requesturi[requesturi.length()-1] != '/') {
                p_client.disconnect();
                done = false;
                numTries--;
                path = path + "/";
              }
            }
#endif
            break;
          case AUTHCANCEL:
            ret = EcceDAVStatus::CANCEL;
            p_status = EcceDAVStatus::text(ret);
            p_client.disconnect();
            done = true;
            break;
          case AUTHMAX:
            ret = EcceDAVStatus::TOO_MANY_AUTH_TRIES;
            p_status = EcceDAVStatus::text(ret);
            p_client.disconnect();
            done = true;
            break;
          case AUTHERROR:
            ret = EcceDAVStatus::UNSUPPORTED_AUTH_SCHEME;
            p_status = EcceDAVStatus::text(ret);
            p_client.disconnect();
            done = true;
            break;
          case AUTHRETRY:
            // the server will/may contain a body when 
            // authorization is required.
            p_client.flush();
            p_client.disconnect();
            numTries--;
            break;
          default:
            cerr << "Missing case statement for auth type" << endl;
            done = true;
            break;

        }
      }
    }
  }
  if (p_progressEvent->m_stop) {
    ret = EcceDAVStatus::INTERRUPTED;
    p_status = EcceDAVStatus::text(ret);
    p_client.disconnect();
  }

// Make sure to disconnect on all non-presistent execute calls.  This fixes
// a bug somewhere in BroadcastMessage/SessionInfo where it will create new
// connections without ever releasing old ones.  Eventually the calling
// process (eccejobstore most notably) will die because it runs out of file
// descriptors.
#ifndef persistentconnections
  if (!p_client.deadConnection())
    p_client.disconnect();
#endif

  // Why is this here - makes no sense
  // commented out by kls 12/9/05
  //p_progressEvent->m_stop = true;

  notifyProgress();

  return ret;
}


void EcceDAVClient::setPutContentRangeHeader(CHTTPHeader& header, int bytesToOverwrite) 
{
   // Is this being retrieved from previous call HEAD?
   // back up requested number of bytes
   int tcl = atoi(p_length.c_str()) - bytesToOverwrite;  
   int requestcl = atoi(header.header(CHTTPHeader::CONTENT_LENGTH).c_str());
   //
   //if content length = 0 then don't set content range, just do a normal PUT.
   // 
   if (tcl > 0) {
     char buf[20];
     sprintf(buf, "%d", tcl);
     string cr_string = "bytes ";
     cr_string += buf;
     cr_string += "-";
     sprintf(buf, "%d", tcl+requestcl - 1);
     cr_string += buf;
     cr_string += "/";
     sprintf(buf, "%d", tcl+requestcl);
     cr_string += buf;
     header.header(CHTTPHeader::CONTENT_RANGE, cr_string) ;
   }
// Example     header.header(CHTTPHeader::CONTENT_RANGE, "bytes 3-5/31") ;

  ; 
}

void EcceDAVClient::setGetContentRangeHeader(CHTTPHeader& header, int start, int length)
{
   // Is this being retrieved from previous call HEAD?
   //
   //if content length = 0 then don't set content range, just do a normal PUT.
   //
   char buf[20];
   sprintf(buf, "%d", start);
   string cr_string = "bytes=";
   cr_string += buf;
   cr_string += "-";
   sprintf(buf, "%d", length + start - 1);
   cr_string += buf;
   header.header(CHTTPHeader::RANGE, cr_string) ;
}


void EcceDAVClient::setDepthHeader(const string& depth) 
{
  p_depth = depth; 
}

string EcceDAVClient::getDepthHeader() const {
  return p_depth; 
}



//////////////////////////////////////////////////////////////////////////////
// canHaveRequestBody() 
//   returns true if the specified method can have a request body
//   according to the protocol specification.
// Pre: Assigned(method)
//////////////////////////////////////////////////////////////////////////////
bool EcceDAVClient::canHaveRequestBody(int method)
{
  bool ret = false;
  switch (method)
  {
    case CDAVRequest::PROPFIND :
    case CDAVRequest::PROPPATCH :
    case CHTTPRequest::GET :  // DAV says yes
    case CHTTPRequest::PUT :
    case CHTTPRequest::POST :
      ret = true;
      break;
    default :
      ret = false;
      break;
  }
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// checkResult
//   Checks to see if the request failed because it needs authorization.
//   If so gets it from cache or listener and sets the global auth
//   variable.
// Pre: Assigned(response), Assigned(header)
// Modifies: p_gAuthStr
// Post: Check response.
// Returns:
//   AUTHOK if the response doesn't indicate a auth is needed.
//   AUTHRETRY if response indicates auth is needed and new auth info can
//             be retrieved.
//   AUTHCANCEL if no new auth info can be found.
//////////////////////////////////////////////////////////////////////////////
EcceDAVClient::AuthStatus EcceDAVClient::checkAuth
( 
  const CDAVResponse& davresponse,
  const CDAVHeader&   header,
  const bool&         newUser
)
{
  AuthStatus ret = AUTHOK;

  CDAVServerInfo  info(header);

  // 401 comes back if you have no credentials OR in the case
  // of mod_dav, if you supplied them but they are not valid??
  // Servers like Slide return 403.  But in this case you can't
  // get the authentication type.  So these 2 forbidden statements
  // are a hack to work more seamlessly with SAM.
  // Added a hack so .htaccess files will not initiate a 
  // prompt for credentials.  This is caused by the forbidden
  // statements, w/o these this wouldn't be necessary.

  string fileName = p_url.getFilePathTail();
  if (fileName.rfind('/') == fileName.length()-1) {
    fileName = fileName.substr(0,fileName.rfind('/'));
  }
  if (davresponse.status() == CHTTPResponse::UNAUTHORIZED ||
      davresponse.status() == CHTTPResponse::FORBIDDEN)
  {
    string realm;
    if (info.authType(realm) == CHTTPServerInfo::AUTH_BASIC ||
       ( (davresponse.status() == CHTTPResponse::FORBIDDEN) &&
         (fileName != ".htaccess")) )
    {
      string  name, pw;
      try {
        if (namePasswordCallback(name, pw, realm, newUser)) {
          p_gAuthStr  = name + ":" + pw;
          p_gAuthStr  = http_uu_encode(p_gAuthStr);
          p_gAuthStr.insert(0, "Basic ");
          ret = AUTHRETRY;
        } else {
          ret = AUTHCANCEL;
        }
      } catch (RetryException&) {
        ret = AUTHMAX;
      }
    }
    else if (info.authType(realm) == CHTTPServerInfo::AUTH_DIGEST)
    {
      ret = AUTHERROR;
    }
    else
    {
      ret = AUTHERROR;
    }
  }
  return ret;
}

void EcceDAVClient::notifyAuthAccepted()
{
  if (p_authListener && p_lastAuthEvent) {
    p_authListener->authorizationAccepted(*p_lastAuthEvent);
  }
}

//////////////////////////////////////////////////////////////////////////////
// createRequest
// Pre: Assigned(method)  - HTTP/DAV method ID
//      Assigned(request) - request line
//      Assigned(header)  - headers
//      Assigned(uri)     - URI for method
//      Assigned(proxy)   - proxy server name (if using a proxy)
//      Assigned(targetdata)    - body or dest uri if needed
// Modifies: request, header
// Post: Set up the request line and headers for the specified method
//////////////////////////////////////////////////////////////////////////////
void EcceDAVClient::createRequest
(
  int           method,
  CDAVRequest&  request,
  CDAVHeader&   header,
  const string& uri,
  istream      *targetdata
)
{
  string          host,
                  path;
  unsigned short  port;
  int             contentLength = 0;

  parse_uri(url().toString(), host, path, port);

  // Using a request body?
  if (canHaveRequestBody(method) )
    contentLength = getStreamSize(targetdata);
  
  // Set up the request line
  request.method(method);

  request.URI(uri);

  // Set default headers
  // According to the spec the port should be included if its not the default port (80)
  if (port == 80) {
    header.header(CHTTPHeader::HOST, host);
  } else {
    char *buf  = new char[host.size() + 8];
    sprintf(buf,"%s:%d",host.c_str(),port);
    header.header(CHTTPHeader::HOST, buf);
    delete [] buf;
  }
//  header.header(CHTTPHeader::CONNECTION, "close");
  header.header(CHTTPHeader::USER_AGENT, HTTPDefUserAgent);


  // Set additional headers
  switch (method)
  {
    case CDAVRequest::COPY :
    case CDAVRequest::MOVE :
      {
        int size = getStreamSize(targetdata);
        char *buf = new char[size+1];
        // get gets size-1 characters
        targetdata->get(buf,size+1);
        EcceURL destinationUrl(buf);
        header.header(CDAVHeader::DESTINATION, UrlEncoder::encode(destinationUrl).toString().c_str());
        delete [] buf;
      }
      break;
    case CDAVRequest::PROPFIND :
      header.header(CHTTPHeader::CONTENT_LENGTH, "0");
      if (p_depth != "") {
        header.header(CDAVHeader::DEPTH, p_depth);
      }
      break;
    default :
      break;
  }
  //EGS
  header.header(CHTTPHeader::CONTENT_LENGTH, to_string(contentLength) );


}

///////////////////////////////////////////////////////////////////////////////
// getBody 
//   Read in the body of a response.  Based on header information, there
//   may not be anything to read.  true is returned in this case.  false
//   is returned if there is an error reading a non-zero length or
//   chunked body.
// Pre: Assigned(client), Assigned(method), Assigned(response),
//      Assigned(header)
// Modifies:
// Post: Get the response body and return true, false on error.
//
// I have hacked this a bit to allow the orignal caller to provide the
// stream to which the data should be deposited.  This is ugly because
// most of the code stays the same - in other words if they specify a
// stream, it gets put there. Otherwise it gets put where getResultsText
// can get it.
///////////////////////////////////////////////////////////////////////////////
bool EcceDAVClient::getBody
(
  CHTTPConnection&    client,
  int                 method,
  const CDAVResponse& davresponse,
  const CDAVHeader&   header,
  ostream *dest  
)
{
  typedef CHTTPConnection::size_type  size_type;

  CDAVServerInfo  info(header); // check for transfer encoding
  int             contentLength = getContentLength(header);

  // bsz should be tailored to the network connection shared by the two
  // machines.  We are optimized for ethernet on our network.  Consult
  // ifconfig man pages.
  const int       bsz = 1500;  
  char            buff[bsz+1];


  // For returned data that is "chunked transfer encoded" there probably
  // is not content length.  For non-chunked data, we do get a length.
  // So for any chunked data, there probably is data or why would the
  // server chunk it.  In this case you always have to read it.
  //
  // NOTE according to the spec, if the output is chunked AND content
  // length is provided, it means the server didn't finish.  We don't
  // address that condition yet. TODO
  if (contentLength == 0 && !info.chunkedTE()) return true;

  // For HEAD, the content length is the size of the document you did a
  // HEAD on!
  if (method == CHTTPRequest::HEAD) return true;

  DavDebug debug;
  if (gdebug) {
    debug.startTimer();
  }

  // Get the body
  size_type totalRead = 0;
  if (info.chunkedTE())
  // Using chunked transfer encoding
  {
    GetBodyCBData cbData; 
    if (dest) {
      delete cbData.m_data;
      cbData.m_data = dest;
    }
    cbData.setChunked();
    totalRead = p_client.getChunkedBody(buff, bsz, getBodyCallback, &cbData);
    if (dest) {
      cbData.m_data = 0; // so user data doesn't get deleted.
    } else {
      *cbData.m_data << ends;
      p_results = ((ostrstream*)cbData.m_data)->str();
    }
  }
  else if (contentLength)
  // We know just how much to get
  {
    GetBodyCBData cbData; 
    if (dest) {
      delete cbData.m_data;
      cbData.m_data = dest;
    }
    cbData.setContentLength(contentLength);
    totalRead = p_client.getBody(buff, bsz, getBodyCallback, &cbData);
    if (dest) {
      cbData.m_data = 0; // so user data doesn't get deleted.
    } else {
      *cbData.m_data << ends;
      p_results = ((ostrstream*)cbData.m_data)->str();
    }
  }
  else
  // Server didn't return the content length, but there may be some
  {
    ostream *mem = NULL;
    if (dest) {
      mem = dest;
    } else {
      mem = new ostrstream;
    }
    size_type nRead;

    while ((nRead = p_client.getBody(buff, bsz)) > 0) {
      (*mem) << buff;
      totalRead += nRead;
    }

    if (dest) {
      mem = 0;
    } else {
      (*mem) << ends;
      p_results = ((ostrstream*)mem)->str();
    }
    if (mem) delete mem;
  }

  if (gdebug) {
    if (dest) {
      debug.dump("Dumped to user defined stream\n","Response Body");
    } else {
      debug.dump(p_results.c_str(),"Response Body");
    }
    debug.endTimer("Elapsed time for GetBody (ms): ");
  }

  // Print out what we got
  if (contentLength == 0)
    return totalRead > 0;
  else
    return contentLength == totalRead;
}

/////////////////////////////////////////////////////////////////////////////
// getBodyCallback 
//  Ok I'm not touching this...
// Pre: Assigned(state)      - transfer state
//      Assigned(buffer)     - transfer buffer
//      Assigned(bsz)        - buffer size to write
//      Assigned(clientData) - body type info
// Modifies:
// Post: Print the body to the standard output and status info to the
//       standard error.
/////////////////////////////////////////////////////////////////////////////
int getBodyCallback
(
  http_body_tx_state state,
  void *             buffer,
  unsigned           bsz,
  void *             clientData
)
{
#if !defined( OLD_CPP_HEADERS )
  using std::min;
#endif

  // Cast void pointer into GetBodyCBData pointer:
  GetBodyCBData *cbData  = static_cast<GetBodyCBData *>(clientData);
  if (!cbData)  // cast fails if wrong data structure passed in
    return -1;

  switch (state)
  {
    case HTTP_BODY_TX_INIT :
      cbData->m_totalBytes = cbData->m_contentLength;  
      // For chunked transfer encoding, the return value is ignored.
      // In this case, we'll just return the buffer size each time.
      cbData->m_bytesLeft  = cbData->m_chunked ? bsz : cbData->m_totalBytes;
      return min(bsz, (unsigned) cbData->m_bytesLeft);
    case HTTP_BODY_TX_CONT :
        cbData->m_data->write((const char *) buffer, bsz);
      if (cbData->m_chunked)  // (see note above)
        return cbData->m_bytesLeft;
      cbData->m_bytesLeft =
          cbData->m_bytesLeft < bsz ? 0 : cbData->m_bytesLeft - bsz;
      return min(bsz, (unsigned) cbData->m_bytesLeft);
    case HTTP_BODY_TX_DONE :
      return 0;
    default :
      return -1;
  }
}

///////////////////////////////////////////////////////////////////////////////
//   Returns the number of characters in the stream.  This works easily
//   with seekg on Linux for both file and istrstream.
//
//   NOTE that this method has the side affect of seeking to 0 when done.
///////////////////////////////////////////////////////////////////////////////
int EcceDAVClient::getStreamSize(istream *data)
{
  int ret  = 0;
  if (data) {
    data->seekg(0,ios::end);
    ret = data->tellg();
    data->seekg(0);
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// getContentLength 
//   Returns size of content by extracting from header.
// Pre: Assigned(header)
///////////////////////////////////////////////////////////////////////////////
int EcceDAVClient::getContentLength(const CHTTPHeader& header)
{
  string lenStr = header.header(CHTTPHeader::CONTENT_LENGTH);
  if (!lenStr.length())
    return 0;
  return atoi(lenStr.c_str());
}


////////////////////////////////////////////////////////////////////////////
// Description
//   We try to be nice to the user and cache passwords for them based on
//   passwords they've provided at one time or another.  Each url can
//   potentially have its own authentication which makes it tricky.
//   The logic we are trying is as follows:
//     . First use the login-user and see if we have a cached password for
//       the current url.  If so return it and true.  The
//       getAuthorization method can be told to use a parent url if the
//       exact url is not found.
//     . If not found, then the current url and its parents have not
//       been authenticated as the current user.  To be extra nice, see
//       if the url or parent has been authenticated as somebody else.
//       If so use that and return true.
//     . Otherwise return false (or call listener to prompt).
//   I'm not sure if we'll like this or not.  We could also use a url of
//   any and allow one user to authenticate for all Dav servers for now.
//
//   returns true if a new username/password has been supplied.
//
// Pre: Assigned(userName), Assigned(password)
// Modifies: userName, password
// Post: Get the user name/password and return true, false on error
////////////////////////////////////////////////////////////////////////////
bool EcceDAVClient::namePasswordCallback
(
  string& userName,
  string& password,
  const string& realm,
  const bool& newUser
)
{
   bool ret = false;

   if (p_authListener != NULL) {
      if (p_lastAuthEvent == 0) {
         p_lastAuthEvent = new AuthEvent();
         p_lastAuthEvent->m_retryCount = 1;
         p_lastAuthEvent->m_realm = realm;
         p_lastAuthEvent->m_url = p_url.toString();
         p_lastAuthEvent->m_newUser = newUser;
      } else {
         p_lastAuthEvent->m_retryCount++;
      }
      if (p_authListener->getAuthorization(*p_lastAuthEvent)) {
         userName = p_lastAuthEvent->m_user;
         password = p_lastAuthEvent->m_password;
         ret = true;
      }
   }

   return ret;
}


/////////////////////////////////////////////////////////////////////////////
// putBody 
//   Send the body of the request off.
// Pre: Assigned(client) - connection to server
//      Assigned(method) - HTTP/DAV method
//      Assigned(header) - request header
//      Assigned(path)   - path/file to send
// Modifies:
// Post: Send the specified path/file to the server and return true, false
//       on error.
/////////////////////////////////////////////////////////////////////////////
bool EcceDAVClient::putBody
(
  CHTTPConnection&  client,
  int               method,
  const CDAVHeader& header,
  istream *ifs
)
{

  // Is there anything to do?
  if (ifs == NULL || !canHaveRequestBody(method)) return true;

  typedef CHTTPConnection::size_type  size_type;


  DavDebug debug;
  string dump;
  if (gdebug) {
    debug.startTimer();
  }

  if (gdebug && !gdebugbodytoo) {
    dump = "Body dumping disabled!\n";
  }

  size_type fileSize  = getContentLength(header),
            totalSent = 0;


  if (fileSize == 0) return true;

  ifs->clear();
  ifs->seekg(0, ios::beg);
  const int bsz = 1024;
  char      buff[bsz+1];
  do
  // Send body
  {
    ifs->read(buff, bsz);
    int nRead = ifs->gcount();
    if (nRead > 0)
    {
      if (gdebugbodytoo) {
        dump.append(buff,nRead);
      }
      if (p_client.sendBody(buff, nRead) != nRead)
      {
        return false;
      }
      totalSent += nRead;
    }
  } while (*ifs);

  if (gdebug) {
    debug.dump(dump,"Request Body");
    debug.endTimer("Elapsed time for PutBody (ms): ");
  }

  // Success if we send the entire body
  return (totalSent == fileSize);
}
