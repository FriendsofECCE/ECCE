//////////////////////////////////////////////////////////////////////////////
//
// DAV status codes from Eric 11/11/2000
//
// Status Code      Description
//
// 100                    Continue
// 101                    Switching Protocols
// 200                    OK
// 201                    Created
// 202                    Accepted
// 203                    Non-Authoritative Information
// 204                    No Content
// 205                    Reset Content
// 206                    Partial Content
// 300                    Multiple Choices
// 301                    Moved Permanently
// 302                    Moved Temporarily
// 303                    See Other
// 304                    Not Modified
// 305                    Use Proxy
// 400                    Bad Request
// 401                    Unauthorized
// 402                    Payment Required
// 403                    Forbidden
// 404                    Not Found
// 405                    Method Not Allowed
// 406                    Not Acceptable
// 407                    Proxy Authentication Required
// 408                    Request Time-out
// 409                    Conflict
// 410                    Gone
// 411                    Length Required
// 412                    Precondition Failed
// 413                    Request Entity Too Large
// 414                    Request-URI Too Large
// 415                    Unsupported Media Type
// 500                    Internal Server Error
// 501                    Not Implemented
// 502                    Bad Gateway
// 503                    Service Unavailable
// 504                    Gateway Time-out
// 505                    HTTP Version not supported
//////////////////////////////////////////////////////////////////////////////
#include <strstream>
  using std::istrstream;
  using std::ostrstream;
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::ends;

#include <stdio.h> // sprintf

#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
using namespace xercesc;

#include "dsm/EcceDAVStatus.H"
#include "dsm/DavEDSI.H"
#include "dsm/VDoc.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/CDAVClient.H"

#include "util/ErrMsg.H"
#include "util/EcceURL.H"
#include "util/UrlDecoder.H"
#include "util/StringConverter.H"
#include "util/ResourceUtils.H"
#include "util/STLUtil.H"
#include "dsm/EDSI.H"
#include "dsm/BasicDOMWriter.H"
#include "dsm/EcceDAVClient.H"
#include "dsm/ResourceDescriptor.H"



// Different servers use different abbreviations for the DAV namespace.
// mod_dav uses D:  slide uses the default namespace.  The alias must be
// computed in all responses.  However we define the one we want to use
// on requests.  It should be possible for this to be the empty string
// (default namespace).
static string DAV_NSALIAS = "E";
static string DAV_NSNAME = "DAV";
static string DAV_NSPREFIX = DAV_NSALIAS.empty() ? (string)"" : (DAV_NSALIAS + ":");

// DAV request elements - include our DAV prefix on these.
static string vREMOVE      = DAV_NSPREFIX + "remove";
static string vSET         = DAV_NSPREFIX + "set";
static string vALLPROP     = DAV_NSPREFIX + "allprop";
static string vPROPNAME    = DAV_NSPREFIX + "propname";
static string vPROPFIND    = DAV_NSPREFIX + "propfind";
static string vPROPUPDATE  = DAV_NSPREFIX + "propertyupdate";
// DAV response elements - dav namespace must be computed
static string vHREF        = "href";
static string vSTATUS      = "status";
static string vPROPSTAT    = "propstat";
static string vRESPONSE    = "response";
// DAV request and response elements - must prepend DAV_NSPREFIX when
// used as part of the request.
static string vPROP        = "prop";




/*************************************************************************
 Method :  Constructors and destructor
 Summary:  Constructors are used to set the base URL and to initialize the
           Xerces XML parser
**************************************************************************/

DavEDSI::DavEDSI() 
{
  p_EcceDAVClient = new EcceDAVClient();
  p_EcceDAVClient->addProgressEventListener(this);
  initializeXMLParser();
  p_progressEvent = new ProgressEvent;
  p_progressListener = 0;
}

DavEDSI::DavEDSI(const EcceURL& url) : EDSI(url)
{
  p_EcceDAVClient = new EcceDAVClient();
  p_EcceDAVClient->addProgressEventListener(this);
  p_EcceDAVClient->url(url);
  p_progressEvent = new ProgressEvent;
  p_progressListener = 0;
  initializeXMLParser();
}

DavEDSI::DavEDSI(const DavEDSI& rhs) : EDSI(rhs)
{
  p_EcceDAVClient = new EcceDAVClient();
  p_EcceDAVClient->addProgressEventListener(this);
  p_EcceDAVClient->url(rhs.p_EcceDAVClient->url());
  p_progressEvent = new ProgressEvent;
  p_progressListener = 0;
}

DavEDSI::~DavEDSI() 
{
  p_EcceDAVClient->removeProgressEventListener(this);
  delete p_EcceDAVClient;
  delete p_progressEvent;
  p_progressListener = 0; // don't delete
}

/*************************************************************************
  * Convenience function to make a string out of a DOMString and ensure
  * no memory is leaked.
**************************************************************************/
string DavEDSI::transcode(const XMLCh* dstr) const
{
  string ret;
  char * cptr = XMLString::transcode(dstr);
  ret = cptr;
  delete cptr;
  return ret;
}

/*************************************************************************
 Method :  addListener & removeListener
 Summary:  Registers an authorized user (who has had password and directory
           permissions verified) to listen for _______ event
**************************************************************************/
void DavEDSI::addAuthEventListener(AuthEventListener *l)
{
  p_EcceDAVClient->addListener(l);
}

void DavEDSI::removeAuthEventListener(AuthEventListener *l)
{
  p_EcceDAVClient->removeListener(l);
}

void DavEDSI::addProgressEventListener(ProgressEventListener *l)
{
  p_progressListener = l;
}

void DavEDSI::removeProgressEventListener(ProgressEventListener *l)
{
  if (p_progressListener == l) {
    p_progressListener = 0;
  }

}

void DavEDSI::initProgress()
{
  p_progressEvent->m_tick = 0;
  p_progressEvent->m_stop = false;
}
void DavEDSI::progressNotice(ProgressEvent& event)
{
  p_progressEvent->m_tick++;
  if (p_progressListener) {
    p_progressListener->progressNotice(*p_progressEvent);
    event.m_stop = p_progressEvent->m_stop;
  }
}

/**
 * Verify that the current object represents a reachable DAV server.
 *
 * Return error message if any error is detected.
 * Use the option command to ensure that DAV level 2 support is provided.
 * This info is returned available in the return header.  Not sure if we
 * need to check which methods are allowed.
 *
 * Example output (5/30/2001):
 * Allow: OPTIONS, GET, HEAD, POST, DELETE, TRACE, PROPFIND, PROPPATCH,
 * COPY, MOVE, LOCK, UNLOCK^M
 * DAV: 1,2,<http://apache.org/dav/propset/fs/1>^M
 *
 * TODO read spec to see if the format is specified - i.e. are the finds
 * I used legitimate?
 * TODO decide if we want to require certain methods be allowed.
 */
bool DavEDSI::checkServer()
{
  m_msgStack.clear();             // public method must clear
  initProgress();
  bool ret = true;
  int code =  p_EcceDAVClient->options();
  if (code >= 200 && code <= 400) {
    string results = p_EcceDAVClient->getHeaderText();
    //NOTE: This is either bad code or slide is bad.  mod_dav has DAV 
    // in the header with the
    //server keyword but slide does not.  Here is what slide has:
    // Server: Apache Tomcat/4.0.1 (HTTP/1.1 Connector)^M
    if (results.find("DAV") == string::npos && 
        results.find(",2") == string::npos) {
      ret = false;
      m_msgStack.add("SERVER_NOT_FOUND",p_url.toString().c_str());
    }
  } else {
    // some dav failure; 
    ret = false;
    m_msgStack.add("SERVER_NOT_FOUND",p_url.toString().c_str());
  }
  return ret;
}

/*************************************************************************
 Method : describeServerMetaData 
 Summary: Returns a list of "well-known" properties we are using on our
          DAV servers.
          Since v4.0, some more "well-known" properties were added.
**************************************************************************/
bool DavEDSI::describeServerMetaData(vector<string>& metadata)
{
  initProgress();
  m_msgStack.clear();             // public method must clear
  //metadata.push_back(DAV_NSNAME +":href");
  metadata.push_back(VDoc::getEcceNamespace() + ":state");
  metadata.push_back(VDoc::getEcceNamespace() + ":reviewed");
#if 000
  metadata.push_back(DAV_NSNAME + ":creationdate");
#else
  metadata.push_back(VDoc::getEcceNamespace() + ":creationdate");
#endif
  metadata.push_back(DAV_NSNAME + ":getlastmodified");
  metadata.push_back(DAV_NSNAME + ":getcontenttype");
  metadata.push_back(VDoc::getEcceNamespace() + ":application");
  metadata.push_back(VDoc::getEcceNamespace() + ":empiricalFormula");
  metadata.push_back(VDoc::getEcceNamespace() + ":name");
  //metadata.push_back(VDoc::getEcceNamespace() + ":creationdate");
  //metadata.push_back(VDoc::getEcceNamespace() + ":resourcetype");
  metadata.push_back(VDoc::getEcceNamespace() + ":annotation");
  metadata.push_back(VDoc::getEcceNamespace() + ":citation");
  metadata.push_back(VDoc::getEcceNamespace() + ":owner");
  metadata.push_back(VDoc::getEcceNamespace() + ":theory");
  metadata.push_back(VDoc::getEcceNamespace() + ":runtype");
  metadata.push_back(VDoc::getEcceNamespace() + ":launch_machine");
  metadata.push_back(VDoc::getEcceNamespace() + ":launch_queue");
  metadata.push_back(VDoc::getEcceNamespace() + ":launch_totalprocs");
  metadata.push_back(VDoc::getEcceNamespace() + ":startdate");
  metadata.push_back(VDoc::getEcceNamespace() + ":completiondate");
  metadata.push_back(VDoc::getEcceNamespace() + ":job_clienthost");
  metadata.push_back(VDoc::getEcceNamespace() + ":job_jobid");
  // added for v4.0
  metadata.push_back(VDoc::getEcceNamespace() + ":symmetrygroup");
  metadata.push_back(VDoc::getEcceNamespace() + ":charge");
  metadata.push_back(VDoc::getEcceNamespace() + ":spinmultiplicity");
  metadata.push_back(VDoc::getEcceNamespace() + ":openshells");
  metadata.push_back(VDoc::getEcceNamespace() + ":coordsys");

  // added for v4.0 - new organizer's calculation panel
  metadata.push_back(VDoc::getEcceNamespace() + ":numFunctions");
  metadata.push_back(VDoc::getEcceNamespace() + ":numPrimitives");
  metadata.push_back(VDoc::getEcceNamespace() + ":ecpName");
  metadata.push_back(VDoc::getEcceNamespace() + ":chargeFittingName");
  metadata.push_back(VDoc::getEcceNamespace() + ":exchangeFittingName");
  metadata.push_back(VDoc::getEcceNamespace() + ":numAtoms");
  metadata.push_back(VDoc::getEcceNamespace() + ":numElectrons");
  metadata.push_back(VDoc::getEcceNamespace() + ":useExpCoef");
  metadata.push_back(VDoc::getEcceNamespace() + ":fragmentName");
  metadata.push_back(VDoc::getEcceNamespace() + ":reviewedDate");
  metadata.push_back(VDoc::getEcceNamespace() + ":contenttype");

  // View all properties - performance problem?
  metadata.clear();
  return true;
}

