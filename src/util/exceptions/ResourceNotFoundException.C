/**
 * @file
 *
 *
 */
#include "util/ResourceNotFoundException.H"

ResourceNotFoundException::ResourceNotFoundException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}

ResourceNotFoundException::ResourceNotFoundException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}
ResourceNotFoundException::ResourceNotFoundException() : EcceException()
{
}

ResourceNotFoundException::ResourceNotFoundException(
                                 const ResourceNotFoundException& rhs)
          : EcceException(rhs)
{
}

ResourceNotFoundException::~ResourceNotFoundException() throw ()
{
}
