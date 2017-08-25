/**
 * @file
 *
 *
 */
#include "util/BadValueException.H"

BadValueException::BadValueException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}

BadValueException::BadValueException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}
BadValueException::BadValueException() : EcceException()
{
}

BadValueException::BadValueException(const BadValueException& rhs)
                     : EcceException(rhs)
{
}

BadValueException::~BadValueException() throw ()
{
}
