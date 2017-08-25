// CSocket.H -- Socket base class using internet domain stream sockets.
// Created: 07/05/1995
// Version: 07/12/2000
//
// Modifications:
//

#if !defined( __CSOCKET_H )
#define __CSOCKET_H         // prevent multiple includes

// include files **********************************************************

#include <string>

#include "dsm/CIPCcsock.H"          // "C" socket interface
#include "dsm/CIPCfiletx.H"         // "C" file transfer interface

namespace ipc {

// class: CSocketError ****************************************************

class CSocketError
{
public :

  typedef std::string string_type;

  enum error_type
  {
    NoError = 0,      // no error
    AcceptError,      // ::accept() failed
    BindError,        // ::bind() failed
    ConnectError,     // ::connect() failed
    CreateError,      // ::socket() failed
    NameError,        // ::getsockname() failed
    ReceiveError,     // ::recv() failed
    SelectError,      // ::select() failed
    SendError,        // ::send() failed
    UnknownHostError  // ::gethostname() failed
  };

  // Constructors
  CSocketError(error_type e = NoError) : err_(e) {}
  CSocketError(const CSocketError& other) : err_(other.err_) {}

  // Destructor
  ~CSocketError(void) {}

  // Return the error type
  error_type getErrorType(void)       { return err_; }
  error_type getErrorType(void) const { return err_; }

  // Return an error message
  virtual string_type message(void) const;

protected :

  error_type  err_; // the error type
};

// class: CSocket *********************************************************

class CSocket
{
public :

  typedef std::string string_type;

  typedef unsigned short  port_type;
  typedef size_t          size_type;

  // Constructors
  CSocket(void) {}
  CSocket(const csocket&) throw (CSocketError);
  CSocket(const CSocket&) throw (CSocketError);

  // Destructor
  virtual ~CSocket(void);

  // Assignment operator
  CSocket& operator = (const CSocket&) throw (CSocketError);

  // Type-cast to csocket *.
  operator const csocket * (void) const { return &sock_; }
  operator csocket *       (void)       { return &sock_; }

  // Type-cast to int -- return the file descriptor.
  operator int (void) const { return fd(); }

  // Return the file descriptor.
  int fd(void) const;

  // Flush all incoming data to this socket.
  // Return the number of bytes flushed.
  // <seconds, microseconds> determines how long to wait
  virtual size_type flush(
    size_type seconds      = 0,
    size_type microseconds = 0) throw (CSocketError);

  // Return the IP address of this socket.  Return "" if there is an error.
  virtual string_type IPAddress(void) const;

  // Check for socket input. Return true if there is input.
  // Return false otherwise.
  // Throw CSocketError::SelectError on error.
  // <seconds, microseconds> determine how long to wait.
  virtual bool poll(
    size_type seconds      = 0,
    size_type microseconds = 0) throw (CSocketError);

  // Return the socket port number.
  virtual port_type portNumber(void) const;
 
  // Receive a message on this socket and return the length of the message
  // received.  If no messages are available at the socket, wait for a
  // message to arrive.
  // The <flags> are the same as used for ::recv()
  // NOTE: <buff> must have at least <nbytes> allocated.
  virtual size_type receive(
    void *    buff,
    size_type nbytes,
    int       flags = 0) throw (CSocketError);
 
  // Send a message on this socket and return the length of the message sent.
  // If no messages space is available at the socket to hold the message
  // to be transmitted, send() blocks until space is available.
  virtual size_type send(const string_type&) throw (CSocketError);
  virtual size_type send(const void * buff, size_type nbytes)
    throw (CSocketError);

protected :

  csocket sock_;  // the socket
};

// class: CClientSocket ***************************************************

class CClientSocket : public CSocket
{
public :

  // Constructors
  CClientSocket(const csocket& s) throw (CSocketError) : CSocket(s) {}
  CClientSocket(
    const string_type& host,
    port_type          port) throw (CSocketError);
  CClientSocket(const CClientSocket& other) throw (CSocketError)
    : CSocket(other) {}

  // Destructor
  virtual ~CClientSocket(void) {}

  // Assignment operator
  CClientSocket& operator = (const CClientSocket& other) throw (CSocketError)
  { CSocket::operator = (other); return *this; }
};

// class: CServerSocket ***************************************************

class CServerSocket : public CSocket
{
public :

  // Constructors
  // NOTE: A port number of 0 selects an unsused socket address
  CServerSocket(port_type = 0) throw (CSocketError);
  CServerSocket(const csocket& s) throw (CSocketError) : CSocket(s) {}
  CServerSocket(const CServerSocket& other) throw (CSocketError)
    : CSocket(other) {}

  // Destructor
  virtual ~CServerSocket(void) {}

  // Assignment operator
  CServerSocket& operator = (const CServerSocket& other) throw (CSocketError)
  { CSocket::operator = (other); return *this; }

  // Wait for a connection to this server and return a new CSocket
  // instance accepted.  If there are no connection requests pending,
  // accept() blocks until one arrives.
  virtual CSocket accept(void) const throw (CSocketError);
};

// class: CFileTX **********************************************************

class CFileTX : private filetx
{
public :

  typedef std::string string_type;

  enum status_type
  {
    CLOSED     = TX_CLOSED,     // Socket connection closed
    ARG_ERR    = TX_ARG_ERR,    // Invalid buffer and/or buffer length
    RECV_ERR   = TX_RECV_ERR,   // Error receiving from the socket
    HEADER_ERR = TX_HEADER_ERR, // Error receiving file header
    LENGTH_ERR = TX_LENGTH_ERR, // Final/original file length did not match
    OPEN_ERR   = TX_OPEN_ERR,   // Error opening file for reading
    WRITE_ERR  = TX_WRITE_ERR,  // Error writing to file
    GOT_HEADER = TX_GOT_HEADER, // Received file header
    WRITE_CONT = TX_WRITE_CONT, // More data to receive
    WRITE_DONE = TX_WRITE_DONE  // Received entire file
  };

  typedef CSocket::size_type  size_type;

  // Constructors
  CFileTX(void) { tx_init(this); }
  CFileTX(const CFileTX&);

  // Destructor
  ~CFileTX(void) { tx_close(this); }

  // Assignment operator
  CFileTX& operator = (const CFileTX&);

  // Return the name of the current/last file being transfered
  string_type fileName(void) const;

  // Receive a transfer from <sock> and return the number of bytes read.
  // Set the state of completion in <status>.
  // Return 0 if <sock> has closed.
  // With the exception of the file header, <buffer> is used to hold
  // the data read from <sock>.
  size_type receive(
    CSocket&     sock,
    void *       buffer,
    size_type    bufLen,
    status_type& status) throw (CSocketError);

  // Send the file <src> over <sock> as <dst> and return the number of
  // bytes sent.  Use the buffer specified by <buffer> and <bufLen> to
  // send the file through the socket.
  // NOTE: If <dst> == "", <src> is used.
  size_type send(
    CSocket&           sock,
    void *             buffer,
    size_type          bufLen,
    const string_type& src,
    const string_type& dst = "") throw (CSocketError);

  // Get/Set user data
  long userData(void) const { return tx_getUserData(this); }
  void userData(long x)     { tx_setUserData(this, x); }
};

} // namespace ipc

#endif   // __CSOCKET_H

// end of CSocket.H ********************************************************