/*************************************************************************
 Method : exists
 Summary: Determines if current URL exists and is available.
**************************************************************************/
bool DavEDSI::exists(const bool& newUser) 
{
  m_msgStack.clear();             // public method must clear
  initProgress();
  p_EcceDAVClient->url(p_url);
  return resourceExists(newUser); 
}

/*************************************************************************
 Method : initializeXMLParser()
 Summary: 
**************************************************************************/
bool DavEDSI::initializeXMLParser() 
{ 
  bool ret = true;
  try
  {
    XMLPlatformUtils::Initialize();
  }

  catch (const XMLException& toCatch)
  {
    char *tempStr = XMLString::transcode(toCatch.getMessage());
    cerr << "Error during initialization! :\n" << tempStr << endl;
    delete [] tempStr;
    ret = false;
  }
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
void DavEDSI::setURL(const EcceURL& url) 
{
  EDSI::setURL(url);
}

istream* DavEDSI::getDataSubSet(int start_position, int length)
{
  initProgress();
  istream *ret = NULL;
  m_msgStack.clear();
  p_EcceDAVClient->url(p_url);
  ostrstream os;
  int status = p_EcceDAVClient->get_document_subset(start_position, length, os);
  if (status != EcceDAVStatus::OK &&
      status != EcceDAVStatus::CREATED &&
     (status/100 != 3))
  {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
  }
  else {
    os << ends;
    ret = new istrstream(os.str());
  }
  return ret;

}

bool DavEDSI::getDataSubSet(ostream& dest, int start_position, int length)
{
  bool ret = true;
  initProgress();
  m_msgStack.clear();

  p_EcceDAVClient->url(p_url);
  int status = p_EcceDAVClient->get_document_subset(start_position, length, dest);

  if (status/100 != 2 && (status/100 != 3)) {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
    ret = false;
  }

  return ret;
}


////////////////////////////////////////////////////////////////////////////
// getDataSet()
//   Load the dataset of the current url into the dest stream.  This
//   allows the user to specify memory or file or...
//   Caller is responsible for adding stream end.
////////////////////////////////////////////////////////////////////////////
bool DavEDSI::getDataSet(ostream& dest)
{
  bool ret = true;
  initProgress();
  m_msgStack.clear();

  p_EcceDAVClient->url(p_url);
  int status = p_EcceDAVClient->get_document(dest);

  if (status != 200 && (status/100 != 3)) {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
    ret = false;
  }

  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
 TODO this needs to be rewritten. massive memory leaking and inefficient.
 While we are at it support a version that doesn't need to be "in
 memory".
 isstream *ret = new isstream(x->getREsultsText().c_str();
 may need to strdup.
**************************************************************************/
istream *DavEDSI::getDataSet() 
{
  initProgress();
  istream *ret = NULL;
  m_msgStack.clear();
  p_EcceDAVClient->url(p_url);
  ostrstream os;
  int status = p_EcceDAVClient->get_document(os);
  if (status != EcceDAVStatus::OK &&
      status != EcceDAVStatus::CREATED &&
     (status/100 != 3))
  {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
  }
  else {
    os << ends;
    ret = new istrstream(os.str());
  }
  return ret;
}

unsigned long DavEDSI::getDataSetSize() 
{
  unsigned long ret = 0;
  string   lenKeyword = "Content-Length: ";

  m_msgStack.clear();             // public methods must clear

  initProgress();
  p_EcceDAVClient->url(p_url);

  int status = p_EcceDAVClient->head();

  if (status == 200) {

    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), 
        EcceDAVStatus::text(status).c_str());

    string rawHeader = p_EcceDAVClient->getHeaderText();
    int contentLengthStart = rawHeader.find(lenKeyword);
    if (contentLengthStart != string::npos) {
      string rawContentLength = 
        rawHeader.substr(contentLengthStart + 17 - 1, rawHeader.length() - 17);
      int ContentLengthValueStart = rawContentLength.find("\n");
      ret = atol(rawContentLength.substr(0,ContentLengthValueStart-1).c_str());
    }
  } else {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
  }

  return ret;
} 



/*************************************************************************
 Method : 
 Summary: 
 TODO set resourcetype here too.
**************************************************************************/
bool DavEDSI::listCollection(const vector<MetaDataRequest> & requests, 
                             vector<ResourceMetaDataResult>& resourceResults) 
{
  bool ret = false;
  initProgress();

  m_msgStack.clear();
  p_EcceDAVClient->url(p_url);
  p_EcceDAVClient->setDepthHeader("1");
  string response;

  vector<MetaDataRequest> local = requests;
  addVDocAttributes(local);

  ret = requestMetaData(local,resourceResults);

  //---------------------------------------------------------------
  /*  Don't search VDoc - only return collection members and their 
      properties - used by Resource::getChildren()

  // Here we add the virtual document code in.
  // For every child that is a virtual document, get the sub meta data
  // properties.
  if (ret) {
    int cnt = resourceResults.size();
    vector<MetaDataResult> submd;
    for (int idx=0; idx<cnt && ret; idx++) {
      if (resourceResults[idx].resourcetype == ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
        submd.clear();
        p_EcceDAVClient->url(resourceResults[idx].url);
        ret = getVirtualMetaData(local, submd);
        if (ret) {
          appendMetaDataResults(resourceResults[idx].metaData, submd);
        }
      }
    }
  }
  */
  //---------------------------------------------------------------

  //
  //Are there any errors in the EDSIMessage stack?  If 
  //only return true if no error messages appear.
  //
  ret = !m_msgStack.messages();
  p_EcceDAVClient->setDepthHeader("0");
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::listCollection(vector<ResourceMetaDataResult>& resourceResults) 
{
  bool ret = false;
  initProgress();

  m_msgStack.clear();
  p_EcceDAVClient->url(p_url);
  p_EcceDAVClient->setDepthHeader("1");
  string response;
  vector<MetaDataRequest> requests;

  ret = requestMetaData(requests,resourceResults);
  // Here we add the virtual document code in.
  // For every child that is a virtual document, get the sub meta data
  // properties.
  if (ret) {
    int cnt = resourceResults.size();
    vector<MetaDataResult> submd;
    for (int idx=0; idx<cnt && ret; idx++) {
      if (resourceResults[idx].resourcetype == ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
        submd.clear();
        p_EcceDAVClient->url(resourceResults[idx].url);
        ret = getVirtualMetaData(requests, submd);
        if (ret) {
          appendMetaDataResults(resourceResults[idx].metaData, submd);
        }
      }
    }
  }

  //
  //Are there any errors in the EDSIMessage stack?  If 
  //only return true if no error messages appear.
  //
  ret = !m_msgStack.messages();

  p_EcceDAVClient->setDepthHeader("0");
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::listCollection(vector<ResourceResult>& resourceResults) 
{
  bool ret = false;
  initProgress();
  m_msgStack.clear();
  p_EcceDAVClient->url(p_url);
  ResourceResult resourceResult;
  p_EcceDAVClient->setDepthHeader("1");

  p_EcceDAVClient->url(p_url);
  p_EcceDAVClient->setDepthHeader("1");

  // An empty request means get it all
  vector<MetaDataRequest> requests;
  requestMetaData(requests,resourceResults);

  p_EcceDAVClient->setDepthHeader("0");

  //
  //Are there any errors in the EDSIMessage stack?  If 
  //only return true if no error messages appear.
  //
  ret = !m_msgStack.messages();
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::putDataSet(const char* putStream)
{
  bool ret = false;
  initProgress();
  p_EcceDAVClient->url(p_url);
  m_msgStack.clear();
  int status = p_EcceDAVClient->put_document(putStream);
  if (status == EcceDAVStatus::CREATED || status == EcceDAVStatus::NO_CONTENT)
    ret = true;
  else {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
    ret = false;
  }
  return ret;
}

/*************************************************************************
 Method : putDataSet
 Summary: Copies the contents of the input stream to the current url.
          Caller is responsible for opening/closing the file streams.
**************************************************************************/
bool DavEDSI::putDataSet(istream& putStream) 
{
  bool ret = false;
  initProgress();
  p_EcceDAVClient->url(p_url);
  m_msgStack.clear();
  int status = p_EcceDAVClient->put_document(&putStream);

  if (status == EcceDAVStatus::CREATED || status == EcceDAVStatus::NO_CONTENT)
    ret = true;
  else {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
    ret = false;
  }
  return ret;
}

/*************************************************************************
 Method :
 Summary: 
**************************************************************************/
bool DavEDSI::appendDataSet(const char* putStream, int bytesToOverwrite)
{
  bool ret = false;
  p_EcceDAVClient->url(p_url);
  m_msgStack.clear();
  int status = p_EcceDAVClient->append_document(putStream, bytesToOverwrite);

  if ( status == EcceDAVStatus::NO_CONTENT  || status == EcceDAVStatus::CREATED)
    ret = true;
  else {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
    ret = false;
  }
  return ret;
}

/*************************************************************************
 Method : appendDataSet
 Summary: Appends the contents of the input stream to the current url.
          Caller is responsible for opening/closing the file streams.
**************************************************************************/
bool DavEDSI::appendDataSet(istream& putStream, int bytesToOverwrite)
{
  bool ret = false;
  p_EcceDAVClient->url(p_url);
  m_msgStack.clear();
  int status = p_EcceDAVClient->append_document(&putStream, bytesToOverwrite);
  if ( status == EcceDAVStatus::NO_CONTENT  || status == EcceDAVStatus::CREATED)    ret = true;
  else {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
    ret = false;
  }
  return ret;
}

/*************************************************************************
 Method : removeBadMetaDataRequests 
 Summary: Removes all MetaDataResults with a blank name
**************************************************************************/
void DavEDSI::removeBadMetaDataRequests(vector<MetaDataResult>& requests) {
    vector<MetaDataResult>::iterator removeIt = requests.begin();
    while (removeIt != requests.end()) {
      if ((*removeIt).name == "")
        requests.erase(removeIt);
      else
        removeIt++;
    }
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
void DavEDSI::removeBadMetaDataRequests(vector<MetaDataRequest>& requests) 
{
  vector<MetaDataRequest>::iterator removeIt = requests.begin();
  while (removeIt != requests.end()) {
    if ((*removeIt).name == "")  {
      requests.erase(removeIt);
    } else {
      removeIt++;
    }
  }
}

/*************************************************************************
 Method : 
 Summary: 
 TODO Remove duplicates first or does insert do that?
**************************************************************************/
void DavEDSI::appendMetaDataResults(
                                 vector<MetaDataResult>& results1, 
                                 const vector<MetaDataResult>& results2 ) 
{
  vector<MetaDataResult> neweccestuff;
  vector<MetaDataResult>::const_iterator it;
  for (it=results2.begin(); it!=results2.end(); it++) {
    if ((*it).name.find(VDoc::getEcceNamespace()) != string::npos) {
      neweccestuff.push_back(*it);
    }
  }
  results1.insert(results1.end(), neweccestuff.begin(), neweccestuff.end());

}

/*************************************************************************
 Method :  getVirtualMetaData
 Summary:  The properties ecce is interested in are currently spread
           across the virtual document.  We have to go to the children
           and get the rest of the data we need.  This code should be at
           a higher level AND the virtual document should be self
           describing so the strategy isn't hardwired.

           Calling function must set url to p_url prior to invocation.
**************************************************************************/
bool DavEDSI::getVirtualMetaData(const vector<MetaDataRequest>& requests, 
                                 vector<MetaDataResult>& results) 
{

  //p_EcceDAVClient->setDepthHeader("1"); 
  // Changed to "infinity" because of new VDoc structure, some of the
  // virtual documents metadata will not be found with a depth 1 
  // search.
  p_EcceDAVClient->setDepthHeader("infinity"); 
  vector<ResourceMetaDataResult> bottom;

  // Bottom contains metadata from subnodes at this point.  However some
  // properties like the things in DAV namespace will be redundant with
  // what we got from the top level and we don't want to overwrite them
  // with the childs info.
  // I think its ok as implemented.  The reason why I'm concerned 
  // is that this requestMetaData does a profind that includes the parent node.

  // Get submetadata
  requestMetaData(requests, bottom);

  // Add non-DAV metadata into the results 
  // Don't include metadata from sub-virtual documents
  int i;
  for (i = 0; i < bottom.size(); i++) { 
    if (bottom[i].resourcetype != ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
      vector<MetaDataResult>::iterator rit = bottom[i].metaData.begin();
      while (rit != bottom[i].metaData.end()) {
        if ((*rit).name.find(DAV_NSNAME) == string::npos ) {
          results.push_back(*rit);  
        }
        rit++;
      }
    }
  }
  p_EcceDAVClient->setDepthHeader("0");

  return true;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::getMetaData(const vector<MetaDataRequest>& requests, 
                                vector<MetaDataResult>& results,
                                bool getVDocMetaData) 
{
    bool ret = false;
    initProgress();
    vector<MetaDataRequest> local = requests;

    p_EcceDAVClient->url(p_url);
    m_msgStack.clear();

    // Add properties we need to determine if its a virtual document
    // This version not used by calcmgr so don't do vdoc stuff here.
    // Otherewise we have to be smart enough to strip them out too.
    //addVDocAttributes(local);

    p_EcceDAVClient->setDepthHeader("0");
    ret = requestMetaData(local, results); 

    // If the current thing is a virtual document, then get more attributes
    // Don't bother doing it though if we've already hit an error.
    if (ret && getVDocMetaData && isVDoc(results)) {
      vector<MetaDataResult> submd ;
      ret = getVirtualMetaData(local, submd);
      if (ret) appendMetaDataResults(results, submd);
    }

    //
    //Are there any errors in the EDSIMessage stack?  If
    //only return true if no error messages appear.
    //
    ret = m_msgStack.size() == 0;
    return ret;
}


bool DavEDSI::isVDoc(const vector<MetaDataResult>& properties) 
{
  bool ret = false;
  vector<MetaDataResult>::const_iterator it;
  for (it = properties.begin(); it != properties.end(); it++) {
    ResourceDescriptor::RESOURCETYPE resourceType =
      ResourceUtils::stringToResourceType((*it).value);
    if ((*it).name == VDoc::getEcceNamespace() + ":resourcetype" && 
        resourceType == ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
      ret = true;
      break;
    }
  }
  return ret;
}


/*************************************************************************
 Method : resourceExists
 Summary: 
          Using HEAD method for documents and collections.
          HEAD returns the same response header and status message
          that the GET method returns.  Since HEAD does not 
          return the body of the document, it makes a lightweight
          system metadata query for checking existance, size etc.

          Checking documents:
          If the URI is a document a successful HEAD request
          returns a "200" status.
          Checking collections:
          If the URI is a collection a successful HEAD request
          usually returns a "301" status.

          Philosophy on Function Success or Failure.  
          Since the question that the function is answering is: 
          "Does the resource exist?"  It seemed only right to deem 
          any status message starting with a "2" or a "3" as
          a success.  Rather than look for exact status success
          codes (e.g. 200 for documents and 301 for collections) 
          our experience with Web Server responses is insufficient
          for determining how many 2xx or 3xx codes might be
          returned in a HEAD request.

New error handling attempt (1/5/2001)
. if 5xx or -1, add error to stack
. if 404, return false
. else return true
**************************************************************************/
bool DavEDSI::resourceExists(const bool& newUser)
{
//
//TODO Need more descriptive errrors of what really went on here
//
  bool ret = true;
  int status = p_EcceDAVClient->head(newUser);
  if (status == 404)
  {
    ret = false;
  } else
  {
    if (status >= 400) {
      m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
      ret = false;
    }
  }
  return ret;
}

/*************************************************************************
 Method : addVDocAttributes
 Summary: adds attributes required to properly set ResourceDescriptor::RESOURCETYPE if they
          are not supplied with the request already.  This is needed to 
          support the virtual document functionality BUT should be 
          implemented at a higher level (someday).
**************************************************************************/
void DavEDSI::addVDocAttributes(vector<MetaDataRequest>& requests) 
{
  bool haveDAVresourcetype = false;
  bool haveDAVgetcontenttype = false;
  bool haveecceresourcetype = false;
  bool haveeccecontenttype = false;

  if ( requests.size() != 0 ) {
    vector<MetaDataRequest>::iterator lit = requests.begin();
    while(lit != requests.end()) {
      if ((*lit).name.find("DAV:resourcetype") != string::npos)
        haveDAVresourcetype = true;
      else if ((*lit).name.find("DAV:getcontenttype") != string::npos)
        haveDAVgetcontenttype = true;
      else if ((*lit).name.find(VDoc::getEcceNamespace() + ":resourcetype") != string::npos )
        haveecceresourcetype = true;
      else if ((*lit).name.find(VDoc::getEcceNamespace() + ":contenttype") != string::npos )
        haveeccecontenttype = true;
      lit++;
    }

    MetaDataRequest lsRequest;
    if (!haveDAVresourcetype) {
      lsRequest.name = "DAV:resourcetype";
      requests.push_back(lsRequest);
    }
    if (!haveDAVgetcontenttype) {
      lsRequest.name = "DAV:getcontenttype";
      requests.push_back(lsRequest);
    }
    if (!haveecceresourcetype) {
      lsRequest.name = VDoc::getEcceNamespace() + ":resourcetype";
      requests.push_back(lsRequest);
    }
    if (!haveeccecontenttype) {
      lsRequest.name = VDoc::getEcceNamespace() + ":contenttype";
      requests.push_back(lsRequest);
    }
  } 

}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::getMetaData(const vector<MetaDataRequest>& requests, 
                                 ResourceMetaDataResult& result,
                                 bool getVDocMetaData) 
{

  initProgress();
  p_EcceDAVClient->url(p_url);
  m_msgStack.clear();
  bool ret = false;

  vector<MetaDataRequest> local = requests;

  // Add properties we need to determine if its a virtual document
  //addVDocAttributes(local);

  // Get the requested meta data for the current url only
  p_EcceDAVClient->setDepthHeader("0");
  ret = requestMetaData(local, result);

  if (ret && 
      getVDocMetaData && 
      result.resourcetype == ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
    vector<MetaDataResult> submd ;
    ret = getVirtualMetaData(local, submd);
    appendMetaDataResults(result.metaData, submd);
  }
  //
  //Are there any errors in the EDSIMessage stack?  If
  //only return true if no error messages appear.
  //
  ret = !m_msgStack.messages();
  return ret;
}


/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
string DavEDSI::createMetaDataRequestString(vector<MetaDataRequest>& requests)
{ 
  string ret = davPropfindHeader();
  if (requests.empty()) {
    ret += davPropfindAllpropNullElement();
    ret += davPropfindFooter();
  } else {
    vector<MetaDataRequest> localRequests ;
    removeBadMetaDataRequests(requests);
    vector<DavNameSpaceAlias> DavNameSpaces = parsePropertyNames(requests);
    ret += davPropElementOpenTag(assembleNameSpaceString(DavNameSpaces));
    ret += assembleMetaDataRequestString(requests);
    ret += davPropElementCloseTag();
    ret += davPropfindFooter();
  }
  return ret;
}


/*************************************************************************
 Method : requestMetaData
 Summary: Fills the results vector with the properties requested in the
          requests vector.  The length of the results vector will be equal
          to the length of the requests vector.  If a particular property in
          the list does not exist for the current URL, then the value of
          that property will be "".

          This version returns
**************************************************************************/
bool DavEDSI::requestMetaData(const vector<MetaDataRequest> requests,
                              vector<EcceURL>& results)
{
  vector<MetaDataRequest> local = requests;
  string davRequest = createMetaDataRequestString(local);
  results.clear();
  string response;
  bool ret = false;


  bool isSuccessful = sendMetaDataRequest(CDAVClient::PROPFIND,
                                          davRequest, response) ;
  if (isSuccessful) {
    if (processFindResponses(local, response, results))
      ret = true;
  }

  return ret;
}


/*************************************************************************
 Method : requestMetaData
 Summary: Fills the results vector with the properties requested in the
          requests vector.  The length of the results vector will be equal
          to the length of the requests vector.  If a particular property in
          the list does not exist for the current URL, then the value of
          that property will be "".

          This version returns 
**************************************************************************/
bool DavEDSI::requestMetaData(const vector<MetaDataRequest> requests, 
                              vector<ResourceMetaDataResult>& results)
{
  vector<MetaDataRequest> local = requests;
  string davRequest = createMetaDataRequestString(local); 
  results.clear();
  string response;
  bool ret = false;


  bool isSuccessful = sendMetaDataRequest(CDAVClient::PROPFIND, 
                                          davRequest, response) ;
  if (isSuccessful) {
    if (processListCollectionResponses(local, response, results))
      ret = true;
  }

  return ret;
}

/*************************************************************************
 Method : requestMetaData
 Summary: 
**************************************************************************/
bool DavEDSI::requestMetaData(const vector<MetaDataRequest> requests,
                              ResourceMetaDataResult& result)
{
  vector<MetaDataRequest> local = requests;
  string davRequest = createMetaDataRequestString(local);
  string response;
  bool ret = false;

  bool isSuccessful = sendMetaDataRequest(CDAVClient::PROPFIND, 
                                          davRequest, response) ;
  if (isSuccessful) {
    if (processDAVResponses(response, result))
      ret = true;
  } 
  return ret;
}

/*************************************************************************
 Method : requestMetaData
 Summary: 
**************************************************************************/
bool DavEDSI::requestMetaData(const vector<MetaDataRequest> requests, 
                              vector<ResourceResult>& results) 
{
  vector<MetaDataRequest> local = requests;
  string davRequest = createMetaDataRequestString(local);
  results.clear();
  string response;
  bool ret = false;

  bool isSuccessful = sendMetaDataRequest(CDAVClient::PROPFIND, 
                                          davRequest, response) ;
  if (isSuccessful) {
    if (processListCollectionResponses(local, response, results))
      ret = true;
  }
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::requestMetaData(const vector<MetaDataRequest> requests,
                              vector<MetaDataResult>& results)
{
  vector<MetaDataRequest> local = requests;
  string davRequest = createMetaDataRequestString(local);
  results.clear();
  string response;
  bool ret = false;


  bool isSuccessful = sendMetaDataRequest(CDAVClient::PROPFIND, 
                                          davRequest, response) ;

  if (isSuccessful) {
    if (processDAVResponses(response, results))
      ret = true;
  }

  return ret;
}

/*************************************************************************
 Method : 
 Summary: Assembles chunk of XML properties for a propfind request.
**************************************************************************/
string DavEDSI::assembleMetaDataRequestString (
                const vector<MetaDataRequest>& requests) const 
{
  string ret;
  int cnt = requests.size();
  for(int idx = 0; idx < cnt; idx++) {
    ret += " <" + requests[idx].name + "/> \n"; 
  }
  return ret;

}
/*************************************************************************
 Method : 
 Summary: Assembles a chunk of XML properties for a proppatch request
**************************************************************************/
string DavEDSI::assembleMetaDataRequestString (
                const vector<MetaDataResult>& requests) const {
  string ret = "";
  int cnt = requests.size();
  for (int idx = 0; idx < cnt; idx++) {
    ret += " <";
    ret +=  requests[idx].name;
    ret +=   ">";
    ret +=  requests[idx].value;
    ret +=   "</";
    ret +=   requests[idx].name;
    ret +=   ">\n"; 
  }
  return ret;
}

/*************************************************************************
 Method : assembleNameSpaceString
 Summary: 
**************************************************************************/
string DavEDSI::assembleNameSpaceString(
                const vector<DavNameSpaceAlias>& davNameSpaces) const 
{
   string ret;


   //////////////////////////////////////////
   // CMCS Namespaces - should not append a ":"
   //   CMCS will not recognize the namespace
   //   with a ":" appended
   //////////////////////////////////////////
   string dublinCore = "http://purl.org/dc/elements/1.1/";
   string standardCMCS = "http://purl.oclc.org/NET/cmcs/schema/";


   int cnt = davNameSpaces.size();
   for (int idx = 0; idx < cnt; idx++) {



     if ( (davNameSpaces[idx].NameSpace == dublinCore) ||
          (davNameSpaces[idx].NameSpace == standardCMCS) 
        ) {  
       ret += " xmlns";
       if (!davNameSpaces[idx].Alias.empty()) ret += ":";
       ret += davNameSpaces[idx].Alias + 
                "=\"" +  
                davNameSpaces[idx].NameSpace + 
                "\" ";
     } 
     else {
       ret += " xmlns";
       if (!davNameSpaces[idx].Alias.empty()) ret += ":";
       ret += davNameSpaces[idx].Alias + 
                "=\"" +  
                davNameSpaces[idx].NameSpace + 
                ":\" ";
     }
   }
   return ret;
}

/*************************************************************************
 Method : putMetaData
 Summary: 
**************************************************************************/
bool DavEDSI::putMetaData(const vector<MetaDataResult>& results) 
{
   p_EcceDAVClient->url(p_url);
   m_msgStack.clear();
   vector<MetaDataResult> localResults ;
   localResults = results;
   removeBadMetaDataRequests(localResults);
   vector<DavNameSpaceAlias> DavNameSpaces = parsePropertyNames(localResults);
   string davRequest = davProppatchHeader(
                                    assembleNameSpaceString(DavNameSpaces)); 
   davRequest += davProppatchSetElementHeader();
   davRequest += davPropElementOpenTag();
   davRequest += assembleMetaDataRequestString(localResults);
   davRequest += davPropElementCloseTag();
   davRequest += davProppatchSetElementFooter();
   davRequest += davProppatchFooter();
   string response;

   return sendMetaDataRequest(CDAVClient::PROPPATCH, davRequest, response);
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::removeMetaData(const vector<MetaDataRequest>& requests) 
{
   initProgress();
   p_EcceDAVClient->url(p_url);
   m_msgStack.clear();
   vector<MetaDataRequest> localRequests ;
   localRequests = requests;
   removeBadMetaDataRequests(localRequests);
   vector<DavNameSpaceAlias> DavNameSpaces = parsePropertyNames(localRequests);
   string davRequest = davProppatchHeader(
         assembleNameSpaceString(DavNameSpaces));
   davRequest += davProppatchRemoveElementHeader();
   davRequest += davPropElementOpenTag();
   davRequest += assembleMetaDataRequestString(localRequests);
   davRequest += davPropElementCloseTag();
   davRequest += davProppatchRemoveElementFooter();
   davRequest += davProppatchFooter();

   string response;
   return sendMetaDataRequest(CDAVClient::PROPPATCH, davRequest, response);
}

/**
 * Get property value from property node.
 *   We anticipate finding any of the following:
 *     <ns:propname><ns:whatever/></ns:propname>
 *     <ns:propname>whatever</ns:propname>
 *     <ns:propname>"whatever with"</ns:propname>
 *     <ns:propname><element><morexml/></element></ns:propname>
 *
 *   This method is meant to extract a property value as a scalar string.
 *   The incoming node is expected to be an element of the form
 *   for example: <creationDate>.  For elements
 *   with just text nodes, we return the text node as a string.
 *   Otherwise, we return an xml string of its entire hierarchy.
 *
 *   Note that if the property has embedded xml, we need to get that instead
 *   of the text string.  You CANNOT assume that the first child is what
 *   you want.  Worse, you may have both a text node and an element child
 *   node but hopefully this only happens if the text is white space.
 *   I will be making that assumption for now and returning the node value.
 *   We do NOT support properties that don't have a wrapper node (ie there
 *   should not be more than one element child).
 *   So three cases are handled:
 *     1) no child nodes - dump element
 *     2) one child - could be text or element
 *     3) two or more children - one or more presumed to be junk text, 
 *        the other the element data we want. (I was seeing two text nodes 
 *        from sam).
 */
string DavEDSI::getPropertyValue(const DOMNode& node) const 
{
   string ret;

   // detect programming errors - compiled out at install
   EE_ASSERT((node.getNodeType()==DOMNode::ELEMENT_NODE),
         EE_WARNING,"getPropertyValue called with bad node type");

   DOMNodeList *children = node.getChildNodes();
   if (children->getLength() == 0) {
      // Spec says children will not be null
      // Not sure what this would result in - empty string???
      ostrstream os;
      BasicDOMWriter::write(node,os,false,true);
      os << ends;
      ret = os.str();
   } else if (children->getLength() == 1) {
      DOMNode *child = node.getFirstChild();
      if (child->getNodeType() == DOMNode::TEXT_NODE) {
         ret = transcode(child->getNodeValue());
      } else if (child->getNodeType() == DOMNode::ELEMENT_NODE) {
         ostrstream os;
         BasicDOMWriter::write(*child,os,false,true);
         os << ends;
         ret = os.str();
      }
   } else if (children->getLength() >= 2) {
      for (int idx=0; idx<children->getLength(); idx++) {
         DOMNode *n = children->item(idx);
         if (n->getNodeType() == DOMNode::ELEMENT_NODE) {
            ostrstream os;
            BasicDOMWriter::write(*n,os,false,true);
            os << ends;
            ret = os.str();
            break;
         }
      }
   }

   return ret;
}


/*************************************************************************
 Method : describeMetaData
 Summary: gets the names of all the properties defined for the current
          url.  This method does NOT handle virtual documents.
**************************************************************************/
bool DavEDSI::describeMetaData(vector<MetaDataResult>& metaDataResults) 
{
  initProgress();
  metaDataResults.clear();
  p_EcceDAVClient->url(p_url);
  m_msgStack.clear();

  bool ret = false;

#ifdef vdochere
  if ( checkVirtualDocument() ) {
     p_EcceDAVClient->setDepthHeader("INFINITY");
  }
  
  if ( p_EcceDAVClient->getDepthHeader().empty() ) {
     p_EcceDAVClient->setDepthHeader("0");
  }
#else
  p_EcceDAVClient->setDepthHeader("0");
#endif

  string davRequest = davPropfindHeader();
  davRequest += davPropfindPropnameNullElement();
  davRequest += davPropfindFooter();

  string response;
  ret = sendMetaDataRequest(CDAVClient::PROPFIND, davRequest, response);
  if (ret) {
    ret = processDescribeResponses(response, metaDataResults);
  }
  
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::parseXmlDocString(string& response, DOMDocument*& doc) {
   BasicDOMParser parser;
   string currentHref; 
   bool ret = false;
   //cout << "PARSING" << endl;
   //cout << response << endl;
   doc = parser.parse(response);
   if ( doc != 0 && doc->hasChildNodes() ) {
     ret = true;
   }

   return ret;
}


/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
void DavEDSI::getResponseNameSpaces(DOMNode& response, 
                                    vector<DavNameSpaceAlias>& nameSpaces) 
{
  DavNameSpaceAlias nameSpace;
  DOMNamedNodeMap *attributes = response.getAttributes();
  int attrCount = 0;
  if (attributes != 0) attrCount = attributes->getLength();

  DOMNode *attribute = 0;

  // Now parse others
  for (int idx = 0; idx < attrCount; idx++) {
    attribute = attributes->item(idx);
    nameSpace.Alias = transcode(attribute->getNodeName());
    if (nameSpace.Alias.find("xmlns:") == 0) {
      nameSpace.Alias.erase(0,6);
    } else {
      // Must be dfault xmlns=
      nameSpace.Alias = "";
    }
    nameSpace.NameSpace = transcode(attribute->getNodeValue());
    if (nameSpace.NameSpace == "http://apache.org/dav/props/") 
      nameSpace.NameSpace = DAV_NSNAME;
    else {
    //
    //We need to do an rfind here because we might have more than one colon
    //in the namespace
    //
      if (nameSpace.NameSpace.rfind('/') != (nameSpace.NameSpace.length() - 1)) {
        int colonLocation = nameSpace.NameSpace.rfind(':');
        if (colonLocation > 0) 
          nameSpace.NameSpace.erase(colonLocation, 1);
      }
    } 
    addNamespaceAlias(nameSpaces,nameSpace.Alias,nameSpace.NameSpace);
   }
}

/*
 Add a namespace alias/namespace pair to the data structure allowing
 for no duplicates.  If the alias exists, then will be replaced by
 new namespace.

 The namespace vector should be a map!
*/
void DavEDSI::addNamespaceAlias( vector<DavNameSpaceAlias>& nameSpaces,
                                 const string& alias,
                                 const string& value)
{
  // Remove any namespaces with matching alias
  vector<DavNameSpaceAlias>::iterator it = nameSpaces.begin();
  while (it != nameSpaces.end()) {
    if ((*it).Alias == alias) {
      // erase operation will increment iterator
      nameSpaces.erase(it);
    }
    else {
      it++;
    }
  }

  // go ahead and add new alias 
  DavNameSpaceAlias dns;
  dns.Alias = alias;
  dns.NameSpace = value;
  nameSpaces.push_back(dns);

}


/*************************************************************************
 Check to see if the response node's url is the same as the
 current url we are interested in.  
**************************************************************************/
bool DavEDSI::isHrefCurrentURL(DOMNode& response) 
{
   bool ret = false;

   string currentFileValue = p_url.getFile();
   if (currentFileValue.rfind('/') == (currentFileValue.length()-1)) {
     currentFileValue.erase(currentFileValue.length()-1);
   }

   string thisurl = getResponseURL(response).getFile();
   if (thisurl.rfind('/') == thisurl.length()-1) {
     //thisurl.erase(thisurl.length()-1);
     thisurl =  thisurl.substr(0, thisurl.rfind('/'));
   }

   if (thisurl == currentFileValue)  {
     ret = true;   
   }

   return ret;
} 



/*************************************************************************
 Method : 
 Summary: 
 Since the HREF string may be
 prepended with a namespace or not, iterate through and look for elements
 of either href of ns:href - can't use getElementsByTagName unless we
 first change code to have the current DAV namespace alias readily
 available.

 Note that this method only looks at the first href it finds.  According
 to the spec, this doesn't look conformant but has worked so far. From
 the spec:
 <!ELEMENT response (href, ((href*, status)|(propstat+)),
          responsedescription?) >
**************************************************************************/
EcceURL 
DavEDSI::getResponseURL(DOMNode& response)
{
  string urlstr;

   DOMNodeList *children = response.getChildNodes();
   int numChildren = 0;
   if (children != 0) numChildren = children->getLength();

   for (int idx=0; idx<numChildren; idx++) {

     DOMNode *href = children->item(idx);
     // We are assuming that the first element is an HREF
     if (href != 0 && href->getNodeType() == DOMNode::ELEMENT_NODE &&
         transcode(href->getNodeName()).find(vHREF) != string::npos) {
       if (href->hasChildNodes()) {
         DOMNode *child = href->getFirstChild();
         urlstr = transcode(child->getNodeValue());
         break;
       }
     }
   }
   EE_ASSERT(!urlstr.empty(),EE_WARNING,"UNable to dig out URL");

   EcceURL ret;
   ret.set(p_url.getProtocol().c_str(),
              p_url.getHost().c_str(),
              p_url.getPort(),
              urlstr.c_str() );

   return UrlDecoder::decode(ret);
   return ret;
} 

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::getResponsePropertyList(DOMNode& response,
                                      DOMNodeList*& propertylist)
{
  bool ret = false;

  // A small hack to get the namespaces in place correctly.  Assume that
  // the ns used in the response node is also used in the sub-elements we
  // wish to look up in this code
  string ns = transcode(response.getNodeName());
  int pos = ns.rfind(':');
  if (pos > 0) {
    ns = ns.substr(0,pos+1);
  } else {
    ns = "";
  }
  string key;

  string message;

  key = ns + vPROPSTAT;
  XMLCh *xStr = XMLString::transcode(key.c_str());
  DOMNodeList *propstat = ((DOMElement &)response).getElementsByTagName(xStr);
  delete [] xStr;

  int numPS = 0;
  if (propstat != 0) numPS = propstat->getLength();

  for (int ps = 0; ps < numPS; ps++) {
    DOMElement *el = (DOMElement *)propstat->item(ps);
    key = ns + vSTATUS;
    XMLCh *xStr = XMLString::transcode(key.c_str());
    DOMNodeList *statuses = el->getElementsByTagName(xStr);
    delete [] xStr;

    DOMElement *status = (DOMElement *) statuses->item(0);
    DOMNode *child = status->getFirstChild();
    string value = transcode(child->getNodeValue());
    if (value.find("200") != string::npos) {
      key = ns + vPROP;
      xStr = XMLString::transcode(key.c_str());
      DOMNodeList *validproperties = el->getElementsByTagName(xStr);
      delete [] xStr;
      DOMElement *prop = (DOMElement *)validproperties->item(0);
      propertylist = prop->getChildNodes();
      ret = true;
    } else if (value.find("404") != string::npos) {
      ret = true;
    }
    else
    {
      m_msgStack.add("INCOMPLETE_TRANSACTION", value.c_str());
    }
  }
  return ret;
}



/**
 * Given propertyelement, fill in the MetaDataResult object.
 * Note that some servers supply the namespace right on the property node.
 * <o:resourcetype xmlns:o="....">value</o:resourcetype>
 * I'm not positive if this code handles the default namespace (i.e.
 * <resourcetype xmlns="....">value</resourcetype>
 * So if we find the namespace there, use it.  Otherwise use the namesapce
 * alias list passed in.
 */
void DavEDSI::getMetaDataResult( DOMElement& property, 
                                 vector<DavNameSpaceAlias>& nameSpaces, 
                                 MetaDataResult& result )  
{
   if (property.getNodeType() == DOMNode::ELEMENT_NODE) {
      vector<DavNameSpaceAlias> localNamespaces = nameSpaces;
      getResponseNameSpaces(property, localNamespaces);
      string values;
      result.name = XMLString::transcode(property.getNodeName());
      assignElementNameSpace(result.name, localNamespaces);
//      cout << "getting property value for " << result.name;
      if (property.hasChildNodes()) {
         values = getPropertyValue(property);
      }
      //cout << values << endl;
      result.value = values;
   }

}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
ResourceDescriptor::RESOURCETYPE DavEDSI::getDavResourceType(string& value) const 
{
  if (value == "") 
    return ResourceDescriptor::RT_DOCUMENT;
  else 
    return ResourceDescriptor::RT_COLLECTION;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
string DavEDSI::getDavContentType(ResourceDescriptor::RESOURCETYPE value, EcceURL& url) const 
{
  string ret = "file";
  if (value == ResourceDescriptor::RT_DOCUMENT) 
    ret = getFileContentType(url);
  else if (value == ResourceDescriptor::RT_COLLECTION)
    // If RT is a collecction then the CT is an ecce project
    ret = ResourceUtils::contentTypeToString(ResourceDescriptor::CT_PROJECT);
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
string DavEDSI::getFileContentType(const EcceURL& url) const 
{
  string ret = url.getFilePathTail();
  //Does the file have a suffix?
  if ((ret.rfind(".") == (ret.size() - 1)) ||
      (ret.rfind(".") == string::npos)) 
    ret = "file";
  else if (ret.rfind(".") == 0)   
    ret = "hidden";
  else 
    ret = ret.substr(ret.rfind(".")+1, ret.size() -1); 
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
void DavEDSI::getResourceMetaDataResult(DOMElement& property,
                                        vector<DavNameSpaceAlias>& nameSpaces,
                                        ResourceMetaDataResult& result) 
{
  // The namespace alias may be at the property level rather than the
  // response or multistatus level so we gotta deal with it.
  getResponseNameSpaces(property, nameSpaces);

  MetaDataResult mr;
  getMetaDataResult(property, nameSpaces, mr);
  string eccens = VDoc::getEcceNamespace();

  ResourceDescriptor::RESOURCETYPE resourceType =
    ResourceUtils::stringToResourceType(mr.value);

  if ((mr.name.find("DAV:resourcetype") != string::npos) &&
      (result.resourcetype != ResourceDescriptor::RT_VIRTUAL_DOCUMENT)) {
    result.resourcetype = getDavResourceType(mr.value);
  } 
  if ((mr.name.find("DAV:getcontenttype") != string::npos) &&
             (result.contenttype.empty()) ) {
    result.contenttype = mr.value;
  } 
  if (mr.name.find(eccens + ":contenttype") != string::npos) {
    result.contenttype = mr.value;
  } 
  if ((mr.name.find(eccens + ":resourcetype") != string::npos) &&
             (resourceType == ResourceDescriptor::RT_VIRTUAL_DOCUMENT))  {
    result.resourcetype = ResourceDescriptor::RT_VIRTUAL_DOCUMENT;
  } 
  result.metaData.push_back(mr);
}



/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::processDescribeResponses(string& response, 
                                       vector<MetaDataResult>& results) {
  MetaDataResult result;
  DavNameSpaceAlias nameSpace;
  vector<DavNameSpaceAlias> nameSpaces;
  DOMNodeList *proplist = 0;
  DOMDocument *doc = 0;

  if (!parseXmlDocString(response, doc)) 
    return false;

  DOMElement *multistatus = doc->getDocumentElement();
  getResponseNameSpaces(*multistatus, nameSpaces);

  DOMNodeList *responses = multistatus->getChildNodes();
  int numResponses = 0;
  if (responses !=0) numResponses = responses->getLength();
  for (int idx = 0; idx < numResponses; idx++) {
    DOMNode *responseNode = responses->item(idx);
    if (transcode(responseNode->getNodeName()).find(vRESPONSE) != string::npos) {
      getResponseNameSpaces(*responseNode, nameSpaces);
      int numProps = 0;
      proplist = (DOMNodeList *) 0;
      if (getResponsePropertyList(*responseNode, proplist)) {
        string values;
        if (proplist != 0)
          numProps = proplist->getLength();
        else
          numProps = 0;
        result.name = "";
        result.value = "";
        for (int pdx = 0; pdx < numProps; pdx++) {
          DOMElement *aprop = (DOMElement *) proplist->item(pdx);
          if (aprop->getNodeType() == DOMNode::ELEMENT_NODE) {
            values = "";
            result.name = transcode(aprop->getNodeName());
            getMetaDataResult(*aprop, nameSpaces, result);
            results.push_back(result);
          }
        }
      } 
    }
  }
  doc->release();
  return true;
}


/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::processDAVResponses(string& response, 
                                  vector<MetaDataResult>& results) {
  MetaDataResult result;
  DavNameSpaceAlias nameSpace;
  vector<DavNameSpaceAlias> nameSpaces;
  DOMNodeList *proplist = 0;
  DOMDocument *doc = 0;

  if (!parseXmlDocString(response, doc))
    return false;

  DOMElement *multistatus = doc->getDocumentElement();
  getResponseNameSpaces(*multistatus, nameSpaces);

  DOMNodeList *responses = multistatus->getChildNodes();
  int numResponses = 0;
  if (responses != 0) numResponses = responses->getLength();

  int numProps = 0;
  for (int idx=0; idx< numResponses; idx++) {
    DOMNode *responseNode = responses->item(idx);
    if (transcode(responseNode->getNodeName()).find(vRESPONSE) != string::npos) {
      getResponseNameSpaces(*responseNode, nameSpaces);
      proplist = (DOMNodeList *) 0;
      if (getResponsePropertyList(*responseNode, proplist)) {
        string values;
        if (proplist != 0)
          numProps = proplist->getLength();
        else
          numProps = 0;
        result.name = "";
        result.value = "";
        for (int pdx=0; pdx<numProps; pdx++) {
          DOMElement *aprop = (DOMElement *) proplist->item(pdx);
          if (aprop->getNodeType() == DOMNode::ELEMENT_NODE) {
            values = "";
            result.name = transcode(aprop->getNodeName());
            if (aprop->hasChildNodes()) {
              getMetaDataResult(*aprop, nameSpaces, result);
              results.push_back(result);
            }
          }
        }
      }
    }
  }
  doc->release();
  return true;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::processDAVResponses(string& response, 
                                  ResourceMetaDataResult& result) 
{
  DavNameSpaceAlias nameSpace;
  vector<DavNameSpaceAlias> nameSpaces;
  DOMNodeList *proplist = 0;
  DOMDocument *doc = 0;

  if (!parseXmlDocString(response, doc))
    return false;


  DOMElement *multistatus = doc->getDocumentElement();
  getResponseNameSpaces(*multistatus, nameSpaces);


  DOMNodeList *responses = multistatus->getChildNodes();

  int numResponses = 0;
  if (responses != 0) numResponses = responses->getLength();

  for (int idx = 0; idx < numResponses; idx++) {
    DOMNode *response = responses->item(idx);
    if (transcode(response->getNodeName()).find(vRESPONSE) != string::npos) {
      getResponseNameSpaces(*response, nameSpaces);
      result.url = getResponseURL(*response);
      int numProps = 0;
      proplist = (DOMNodeList *) 0;
      if (getResponsePropertyList(*response, proplist)) {
        if (proplist != 0)
          numProps = proplist->getLength();
        else
          numProps = 0;
        result.metaData.clear();
        result.resourcetype = ResourceDescriptor::RT_DOCUMENT;
        result.contenttype = "";
        for (int pdx = 0; pdx < numProps; pdx++) {
          DOMElement *aprop = (DOMElement *) proplist->item(pdx);
          if (aprop->getNodeType() == DOMNode::ELEMENT_NODE) {
            if (aprop->hasChildNodes())
              getResourceMetaDataResult(*aprop, nameSpaces, result);
          }
        }
      }
      if (result.contenttype.empty())
        result.contenttype = getDavContentType(result.resourcetype, result.url);
    }
  }
  doc->release();
  return true;
}


bool DavEDSI::processFindResponses(vector<MetaDataRequest>& requests,
                                   string& response,
                                   vector<EcceURL>& results )
{
  EcceURL url;
  DavNameSpaceAlias nameSpace;
  vector<DavNameSpaceAlias> nameSpaces;
  DOMDocument *doc = 0;

  if (!parseXmlDocString(response, doc))
    return false;

  DOMElement *multistatus = doc->getDocumentElement();
  getResponseNameSpaces(*multistatus, nameSpaces);

  DOMNodeList *responses = multistatus->getChildNodes();

  int numResponses = 0;
  if (responses != 0) numResponses = responses->getLength();

  for (int idx=0; idx< numResponses; idx++) {
    DOMNode *response = responses->item(idx);
    if (transcode(response->getNodeName()).find(vRESPONSE) != string::npos) {
      getResponseNameSpaces(*response, nameSpaces);
      url = getResponseURL(*response);
      results.push_back(url);
      if (isHrefCurrentURL(*response))
        continue; //break;
    }
  }
  doc->release();
  return true;
}



/*************************************************************************
 Method : processListCollectionResponses
 Summary: The processed data is essentially the result of a propfind.
          Namespaces may be defined in each vRESPONSE element or they may
          be defined primarily in the vMULTISTATUS element (slide) so we
          have to look both places.
**************************************************************************/
bool DavEDSI::processListCollectionResponses(
                                  vector<MetaDataRequest>& requests,
                                  string& response, 
                                  vector<ResourceMetaDataResult>& results ) 
{
  ResourceMetaDataResult resourceResult;
  DavNameSpaceAlias nameSpace;
  vector<DavNameSpaceAlias> nameSpaces;
  DOMNodeList *proplist = 0;
  DOMDocument *doc = 0;

  if (!parseXmlDocString(response, doc))
    return false;

  // Namespace alii may be in the multistatus element
  DOMElement *multistatus = doc->getDocumentElement();
  getResponseNameSpaces(*multistatus, nameSpaces);

  // According to the spec, the responses are children of the multistatus
  // node.  If this fails for some reason, then need to figure out a
  // different way to figure out how to get response nodes.  You have to
  // know the namespace - could be response or d:response...
  DOMNodeList *responses = multistatus->getChildNodes();
  int numResponses = 0;
  if (responses != 0) numResponses = responses->getLength();

  for (int idx=0; idx< numResponses; idx++) {
    DOMNode *response = responses->item(idx);
    if (transcode(response->getNodeName()).find(vRESPONSE) != string::npos) {
      if (isHrefCurrentURL(*response)) 
        continue; //break; 

      getResponseNameSpaces(*response, nameSpaces);

      resourceResult.url = getResponseURL(*response);
      int numProps = 0;
      proplist = (DOMNodeList *) 0;
      if (getResponsePropertyList(*response, proplist)) {
        if (proplist != 0)
          numProps = proplist->getLength();
        else
          numProps = 0;
        resourceResult.metaData.clear();
        resourceResult.resourcetype = ResourceDescriptor::RT_DOCUMENT;
        resourceResult.contenttype = "";
        for (int pdx=0; pdx<numProps; pdx++) {
          DOMElement *aprop = (DOMElement *) proplist->item(pdx);
          if (aprop->getNodeType() == DOMNode::ELEMENT_NODE) {
            if (aprop->hasChildNodes()) 
              getResourceMetaDataResult(*aprop, nameSpaces,  resourceResult);
          }
        }
      }
      if (resourceResult.contenttype.empty()) 
        resourceResult.contenttype = 
          getDavContentType(resourceResult.resourcetype, resourceResult.url);

      results.push_back(resourceResult);
    }
  }
  doc->release();
  return true;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::processListCollectionResponses(
                                          vector<MetaDataRequest>& requests, 
                                          string& response, 
                                          vector<ResourceResult>& results ) 
{ 
  ResourceMetaDataResult resourceResult;
  DavNameSpaceAlias nameSpace;
  vector<DavNameSpaceAlias> nameSpaces;
  DOMNodeList *proplist = 0;
  DOMDocument *doc = 0;

  if (!parseXmlDocString(response, doc))
    return false;

  DOMElement *multistatus = doc->getDocumentElement();
  getResponseNameSpaces(*multistatus, nameSpaces);

  DOMNodeList *responses = multistatus->getChildNodes();
  int numResponses = 0;
  if (responses != 0) numResponses = responses->getLength();

  for (int idx=0; idx< numResponses; idx++) {
    DOMNode *response = responses->item(idx);
    if (transcode(response->getNodeName()).find(vRESPONSE) != string::npos) {
      if (p_EcceDAVClient->url().toString().find("0") != string::npos
          && isHrefCurrentURL(*response) )
        continue; //break; 

      getResponseNameSpaces(*response, nameSpaces);
      resourceResult.url = getResponseURL(*response);
      int numProps = 0;
      proplist = (DOMNodeList *) 0;
      if (getResponsePropertyList(*response, proplist)) {
        if (proplist != 0)
          numProps = proplist->getLength();
        else
          numProps = 0;
        resourceResult.metaData.clear();
        resourceResult.resourcetype = ResourceDescriptor::RT_DOCUMENT;
        resourceResult.contenttype = "";
        for (int pdx=0; pdx<numProps; pdx++) {
          DOMElement *aprop = (DOMElement *) proplist->item(pdx);
          if (aprop->getNodeType() == DOMNode::ELEMENT_NODE) {
            if (aprop->hasChildNodes())
              getResourceMetaDataResult(*aprop, nameSpaces, resourceResult);
          }
        }
      }
      if (resourceResult.contenttype.empty()) 
        resourceResult.contenttype = 
          getDavContentType(resourceResult.resourcetype, resourceResult.url);
      results.push_back(resourceResult);
    }
  }
  doc->release();
  return true;
}

/*************************************************************************
 Method : 
 Summary: Using the data structure that contains a mapping of namespace
 aliases to namespace names, convert the alias of a given element to its
 proper name.  Have to be aware that the default namespace may be used by
 the server.

 Examples: 
    ns0:created => http://www.emsl.pnl.gov/ecce:created
    resourcetype => DAV:resourcetype
    D:resourcetype => DAV:resourcetype
**************************************************************************/
void DavEDSI::assignElementNameSpace(string & element, 
                                     vector<DavNameSpaceAlias>& nameSpaces) 
{
  int endOfNameSpace = element.find(':');
  string alias;  // empty string = default ns

  // I think it can't be 0.
  //    ns0:thing -> ok
  //    thing -> ok
  //    :thing -> not ok
  if (endOfNameSpace > 0) {
    alias = element.substr(0, endOfNameSpace);
    element.erase(0, endOfNameSpace+1);
  }

  // alias
  for (int i = 0; i < nameSpaces.size(); i++) {
    if (nameSpaces[i].Alias == alias ) {
      element.insert(0, ":");
      element.insert(0, nameSpaces[i].NameSpace);
      break;
    }
  }
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::moveResource(EcceURL& targetURL, EDSIOverwrite overwrite) 
{
  bool ret = false;
  initProgress();
  p_EcceDAVClient->url(p_url);
  m_msgStack.clear();
  EcceURL localTargetURL = targetURL;
  int istatus;
  string message;

  switch (overwrite) {
    case YES:  {
      istatus = p_EcceDAVClient->move(localTargetURL);
      break;
    }
    case NO:  {
      p_EcceDAVClient->url(localTargetURL);
      if (resourceExists()) 
        istatus = EcceDAVStatus::PRECONDITION_FAILED;
      else {
        p_EcceDAVClient->url(p_url);
        istatus = p_EcceDAVClient->move(localTargetURL); 
      }
      m_msgStack.clear();
      break;
    }
    case SORTOF:  {
      localTargetURL = uniqueName(localTargetURL);
      //
      //Had to set p_EcceDAVClient here because uniqueName uses this 
      //to check for the existance of URL.
      //
      p_EcceDAVClient->url(p_url);
      istatus = p_EcceDAVClient->move(localTargetURL);
      break;
    }
  }
  p_EcceDAVClient->url(localTargetURL);  

  if (istatus != 201 && istatus != 204) {
    m_msgStack.clear();
    m_msgStack.add(EcceDAVStatus::edsiMessage(istatus).c_str(), EcceDAVStatus::text(istatus).c_str());
  } else 
  {
    ret = true;
  }

  targetURL = localTargetURL;
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::copyResource(EcceURL& targetURL, EDSIOverwrite overwrite) 
{
  bool ret = false;
  initProgress();
  EcceURL localTargetURL = targetURL;
  p_EcceDAVClient->url(p_url);
  int status;
  string message;

  switch (overwrite) {
    case YES:  {
      status = p_EcceDAVClient->copy(localTargetURL);
      break;
    }
    case NO:  {
      p_EcceDAVClient->url(localTargetURL);
      if (resourceExists())
        message = "overwrite condition = NO";
      else {
        p_EcceDAVClient->url(p_url);
        status = p_EcceDAVClient->copy(localTargetURL);
      }
      m_msgStack.clear();
      break;
    }
    case SORTOF:  {
      localTargetURL = uniqueName(localTargetURL);
      //
      //Have to set p_EcceDAVClient here because uniqueName uses this 
      //to check for the existance of URL.
      //
      p_EcceDAVClient->url(p_url);

      status = p_EcceDAVClient->copy(localTargetURL);
      break;
    }
  }
  if ( status != EcceDAVStatus::NO_CONTENT  && status != EcceDAVStatus::CREATED)
  {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
  } else {
    ret = true;
  }

  // Modify the in/out parameter targetURL to new name
  targetURL = localTargetURL;
  return ret;

}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::removeResource() 
{
  bool ret = false;
  initProgress();
  p_EcceDAVClient->url(p_url);
  int status = p_EcceDAVClient->delete_document();
  p_EcceDAVClient->url(p_url);
  if (status != EcceDAVStatus::NO_CONTENT) {
    m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
  } else {
    ret = true;
  }
  return ret;
}

/*************************************************************************
 Method : 
 Summary: Always returning true right now.
**************************************************************************/
bool DavEDSI::removeResources(const vector<EcceURL> urls) 
{
  bool ret = true;
  initProgress();
  int numUrls = urls.size();

  for (int index = 0; index != numUrls; index++) {
    p_EcceDAVClient->url(urls[index]);
    int status = p_EcceDAVClient->delete_document();
    p_EcceDAVClient->url(urls[index]);
    if (status != EcceDAVStatus::NO_CONTENT) {
      m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
      ret = false;
    }
  }
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
EcceURL *DavEDSI::makeCollection(const string& base, const string& pattern)
{
  p_EcceDAVClient->url(p_url);
  initProgress();
  m_msgStack.clear();
  EcceURL *ret = NULL;

  // At a minimum pattern must include int  
  if (pattern.find("%d") != string::npos) {
    string baseName = uniqueName(base, pattern); 
    if (m_msgStack.size() == 0) {
       // May fail to generate a name - maybe no credentials etc so
       // don't try to move on
       string newURLString = p_url.toString().c_str();

       if (newURLString.rfind('/') == newURLString.length()-1) {
          newURLString = newURLString + baseName.c_str();
       } else {
          newURLString = newURLString + "/" + baseName.c_str();
       }
       newURLString = newURLString + "/";

       p_EcceDAVClient->url(newURLString);
       int status = p_EcceDAVClient->create_collection();
       if (status == EcceDAVStatus::CREATED) {
          ret = new EcceURL(newURLString.c_str());
       }  else {
          m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
       }
    }
  }
  return ret;
}

/*************************************************************************
 Methods: Tag generator functions 
 Summary: used to create proper XML opening and closing tags for Dav requests
**************************************************************************/
string DavEDSI::davXMLDocumentString() const 
{
  return "<?xml version=\"1.0\" encoding=\"utf-8\" ?> \n";
}

string DavEDSI::davPropfindAllpropNullElement() const 
{
  return "<" + vALLPROP + "/> \n";
}

string DavEDSI::davPropElementOpenTag(const string& nameSpaceString) const
{
  string ret = "<" + DAV_NSPREFIX + vPROP + " ";
         ret += nameSpaceString;
         ret += "> \n";
  return ret;
}
string DavEDSI::davPropElementCloseTag() const 
{
  string ret = "</" + DAV_NSPREFIX + vPROP + "> \n";
  return ret; 
}
string DavEDSI::davPropfindPropnameNullElement() const 
{
  string ret = "<" + vPROPNAME + "/> \n";
  return ret; 
}
string DavEDSI::davPropfindPropnameElementHeader() const 
{
  string ret = "<" + vPROPNAME + "> \n";
  return ret; 
}
string DavEDSI::davPropfindPropnameElementFooter() const 
{
  string ret = "</" + vPROPNAME + " \n";
  return ret; 
}
string DavEDSI::davProppatchHeader(const string& nameSpaceString) const
{
  string ret = davXMLDocumentString(); 
         ret += "<" + vPROPUPDATE + " xmlns";
         if (!DAV_NSALIAS.empty()) ret += ":";
         ret += DAV_NSALIAS + "=\"" + DAV_NSNAME + ":\" ";
         ret += nameSpaceString;
         ret += "> \n";
  return ret;
}

string DavEDSI::davProppatchFooter()  const
{
  string footer = "</" + vPROPUPDATE + ">\n";
  return footer;
}

string DavEDSI::davPropfindHeader() const
{
  string ret = davXMLDocumentString(); 
         ret += "<" + vPROPFIND + " xmlns";
         if (!DAV_NSALIAS.empty()) ret += ":";
         ret += DAV_NSALIAS + "=\"" + DAV_NSNAME + ":\">\n";
  return ret;
}

string DavEDSI::davPropfindFooter() const
{
  return "</" + vPROPFIND + ">\n";
}

string DavEDSI::davProppatchRemoveElementFooter() const
{
  return "</" + vREMOVE + ">\n";
}

string DavEDSI::davProppatchSetElementFooter()  const
{
  return "</" + vSET + ">\n";
}

string DavEDSI::davProppatchRemoveElementHeader()  const
{
  return "<" +vREMOVE + ">\n";
}

string DavEDSI::davProppatchSetElementHeader()  const
{
  return "<" + vSET + ">\n";
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
vector<DavNameSpaceAlias> DavEDSI::parsePropertyNames(
                                        vector<MetaDataRequest> & requests) 
{
  vector<DavNameSpaceAlias> namespaces;

  char buffer[32];  // for namespace numbers.  Should be small
  DavNameSpaceAlias newNameSpaceAlias;
  for( int r_idx = 0; r_idx < requests.size(); r_idx++) {
    //cout << r_idx << " " << requests[r_idx].name << endl;
    //
    //We need to do an rfind here because we might have more than one colon
    //in the namespace
    //
    int endOfNameSpace = requests[r_idx].name.rfind(':');
    if (endOfNameSpace > 0) {
       string newNameSpace = requests[r_idx].name.substr(0, endOfNameSpace);
       bool isDuplicate = false;
       for (int ns_idx = 0; ns_idx < namespaces.size(); ns_idx++) {
         if (namespaces[ns_idx].NameSpace.compare(newNameSpace) == 0) { 
           isDuplicate = true;
           if (namespaces[ns_idx].Alias.empty()) {
           requests[r_idx].name.replace(0, endOfNameSpace+1,
                                        namespaces[ns_idx].Alias);
           } else {
           requests[r_idx].name.replace(0, endOfNameSpace,
                                        namespaces[ns_idx].Alias);
           }
           break;
         }
       }
       if (!isDuplicate && newNameSpace != DAV_NSNAME) {
         newNameSpaceAlias.NameSpace = newNameSpace;
         sprintf(buffer, "I%zu", namespaces.size());
         newNameSpaceAlias.Alias = buffer;
         addNamespaceAlias(namespaces,buffer,newNameSpace);
         requests[r_idx].name.replace(0, endOfNameSpace, 
                                      newNameSpaceAlias.Alias);
       } else if ((!isDuplicate) && (newNameSpace == DAV_NSNAME)){
         newNameSpaceAlias.NameSpace = newNameSpace;
 //        newNameSpaceAlias.Alias = DAV_NSALIAS;
         sprintf(buffer, "D%zu", namespaces.size());
 //        newNameSpaceAlias.Alias = DAV_NSALIAS;
         newNameSpaceAlias.Alias = buffer;
//         addNamespaceAlias(namespaces,DAV_NSALIAS,newNameSpace);
         addNamespaceAlias(namespaces,buffer,newNameSpace);
//         if (DAV_NSALIAS.empty()) {
//           requests[r_idx].name.erase(0, endOfNameSpace+1 );
//         } else {
         requests[r_idx].name.replace(0, endOfNameSpace, 
                                      newNameSpaceAlias.Alias);
//           requests[r_idx].name.erase(0, endOfNameSpace );
//           requests[r_idx].name.insert(0, newNameSpaceAlias.Alias);
//         }
       }
    }
    
  }
  return namespaces;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
vector<DavNameSpaceAlias> DavEDSI::parsePropertyNames(
                                          vector<MetaDataResult> & results) 
{
  vector<DavNameSpaceAlias> namespaces;

  DavNameSpaceAlias newNameSpaceAlias;
  char buffer[32];  // for namespace numbers.  Should be small
  for (int rIdx = 0; rIdx < results.size(); rIdx++) {
    //
    //We need to do an rfind here because we might have more than one colon
    //in the namespace
    //
    int endOfNameSpace = results[rIdx].name.rfind(':');
    if (endOfNameSpace > 0) {
       string newNameSpace = results[rIdx].name.substr(0, endOfNameSpace);
       bool isDuplicate = false;
       for (int nsIdx = 0; nsIdx < namespaces.size(); nsIdx++) {
         if (namespaces[nsIdx].NameSpace.compare(newNameSpace) == 0) {
           isDuplicate = true;
           results[rIdx].name.replace(0, endOfNameSpace,
                                      namespaces[nsIdx].Alias);
         }
       }
       if (!isDuplicate && newNameSpace != DAV_NSNAME) {
         newNameSpaceAlias.NameSpace = newNameSpace.c_str();
         sprintf(buffer, "I%zu", namespaces.size());
         newNameSpaceAlias.Alias.append(buffer);
         addNamespaceAlias(namespaces,newNameSpaceAlias.Alias,newNameSpace);
         results[rIdx].name.replace(0, endOfNameSpace, 
                                    newNameSpaceAlias.Alias);
       } else if ((!isDuplicate) && (newNameSpace == DAV_NSNAME)){


         sprintf(buffer, "D%zu", namespaces.size());
         newNameSpaceAlias.Alias = buffer;
         addNamespaceAlias(namespaces,buffer,newNameSpace);
//         results[rIdx].name.insert(0, newNameSpaceAlias.Alias);
         results[rIdx].name.replace(0, endOfNameSpace, 
                                    newNameSpaceAlias.Alias);

//         newNameSpaceAlias.NameSpace = newNameSpace;
//         newNameSpaceAlias.Alias = DAV_NSALIAS;
//         addNamespaceAlias(namespaces,DAV_NSALIAS,newNameSpace);
//         results[rIdx].name.erase(0, endOfNameSpace );
//         results[rIdx].name.insert(0, newNameSpaceAlias.Alias);
       }

    }
  }
  return namespaces;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::sendMetaDataRequest(const int davMethod, 
                                  const string davRequest, string& response) 
{
  bool ret = true;

  switch (davMethod) {
    case CDAVClient::PROPFIND :
    { 
      // This returns 400 if a requested property is not defined.
      int status = p_EcceDAVClient->get_properties(davRequest);
      if (status != EcceDAVStatus::MULTISTATUS) {
          m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
        ret = false;
      } else 
        response = p_EcceDAVClient->getResultsText();
  
      break;
    }
    case CDAVClient::PROPPATCH :
    {
      p_EcceDAVClient->setDepthHeader("0");
      int status = p_EcceDAVClient->put_properties(davRequest);
      if (status !=  EcceDAVStatus::MULTISTATUS) {
        m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
       ret = false;
      } else 
        response = p_EcceDAVClient->getResultsText();
      
      break;
    }
  }
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
EcceURL *DavEDSI::makeDataSet(const string& base)
{
  EcceURL *ret = NULL;
  initProgress();
  p_EcceDAVClient->url(p_url);
  m_msgStack.clear();

  string baseName = uniqueName(base);
  if (m_msgStack.size() == 0) {
    string newURLString = p_url.toString();

    if (newURLString[newURLString.length()-1] != '/') {
      newURLString.append("/");
    }
    newURLString.append(baseName);

    p_EcceDAVClient->url(newURLString);

     // We used to pass a "" string to make a null document 
     // but since we have a put_document function that accepts
     // an InputStream and checks for NULL when a putBody is called
     // in EcceDAVClient passing a NULL InputStream seemed to make
     // more sense to me.
     // Eric 10/4/2002
    
    istream *istr=NULL;
    int status = p_EcceDAVClient->put_document(istr);
    if ( status ==  EcceDAVStatus::CREATED)  {
      EcceURL  *newUrl = new EcceURL(newURLString.c_str());
      ret = newUrl;
    } else {
      m_msgStack.add(EcceDAVStatus::edsiMessage(status).c_str(), EcceDAVStatus::text(status).c_str());
    }
  }
  return ret;
}

/*************************************************************************
 Method : uniqueName
 Summary: Look at the names of children for the url and generate one that
          doesn't clash.  Note that it is possible to find an unused name
          and have it become used before the caller can take it.  We are
          not dealing with this condition at the moment.
          This code was modified to do its unique name stuff taking into
          account file extensions.  That is, the file for Untitled.mvm
          we start to look for Untitled_1.mvm...not Untitled.mvm_1.
 TODO speed this up by having a listCollection that just returns names,
 not ResourceResult objects.
**************************************************************************/
EcceURL DavEDSI::uniqueName(const EcceURL& url, const string& pattern)
{
  EcceURL restoreURL = getURL();
  initProgress();
  m_msgStack.clear();

  EcceURL ret = url;
  string baseURL = url.toString().c_str();

  EcceURL parent = url.getParent();
  setURL(parent);

  vector<ResourceResult> rrchildren;
  if (listCollection(rrchildren)) {

    // The name we are basing our unique name on...
    string name = url.getFilePathTail();
    string basename = name;
    string ext;
    int pos = name.rfind(".");
    if (pos != string::npos) {
      basename = name.substr(0,pos);
      ext = name.substr(pos);
    }

    int idx;

    int cnt = rrchildren.size();
    vector<string> children;
    string tmp;
    // Make a vector of strings of possible name clashes that we will
    // check through.
    for (idx=0; idx<cnt; idx++) {
      tmp = rrchildren[idx].url.getFilePathTail();
      if (tmp.find(basename) == 0) {
        children.push_back(tmp);
      }
    }

    // Make big enough for name plus up to 10 digits
    char *buf = new char[name.length() + 10 + 1];
    sprintf(buf,"%s",name.c_str());  // first time through - no numbers
    for (idx=1; idx<10000; idx++) {  // big number but not infinite
      if (find(children.begin(),children.end(),buf) == children.end()) {
        ret = parent.getChild(buf);
        break;
      }
      string format = "%s"+pattern+"%s";
      sprintf(buf,format.c_str(),basename.c_str(),idx,ext.c_str());
    }
    delete [] buf;
  }

  // reset the contect to the users requested url
  setURL(restoreURL);

  return ret;
}


/*************************************************************************
 Method : getClassName
 Summary: Returns class name
**************************************************************************/
string DavEDSI::getClassName()
{
  return "DavEDSI";
}


/*************************************************************************
 Method : uniqueName
 Summary: See other uniqueName method.
**************************************************************************/
string DavEDSI::uniqueName(const string& base, const string& pattern)
{
  initProgress();
  m_msgStack.clear();
  string ret = base;

  EcceURL tmp = p_url.getChild(base.c_str());

  EcceURL newurl = uniqueName(tmp, pattern);

  return newurl.getFilePathTail();
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::isWritable() {

  bool ret = false;
  initProgress();
  p_EcceDAVClient->url(p_url);
  vector<MetaDataRequest> requests;
  ResourceMetaDataResult result;
  getMetaData(requests, result);
  int code =  p_EcceDAVClient->options();
  string status = p_EcceDAVClient->getHeaderText();
  // TODO add code==200 to the if statement
  if ((status.rfind("DELETE")    != string::npos) &&
        (status.rfind("PROPPATCH") != string::npos) && 
        (status.rfind("COPY")      != string::npos) && 
        (status.rfind("MOVE")      != string::npos) && 
        (status.rfind(" LOCK")     != string::npos) && 
        (status.rfind("UNLOCK")    != string::npos)) 
    ret = true;
/*
  IsWritable is not fully functional right now.
  Here are the problems: 

  1)  I cannot get OPTIONS to return generic security information for a given collection.  It only returns what permissions are available to that given collection.  Why is this a problem?  You cannot PUT a collection therefore the PUT method doesn't show up when you request OPTIONS on http://ecce-dav-server.gov:1080/Ecce/users/d3a303.  PUT does show up for existing documents.  In HTTP 1.1  RFC2616 OPTIONS should allow you to find generic server information using an "*" at the end of a URL to find generic permissions.

  2)  Bug: OPTIONS does not return MKCOL permissions on a collection even though the permission is available. 

  if (result.resourcetype == ResourceDescriptor::RT_DOCUMENT) {
    if (status.rfind("PUT")   == string::npos)
      ret = false; 
  } else {
    if (status.rfind("MKCOL")   == string::npos)
      ret = false; 
  }
*/
  if (ret == false) {
    m_msgStack.add(EcceDAVStatus::edsiMessage(code).c_str(), EcceDAVStatus::text(code).c_str());
  }
  return ret;
}

/*************************************************************************
 Method : 
 Summary: 
**************************************************************************/
bool DavEDSI::isLocked(string& locker)
{
  m_msgStack.clear();
  initProgress();
  p_EcceDAVClient->url(p_url);
  return false;
}

/*************************************************************************
 Method : 
 Summary: A HACKER find method.  For now all we really want to do is
 finds on names.  These are hrefs which really are not properties that
 can be searched.  So we will search on a predefined DAV key but then
 look for substring matches against the href only.
**************************************************************************/
bool DavEDSI::efind(const string& key, const string& substring, 
                    const EcceURL& start, vector<EcceURL>& matches)
{
  bool ret = false;
  m_msgStack.clear();
  initProgress();

  //
  //  Assign the url to the place where we start searching. 
  //

  p_url = start;
  p_EcceDAVClient->url(start);

  //
  //  Set the HTTP header to do an infinite search from 
  //  url starting point.
  //

  p_EcceDAVClient->setDepthHeader("infinity");
  MetaDataRequest request;
  vector<MetaDataRequest> requests;
  vector<ResourceMetaDataResult> resourceResults;
  request.name = key;
  requests.push_back(request);

  // Add this so that the resourcetype field is set and we can weed out
  // subparts of the virtual document.
  request.name = VDoc::getEcceNamespace() + ":resourcetype";
  requests.push_back(request);

  ret = requestMetaData(requests, resourceResults);

  //
  // Set up loop to find matching substrings for return
  //

  int resultsSize = resourceResults.size();
  //int metaDataSize = 0;
  bool itmatches = false;
  string lastVDoc;
  for (int resultsIndex = resultsSize-1; resultsIndex >= 0; resultsIndex--)
  {
    //metaDataSize = resourceResults[resultsIndex].metaData.size(); 
    itmatches = false;
    
    /*if (resourceResults[resultsIndex].resourcetype == 
          ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
        lastVDoc = resourceResults[resultsIndex].url.toString();
    }*/

    if (resourceResults[resultsIndex].url.toString().rfind(substring)
        != string::npos) 
      itmatches = true;
      // Later we want to really search on meta data - not now.
      /*
    for (int metaDataIndex = 0; metaDataIndex < metaDataSize; metaDataIndex++)
    {

      if ((resourceResults[resultsIndex].metaData[metaDataIndex].name.rfind(
        key) != string::npos) && 
        (resourceResults[resultsIndex].metaData[metaDataIndex].value.rfind(
        substring) != string::npos))
    }
        */
    if (itmatches) 
    {
      // NOTE: commented out VDoc checking, not sure why it was in here since
      // it was keeping finds at the calculation level from working 3/21/06
      //
      // Currently left in reverse order
      //string url = resourceResults[resultsIndex].url.toString();
      //if (url == lastVDoc) 
      matches.push_back(resourceResults[resultsIndex].url);  
      //else if (url.find(lastVDoc) != 0)
        //matches.push_back(resourceResults[resultsIndex].url);  
    }

  }
  
  p_EcceDAVClient->setDepthHeader("0");
  return ret;
}



/*************************************************************************
 Method : 
 Summary: A HACKER find method.  Simply finds resources that exist
 in a Properties directory, with search starting at the provided
 URL.  Based on the efind method.
**************************************************************************/
bool DavEDSI::efindProp(const string& substring, const EcceURL& start, 
                        vector<EcceURL>& matches)
{
  bool ret = false;
  string key = VDoc::getEcceNamespace() + ":state";
  m_msgStack.clear();
  initProgress();

  //
  //  Assign the url to the place where we start searching.
  //

  p_url = start;
  p_EcceDAVClient->url(start);

  //
  //  Set the HTTP header to do an infinite search from
  //  url starting point.
  //

  p_EcceDAVClient->setDepthHeader("infinity");
  MetaDataRequest request;
  vector<MetaDataRequest> requests;
  vector<ResourceMetaDataResult> resourceResults;
  request.name = key;
  requests.push_back(request);

  // Add this so that the resourcetype field is set and we can weed out
  // subparts of the virtual document.
  request.name = VDoc::getEcceNamespace() + ":resourcetype";
  requests.push_back(request);

  ret = requestMetaData(requests, resourceResults);

  //
  // Set up loop to find matching substrings for return
  //

  int resultsSize = resourceResults.size();
  string lastVDoc;

  string searchString = substring;
  STLUtil::toUpper(searchString); 
  for (int resultsIndex = resultsSize-1; resultsIndex >= 0; resultsIndex--) {
    if (resourceResults[resultsIndex].resourcetype == ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
      if (resourceResults[resultsIndex].url.getFilePathTail() != ".htaccess") {
        vector<MetaDataResult> mdr;
        setURL(resourceResults[resultsIndex].url);
        m_msgStack.clear();
        MetaDataRequest rq;
        vector<MetaDataRequest> mdq;
        rq.name = key;
        mdq.push_back(rq);
        if (getMetaData(mdq, mdr)) {
          for (int i = 0; i < mdr.size(); i++) {
            if (mdr[i].name == key) {
              string val = mdr[i].value;
              STLUtil::toUpper(val); 
              if (val == searchString) {
                matches.push_back(resourceResults[resultsIndex].url);
              }
            }
          }
        }
      }
    }
  }

  p_EcceDAVClient->setDepthHeader("0");
  return ret;
}

