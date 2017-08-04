/* CIPCfilepacket.C -- File packet for socket file transfers
 * Created: 09/03/1996
 * Version: 11/11/1999
 *
 * Description: 
 *
 * Modifications:
 */

/* include files *********************************************************/

#include <string.h>        /* for memcpy(), memset() */

#include "dsm/CIPCcsock.H"
#include "dsm/CIPCfilepacket.H"

/* Some macros ***********************************************************/

#define FPKLENGTH (p + sizeof(char))
#define FPKSTRING (p + sizeof(char) + sizeof(unsigned long))
#define FPKTYPE   (p)

/* fpk_getBufferLength ***************************************************/

unsigned long fpk_getBufferLength(const filepacket p)
/* Pre: Assigned(p)
 * Modifies:
 * Post: Get the buffer length specification in <p>.
 */
{
  return ntohl((unsigned long) *((unsigned long *) FPKLENGTH));
}

/* fpk_getString *********************************************************/

const char * fpk_getString(const filepacket p)
/* Pre: Assigned(p)
 * Modifies:
 * Post: Get the string data in <p>.
 * NOTE: This should be the key for FPK_KEY messages and the file name
 *       for other messages.
 */
{
  return (const char *) FPKSTRING;
}

/* fpk_getType ***********************************************************/

short fpk_getType(const filepacket p)
/* Pre: Assigned(p)
 * Modifies:
 * Post: Get the packet type in <p>.
 */
{
  return (short) *((char *) FPKTYPE);
}

/* fpk_init **************************************************************/

void fpk_init(filepacket p)
/* Pre: Assigned(p)
 * Modifies:
 * Post: Initialize the buffer length, string, and type in <p>.
 */
{
  memset(p, 0, FPK_DATA_LEN);
}

/* fpk_setBufferLength *************************************************/

void fpk_setBufferLength(
  filepacket    p,
  unsigned long n)
/* Pre: Assigned(p), Assigned(n)
 * Modifies:
 * Post: Set the buffer length specification in <p> to <n>.
 */
{
  n = htonl(n);
  memcpy(FPKLENGTH, (const void *) &n, sizeof(unsigned long));
}

/* fpk_setString *********************************************************/

void fpk_setString(
  filepacket   p,
  const char * s)
/* Pre: Assigned(p), Assigned(s)
 * Modifies:
 * Post: Set the string data in <p> to <s>.
 * NOTE: This should be the key for FPK_KEY messages and the file name
 *       for other messages.
 */
{
  if (s)
    strcpy(FPKSTRING, s);
}

/* fpk_setType ***********************************************************/

void fpk_setType(
  filepacket p,
  fpktype    t)
/* Pre: Assigned(p), Assigned(t)
 * Modifies:
 * Post: Get the packet type in <p> to <t>.
 */
{
  char  n = (char) t;
  memcpy(FPKTYPE, (const void *) &n, sizeof(char));
}

/* end of CIPCfilepacket.C ***********************************************/

