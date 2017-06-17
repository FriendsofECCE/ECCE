/**
 * @file 
 *
 *
 */
#ifndef RESOURCENOTFOUNDEXCEPTION_HH
#define RESOURCENOTFOUNDEXCEPTION_HH

#include "util/EcceException.H"


/**
 * ResourceNotFoundException means that a resource, identified by some
 * unique identifier (e.g. url) could not be located.
 */
class ResourceNotFoundException : public EcceException
{
   public:

      ResourceNotFoundException(const string& message, 
                                const char *file, int line);
      ResourceNotFoundException(const char *message, 
                                const char *file, int line);
      ResourceNotFoundException(const ResourceNotFoundException& rhs);


      virtual ~ResourceNotFoundException() throw ();

   protected:

      ResourceNotFoundException();


}; 


#endif
