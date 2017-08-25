// CHTTPConnection.C
// Created: 05/11/2000
// Version: 07/12/2000
//
// Description:
//
// Modifications:

// include files ***********************************************************

//#define timertests
#ifdef timertests
#include <sys/timeb.h>
#endif

#if 000
#include <iostream>
using std::cout;
using std::endl;
#endif

#include "dsm/CHTTPConnection.H"
#include <algorithm>

// Constructors: CHTTPConnection *******************************************

CHTTPConnection::CHTTPConnection(void)
  : deadConnection_(true), host_(""), proxy_(""),
    port_(0), server_(0), responseStr_(""), headerStr_("")
// Pre:
// Modifies:
// Post: Create a new instance
{
}

CHTTPConnection::CHTTPConnection(const CHTTPConnection& other)
  : deadConnection_(other.deadConnection_), host_(other.host_),
    proxy_(other.proxy_), port_(other.port_), server_(0),
    responseStr_(other.responseStr_), headerStr_(other.headerStr_)
// Pre: Assigned(other)
// Modifies:
// Post: Create a new instance identical to <other>
{
  if (other.server_)
    server_ = new socket_type(*other.server_);
}

// Destructor: CHTTPConnection *********************************************

CHTTPConnection::~CHTTPConnection(void)
// Pre:
// Modifies:
// Post: Destroy self
{
  disconnect();
}

// CHTTPConnection::operator = *********************************************

CHTTPConnection&
CHTTPConnection::operator = (const CHTTPConnection& other)
// Pre: Assigned(other)
// Modifies: *this
// Post: Copy <other> to *this and return *this
{
  if (&other != this)
  {
    deadConnection_ = other.deadConnection_;
    host_           = other.host_;
    proxy_          = other.proxy_;
    port_           = other.port_;
    if (server_)
      delete server_;
    server_         = other.server_ ? new socket_type(*other.server_) : 0;
    responseStr_    = other.responseStr_;
    headerStr_      = other.headerStr_;
  }
  return *this;
}

// CHTTPConnection::connect ************************************************

bool CHTTPConnection::connect(void)
// Pre:
// Modifies:
// Post: Connect to the server
{
  if (!deadConnection())
    disconnect();
  try
  {
    string_type h   = proxy_.length() ? proxy_ : host_;
    server_         = new socket_type(h, port_);
    deadConnection_ = false;
    return true;
  }
  catch (...)
  {
    return false;
  }
}

bool CHTTPConnection::connect(
  const string_type& host,
  port_type          port,
  const string_type& proxy)
// Pre: Assigned(host), Assigned(port), Assigned(proxy)
// Modifies:
// Post: Connect to the server
{
  // Save new values
  host_  = host;
  port_  = port;
  proxy_ = proxy;

  // Try to connect
  return connect();
}

// CHTTPConnection::disconnect *********************************************

void CHTTPConnection::disconnect(void)
// Pre:
// Modifies:
// Post: Disconnect from the server
{
  if (server_)
  {
    delete server_;
    server_ = 0;
  }
  deadConnection_ = true;
}

// CHTTPConnection::flush **************************************************

void CHTTPConnection::flush(void)
// Pre:
// Modifies:
// Post: Flush all incoming data from the server.
{
  using ipc::CSocketError;

  // Flush incoming data from the server.
  try
  {
    if (server_)
      server_->flush(0, 1000);
    else
      deadConnection_ = true;
  }
  catch (CSocketError&)
  {
    deadConnection_ = true;
  }
}

// CHTTPConnection::getBody ************************************************

CHTTPConnection::size_type
CHTTPConnection::getBody(
  void *                txBuffer,
  unsigned              txBufferSize,
  http_body_tx_callback cb,
  void *                clientData)
// Pre: Assigned(txBuffer)     - transfer buffer
//      Assigned(txBufferSize) - transfer buffer capacity
//      Assigned(cb)           - callback for getting body
//      Assigned(clientData)   - client-defined
// Modifies:
// Post: Get the reponse body from a request.
//       Return the body size.
{
  static string_type  cbErrStr("Error saving transfer buffer"),
                      txErrStr("Error receiving message body");

  if (deadConnection() || !txBuffer || !txBufferSize || !cb)
    return 0;

  size_type nRead    = 0,
            nTried   = 0;
  int       cbResult;

  // Report that we're starting
  cbResult = cb(HTTP_BODY_TX_INIT, txBuffer, txBufferSize, clientData);

  do
  {
    if (cbResult > txBufferSize)
    {
      cbResult = -1;
      break;
    }
    nTried = getBody(txBuffer, cbResult);
    if (nTried == cbResult)
    {
      nRead += nTried;
      cbResult = cb(HTTP_BODY_TX_CONT, txBuffer, cbResult, clientData);
    }
    else
    // error
      cbResult = -1;
  } while (!deadConnection() && cbResult > 0);

  if (cbResult == 0 && !deadConnection())
    cb(HTTP_BODY_TX_DONE, txBuffer, txBufferSize, clientData);
  else if (cbResult < 0)
    cb(HTTP_BODY_TX_ERROR, (void *) cbErrStr.c_str(),
       cbErrStr.length(), clientData);
  else if (deadConnection())
    cb(HTTP_BODY_TX_ERROR, (void *) txErrStr.c_str(),
       txErrStr.length(), clientData);

  return nRead;
}

