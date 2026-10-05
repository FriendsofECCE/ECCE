/**
 * @file
 *
 *
 */
#include "util/InternalException.H"
#include "util/ThrowLog.H"

InternalException::InternalException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
#ifndef INSTALL
   ThrowLog::fault(*this);
#endif
}

InternalException::InternalException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
#ifndef INSTALL
   ThrowLog::fault(*this);
#endif
}
InternalException::InternalException() : EcceException()
{
}

InternalException::InternalException(const InternalException& rhs)
                     : EcceException(rhs)
{
}

InternalException::~InternalException() noexcept
{
}
