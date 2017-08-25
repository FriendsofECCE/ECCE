#include <iostream>
  using std::cerr;
  using std::endl;

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

  p_msgs.append("DOM Print error - severity: " + severity);
  p_msgs.append("Line: " + location->getLineNumber());
  p_msgs.append(" Column: " + location->getColumnNumber());
  p_msgs.append(" Offset: " + location->getOffset());
  p_msgs.append("\n");
  p_msgs.append("Message: ");
  p_msgs.append(msg);
  p_msgs.append("\n");

  cerr << "\n";
  cerr << "DOM Print error - severity: " << severity;
  cerr << "Line: " << location->getLineNumber();
  cerr << " Column: " << location->getColumnNumber();
  cerr << " Offset: " << location->getOffset() << endl;
  cerr << "Message: " << msg << endl;

  delete msg;
  return true;
}

string PrintErrorHandler::getMessages() const
{
  return p_msgs;
}
