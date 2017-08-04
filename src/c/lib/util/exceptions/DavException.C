/**
 * @file
 *
 *
 */
#include "util/DavException.H"



DavException::DavException(const string& msg, const char *file, int line)
         : EcceException(msg, file, line)
{
}

DavException::DavException(const char *msg, const char *file, int line)
         : EcceException(msg, file, line)
{
}

DavException::DavException() : EcceException()
{
}


DavException::DavException(const DavException& rhs)
                     : EcceException(rhs)
{
}

DavException::~DavException() throw ()
{
}


