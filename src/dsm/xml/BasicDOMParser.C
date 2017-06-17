#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
using namespace xercesc;

#include "util/Ecce.H"
//#include "util/ErrMsg.H"
#include "util/SFile.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/ParseErrorReporter.H"

/**
 * Constructor.
 */
BasicDOMParser::BasicDOMParser()
{
}

/**
 * Destructor.
 */
BasicDOMParser::~BasicDOMParser()
{
}

void BasicDOMParser::initialize() throw (EcceException)
{
  try
  {
    XMLPlatformUtils::Initialize();
  } catch (const XMLException& toCatch) {
    string msg = "Error during XML Parser initialization! :\n";
    char *tmpMsg = XMLString::transcode(toCatch.getMessage());
    msg += tmpMsg;
    delete [] tmpMsg;
    throw EcceException(msg, WHERE);
  }
}

/**
 * Parse XML document from input source and return to caller.  Caller is 
 * responsible for cleaning the document by calling the release() method
 */
DOMDocument* BasicDOMParser::parse(const InputSource& is,
                             const bool& logErrors) throw (ParseException)
{
  initialize();

  XercesDOMParser parser;
  parser.setValidationScheme(AbstractDOMParser::Val_Never);  // No DTD
  ParseErrorReporter *errs;
  bool logFlag = logErrors || getenv("ECCE_DEVELOPER");
  if (logFlag) {
    errs = new ParseErrorReporter();
    parser.setErrorHandler(errs);
  }
  parser.parse(is);
  if (logFlag && !errs->getMessages().empty()) 
    throw ParseException(
               errs->isFatal()?ParseException::FATAL:ParseException::WARNING,
               errs->getMessages(), WHERE);

  DOMDocument *pdoc = parser.getDocument();
  pdoc = (DOMDocument *)pdoc->cloneNode(true);

  // Maybe we should be throwing an exception here...
  if ((pdoc == 0) || (pdoc && !pdoc->hasChildNodes())) {
    throw ParseException( ParseException::WARNING,
                          "Document parsed but its empty", WHERE);
  }

  if (logFlag)
    delete errs;

  return pdoc;
}

/**
 * Parse XML document from file and return to caller.  Caller is responsible 
 * for cleaning the document by calling the release() method
 */
DOMDocument* BasicDOMParser::parse(const SFile& file,
                             const bool& logErrors) throw (ParseException)
{

  initialize();

  XMLCh* xStr = XMLString::transcode(file.path().c_str());
  InputSource* is = new LocalFileInputSource(xStr);
  delete [] xStr;

  DOMDocument* pdoc = parse(*is, logErrors);
  delete is;

  return pdoc;
}


/**
 * Parse XML document from an input stream.  Caller is responsible 
 * for cleaning the document by calling the release() method
 */
DOMDocument* BasicDOMParser::parse(istream& data,
                             const bool& logErrors) throw (ParseException)
{

  initialize();

  // There must be a better way than to make two copies?
  int numBytes = data.rdbuf()->in_avail();
  char *buf = new char[numBytes+1];
  buf[numBytes] = '\0';
  numBytes = data.rdbuf()->sgetn(buf,numBytes);

  InputSource* is = new MemBufInputSource((const XMLByte*)buf, numBytes,
                                           "InputStream", false);

  DOMDocument* pdoc = parse(*is, logErrors);

  // InputSource uses the buffer in a shared manner so don't delete until 
  // after parsing.
  delete [] buf;
  delete is;

  return pdoc;
}




/**
 * Parse XML document from a std::string.  Caller is responsible 
 * for cleaning the document by calling the release() method
 */
DOMDocument* BasicDOMParser::parse(const string& data,
                             const bool& logErrors) throw (ParseException)
{

  initialize();

  MemBufInputSource* memBufIS = new MemBufInputSource (
        ( const XMLByte*)data.c_str(),
        data.size(),
        "string", false );


  DOMDocument* pdoc = parse(*memBufIS, logErrors);

  delete memBufIS;

  return pdoc;
}
