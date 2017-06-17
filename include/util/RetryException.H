/**
 * @file 
 *
 *
 */
#ifndef RETRYEXCEPTION_H
#define RETRYEXCEPTION_H

#include "util/EcceException.H"

/**
 * RetryException reports that the maximum number of retries has been exceeded.
 * Examples include user input of passwords, retry of socket operations.
 */
class RetryException : public EcceException
{
   public:

      RetryException(const string& message, const char *file, int line);
      RetryException(const char *message, const char *file, int line);
      RetryException(const RetryException& rhs);

      virtual ~RetryException() throw ();

   protected:

      RetryException();


}; 

#endif 
