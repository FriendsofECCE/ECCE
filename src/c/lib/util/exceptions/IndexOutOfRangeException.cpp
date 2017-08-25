/**
 * @file
 *
 *
 */
#include <stdio.h>

#include "util/IndexOutOfRangeException.H"


/**
 * Formats a message as a string given the range and value.
 * I do this in order to pass a complete message up to the superclass
 * so that construction-level logging and debugging work.
 * The legit way to do this would be to require all constructors in
 * all classes to log but that seems error prone.
 */
static string formatMessage(int lower, int upper, int value) 
{
   char buf[128];
   sprintf(buf,"Invalid value: %d.  Expected range [%d, %d]",
           value, lower, upper);
   return buf;
}

static string formatMessage(int lower, size_t upper, int value) 
{
   char buf[128];
   sprintf(buf,"Invalid value: %d.  Expected range [%d, %zu]",
           value, lower, upper);
   return buf;
}

IndexOutOfRangeException::IndexOutOfRangeException() : EcceException()
{
   p_lower = p_upper = p_value = 0;
}


IndexOutOfRangeException::IndexOutOfRangeException(int lower, 
                                                   int upper, 
                                                   int value,
                                                   const char *file, int line) 
                               : EcceException(formatMessage(lower,upper,value),
                                               file, line)
{
   p_lower = lower;
   p_upper = upper;
   p_value = value;
}


IndexOutOfRangeException::IndexOutOfRangeException(int lower, 
                                                   size_t upper, 
                                                   int value,
                                                   const char *file, int line) 
                               : EcceException(formatMessage(lower,upper,value),
                                               file, line)
{
   p_lower = lower;
   p_upper = upper;
   p_value = value;
}




IndexOutOfRangeException::IndexOutOfRangeException(
                            const IndexOutOfRangeException& rhs)
                            : EcceException(rhs)
{
   p_lower = rhs.p_lower;
   p_upper = rhs.p_upper;
   p_value = rhs.p_value;
}



IndexOutOfRangeException::~IndexOutOfRangeException() throw ()
{
}



EcceException *IndexOutOfRangeException::clone() const throw()
{
   return new IndexOutOfRangeException(*this);
}



/**
 * Overload of std::exception::what()
 */
const char *IndexOutOfRangeException::what() const throw()
{
   string ret = formatMessage(p_value, p_lower, p_upper);;
   return ret.c_str();
}
