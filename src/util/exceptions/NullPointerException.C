/**
 * @file
 *
 *
 */
#include "util/NullPointerException.H"

NullPointerException::NullPointerException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}

NullPointerException::NullPointerException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}
NullPointerException::NullPointerException() : EcceException()
{
}

NullPointerException::NullPointerException(const NullPointerException& rhs)
                     : EcceException(rhs)
{
}

NullPointerException::~NullPointerException() throw ()
{
}
