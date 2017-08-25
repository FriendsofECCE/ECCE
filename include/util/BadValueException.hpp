/**
 * @file 
 *
 *
 */
#ifndef BADVALUEEXCEPTION_HH
#define BADVALUEEXCEPTION_HH

#include "util/EcceException.H"


/**
 * BadValueException usually signals that an argument has an invalid value.
 * IndexOutOfRangeException should be used for index arguments while 
 * BadValueException can be used for strings, doubles, enums etc.
 */
class BadValueException : public EcceException
{
   public:

      BadValueException(const string& message, const char *file, int line);
      BadValueException(const char *message, const char *file, int line);
      BadValueException(const BadValueException& rhs);


      virtual ~BadValueException() throw ();

   protected:

      BadValueException();


}; 

#ifdef INSTALL
#define  BADVALUEEXCEPTION(expr, txt)
#else
#define  BADVALUEEXCEPTION(expr, txt) \
if ( !(expr) ) \
{ \
  throw BadValueException(txt, __FILE__, __LINE__); \
}
#endif /* INSTALL */

#endif
