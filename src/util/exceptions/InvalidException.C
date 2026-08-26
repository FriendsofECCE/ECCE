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

InvalidException::~InvalidException() noexcept
{
}
EcceException *InvalidException::clone() const noexcept
{
   return new InvalidException(*this);
}

