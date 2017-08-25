#include <string.h>
#include <stdlib.h>

#include <string>
   using std::string;

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLUniDefs.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMWriter.hpp>
  using namespace xercesc;


#include "dsm/BasicDOMWriter.H"
#include "dsm/PrintErrorHandler.H"

static const string gEncoding = "UTF-8";
static const string gVersion = "1.0";
static const string gStandalone = "yes";
static const string gDocDecl = "<?xml version=\""+gVersion+"\" encoding=\""+gEncoding+"\" standalone=\""+gStandalone+"\" ?>\n";

string BasicDOMWriter::XMLDocumentDeclaration(void)
{
   return gDocDecl;
}

/**
 * Printing values can be EXCEEDINGLY slow if you check for chars that
 * must be escaped in the body of the text.  So expose the mechanism to
 * skip that logic for printing things like data sets of numbers.
 * Performance on 12MB can be 10-15 minutes.  Killer.
 * Be careful NOT to assume you can always do this because there may
 * be cases where its inappropriate.  The default is to not escape them.
 * You need to look at the Xerces XMLFormatter code (formatBuf).
 */
void BasicDOMWriter::write(const DOMNode& dn, ostream& os, bool escaped, 
                           bool prettyPrint) throw (EcceException)
{
  try {
    XMLPlatformUtils::Initialize();
  } catch (const XMLException& toCatch) {
    string msg = "Error during DOM Writer initialization! :\n";
    char *tmpMsg = XMLString::transcode(toCatch.getMessage());
    msg += tmpMsg;
    delete [] tmpMsg;
    throw EcceException(msg, WHERE);
  }

  XMLCh *xStr = XMLString::transcode("LS");
  DOMImplementation *impl = DOMImplementationRegistry::getDOMImplementation(xStr);
  delete [] xStr;

  DOMWriter *dw = ((DOMImplementationLS*)impl)->createDOMWriter();

  dw->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, prettyPrint);
  
  PrintErrorHandler *peh = new PrintErrorHandler();
  dw->setErrorHandler(peh);

  XMLCh *uEncoding = XMLString::transcode(gEncoding.c_str());
  dw->setEncoding(uEncoding);
  delete [] uEncoding;

  MemBufFormatTarget destination;
  dw->writeNode(&destination, dn);

  if (!peh->getMessages().empty()) 
    throw EcceException(peh->getMessages(), WHERE);

  //XMLByte is an unsigned char, so just cast - transcode isn't needed
  char *results = (char *) destination.getRawBuffer();
  os << results;

  delete peh;
  delete dw;
} 
