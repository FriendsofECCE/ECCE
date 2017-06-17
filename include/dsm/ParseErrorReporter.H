#ifndef PARSEERRORREPORTER__HH
#define PARSEERRORREPORTER__HH
/////////////////////////////////////////////////////////////////////////////
// Class: ParseErrorReporter
//
// Synopsis: Reports errors from XML document parsing.
//
// Description:
//   Report them to stderr at this time.
//   Currently only catches errors and fatal errors.
/////////////////////////////////////////////////////////////////////////////

#include <xercesc/sax/HandlerBase.hpp> 
using namespace xercesc;

#include <string>

using std::string;


class ParseErrorReporter : public HandlerBase {
  public:

     ParseErrorReporter();

     virtual void error (const SAXParseException& ex);

     virtual void fatalError (const SAXParseException& ex);

     string getMessages() const;
     bool   isFatal() const;

  private:
     string p_msgs;
     bool   p_fatal;

};

#endif 
