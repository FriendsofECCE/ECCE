/**
 * @file 
 *
 *
 */
#ifndef PARSEEXCEPTION_H
#define PARSEEXCEPTION_H

#include "util/EcceException.H"

/**
 * ParseException indicates a problem parsing text data including XML.
 * Parse errors can be flagged as non-fatal.
 */
class ParseException : public EcceException
{
   public:
      enum Severity { WARNING, FATAL };

      ParseException(ParseException::Severity severity, const string& message,
                     const char *file, int line);
      ParseException(ParseException::Severity severity, const char *message,
                     const char *file, int line);
      ParseException(const ParseException& rhs);
      virtual ~ParseException() throw ();

      virtual EcceException *clone() const throw ();

      virtual ParseException::Severity severity() const;

   protected:

      ParseException();
      ParseException::Severity p_severity;

}; 

#endif 
