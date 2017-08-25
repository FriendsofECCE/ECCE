/**
 * @file
 *
 *
 */
#include "util/CancelException.H"


CancelException::CancelException(const char *file, int line)
         : EcceException("The operation was canceled by the user.", file, line)
{
}


CancelException::CancelException(const CancelException& rhs) 
         : EcceException(rhs)
{
}

CancelException::CancelException() : EcceException()
{
}


CancelException::~CancelException() throw ()
{
}
