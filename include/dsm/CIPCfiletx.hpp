/* CIPCfiletx.h
 * Created: 08/29/1996
 * Version: 03/25/1999
 *
 * Description: 
 *
 *   CIPCfiletx.h defines a stucture "filetx" and functions for transferring
 *   files through sockets.
 *
 * Modifications:
 */

#if !defined( __CIPCFILETX_H_ )
#define __CIPCFILETX_H_    /* prevent multiple includes */

/* include files *********************************************************/

#include "dsm/CIPCcsock.H"       /* for csocket */
#include "dsm/CIPCfilepacket.H"  /* for filepacket */

extern "C" {

/* struct: filetx *********************************************************/

struct filetx
{
  long        userData_;  /* User specific data */
  filepacket  header_;    /* Header for file being received */
  int         ofd_;       /* File descriptor for file being received */
  size_t      len_,       /* Length in bytes of file being received */
              nRead_;     /* Number of bytes read so far in file */
                          /* being received */
};

enum  /* tx_receive() status */
{
  TX_CLOSED,              /* Socket connection closed */
  TX_ARG_ERR,             /* Invalid buffer and/or buffer length */
  TX_RECV_ERR,            /* Error receiving from the socket */
  TX_HEADER_ERR,          /* Error receiving file header */
  TX_LENGTH_ERR,          /* Final/original file length did not match */
  TX_OPEN_ERR,            /* Error opening file for reading */
  TX_WRITE_ERR,           /* Error writing to file */
  TX_GOT_HEADER,          /* Received file header */
  TX_WRITE_CONT,          /* More data to receive */
  TX_WRITE_DONE           /* Received entire file */
};

/* tx_init ****************************************************************/

extern void tx_init(filetx * tx);
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Initialize <tx>
 */

/* tx_close ***************************************************************/

extern void tx_close(filetx * tx);
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Close tx->ofd_
 */

/* tx_fileName ************************************************************/

extern const char * tx_fileName(const filetx * tx);
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Return the name of the current/last file being received in <tx>.
 */

/* tx_getUserData *********************************************************/

extern long tx_getUserData(const filetx * tx);
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Get user specified data in <tx>.
 */

/* tx_receive *************************************************************/

extern long tx_receive(
  filetx *  tx,
  csocket * sock,
  void *    buffer,
  size_t    bufLen,
  int *     status);
/* Pre: Assigned(tx), Assigned(sock), Assigned(buffer), Assigned(bufLen),
 *    Assigned(status)
 * Modifies: <*status>
 * Post: Receive a transfer in <tx> from <sock> and return the number
 *       of bytes read.  Set the state of completion in <status>.
 *       Return 0 if <sock> has closed.  Return -1 if there was an error.
 *       With the exception of the file header, <buffer> is used to hold
 *       the data read from <sock>.
 */

/* tx_send ****************************************************************/

extern long tx_send(
  filetx *     tx,
  csocket *    sock,
  void *       buffer,
  size_t       bufLen,
  const char * src,
  const char * dst);
/* Pre: Assigned(tx), Assigned(sock), Assigned(buffer), Assigned(bufLen),
 *      Assigned(src), Assigned(dst)
 * Modifies: <*status>
 * Post: Send the file <src> over <sock> as <dst> and return the number
 *       of bytes sent.  Return -1 if there was an error.
 *       Use the buffer specified by <buffer> and <bufLen> to send the file
 *       through the socket.
 * NOTE: If <dst> == 0, <src> is used.
 */

/* tx_setUserData *********************************************************/

extern void tx_setUserData(
  filetx * tx,
  long     x);
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Set user specified data in <tx> to <x>.
 */

} // extern "C"

#endif	/* __CIPCFILETX_H_ */

/* end of CIPCfiletx.h ****************************************************/

