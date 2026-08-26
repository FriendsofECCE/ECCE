/**
 * @file 
 *
 *
 */
#ifndef COMMANDEXCEPTION_HH
#define COMMANDEXCEPTION_HH

#include "util/EcceException.H"

/**
 * SystemCommandException represents failures of system type commands.
 * Such commands have associated status codes that are included with
 * the exception.
 */ 
class SystemCommandException : public EcceException
{
   public:

      SystemCommandException(int code, const string& message, 
                             const char *file, int line);
      SystemCommandException(int code, const char *message, 
                             const char *file, int line);
      SystemCommandException(const SystemCommandException& rhs);

      virtual ~SystemCommandException() noexcept;

      virtual EcceException *clone() const noexcept;

      virtual const char *what() const noexcept;


   protected:
      SystemCommandException();

      /** Status code from a command. */
      int p_code;


}; 

#endif
