/**
 * @file
 *
 *
 */
#include "util/IOException.H"



IOException::IOException(const string& msg, const char *file, int line)
         : EcceException(msg, file, line)
{
}

IOException::IOException(const char *msg, const char *file, int line)
         : EcceException(msg, file, line)
{
}

IOException::IOException() : EcceException()
{
}


IOException::IOException(const IOException& rhs)
                     : EcceException(rhs)
{
}

IOException::~IOException() throw ()
{
}


