// CSocket.C  -- Socket base class using internet domain stream sockets.
// Created: 07/28/1995
// Version: 07/12/2000
//
// Modifications:
//
//   09/03/1996 - K. Walker - Added a <sockfd> argument and int return value
//     to receive().
//   09/05/1996 - K. Walker - Added an int return value to send().  Added
//     a protected send(sockfd, ...) method.
//   03/24/1999 - K. Walker - Put in "ipc" namespace
//   03/25/1999 - K. Walker - removed cerr messages.  Moved classes:
//     CSocketError, CClientSocket, CServerSocket to this file.
//   05/09/2000 - K. Walker - Changed CSocket::poll() to throw an
//     exception on select() errors
//   05/12/2000 - K. Walker - Added <flags> argument to CSocket::receive()
//   07/12/2000 - K. Walker - Added CSocket::flush()

// include files **********************************************************

#include "dsm/CSocket.H"

#include <stdlib.h>        // for free()
#include <string.h>        // for memcpy()

namespace ipc {

// CSocketError::message **************************************************

CSocketError::string_type CSocketError::message(void) const
// Pre:
// Modifies:
// Post: Return an error message.
{
  string_type msg = "Error:";

  switch (err_)
  {
    case NoError :
      msg = "";
      break;
    case AcceptError :
      msg += " accept";
      break;
    case BindError :
      msg += " bind";
      break;
    case ConnectError :
      msg += " connect";
      break;
    case CreateError :
      msg += " creating socket";
      break;
    case NameError :
      msg += " gettting socket name";
      break;
    case ReceiveError :
      msg += " receive";
      break;
    case SelectError :
      msg += " select";
      break;
    case SendError :
      msg += " send";
      break;
    case UnknownHostError :
      msg += " unknown host";
      break;
    default:
      msg += " unknown error";
      break;
  }

  return msg;
}

// Constructors: CSocket **************************************************

CSocket::CSocket(const csocket& other) throw (CSocketError)
// Pre: Assigned(other)
// Modifies:
// Post: Create a new CSocket identical to <other>.
{
  if (!csocket_copy(&sock_, &other) )
    throw CSocketError(CSocketError::CreateError);
}

CSocket::CSocket(const CSocket& other) throw (CSocketError)
// Pre: Assigned(other)
// Modifies:
// Post: Create a new CSocket identical to <other>.
{
  if (!csocket_copy(&sock_, &(other.sock_)) )
    throw CSocketError(CSocketError::CreateError);
}

// Destructor: CSocket ****************************************************

CSocket::~CSocket(void)
// Pre:
// Modifies:
// Post: Shutdown the socket.
{
  csocket_close(&sock_);
}

// CSocket::operator = *****************************************************

CSocket& CSocket::operator = (const CSocket& other) throw (CSocketError)
// Pre: Assigned(other)
// Modifies: sock_
// Post: Copy <other> to *this and return *this
{
  if (&other != this)
  {
    csocket_close(&sock_);
    if (!csocket_copy(&sock_, other) )
      throw CSocketError(CSocketError::CreateError);
  }
  return *this;
}

// CSocket::fd ************************************************************

int CSocket::fd(void) const
// Pre:
// Modifies:
// Post: Return the file descriptor.
{
  return csocket_fd(&sock_);
}

// CSocket::flush **********************************************************

CSocket::size_type CSocket::flush(
  size_type seconds,
  size_type microseconds) throw (CSocketError)
// Pre: Assigned(seconds), Assigned(microseconds)
// Modifies:
// Post: Flush all incoming data to this socket.
//       Return the number of bytes flushed.
//       <seconds, microseconds> determines how long to wait
// KLS 5/2001 This sometimes hangs.  poll is returning true right away
// but there are no bytes to read so I added logic to break from the loop
// if no bytes were actually recieved.
{
  try
  {
    size_type result = 0,
              n;
    const int bsz    = 1024;
    char      buff[bsz];
    while (poll(seconds, microseconds))
    {
      n       = receive(buff, bsz);
      result += n;
      if (n==0) { break; }
    }
    return result;
  }
  catch (CSocketError& err)
  {
    throw err;
  }
}

// CSocket::IPAddress *****************************************************

CSocket::string_type CSocket::IPAddress(void) const
// Pre:
// Modifies:
// Post: Return the IP address of this socket.  Return "" if there is an
//       error.
{
  const char *  s = csocket_IPAddress(&sock_);
  return string_type(s ? s : "");
}

// CSocket::poll **********************************************************

bool CSocket::poll(
  size_type seconds,
  size_type microseconds) throw (CSocketError)
// Pre: Assigned(seconds), Assigned(microseconds)
// Modifies:
// Post: Check for socket input. Return true if there is input.
//       Return false otherwise.
//       <seconds> and <microseconds> determine how long to wait.
//       Throw CSocketError::SelectError on error.
{
  int rval = csocket_poll(&sock_, seconds, microseconds);
  if (rval < 0)
    throw CSocketError(CSocketError::SelectError);
  return rval == 1;
}

// CSocket::portNumber ****************************************************

CSocket::port_type CSocket::portNumber(void) const
// Pre:
// Modifies:
// Post: Return the socket port number.
{
  return (port_type) csocket_portNumber(&sock_);
}

// CSocket::receive *******************************************************

CSocket::size_type CSocket::receive(
  void *    buff,
  size_type nbytes,
  int       flags) throw (CSocketError)
// Pre: Assigned(buff)   -- buffer to use
//      Assigned(nbytes) -- buffer capacity
// Modifies: <buffer>
// Post: Receive a message and return the length of the message received.
//       If no messages are available at the socket, receive() call waits
//       for a message to arrive.
//       The <flags> are the same as used for ::recv()
// NOTE: <buff> must have at least <nbytes> allocated.
{
  int   rval = csocket_receive(&sock_, buff, nbytes, flags);
  if (rval == -1)
    throw CSocketError(CSocketError::ReceiveError);

  return (size_type) rval;
}

// CSocket::send **********************************************************

CSocket::size_type CSocket::send(const string_type& s) throw (CSocketError)
// Pre: Assigned(s) -- string to send
// Modifies:
// Post: Send a string and return the length of the message sent.
//       If no messages space is available at the socket to hold the
//       message to be transmitted, send() blocks until space is available.
{
  long  result = csocket_send(&sock_, s.c_str(), s.length() );
  if (result == -1)
    throw CSocketError(CSocketError::SendError);

  return (size_type) result;
}

CSocket::size_type CSocket::send(
  const void * buff,
  size_type    nbytes) throw (CSocketError)
// Pre: Assigned(buff)   -- buffer to use
//      Assigned(nbytes) -- message length
// Modifies:
// Post: Send a message and return the length of the message sent.
//       If no messages space is available at the socket to hold the
//       message to be transmitted, send() blocks until space is available.
{
  long  result = csocket_send(&sock_, buff, nbytes);
  if (result == -1)
    throw CSocketError(CSocketError::SendError);

  return (size_type) result;
}

// Constructor: CClientSocket *********************************************

CClientSocket::CClientSocket(
  const string_type& host,
  port_type          port) throw (CSocketError)
  : CSocket()
// Pre: Assigned(host), Assigned(port)
// Modifies:
// Post: Create a new instance
{
  if (!csocket_openClient(&sock_, host.c_str(), port))
    throw CSocketError(CSocketError::CreateError);
}

// Constructor: CServerSocket ********************************************

CServerSocket::CServerSocket(port_type port) throw (CSocketError)
  : CSocket()
// Pre: Assigned(port)
// Modifies:
// Post: Create a new server socket.
{
  if (!csocket_openServer(&sock_, port))
    throw CSocketError(CSocketError::CreateError);
}

// CServerSocket::accept **************************************************

CSocket CServerSocket::accept(void) const throw (CSocketError)
// Pre:
// Modifies:
// Post: Wait for a connection to this server and return a new CSocket
//       instance accepted.  If there are no connection requests pending,
//       accept() blocks until one arrives.
{
  csocket  *s = csocket_accept((csocket *) &sock_);
  if (!s)
    throw CSocketError(CSocketError::AcceptError);

  // Create a socket to return
  CSocket  cs(*s);

  csocket_close(s);
  free(s);

  return cs;
}

// Constructor: CFileTX ****************************************************

CFileTX::CFileTX(const CFileTX& other)
// Pre: Assigned(other)
// Modifies:
// Post: Create a new instance identical to <other>
{
  memcpy(this, &other, sizeof(filetx) );
}

// CFileTX::operator = *****************************************************

CFileTX& CFileTX::operator = (const CFileTX& other)
// Pre: Assigned(other)
// Modifies: *this
// Post: Copy <other> to *this and return *this
{
  if (&other != this)
    memcpy(this, &other, sizeof(filetx) );
  return *this;
}

// CFileTX::fileName *******************************************************

CFileTX::string_type CFileTX::fileName(void) const
// Pre:
// Modifies:
// Post: Return the name of the current/last file being received
{
  const char *  s = tx_fileName(this);
  return string_type(s ? s : "");
}

// CFileTX::receive ********************************************************

CFileTX::size_type CFileTX::receive(
  CSocket&     sock,
  void *       buffer,
  size_type    bufLen,
  status_type& status) throw (CSocketError)
// Pre: Assigned(sock), Assigned(buffer), Assigned(bufLen), Assigned(status)
// Modifies: status
// Post: Receive a transfer from <sock> and return the number of bytes read.
//       Set the state of completion in <status>.
//       Return 0 if <sock> has closed.
//       With the exception of the file header, <buffer> is used to hold
//       the data read from <sock>.
{
  int   st;
  long  result = tx_receive(this, sock, buffer, bufLen, &st);
  status = (status_type) st;
  if (result == -1)
    throw CSocketError(CSocketError::ReceiveError);

  return (size_type) result;
}

// CFileTX::send ***********************************************************

CFileTX::size_type CFileTX::send(
  CSocket&           sock,
  void *             buffer,
  size_type          bufLen,
  const string_type& src,
  const string_type& dst) throw (CSocketError)
// Pre: Assigned(sock), Assigned(buffer), Assigned(bufLen),
//      Assigned(src), Assigned(dst)
// Modifies:
// Post: Send the file <src> over <sock> as <dst> and return the number of
//       bytes sent.  Use the buffer specified by <buffer> and <bufLen> to
//       send the file through the socket.
// NOTE: If <dst> == "", <src> is used.
{
  const char  *d     = dst.length() ? dst.c_str() : 0;
  long        result = tx_send(this, sock, buffer, bufLen, src.c_str(), d);
  if (result == -1)
    throw CSocketError(CSocketError::SendError);

  return (size_type) result;
}

} // namespace ipc

// end of CSocket.C *****************************************************

