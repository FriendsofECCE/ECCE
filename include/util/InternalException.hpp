/**
 * @file 
 *
 *
 */
#ifndef INTERNALEXCEPTION_HH
#define INTERNALEXCEPTION_HH

#include "util/EcceException.H"


/**
 * Indicates that some sort of internal program error has occurred.
 * Use more specific exceptions when available such as NullPointerException,
 * MismatchedTypeException, IndexOutOfRangeException, or BadValueException.
 */
class InternalException : public EcceException
{
   public:

      InternalException(const string& message, const char *file, int line);
      InternalException(const char *message, const char *file, int line);
      InternalException(const InternalException& rhs);


      virtual ~InternalException() throw ();

   protected:

      InternalException();


}; 

#ifdef INSTALL
#define  INTERNALEXCEPTION(expr, txt)
#else
#define  INTERNALEXCEPTION(expr, txt) \
if ( !(expr) ) \
{ \
  throw InternalException(txt, __FILE__, __LINE__); \
}
#endif /* INSTALL */

#endif
