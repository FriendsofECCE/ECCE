// CHTTPConnection.H
// Created: 05/11/2000
// Version: 05/18/2000
//
// Description:
//
// Modifications:

#if !defined( __CHTTPCONNECTION_H )
#define __CHTTPCONNECTION_H   // prevent multiple includes

// include files ***********************************************************

#include <string>

#include "dsm/CHTTPDefs.H"
#include "dsm/CHTTPHeader.H"
#include "dsm/CHTTPRequest.H"
#include "dsm/CHTTPResponse.H"

#include "dsm/CSocket.H"

// class: CHTTPConnection **************************************************

class CHTTPConnection
{
public :

  typedef std::string             string_type;
  typedef ipc::CSocket::port_type port_type;
  typedef string_type::size_type  size_type;

  // Constructors
  CHTTPConnection(void);
  CHTTPConnection(const CHTTPConnection&);

  // Destructor
  virtual ~CHTTPConnection(void);

  // Assignment operator
  CHTTPConnection& operator = (const CHTTPConnection&);

  // Connect to the server
  virtual bool connect(void); // uses current values
  virtual bool connect(
    const string_type& host,
    port_type          port  =  HTTPDefPort,
    const string_type& proxy = "");

  // Did the connection to the server die?
  virtual bool deadConnection(void) const
  { return deadConnection_ || !server_; }

  // Disconnect from the server
  virtual void disconnect(void);

  // Flush all incoming data from the server.
  virtual void flush(void);

  // Get the reponse body from a request.
  // Return the body size.
  // NOTE: The first version uses a callback so the body can be retrieved
  //       with one call to getBody().
  //       Both version assume the caller knows the body content length.
  virtual size_type getBody(
    void *                txBuffer,         // transfer buffer
    unsigned              txBufferSize,     // transfer buffer capacity
    http_body_tx_callback cb,               // callback for getting body
    void *                clientData = 0);  // client-defined
  virtual size_type getBody(
    void *                txBuffer,         // transfer buffer
    unsigned              txBufferSize);    // transfer buffer capacity

  // Get the reponse body from a request using chunked transfer encoding.
  // Return the body size.
  virtual size_type getChunkedBody(
    void *                txBuffer,         // transfer buffer
    unsigned              txBufferSize,     // transfer buffer capacity
    http_body_tx_callback cb,               // callback for getting body
    void *                clientData = 0);  // client-defined

  // Get a line from the server.
  // The trailing CRLF is left at the end of the line.
  // Return true on success, false if there was no input or on error.
  bool getLine(string_type&);

  // Get a response from the request at the front of the queue.
  // Return true if if a response was obtained, false otherwise.
  // <seconds> and <microseconds> determine how long to wait.
  // NOTE: sendRequest() must be called before this call.
  virtual bool getResponse(
    CHTTPResponse& response,
    CHTTPHeader&   header,
    size_type      seconds      = 0,
    size_type      microseconds = 0);

  // Send a request body to the server.
  // Return the length sent.
  // NOTE: The first version uses a callback so the body can be sent
  //       with one call to sendBody().
  //       The second version requires multiple calls to sendBody() until
  //       it returns 0.
  virtual size_type sendBody(
    void *                txBuffer,         // transfer buffer
    unsigned              txBufferSize,     // transfer buffer capacity
    http_body_tx_callback cb,               // callback for sending body
    void *                clientData = 0);  // client-defined
  virtual size_type sendBody(
    void *                txBuffer,         // transfer buffer
    unsigned              txBufferSize);    // transfer buffer capacity

  // Send a request to the server
  // Return true on success, false otherwise.
  virtual bool sendRequest(
    const CHTTPRequest& request,
    const CHTTPHeader&  header);

  // Test the connection to the server.
  // Return true if good, false otherwise
  virtual bool test(void);

protected :

  typedef ipc::CClientSocket        socket_type;

  bool            deadConnection_;    // did the connection to the server die?
  string_type     host_,              // server host name
                  proxy_;             // proxy server ("" if no proxy)
  port_type       port_;              // server port
  socket_type     *server_;           // client socket to server

private :

  string_type   responseStr_,       // repsonse buffer
                headerStr_;         // header buffer

  // Get the header
  bool getHeader(void);

  // Get the server response
  bool getResponseLine(void);
};

#endif  // __CHTTPCONNECTION_H

// end of CHTTPConnection.H ************************************************

