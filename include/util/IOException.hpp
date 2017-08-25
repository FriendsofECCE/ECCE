/**
 * @file 
 *
 *
 */
#ifndef IOEXCEPTION_HH
#define IOEXCEPTION_HH

#include "util/EcceException.H"

/**
 * IOException indicates some IO failure.
 * IO failures can pertain to files, sockets, etc.
 */
class IOException : public EcceException
{
   public:

      IOException(const string& message, const char *file, int line);
      IOException(const char *message, const char *file, int line);
      IOException(const IOException& rhs);

      virtual ~IOException() throw ();

   protected:

      IOException();

}; 

#endif
