/**
 * @file
 *
 *
 */
#include <stdio.h>
#include <string.h>
#include "util/SystemCommandException.H"


SystemCommandException::SystemCommandException(int code, const char *msg, 
                                   const char *file, int line)
         : EcceException(msg, file, line)
{
   p_code = code;
}



SystemCommandException::SystemCommandException(int code, const string& msg, 
                                   const char *file, int line)
         : EcceException(msg, file, line)
{
   p_code = code;
}


/**
 * Hidden default constructor.
 */
SystemCommandException::SystemCommandException() : EcceException()
{
   p_code = 0;
}



SystemCommandException::SystemCommandException(const SystemCommandException& rhs)
                     : EcceException(rhs)
{
   p_code = rhs.p_code;
}



SystemCommandException::~SystemCommandException() throw ()
{
}


/**
 * Reports the exception contents with error code and message text.
 */
const char *SystemCommandException::what() const throw()
{
  string ret;

  const char *baseMessage = EcceException::what();
  char *buf = new char[strlen(baseMessage) + 10];
  sprintf(buf,"%d %s",p_code,baseMessage);
  ret = buf;
  delete [] buf;
  return ret.c_str();
}


EcceException *SystemCommandException::clone() const throw()
{
   return new SystemCommandException(*this);
}
