/**
 * @file 
 *
 *
 */
#ifndef DAVEXCEPTION_HH
#define DAVEXCEPTION_HH

#include "util/EcceException.H"

/**
 * DavException indicates some failure with a WebDav operation.
 */
class DavException : public EcceException
{
   public:

      DavException(const string& message, const char *file, int line);
      DavException(const char *message, const char *file, int line);
      DavException(const DavException& rhs);

      virtual ~DavException() throw ();

   protected:

      DavException();

}; 

#endif
