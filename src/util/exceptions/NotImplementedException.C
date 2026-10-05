/**
 * @file
 *
 *
 */
#include "util/NotImplementedException.H"
#include "util/ThrowLog.H"

NotImplementedException::NotImplementedException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
#ifndef INSTALL
   ThrowLog::fault(*this);
#endif
}

NotImplementedException::NotImplementedException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
#ifndef INSTALL
   ThrowLog::fault(*this);
#endif
}
NotImplementedException::NotImplementedException() : EcceException()
{
}

NotImplementedException::NotImplementedException(
                            const NotImplementedException& rhs)
         : EcceException(rhs)
{
}

NotImplementedException::~NotImplementedException() noexcept
{
}