CHTTPConnection::size_type
CHTTPConnection::getBody(
  void *                txBuffer,
  unsigned              txBufferSize)
// Pre: Assigned(txBuffer)     - transfer buffer
//      Assigned(txBufferSize) - transfer buffer capacity
// Modifies:
// Post: Get the reponse body from a request.
//       Return the body size.
{
  using ipc::CSocketError;

#if 111
    // Fix for horrible performance with certain platforms/sites such
    // as Rob Shroll at SSI running under NFS
    // This one I didn't originally change and got bit with it a couple
    // weeks later after more painful remote debugging
    // Gary Black, 12/19/2009
    static const int receiveFlags = getenv("ECCE_OLD_RECEIVE")!=NULL? 
                                           MSG_WAITALL: 0;
#endif

  if (deadConnection() || !txBuffer || !txBufferSize)
    return 0;

#if 000
    time_t now;
#endif

  try
  {
    size_type nRead, toRead;
    char *bufPtr;
#if 000
      now = time(NULL);
      cout << "*** CHTTPConnection::getBody before receive: " << ctime(&now);
#endif
    for (toRead=txBufferSize, bufPtr=(char*)txBuffer; toRead>0;
         toRead-=nRead, bufPtr+=nRead)
      nRead = server_->receive((void*)bufPtr, toRead, receiveFlags);

#if 000
      now = time(NULL);
      cout << "*** CHTTPConnection::getBody after receive: " << ctime(&now);
#endif
    return txBufferSize;
  }
  catch (CSocketError&)
  {
    deadConnection_ = true;
    return 0;
  }
}

CHTTPConnection::size_type
CHTTPConnection::getChunkedBody(
  void *                txBuffer,
  unsigned              txBufferSize,
  http_body_tx_callback cb,
  void *                clientData)
// Pre: Assigned(txBuffer)     - transfer buffer
//      Assigned(txBufferSize) - transfer buffer capacity
//      Assigned(cb)           - callback for getting body
//      Assigned(clientData)   - client-defined
// Modifies:
// Post: Get the reponse body from a request using chunked transfer encoding.
//       Return the body size.
// NOTE: The chunked encoding is:
//         <chunksize>CRLF<chunk>CRLF...<chunksize>CRLF
//       where <chunksize> is in hex and the final <chunksize> is 0
{
  using std::min;
  using std::strtol;

  static string_type  cbErrStr("Error saving transfer buffer"),
                      txErrStr("Error receiving message body");

  if (deadConnection() || !txBuffer || !txBufferSize || !cb)
    return 0;

  size_type nRead    = 0;
  int       cbResult = 0;

#ifdef timertests
timeb startt;
ftime(&startt);
unsigned long stms = (unsigned long)(startt.time*1000 + startt.millitm);
#endif

  // Report that we're starting
  cb(HTTP_BODY_TX_INIT, txBuffer, txBufferSize, clientData);

  do
  {
    string_type chunkSizeStr;
    size_type   chunkSize = 0,
                nTried,
                nToGet;

    // Get the chunk size
    if (!getLine(chunkSizeStr) )
      break;

    // Remove CRLF and get chunk size
    chunkSizeStr.erase(chunkSizeStr.length() - 2);
    chunkSize = strtol(chunkSizeStr.c_str(), 0, 16);
    if (!chunkSize)
      break;

    do
    {
      // Figure out how much to get now
      nToGet    = (txBufferSize > chunkSize)?chunkSize : txBufferSize;
      nTried    = getBody(txBuffer, nToGet);

      if (nTried == nToGet)
      {
        nRead += nTried;
        cbResult = cb(HTTP_BODY_TX_CONT, txBuffer, nTried, clientData);
      }
      else
      // error
        cbResult = -1;

      // Figure out how much to get next pass
      chunkSize = (chunkSize > txBufferSize ? chunkSize - nToGet : 0);
    } while (chunkSize && !deadConnection() && cbResult > 0);
    // If we read an entire chunk, read a CRLF
    if (!deadConnection() && cbResult > 0)
    {
      string_type crlf;
      if (!getLine(crlf))
        break;
    }
  } while (!deadConnection() && cbResult > 0);



  // Added by KLS May 2001.  With persistent connections we sometimes had
  // data left over on the socket which caused failures on ensuing
  // operations.  My reading of the spec was that the server might send
  // more data - I can't really recall this exactly since its been a
  // couple of months since I looked at it.  I would consider this code
  // suspect.  Read the spec before working on this.  Also with this code
  // in and using persistent connections, they work BUT they are slower
  // than re-establishing connections.  My tracing seemed to indicate it
  // had nothing to do with this code but was in the poll (csock) waiting
  // for the server to resond.
  string_type junk;
  bool status;
  do {
    status = getLine(junk);
  } while (status && junk.length() > 2);


#ifdef timertests
timeb endt;
ftime(&endt);
unsigned long etms = (unsigned long)(endt.time*1000 + endt.millitm);
cout << "Timer to read was " << etms - stms << endl;
#endif


  if (cbResult == 0 && !deadConnection())
    cb(HTTP_BODY_TX_DONE, txBuffer, txBufferSize, clientData);
  else if (cbResult < 0)
    cb(HTTP_BODY_TX_ERROR, (void *) cbErrStr.c_str(),
       cbErrStr.length(), clientData);
  else if (deadConnection())
    cb(HTTP_BODY_TX_ERROR, (void *) txErrStr.c_str(),
       txErrStr.length(), clientData);

  return nRead;
}

