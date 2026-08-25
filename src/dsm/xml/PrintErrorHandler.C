#include <iostream>
  using std::cerr;
  using std::endl;
#include <sstream>

#include <xercesc/dom/DOMError.hpp>
#include <xercesc/dom/DOMLocator.hpp>
#include <xercesc/util/XMLString.hpp>
  using namespace xercesc;

#include "dsm/PrintErrorHandler.H"

/**
 * Constructor.
 */
PrintErrorHandler::PrintErrorHandler() {
}

/**
 * Destructor.
 */
PrintErrorHandler::~PrintErrorHandler() {
}

bool PrintErrorHandler::handleError (const DOMError &domError)
{
  const XMLCh *message = domError.getMessage();
  short severity = domError.getSeverity();
  DOMLocator *location = domError.getLocation();
  char *msg = XMLString::transcode(message);

  std::ostringstream oss;
  oss << "DOM Print error - severity: " << severity;
  oss << "Line: " << location->getLineNumber();
  oss << " Column: " << location->getColumnNumber();
  oss << " Offset: " << location->getByteOffset();
  oss << "\n";
  oss << "Message: " << msg;
  oss << "\n";
  p_msgs.append(oss.str());

  cerr << "\n";
  cerr << "DOM Print error - severity: " << severity;
  cerr << "Line: " << location->getLineNumber();
  cerr << " Column: " << location->getColumnNumber();
  cerr << " Offset: " << location->getByteOffset() << endl;
  cerr << "Message: " << msg << endl;

  delete msg;
  return true;
}

string PrintErrorHandler::getMessages() const
{
  return p_msgs;
}
