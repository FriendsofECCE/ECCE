/**
 * @file 
 *
 *
 */
#ifndef ECCEEXCEPTION_H
#define ECCEEXCEPTION_H

#include <string>
#include <stdexcept>

using std::string;
using std::runtime_error;


/**
 * Base class for all Ecce exceptions.
 * EcceException improves upon the std exceptions by optionally
 * logging exceptions(TBD) and tracking file and line numbers to 
 * improve debugging (that is, it incorporates the features we liked
 * about the ASSERT macros.
 * 
 * Note that not all exceptions will be raised from this hierarchy.  For 
 * example, there may be std::logic_error instances or exceptions not related
 * to std exceptions from 3rd party tools.
 *
 * Additionally EcceException houses the default exception handler which can
 * report the details of the last exception thrown (or dump the log(TBD)).
 *
 * Examples:
 *   throw EcceException("test", WHERE);
 *   throw EcceException("test", __FILE__, __LINE__)
 * The first example is preferred as it provides useful debug information
 * for uncaught exceptions.
 *
 * All subclasses should support only constructors with the __FILE__
 * and __LINE__ arguments.
 * All subclasses that add more data should re-implement clone.
 * It is probably not necessary to provide a custom report method.
 *
 * @todo log all exceptions to file for debugging?
 */

#ifdef WHERE
#undef WHERE
#endif

/**
 * Define WHERE as shorthand to __FILE__ and __LINE__
 * Undefined if defined because its already defined in ErrMsg.H
 * which hopefully one day be obsolete.
 */
#define  WHERE   __FILE__, __LINE__

class EcceException : public runtime_error
{
   public:

      EcceException(const string& message, const char *file, int line);
      EcceException(const char *message, const char *file, int line);
      EcceException(const EcceException& rhs);
      virtual ~EcceException() throw ();

      virtual EcceException *clone() const throw ();
      virtual void report() const throw();

      virtual const char *what() const throw();

      static void setDefaultHandler();
      static void log(const string& msg);

   protected:

      EcceException();

      /** Compiler generated __FILE__ */
      string p_file;

      /** Compiler generated __LINE__ */
      int    p_line;

      /** Pointer to copy of last exception thrown.  Used by default handler */
      static const EcceException *p_last;

      /** Standard Ecce default exception handler. */
      static void defaultUncaughtHandler();

      void saveLast();


}; 

#endif 
