/**
 * @file
 *
 *
 */
#include "util/MismatchedTypeException.H"

MismatchedTypeException::MismatchedTypeException(const string& msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}

MismatchedTypeException::MismatchedTypeException(const char *msg, 
                                           const char *file, int line)
         : EcceException(msg, file, line)
{
}
MismatchedTypeException::MismatchedTypeException() : EcceException()
{
}

MismatchedTypeException::MismatchedTypeException(
                             const MismatchedTypeException& rhs)
         : EcceException(rhs)
{
}

MismatchedTypeException::~MismatchedTypeException() throw ()
{
}
