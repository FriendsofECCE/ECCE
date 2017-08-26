/**
 * @file
 *
 *
 */
#include "util/BadDataException.hpp"

BadDataException::BadDataException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}

BadDataException::BadDataException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}
BadDataException::BadDataException() : EcceException()
{
}

BadDataException::BadDataException(const BadDataException& rhs)
                     : EcceException(rhs)
{
}

BadDataException::~BadDataException() throw ()
{
}
