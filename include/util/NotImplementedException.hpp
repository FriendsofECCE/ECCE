/**
 * @file 
 *
 *
 */
#ifndef NOTIMPLEMENTEDEXCEPTION_HH
#define NOTIMPLEMENTEDEXCEPTION_HH

#include "util/EcceException.H"


/**
 * NotImplementedException can be used to make it plainly obvious that
 * the caller invoked a method that is not completely implemented
 * or is otherwise unsafe to use.  While it is admittedly bad practice to
 * leave code partially completed, it does happen and this could save time
 * tracking down the problem.
 */
class NotImplementedException : public EcceException
{
   public:

      NotImplementedException(const string& message, const char *file,int line);
      NotImplementedException(const char *message, const char *file, int line);
      NotImplementedException(const NotImplementedException& rhs);


      virtual ~NotImplementedException() throw ();

   protected:

      NotImplementedException();


}; 

#ifdef INSTALL
#define  NOTIMPLEMENTEDEXCEPTION(expr, txt)
#else
#define  NOTIMPLEMENTEDEXCEPTION(expr, txt) \
if ( !(expr) ) \
{ \
  throw NotImplementedException(txt, __FILE__, __LINE__); \
}
#endif /* INSTALL */

#endif
