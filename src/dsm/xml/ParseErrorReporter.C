#include <iostream>
  using std::cerr;
  using std::endl;

#include "dsm/ParseErrorReporter.H"

ParseErrorReporter::ParseErrorReporter()
{
   p_msgs = "";
   p_fatal = false;
}

void ParseErrorReporter::error (const SAXParseException& ex)
{
  p_msgs.append("XML Parse recoverable error - ");
  p_msgs.append("Line: " + ex.getLineNumber());
  p_msgs.append(" Column: " + ex.getColumnNumber());
  p_msgs.append("Look for unmatched tags - matches are case sensitive.");

  cerr << "\n";
  cerr << "XML Parse recoverable error - ";
  cerr << "Line: " << ex.getLineNumber();
  cerr << " Column: "<< ex.getColumnNumber() << endl;
  cerr << "Look for unmatched tags - matches are case sensitive." << endl;
}

void ParseErrorReporter::fatalError (const SAXParseException& ex)
{
  p_msgs.append("XML Parse FATAL error - ");
  p_msgs.append("Line: " + ex.getLineNumber());
  p_msgs.append(" Column: " + ex.getColumnNumber());
  p_msgs.append("Look for unmatched tags - matches are case sensitive.");
  p_fatal = true;

  cerr << "\n";
  cerr << "XML Parse FATAL error - ";
  cerr << "Line: " << ex.getLineNumber();
  cerr << " Column: "<< ex.getColumnNumber() << endl;
  cerr << "Look for unmatched tags - matches are case sensitive." << endl;
}

string ParseErrorReporter::getMessages() const
{
  return p_msgs;
}

bool ParseErrorReporter::isFatal() const
{
   return p_fatal;
}