// CHTTPConnection::getHeader **********************************************

bool CHTTPConnection::getHeader(void)
// Pre:
// Modifies:
// Post: Get the header
{
  int         result;
  string_type line;

  headerStr_ = "";
  while ((result = getLine(line) ) > 0 && line != CRLF)
    headerStr_ += line;

  return result > 0;
}

// CHTTPConnection::getLine ************************************************

bool CHTTPConnection::getLine(string_type& line)
// Pre: Assigned(line)
// Modifies: line
// Post: Get a line from the server.
//       The trailing CRLF is left at the end of the line.
//       Return true on success, false if there was no input or on error.
{
  using ipc::CSocketError;

  line = "";
  if (deadConnection())
    return false;
  try
  {
#if 111
    const int bsz = 1500;
#else
    const int bsz = 128;
#endif
    char      buff[bsz];
    int       nRead;
    bool      done = false;
#if 111
    // Fix for horrible performance with certain platforms/sites such
    // as Rob Shroll at SSI running Fedora Core 11
    // Gary Black, 12/2/2009
    static const int receiveFlags = getenv("ECCE_OLD_RECEIVE")!=NULL? 
                                           (MSG_PEEK | MSG_WAITALL): MSG_PEEK;
#endif
#if 000
    time_t now;
#endif

    do  // block from here on
    {
#if 000
      now = time(NULL);
      cout << "*** CHTTPConnection::getLine before receive: " << ctime(&now);
#endif
      nRead = server_->receive(buff, bsz, receiveFlags);
#if 000
      now = time(NULL);
      cout << "*** CHTTPConnection::getLine after receive: " << ctime(&now);
#endif
      if (!nRead)
      {
        deadConnection_ = true;
        line            = "";
        return false;
      }
      char  *ptr = (char *) memchr(buff, '\n', nRead);
      if (ptr == 0)
      // need more
        line.append(buff, nRead);
      else
      {
        nRead = ptr - ((const char *) buff) + 1;
        done  = true;
        line.append(buff, nRead);
        // Make sure the line ends with CRLF
        string_type::size_type  len = line.length();
        if (len == 1)
          line = CRLF;
        else if (line[len - 2] != '\r')
          line.replace(len - 1, 1, CRLF);
      }
      // Remove what we've used from the read queue
      nRead = server_->receive(buff, nRead);
    } while (!done && nRead);

    return line.length() > 0;
  }
  catch (CSocketError&)
  {
    deadConnection_ = true;
    line            = "";
    return false;
  }
}

// CHTTPConnection::getResponse ********************************************

bool CHTTPConnection::getResponse(
  CHTTPResponse& response,
  CHTTPHeader&   header,
  size_type      seconds,
  size_type      microseconds)
