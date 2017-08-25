/* CIPCfilepacket.H -- File packet for socket file transfers
 * Created: 09/03/1996
 * Version: 11/11/1999
 *
 * Description: 
 *
 *   filepacket.h defines a file header struct used for transfering files
 *   in chunks.  Each "packet" sent could contain a filepacket followed by
 *   the file data (except where noted otherwise).  For example:
 *
 *   -----------------------------------
 *   | header (filepacket instance)    |
 *   -----------------------------------
 *   | file data                       |
 *   |    .                            |
 *   |    .                            |
 *   |    .                            |
 *   |    .                            |
 *   -----------------------------------
 *
 *   The FPK_HEADER and FPK_ABORT packet types should not be followed by any
 *   file data.  A FPK_HEADER type message need specify the file name and
 *   file length to be sent in subsequent packets.  A typical file transfer
 *   is as follows:
 *
 *   filepacket p;
 *   fpk_init(&p);
 *   -- create header
 *   fpk_setType(&p, FPK_HEADER);
 *   fpk_setString(&p, "myfile");
 *   fpk_setBufferLength(&p, <size in bytes of "myfile">);
 *   -- send <p> through socket
 *   fpk_setType(&p, FPK_START);
 *   fpk_setBufferLength(&p, <length of buffer read from "myfile">);
 *   -- Send <p> followed by the first buffer read from "myfile"
 *   fpk_setType(&p, FPK_CONT);
 *   fpk_setBufferLength(&p, <length of buffer read from "myfile">);
 *   -- Send <p> followed by the next buffer read from "myfile"
 *   .
 *   .
 *   .
 *   fpk_setType(FPK_END);
 *   fpk_setBufferLength(<length of buffer read from "myfile">);
 *   -- Send <p> followed by the last buffer read from "myfile"
 *
 * Modifications: 
 */

#if !defined( __FILEPACKET_H_ )
#define __FILEPACKET_H_    /* prevent multiple includes */

extern "C" {

/* struct: filepacket ****************************************************/

/* Length in bytes of internal data */
#define FPK_DATA_LEN   128

typedef char filepacket[FPK_DATA_LEN];

enum fpktype
{
  FPK_KEY,    /* key to validate a new transfer */
  FPK_HEADER, /* file header (before transfer) */
  FPK_START,  /* first packet in a transfer follows */
  FPK_CONT,   /* continued packet in a transfer follows */
  FPK_END,    /* last packet in a transfer follows */
  FPK_ABORT   /* abort the current transfer */
};

/* fpk_getBufferLength **************************************************/

extern unsigned long fpk_getBufferLength(const filepacket p);
/* Pre: Assigned(p)
 * Modifies:
 * Post: Get the buffer length specification in <p>.
 */

/* fpk_getString ********************************************************/

extern const char * fpk_getString(const filepacket p);
/* Pre: Assigned(p)
 * Modifies:
 * Post: Get the string data in <p>.
 * NOTE: This should be the key for FPK_KEY messages and the
 *     file name for other messages.
 */

/* fpk_getType **********************************************************/

extern short fpk_getType(const filepacket p);
/* Pre: Assigned(p)
 * Modifies:
 * Post: Get the packet type in <p>.
 */

/* fpk_init *************************************************************/

extern void fpk_init(filepacket p);
/* Pre: Assigned(p)
 * Modifies:
 * Post: Initialize the buffer length, string, and type in <p>.
 */

/* fpk_setBufferLength **************************************************/

extern void fpk_setBufferLength(
  filepacket    p,
  unsigned long n);
/* Pre: Assigned(p), Assigned(n)
 * Modifies:
 * Post: Set the buffer length specification in <p> to <n>.
 */

/* fpk_setString ********************************************************/

extern void fpk_setString(
  filepacket   p,
  const char * s);
/* Pre: Assigned(p), Assigned(s)
 * Modifies:
 * Post: Set the string data in <p> to <s>.
 * NOTE: This should be the key for FPK_KEY messages and the file name
 *     for other messages.
 */

/* fpk_setType **********************************************************/

extern void fpk_setType(
  filepacket p,
  fpktype    t);
/* Pre: Assigned(p), Assigned(t)
 * Modifies:
 * Post: Get the packet type in <p> to <t>.
 */

} // extern "C"

#endif  /* __CIPCFILEPACKET_H_ */

/* end of CIPCfilepacket.H ***********************************************/

