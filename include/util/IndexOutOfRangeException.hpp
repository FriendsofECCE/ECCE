/**
 * @file 
 *
 *
 */
#ifndef INDEXOUTOFRANGEEXCEPTION_H
#define INDEXOUTOFRANGEEXCEPTION_H

#include "util/EcceException.H"

/**
 * An IndexOutOfRangeException is usually used to signal that the caller
 * tried to access an ordered collection at and illegal index  (i.e. <0
 * and >=size())
 */
class IndexOutOfRangeException : public EcceException
{
   public:

      IndexOutOfRangeException(int lower, int upper, int value,
                               const char *file, int line);
      IndexOutOfRangeException(int lower, size_t upper, int value,
                               const char *file, int line);
      IndexOutOfRangeException(const IndexOutOfRangeException& rhs);
      virtual ~IndexOutOfRangeException() throw ();

      virtual EcceException *clone() const throw ();

      virtual const char *what() const throw();

   protected:

      IndexOutOfRangeException();

      int    p_lower;
      size_t p_upper; // used size_t for compatability with std*::size()
      int    p_value;

}; 


#ifdef INSTALL
#define  INDEXOUTOFRANGEEXCEPTION(expr, lower, upper, value)
#else
#define  INDEXOUTOFRANGEEXCEPTION(expr, lower, upper, value) \
if ( !(expr) ) \
{ \
  throw IndexOutOfRangeException(lower, upper, value,  __FILE__, __LINE__); \
}
#endif /* INSTALL */


#endif 
