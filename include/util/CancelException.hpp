/**
 * @file 
 *
 *
 */
#ifndef CANCELEXCEPTION_H
#define CANCELEXCEPTION_H

#include "util/EcceException.H"

/**
 * CancelException indicates that the operation was canceled by a user.
 */
class CancelException : public EcceException
{
   public:

      CancelException(const char *file, int line);
      CancelException(const CancelException& rhs);

      virtual ~CancelException() throw ();


   protected:
      CancelException();


}; 

#endif 
