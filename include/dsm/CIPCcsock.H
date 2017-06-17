/* CIPCcsock.H
 * Created: 09/27/1996
 * Version: 07/18/2000
 *
 * Description: 
 *
 *   CIPCcsock.H defines C socket struct "csocket" and routines using
 *   internet domain stream sockets.
 *
 * Modifications:
 */

#if !defined( __CIPCCSOCK_H_ )
#define __CIPCCSOCK_H_    /* prevent multiple includes */

/* include files *********************************************************/

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

extern "C" {

/* struct: csocket *******************************************************/

struct csocket
{
  int                 sock_;  /* the socket fd */
  struct sockaddr_in  addr_;  /* socket address */
};

/* csocket_accept ********************************************************/

extern csocket * csocket_accept(csocket * s);
/* Pre: Assigned(s)
 * Modifies:
 * Post: Wait for a connection to <s> and return a new csocket instance
 *       accepted.  Return 0 if there was an error.  If there are
 *       no connection requests pending, csocket_accept() blocks until one
 *       arrives.
 * NOTE: The caller is responsible for deleting the return value.
 */

/* csocket_copy **********************************************************/

extern int csocket_copy(csocket * dst, const csocket * src);
/* Pre: Assigned(dst), Assigned(src)
 * Modifies: <*dst>
 * Post: Copy <*src> to <*dst> and return 1.
 *       Return 0 if the copy failed.
 */

/* csocket_close *********************************************************/

extern int csocket_close(csocket * s);
/* Pre: Assigned(s)
 * Modifies:
 * Post: Close the socket in <s>.
 */

/* csocket_fd ************************************************************/

extern int csocket_fd(const csocket * s);
/* Pre: Assigned(s)
 * Modifies:
 * Post: Return the file descriptor in <s>.
 */

/* csocket_IPAddress *****************************************************/

extern const char * csocket_IPAddress(const csocket * s);
/* Pre: Assigned(s)
 * Modifies:
 * Post: Return the IP address of <s>.  Return 0 if there is an error.
 * NOTE: The return value points to static information which is
 *       overwritten in each call.
 */

/* csocket_open **********************************************************/

extern int csocket_open(csocket * s);
/* Pre: Assigned(s)
 * Modifies:
 * Post: Create the socket in <s>.
 */

/* csocket_openClient ****************************************************/

extern int csocket_openClient(
  csocket *      s,
  const char *   host,
  unsigned short port);
/* Pre: Assigned(s), Assigned(host), Assigned(port),
 * Modifies:
 * Post: Create a new socket in <s> connected to <host, port> and return 1.
 *       Return 0 if there was an error.
 * NOTE: csocket_open() is used to create the socket.
 */

/* csocket_openServer ****************************************************/

extern int csocket_openServer(
  csocket *      s,
  unsigned short port);
/* Pre: Assigned(s), Assigned(host), Assigned(port),
 * Modifies:
 * Post: Create a new server socket in <s> on <port> and return 1.
 *       Return 0 if there was an error.  A <port> of 0 selects an
 *       unsused socket address.
 * NOTE: csocket_open() is used to create the socket.
 */

/* csocket_poll **********************************************************/

extern int csocket_poll(
  csocket * s,
  long      seconds,
  long      microseconds);
/* Pre: Assigned(s), Assigned(seconds), Assigned(microseconds)
 * Modifies:
 * Post: Check for socket input on <s>. Return 1 if there is input.
 *       Return 0 if poll timed out, -1 on error.
 *       <seconds> and <microseconds> determine how long to wait.
 */

/* csocket_portNumber ****************************************************/

extern unsigned short csocket_portNumber(const csocket * s);
/* Pre: Assigned(s)
 * Modifies:
 * Post: Return the socket port number in <s>
 */

/* csocket_receive *******************************************************/

extern long csocket_receive(
  csocket * s,
  void *    buff,
  unsigned  nbytes,
  int       flags);
/* Pre: Assigned(s), Assigned(buff), Assigned(nbytes), Assigned(flags)
 * Modifies: <buff>
 * Post: Receive a message on <s> and return the length of the message
 *       received.  Return -1 if an error occurred.
 *       If no messages are available at the socket, wait for a message
 *       to arrive.
 *       The <flags> are the same as used for ::recv()
 * NOTE: <buff> must have at least <nbytes> allocated.
 */

/* csocket_send **********************************************************/

extern long csocket_send(
  csocket *    s,
  const void * buff,
  long         nbytes);
/* Pre: Assigned(s), Assigned(buff), Assigned(nbytes)
 * Modifies:
 * Post: Send a message through <s> and return the length of the message
 *       sent.  Return -1 f an error occurred.
 *       If no messages space is available at the socket to hold the
 *       message to be transmitted, send() blocks until space is available.
 *       If <nbytes> is negative, <buff> is assumed to be a null-terminated
 *       string.
 */

} // extern "C"

#endif  /* __CIPCCSOCK_H_ */

/* end of CIPCcsock.H ****************************************************/

