/**
 * @file
 *
 *
 */
#include "util/NotImplementedException.H"

NotImplementedException::NotImplementedException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}

NotImplementedException::NotImplementedException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}
NotImplementedException::NotImplementedException() : EcceException()
{
}

NotImplementedException::NotImplementedException(
                            const NotImplementedException& rhs)
         : EcceException(rhs)
{
}

NotImplementedException::~NotImplementedException() throw ()
{
}
