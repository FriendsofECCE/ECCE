/**
 * @file 
 *
 *
 */
#ifndef NULLPOINTEREXCEPTION_HH
#define NULLPOINTEREXCEPTION_HH

#include "util/EcceException.H"


/**
 * Indicatates an unexpected NULL pointer was encountered. 
 * Often used to check argument values but is applicable in other situations.
 */
class NullPointerException : public EcceException
{
   public:

      NullPointerException(const string& message, const char *file, int line);
      NullPointerException(const char *message, const char *file, int line);
      NullPointerException(const NullPointerException& rhs);


      virtual ~NullPointerException() throw ();

   protected:

      NullPointerException();


}; 

#ifdef INSTALL
#define  NULLPOINTEREXCEPTION(expr, txt)
#else
#define  NULLPOINTEREXCEPTION(expr, txt) \
if ( !(expr) ) \
{ \
  throw NullPointerException(txt, __FILE__, __LINE__); \
}
#endif /* INSTALL */

#endif
