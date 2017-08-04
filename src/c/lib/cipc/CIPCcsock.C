/* CIPCcsock.C
 * Created: 09/27/1996
 * Version: 05/12/2000
 *
 * Description: 
 *
 * Modifications:
 *
 */

/* include files *********************************************************/

#include <stdio.h>      /* for fprintf() */
#include <sys/time.h>   /* for timeval */
#include <sys/select.h> /* for select(), ... */
#include <stdlib.h>     /* for free(), malloc() */

#include "dsm/CIPCcsock.H"

/* csocket_accept ********************************************************/

csocket * csocket_accept(csocket * s)
/* Pre: Assigned(s)
 * Modifies:
 * Post: Wait for a connection to <s> and return a new csocket instance
 *       accepted.  Return 0 if there was an error.  If there are
 *       no connection requests pending, csocket_accept() blocks until one
 *       arrives.
 * NOTE: The caller is responsible for deleting the return value.
 */
{
  csocket              *result = 0;
  struct sockaddr_in   addr;
  socklen_t            addr_len = sizeof(addr);
  int                  sockfd;

  if (!s)
    return result;

  /* Try to accept a connection */
  sockfd = accept(s->sock_, (struct sockaddr *) &addr, &addr_len);
  if (sockfd == -1)
  {
#ifdef DEBUG
    fprintf(stderr, "csocket_accept(): error accept()\n");
#endif
  }
  else
  {
    /* Get socket name info */
    getpeername(sockfd, (struct sockaddr *) &addr, &addr_len);

    /* Create a csocket to return */
    result = (csocket *) malloc(sizeof(csocket));
    result->sock_ = sockfd;
    memcpy(&(result->addr_), &addr, addr_len);
  }

  return result;
}

/* csocket_copy **********************************************************/

int csocket_copy(csocket * dst, const csocket * src)
/* Pre: Assigned(dst), Assigned(src)
 * Modifies: <*dst>
 * Post: Copy <*src> to <*dst> and return 1.
 *       Return 0 if the copy failed.
 */
{
  if (!dst || !src)
    return 0;

  /* dup() socket fd */
  dst->sock_ = dup(src->sock_);
  if (dst->sock_ == -1)
    return 0;

  /* Copy socket address */
  memcpy(&(dst->addr_), &(src->addr_), sizeof(struct sockaddr_in));

  return 1;
}

/* csocket_close *********************************************************/

int csocket_close(csocket * s)
/* Pre: Assigned(s)
 * Modifies:
 * Post: Close the socket in <s>.
 */
{
  if (!s)
    return 0;

  close(s->sock_);

  return 1;
}

/* csocket_fd ************************************************************/

int csocket_fd(const csocket * s)
/* Pre: Assigned(s)
 * Modifies:
 * Post: Return the file descriptor in <s>.
 */
{
  return s ? s->sock_ : -1;
}

/* csocket_IPAddress *****************************************************/

const char * csocket_IPAddress(const csocket * s)
/* Pre: Assigned(s)
 * Modifies:
 * Post: Return the IP address of <s>.  Return 0 if there is an error.
 * NOTE: The return value points to static information which is
 *       overwritten in each call.
 */
{
  return s ? inet_ntoa(s->addr_.sin_addr) : ((const char *) 0);
}

/* csocket_open **********************************************************/

int csocket_open(csocket * s)
/* Pre: Assigned(s)
 * Modifies:
 * Post: Create the socket in <s>.
 */
{
  if (!s)
    return 0;

  s->sock_ = socket(AF_INET, SOCK_STREAM, 0);
  if (s->sock_ == -1)
  {
#ifdef DEBUG
    fprintf(stderr, "csocket_open(): error creating socket\n");
#endif
    return 0;
  }
  memset(&(s->addr_), 0, sizeof(s->addr_));

  return 1;
}

/* csocket_openClient ****************************************************/

int csocket_openClient(
  csocket *      s,
  const char *   host,
  unsigned short port)
/* Pre: Assigned(s), Assigned(host), Assigned(port),
 * Modifies:
 * Post: Create a new socket in <s> connected to <host, port> and return 1.
 *       Return 0 if there was an error.
 * NOTE: csocket_open() is used to create the socket.
 */
{
  int            rval = 0;
  socklen_t      addr_len;
  struct hostent *hp;
  if (!csocket_open(s))
    return rval;

  hp = gethostbyname(host);
  if (hp == 0)
  {
#ifdef DEBUG
    fprintf(stderr,
            "csocket_openClient(): error unknown host: %s\n",
            host);
#endif
    return rval;
  }

  /* Set up the address for an internet socket */
  s->addr_.sin_family = AF_INET;
  memcpy(&(s->addr_.sin_addr), hp->h_addr, hp->h_length);
  s->addr_.sin_port = htons(port);

  /* Try to connect to the server */
  rval = connect(s->sock_, (struct sockaddr *) &(s->addr_), sizeof(s->addr_));
  if (rval == -1)
  {
#ifdef DEBUG
    fprintf(stderr, "csocket_openClient(): error connect\n");
#endif
    rval = 0;
  }
  else
  {
    /* Get socket name info */
    addr_len = sizeof(s->addr_);
    getsockname(s->sock_, (struct sockaddr *) &(s->addr_), &addr_len);
    rval = 1;
  }

  return rval;
}

