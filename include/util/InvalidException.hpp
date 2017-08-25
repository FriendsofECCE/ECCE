/**
 * @file 
 *
 *
 */
#ifndef INVALIDEXCEPTION_HH
#define INVALIDEXCEPTION_HH

#include "util/EcceException.H"

/**
 * InvalidException can be used to indicate invalid arguments to a request
 * or a request that cannot be performed because the object is in an
 * invalid state or invalid data was detected.
 */
class InvalidException : public EcceException
{
   public:

      InvalidException(const string& message, const char *file, int line);
      InvalidException(const char *message, const char *file, int line);
      InvalidException(const InvalidException& rhs);

      virtual EcceException *clone() const throw ();

      virtual ~InvalidException() throw ();

   protected:

      InvalidException();


}; 

#ifdef INSTALL
#define INVALIDEXCEPTION(expr, txt)
#else
#define INVALIDEXCEPTION(expr, txt) \
if ( !(expr) ) \
{ \
  throw InvalidException(txt, __FILE__, __LINE__); \
}
#endif // INSTALL

#endif
