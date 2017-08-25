/**
 * @file
 *
 *
 */
#include "util/RetryException.H"

RetryException::RetryException() : EcceException()
{
}


RetryException::RetryException(const string& message, 
                               const char *file, int line) 
                     : EcceException(message, file, line)
{
}

RetryException::RetryException(const char *message, 
                               const char *file, int line) 
                     : EcceException(message, file, line)
{
}


RetryException::RetryException(const RetryException& rhs)
                     : EcceException(rhs)
{
}



RetryException::~RetryException() throw ()
{
}