/* csocket_openServer ****************************************************/

int csocket_openServer(
  csocket *      s,
  unsigned short port)
/* Pre: Assigned(s), Assigned(host), Assigned(port),
 * Modifies:
 * Post: Create a new server socket in <s> on <port> and return 1.
 *       Return 0 if there was an error.  A <port> of 0 selects an
 *       unsused socket address.
 * NOTE: csocket_open() is used to create the socket.
 */
{
  int on   = 1,
      rval = 0;
  socklen_t addr_len;

  if (!csocket_open(s))
    return rval;

  s->addr_.sin_family      = AF_INET;
  s->addr_.sin_addr.s_addr = htonl(INADDR_ANY);
  s->addr_.sin_port        = port;

  /* Let the socket be reused right away */
  setsockopt(s->sock_, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof(on));
  if (bind(s->sock_, (struct sockaddr *) &(s->addr_), sizeof(s->addr_) )
      == -1)
  {
#ifdef DEBUG
    fprintf(stderr, "csocket_openServer(): error bind()\n");
#endif
    csocket_close(s);
    return 0;
  }
  else
  {
    /* Get socket name info */
    addr_len = sizeof(s->addr_);
    getsockname(s->sock_, (struct sockaddr *) &(s->addr_), &addr_len);

    /* Listen for messages */
    listen(s->sock_, 5);

    rval = 1;
  }

  return rval;
}

/* csocket_poll **********************************************************/

int csocket_poll(
  csocket * s,
  long      seconds,
  long      microseconds)
/* Pre: Assigned(s), Assigned(seconds), Assigned(microseconds)
 * Modifies:
 * Post: Check for socket input on <s>. Return 1 if there is input.
 *       Return 0 otherwise.
 *       <seconds> and <microseconds> determine how long to wait.
 */
{
#if 000
  time_t now;
#endif
  fd_set         readmask;
  struct timeval waitTime;
  int            rval = 0;

  if (!s)
    return rval;

  if (seconds < 0 || microseconds < 0)
  /* We can't wait "backwards" */
    return rval;

  waitTime.tv_sec  = seconds;
  waitTime.tv_usec = microseconds;

  /* Fill the set */
  FD_ZERO(&readmask);
  FD_SET(s->sock_, &readmask);

#if 000
  now = time(NULL);
  printf("**** CIPC csocket_poll before select time: %s\n", ctime(&now));
#endif

  /* Check if we have a message */
  rval = select(
    (size_t) FD_SETSIZE,
    &readmask,
    (fd_set *) 0,
    (fd_set *) 0,
    &waitTime);

#if 000
  now = time(NULL);
  printf("**** CIPC csocket_poll after select time: %s\n", ctime(&now));
#endif

  if (rval == -1)
  {
#ifdef DEBUG
    fprintf(stderr, "csocket_poll(): error returned from select()\n");
#endif
    return rval;
  }

  return FD_ISSET(s->sock_, &readmask) != 0;
}

/* csocket_portNumber ****************************************************/

unsigned short csocket_portNumber(const csocket * s)
/* Pre: Assigned(s)
 * Modifies:
 * Post: Return the socket port number in <s>
 */
{
  return s ? ntohs(s->addr_.sin_port) : 0;
}

/* csocket_receive *******************************************************/

long csocket_receive(
  csocket * s,
  void *    buff,
  unsigned  nbytes,
  int       flags)
/* Pre: Assigned(s), Assigned(buff), Assigned(nbytes), Assigned(flags)
 * Modifies: <buff>
 * Post: Receive a message on <s> and return the length of the message
 *       received.  Return -1 if an error occurred.
 *       If no messages are available at the socket, wait for a message
 *       to arrive.
 *       The <flags> are the same as used for ::recv()
 * NOTE: <buff> must have at least <nbytes> allocated.
 */
{
  long  rval = -1;
  if (s)
  {
    rval = recv(s->sock_, buff, nbytes, flags);
#ifdef DEBUG
    if (rval == -1)
      fprintf(stderr, "csocket_receive(): error returned from recv()\n");
#endif
  }

  return rval;
}

/* csocket_send **********************************************************/

long csocket_send(
  csocket *    s,
  const void * buff,
  long         nbytes)
/* Pre: Assigned(s), Assigned(buff), Assigned(nbytes)
 * Modifies:
 * Post: Send a message through <s> and return the length of the message
 *       sent.  Return -1 f an error occurred.
 *       If no messages space is available at the socket to hold the
 *       message to be transmitted, send() blocks until space is available.
 *       If <nbytes> is negative, <buff> is assumed to be a null-terminated
 *       string.
 */
{
  long  rval,
        offset;
  if (s)
  {
    if (nbytes < 0)
    {
      if (!buff)
        return 0;   /* no message to send */

      /* Save string length + null terminator */
      nbytes = strlen((const char *) buff) + 1;
    }

    offset = 0;
    while (offset != nbytes)
    {
      rval = send(
        s->sock_,
        (const void *) ((const char *) buff + offset),
        nbytes - offset,
        0);
        if (rval == -1)
        {
#ifdef DEBUG
          fprintf(stderr, "csocket_send(): error returned from send()\n");
#endif
          return rval; 
        }
        offset += rval;
     }
  }

  return offset;
}

/* end of CIPCcsock.C ****************************************************/