// Pre: Assigned(response), Assigned(header)
// Modifies: response, header
// Post: Get a response from the request at the front of the queue.
//       Return true if if a response was obtained, false otherwise.
//       <seconds> and <microseconds> determine how long to wait.
// NOTE: sendRequest() must be called before this call.
{
#if 000
  time_t now;
#endif
  using ipc::CSocketError;

  if (deadConnection())
    return false;

  try
  {
#if 000
    now = time(NULL);
    cout << "*** CHTTPConnection::getResponse before poll time: " << ctime(&now);
#endif
#if 111
    // Tried removing this call to fix an NFS issue, but the problem just
    // migrated downstream to the underlying socket recv() calls so it's safer
    // to still use the select()  GDB 12/21/2009
    if (!server_->poll(seconds, microseconds) )
      return false;
#endif
#if 000
    now = time(NULL);
    cout << "*** CHTTPConnection::getResponse after poll time: " << ctime(&now);
#endif
    if (!getResponseLine() || !getHeader() )
      return false;
//cout << "response str " << responseStr_ << endl;
//cout << "header str " << headerStr_ << endl;
    return response.parse(responseStr_) &&
           header.parse(headerStr_);
  }
  catch (CSocketError&)
  {
    deadConnection_ = true;
    return false;
  }
}

// CHTTPConnection::getResponseLine ****************************************

bool CHTTPConnection::getResponseLine(void)
// Pre:
// Modifies: responseStr_
// Post: Get the server response
{
  return getLine(responseStr_);
}

// CHTTPConnection::sendBody ***********************************************

CHTTPConnection::size_type
CHTTPConnection::sendBody(
  void *                txBuffer,
  unsigned              txBufferSize,
  http_body_tx_callback cb,
  void *                clientData)
// Pre: Assigned(txBuffer)     - transfer buffer
//      Assigned(txBufferSize) - transfer buffer capacity
//      Assigned(cb)           - callback for sending body
//      Assigned(clientData)   - client-defined
// Modifies:
// Post: Send a request body to the server using a callback.
//       Return the length sent.
{
  static string_type  cbErrStr("Error loading transfer buffer"),
                      txErrStr("Error sending message body");

  if (!txBuffer || !txBufferSize || !cb)
    return 0;
  size_type nSent  = 0,
            nTried;
  int       cbResult = 0;
  cbResult = cb(HTTP_BODY_TX_INIT, txBuffer, txBufferSize, clientData);

  do
  {
    if (cbResult > txBufferSize)
    {
      cbResult = -1;
      break;
    }
    cbResult = cb(HTTP_BODY_TX_CONT, txBuffer, cbResult, clientData);
    if (cbResult < 1)
      break;
    nTried = sendBody(txBuffer, cbResult);
    if (nTried == cbResult)
      nSent += nTried;
  } while (!deadConnection() && cbResult > 0);

  if (cbResult == 0 && !deadConnection())
    cb(HTTP_BODY_TX_DONE, txBuffer, txBufferSize, clientData);
  else if (cbResult < 0)
    cb(HTTP_BODY_TX_ERROR, (void *) cbErrStr.c_str(),
       cbErrStr.length(), clientData);
  else if (deadConnection())
    cb(HTTP_BODY_TX_ERROR, (void *) txErrStr.c_str(),
       txErrStr.length(), clientData);

  return nSent;
}

CHTTPConnection::size_type
CHTTPConnection::sendBody(
  void *   txBuffer,
  unsigned txBufferSize)
// Pre: Assigned(txBuffer)     - transfer buffer
//      Assigned(txBufferSize) - transfer buffer capacity
// Modifies:
// Post: Send a request body to the server.
//       Return the length sent.
{
  using ipc::CSocketError;

  if (deadConnection())
    return 0;

  try
  {
    size_type nSent;
    nSent = server_->send(txBuffer, txBufferSize);
    if (nSent != txBufferSize)
      deadConnection_ = true;

    return nSent;
  }
  catch (CSocketError&)
  {
    deadConnection_ = true;
    return 0;
  }
}

// CHTTPConnection::sendRequest ********************************************

bool CHTTPConnection::sendRequest(
  const CHTTPRequest& request,
  const CHTTPHeader&  header)
// Pre: Assigned(request), Assigned(header)
// Modifies:
// Post: Send a request to the server
//       Return true on success, false otherwise.
{
  using ipc::CSocketError;

  if (deadConnection())
    return false;

  try
  {
    string_type             msg;
    socket_type::size_type  nSent;
    msg  = request.toString();
    msg += header.toString();
    msg += CRLF;
    nSent = server_->send(msg);
    if (nSent != msg.length() )
    {
      deadConnection_ = true;
      return false;
    }
    return true;
  }
  catch (CSocketError&)
  {
    deadConnection_ = true;
    return false;
  }
}

// CHTTPConnection::test ***************************************************

bool CHTTPConnection::test(void)
// Pre:
// Modifies:
// Post: Test the connection to the server.
//       Return true if good, false otherwise
{
  using ipc::CSocketError;

  if (deadConnection())
    return false;

  if (deadConnection())
    return false;

  try
  {
    // Just make sure the connection didn't die
    server_->poll();
    return true;
  }
  catch (CSocketError&)
  {
    deadConnection_ = true;
    return false;
  }
}

// end of CHTTPConnection.C **********************************************

