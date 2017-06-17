/**
 * @file 
 *
 *
 */
#ifndef BADDATAEXCEPTION_HH
#define BADDATAEXCEPTION_HH

#include "util/EcceException.H"


/**
 * BadDataExceptions indicates the data in some data set is found to be
 * improperly formatted, unparsable, containing missing or erroneous values.
 */
class BadDataException : public EcceException
{
   public:

      BadDataException(const string& message, const char *file, int line);
      BadDataException(const char *message, const char *file, int line);
      BadDataException(const BadDataException& rhs);


      virtual ~BadDataException() throw ();

   protected:

      BadDataException();


}; 


#endif
