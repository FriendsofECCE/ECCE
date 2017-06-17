/**
 * @file
 *
 *
 */
#include "util/InvalidException.H"

InvalidException::InvalidException(const string& msg, 
                                   const char *file, int line)
         : EcceException(msg, file, line)
{
}

InvalidException::InvalidException(const char *msg, const char *file, int line)
         : EcceException(msg, file, line)
{
}
InvalidException::InvalidException() : EcceException()
{
}

InvalidException::InvalidException(const InvalidException& rhs)
                     : EcceException(rhs)
{
}

InvalidException::~InvalidException() throw ()
{
}
EcceException *InvalidException::clone() const throw()
{
   return new InvalidException(*this);
}

