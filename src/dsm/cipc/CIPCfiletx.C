/* CIPCfiletx.C
 * Created: 08/29/1996
 * Version: 03/25/1999
 *
 * Description: 
 *
 *   CIPCfiletx.c implements functions for transferring files through sockets.
 *
 * Modifications:
 *
 */

/* include files *********************************************************/

#include <unistd.h>     /* for close(), open() */
#include <sys/stat.h>   /* for stat() */
#include <stdio.h>      /* for fprintf() */
#include <fcntl.h>      /* for O_CREAT, O_RDONLY, O_WRONLY */

#include "dsm/CIPCfiletx.H"

/* tx_close **************************************************************/

void tx_close(filetx * tx)
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Close tx->ofd_
 */
{
  if (tx)
  {
    if (tx->ofd_ != -1)
      close(tx->ofd_);
  }
}

/* tx_fileName ***********************************************************/

const char * tx_fileName(const filetx * tx)
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Return the name of the current/last file being received in <tx>.
 */
{
  return tx ? fpk_getString(tx->header_) : ((const char *) 0);
}

/* tx_init ***************************************************************/

void tx_init(filetx * tx)
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Initialize <tx>
 */
{
  if (tx)
  {
    tx->userData_ = 0L;
    fpk_init(tx->header_);
    tx->ofd_ = -1;
    tx->len_ = 0;
    tx->nRead_ = 0;
  }
}

/* tx_getUserData ********************************************************/

long tx_getUserData(const filetx * tx)
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Get user specified data in <tx>.
 */
{
  return tx ? tx->userData_ : 0;
}

/* tx_receive ************************************************************/

long tx_receive(
  filetx *  tx,
  csocket * sock,
  void *    buffer,
  size_t    bufLen,
  int *     status)
/* Pre: Assigned(tx), Assigned(sock), Assigned(buffer), Assigned(bufLen),
 *      Assigned(status)
 * Modifies: <*status>
 * Post: Receive a transfer in <tx> from <sock> and return the number
 *       of bytes read.  Set the state of completion in <status>.
 *       Return 0 if <sock> has closed.  Return -1 if there was an error.
 *       With the exception of the file header, <buffer> is used to hold
 *       the data read from <sock>.
 */
{
  long    rlen,
          n,
          nLeft;

  if (!tx || !buffer || !bufLen || !status)
  /* Bad arguments */
  {
    *status = TX_ARG_ERR;
    return -1;
  }

  if (!tx->len_)
  /* New file */
  {
    rlen = csocket_receive(sock, tx->header_, FPK_DATA_LEN, 0);
    if (rlen <= 0)
    {
      *status = rlen < 0 ? TX_RECV_ERR : TX_CLOSED;
      return rlen;
    }

    /* Save file length. */
    tx->len_ = fpk_getBufferLength(tx->header_);
    if (!tx->len_)
    /* Error getting file header */
    {
      *status = TX_HEADER_ERR;
      tx->len_ = 0;
      return rlen;
    }

    /* Open file descriptor */
    tx->ofd_ = open(tx_fileName(tx), O_WRONLY | O_CREAT, 0666);
    if (tx->ofd_ == -1)
    /* Error opening fileName for writing */
    {
      *status = TX_OPEN_ERR;
      tx->len_ = 0;
      return rlen;
    }

    tx->nRead_ = 0;
    *status = TX_GOT_HEADER;
#ifdef DEBUG
    fprintf(stderr, "tx_receive() got header. %ld bytes to go\n",
               tx->len_);
#endif
    return FPK_DATA_LEN;
  }

  /* Read a chunk from the sent file */
  rlen = csocket_receive(sock, buffer, bufLen, 0);
  if (rlen <= 0)
  {
    *status = TX_RECV_ERR;
    return rlen;
  }

  /* Write a chunk to tx->ofd_ */
  n    = rlen,
  nLeft = tx->len_ - tx->nRead_;
  if (nLeft < n)
    n = nLeft;
#ifdef DEBUG
  fprintf(stderr, "tx_receive() got %ld bytes %ld bytes to go\n",
              rlen, nLeft - n);
#endif
  if (write(tx->ofd_, buffer, n) != n)
  /* Error writing to file */
  {
    *status = TX_WRITE_ERR;
    close(tx->ofd_);
    tx->ofd_ = -1;
    tx->len_ = tx->nRead_ = 0;
    return rlen;
  }
  tx->nRead_ += n;

  /* Check if we're done */
  if (tx->nRead_ == tx->len_)
  /* Done recieving file */
  {
    *status = TX_WRITE_DONE;
    close(tx->ofd_);
    tx->ofd_ = -1;
    tx->len_ = tx->nRead_ = 0;
    return rlen;
  }
  if (tx->nRead_ > tx->len_)
  /* Final file length did not match the original */
  {
    *status = TX_LENGTH_ERR;
    close(tx->ofd_);
    tx->ofd_ = -1;
    tx->len_ = tx->nRead_ = 0;
    return rlen;
  }

  *status = TX_WRITE_CONT;

  return rlen;
}

/* tx_send ***************************************************************/

long tx_send(
  filetx *     tx,
  csocket *    sock,
  void *       buffer,
  size_t       bufLen,
  const char * src,
  const char * dst)
/* Pre: Assigned(tx), Assigned(sock), Assigned(buffer), Assigned(bufLen),
 *    Assigned(src), Assigned(dst)
 * Modifies: <*status>
 * Post: Send the file <src> over <sock> as <dst> and return the number
 *       of bytes sent.  Return -1 if there was an error.
 *       Use the buffer specified by <buffer> and <bufLen> to send the file
 *       through the socket.
 * NOTE: If <dst> == 0, <src> is used.
 */
{
  int         ifd;
  long        n,
              result = 0;
  struct stat sBuf;
  off_t       len;
  filepacket  p;

  if (!tx || !sock || !buffer || !bufLen || !src)
    return 0;

  if (!dst || !*dst)
    dst = src;

  ifd = open(src, O_RDONLY);
  if (ifd == -1)
    return 0;

  /* Get file length */
  if (stat(src, &sBuf) != 0)
    return 0;
  len = sBuf.st_size;

  /* Send header to server */
  fpk_init(p);
  fpk_setType(p, FPK_HEADER);
  fpk_setString(p, dst);
  fpk_setBufferLength(p, len);
  if ((n = csocket_send(sock, p, FPK_DATA_LEN)) <= 0)
  {
    close(ifd);
    return n == 0 ? 0 : -1;
  }

  /* Send file */
  while (len > 0)
  {
    n = read(ifd, buffer, bufLen);
    if (n <= 0 || (n = csocket_send(sock, buffer, n)) <= 0)
    {
      close(ifd);
      return n == 0 ? 0 : -1;
    }
    result += n;
    len    -= n;
  }

  close(ifd);

  return result;
}

/* tx_setUserData ********************************************************/

void tx_setUserData(
  filetx * tx,
  long     x)
/* Pre: Assigned(tx)
 * Modifies:
 * Post: Set user specified data in <tx> to <x>.
 */
{
  if (tx)
    tx->userData_ = x;
}

/* end of CIPCfiletx.C ***************************************************/

