/**
 * @file
 *
 *
 */
#include "util/InternalException.H"

InternalException::InternalException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}

InternalException::InternalException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}
InternalException::InternalException() : EcceException()
{
}

InternalException::InternalException(const InternalException& rhs)
                     : EcceException(rhs)
{
}

InternalException::~InternalException() throw ()
{
}
