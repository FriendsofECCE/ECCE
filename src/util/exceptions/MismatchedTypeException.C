/**
 * @file
 *
 *
 */
#include "util/MismatchedTypeException.H"
#include "util/ThrowLog.H"

MismatchedTypeException::MismatchedTypeException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
#ifndef INSTALL
   ThrowLog::fault(*this);
#endif
}

MismatchedTypeException::MismatchedTypeException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
#ifndef INSTALL
   ThrowLog::fault(*this);
#endif
}
MismatchedTypeException::MismatchedTypeException() : EcceException()
{
}

MismatchedTypeException::MismatchedTypeException(
                             const MismatchedTypeException& rhs)
         : EcceException(rhs)
{
}

MismatchedTypeException::~MismatchedTypeException() noexcept
{
}
