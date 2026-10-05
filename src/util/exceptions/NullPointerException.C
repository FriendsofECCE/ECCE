/**
 * @file
 *
 *
 */
#include "util/NullPointerException.H"
#include "util/ThrowLog.H"

NullPointerException::NullPointerException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
#ifndef INSTALL
   ThrowLog::fault(*this);
#endif
}

NullPointerException::NullPointerException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
#ifndef INSTALL
   ThrowLog::fault(*this);
#endif
}
NullPointerException::NullPointerException() : EcceException()
{
}

NullPointerException::NullPointerException(const NullPointerException& rhs)
                     : EcceException(rhs)
{
}

NullPointerException::~NullPointerException() noexcept
{
}
