/**
 * @file 
 *
 *
 */
#ifndef MISMATCHEDTYPEEXCEPTION_HH
#define MISMATCHEDTYPEEXCEPTION_HH

#include "util/EcceException.H"


/**
 * MismatchedTypeException indicates the caller supplied the wrong type of 
 * parameter.  This can occur when performing dynamic casts or when
 * making use of unions or other constructs such as the "Any" style object.
 */
class MismatchedTypeException : public EcceException
{
   public:

      MismatchedTypeException(const string& message, const char *file,int line);
      MismatchedTypeException(const char *message, const char *file, int line);
      MismatchedTypeException(const MismatchedTypeException& rhs);

      virtual ~MismatchedTypeException() throw ();

   protected:

      MismatchedTypeException();


}; 

#ifdef INSTALL
#define  MISMATCHEDTYPEEXCEPTION(expr, txt)
#else
#define  MISMATCHEDTYPEEXCEPTION(expr, txt) \
if ( !(expr) ) \
{ \
  throw MismatchedTypeException(txt, __FILE__, __LINE__); \
}
#endif /* INSTALL */

#endif
