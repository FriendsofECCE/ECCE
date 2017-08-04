/**
 * @file
 *
 *
 */
#include  <iostream>
using std::cout;
using std::cerr;
using std::endl;

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

#include "util/EcceException.H"

const EcceException *EcceException::p_last = 0;


/**
 * Ecce default exception handler.
 * Invokes the report method on the last Ecce Exception that was
 * thrown.  Note that an exception that is not an EcceException will
 * not be saved for reporting.  The handler only knows how to report
 * the last EcceException.
 *
 * If not built INSTALL, a core is dumped.
 */
void EcceException::defaultUncaughtHandler()
{
   cout << "---------------------------------------\n";
   cout << "An uncaught ECCE exception has occurred.  \n";
   cout << "The last uncaught EcceException was:\n";
   cout << "---------------------------------------\n";
   if (p_last) {
     p_last->report();
   }
#ifndef INSTALL
   abort();
#else
   exit(-1);
#endif
}



/**
 * Hidden default constructor.
 */
EcceException::EcceException() : runtime_error("No message")
{
   p_file = "";
   p_line = -1;
   saveLast();
}



EcceException::EcceException(const string& message, const char *file, int line)
         : runtime_error(message.c_str())
{
   p_file = file;
   p_line = line;
   saveLast();
#ifndef INSTALL
   cerr << "\nThrow Log: " << endl;
   cerr << "---------- " << endl;
   report();
   cerr << endl;
#endif
}




EcceException::EcceException(const char *message, const char *file, int line)
         : runtime_error(message)
{
   p_file = file;
   p_line = line;
   saveLast();
#ifndef INSTALL
   cerr << "\nThrow Log: " << endl;
   cerr << "---------- " << endl;
   report();
   cerr << endl;
#endif
}



/**
 * Copy Constructor.
 */
   EcceException::EcceException(const EcceException& rhs) 
: runtime_error(rhs.what())
{
   p_file = rhs.p_file;
   p_line = rhs.p_line;
}



EcceException *EcceException::clone() const throw()
{
   return new EcceException(*this);
}



/**
 * Destructor.
 */
EcceException::~EcceException() throw ()
{
}



/**
 * save last exception pointer.
 * This is a bit tricky.  Its not possible to invoke clone() 
 * if this method is used during EcceException time.  Be definition,
 * and object doesn't exist until its construtor completes.  In the case
 * of subclasses, only EcceException will exist and only its clone method
 * will be called.  It SEEMS to work to just save the pointer. Its not
 * clear to me that this pointer will be valid because the spec says
 * an exception may be copied (by why doesn't it require copy construtor
 * then?).
 * If this fails, we can have all subclasses call saveLast in each
 * construction.  Tedious and error prone so I'm trying this first.
 */
void EcceException::saveLast()
{
   /*
   if (p_last != 0) {
      delete p_last;
      p_last = 0;
   }
   p_last = clone();
   */
   p_last = this;
}



/** 
 * Report exception to stderr.
 *
 * The message text is reported along with the file and line number
 * if available.
 */
void EcceException::report() const throw()
{
   if (p_file.size() > 0 ) {
      char buf[32];
      sprintf(buf, "%d", p_line);
      cerr << "In " << p_file << "  line: " << buf << endl;
   }
   cerr << what() << endl;
}



/**
 * Overload of std::exception::what()
 */
const char *EcceException::what() const throw()
{
   string baseMessage = runtime_error::what();
   return baseMessage.c_str();
}


void EcceException::setDefaultHandler()
{
   std::set_terminate(EcceException::defaultUncaughtHandler);
}



/**
 * Log the message to stderr.
 * This provides a mechanism to notice odd behaviors that shouldn't raise 
 * an exception because there would not be any way of dealing with it anyway.
 * An example: return from blocked read but there is no data.
 * The message is only logged if INSTALL is not defined.
 */
void EcceException::log(const string& msg)
{
#ifndef INSTALL
   cerr << msg << endl;
#endif
}
