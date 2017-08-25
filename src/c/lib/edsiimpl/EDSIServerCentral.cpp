#include <strstream>
  using std::ostrstream;
#include <iostream>
  using std::cout;
  using std::cerr;
  using std::endl;
  using std::ends;

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
using namespace xercesc;

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/KeyValueReader.H"
#include "util/SFile.H"
#include "util/EcceException.H"
#include "util/Bookmark.H"

#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSIGaussianBasisSetLibrary.H"
#include "dsm/EDSI.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EcceDAVClient.H"
#include "dsm/BasicDOMParser.H"
#include "dsm/ParseErrorReporter.H"
#include "dsm/ResourceDescriptor.H"



// Class statics
vector<Bookmark> EDSIServerCentral::p_mountPoints;


EDSIServerCentral::EDSIServerCentral()
{
  static bool firstTime = true;
  if (firstTime) {
    firstTime = false;
    try
    {
      XMLPlatformUtils::Initialize();
    } catch (const XMLException& toCatch) {
      string msg = "Error during EDSIServerCentral initialization! :\n";
      char *tmpMsg = XMLString::transcode(toCatch.getMessage());
      msg += tmpMsg;
      delete [] tmpMsg;
      EE_RT_ASSERT(false, EE_FATAL, msg);
    }
  }

  p_mountFile = Ecce::ecceHome();
  if (getenv("ECCE_REMOTE_SERVER")) {
    p_mountFile += "/siteconfig/RemoteServer/DataServers";
  } else {
    p_mountFile += "/siteconfig/DataServers";
  }
  p_doc = 0;

  BasicDOMParser parser;
  try {
    p_doc = parser.parse(SFile(p_mountFile));
  } catch (const EcceException& toCatch) {
    EE_RT_ASSERT(false, EE_FATAL, toCatch.what());
  }

  p_serverKey = "EcceServer";
  p_UrlKey = "Url";
  p_DescKey = "Desc";
  p_BasisSetKey = "BasisSet";
  p_StructureLibKey = "StructureLib";
  p_forceFieldKey = "ForceField";
  p_paramFileKey = "ParamFile";
  p_paramFilesKey = "ParamFiles";
  p_segmentDirKey = "SegmentDir";
  p_segmentDirsKey = "SegmentDirs";
}


EDSIServerCentral::~EDSIServerCentral()
{
  if (p_doc != 0) p_doc->release();
  p_doc = 0;
}


bool EDSIServerCentral::checkServer() throw (RetryException,EcceException)
{
  bool ret = true;
  loadMountPointInfo();
  Bookmark mount = getDefaultProvider();
  EDSI* connection = EDSIFactory::getEDSI(mount.getEcceUrl());
  if (connection) {
    if (!connection->checkServer()) {
      if (connection->m_msgStack.findKey("CANCELED"))
        ret = false;
      else if (connection->m_msgStack.findKey("TOO_MANY_RETRIES"))
        throw RetryException(connection->m_msgStack.getMessage(), WHERE);
      else
        throw EcceException(connection->m_msgStack.getMessage(), WHERE);
    }
  } else {
    string msg = "Unable to interpret url:\n";
    msg += mount.getUrl();
    throw EcceException(msg, WHERE);
  }
  delete connection;
  return ret;
}


string EDSIServerCentral::readRealm()
{
  string realm = Ecce::serverUser();

  // IA64 has problems with getpwnam (system shared lib conflicts)
  // so do it the hard way
/*
  // this would be the easy way to get the full name from /etc/passwd
  struct passwd* pwd = getpwnam(Ecce::serverUser());
  if (pwd!=(struct passwd*)0 && pwd->pw_gecos!=NULL) {
    realm = pwd->pw_gecos;
    for (int i=0; i<realm.length(); i++)
      if (realm[i]==' ' || realm[i]=='\t')
        realm[i] = '.';
  }
*/
  // here's the hard way
  FILE *pwfile = fopen("/etc/passwd", "r");
  if (pwfile != NULL) {
    char pwbuf[256];
    string pwcheck = realm + ":";
    while (fgets(pwbuf, 256, pwfile)!=NULL && 
           strncmp(pwbuf, pwcheck.c_str(), pwcheck.length())!=0);
    if (pwbuf!=NULL && strncmp(pwbuf, pwcheck.c_str(), pwcheck.length())==0) {
      // match 4 colon characters
      char* colfind = pwbuf;
      int it;
      for (it=0; it<4 && (colfind = strchr(colfind, ':'))!=NULL;colfind++,it++);
      if (colfind != NULL) {
        char* start = colfind;
        if ((colfind = strchr(start, ':')) != NULL) {
          *colfind = '\0';
          realm = start;
          for (it=0; it<realm.length(); it++)
            if (realm[it]==' ' || realm[it]=='\t')
              realm[it] = '.';
        }
      }
    }
    fclose(pwfile);
  }
  return realm;
}


/**
 * Returns false if canceled or too many retries without completing checks.
 */
bool EDSIServerCentral::checkServerSetup() throw (RetryException,EcceException)
{
  bool ret = true;

  loadMountPointInfo();
  Bookmark mount = getDefaultProvider();
  EDSI* connection = EDSIFactory::getEDSI(mount.getEcceUrl());
  string errMessage;
  if (connection) {
    static bool autoAllow = Ecce::ecceAutoAccounts();
    bool newUser = false;

    if (autoAllow) {
      // check whether user exists before attempting connection so we
      // can create an account before a password dialog is ever shown
      string urlbase = mount.getUrl();
      string::size_type pos = urlbase.rfind("/");
      if (pos != string::npos)
        urlbase.replace(pos+1, urlbase.length()-pos-1, "");

      string urlcgi = urlbase + "cgi-bin/ecce_accounts?parama=";
      string urlstr = urlcgi + "exists&paramu=";
      urlstr += Ecce::serverUser();
      EcceDAVClient edcExists(urlstr);
      ostrstream ostr;
      newUser = (edcExists.doPost(ostr) == 500);
    }

    if (!connection->exists(newUser)) {
      if (connection->m_msgStack.messages() &&
          connection->m_msgStack.getMessage()=="Canceled by user")
        exit(0);
      else if (connection->m_msgStack.messages() &&
            connection->m_msgStack.getMessage().find("You have exceeded")==0) {
        cerr << "\n\n  ERROR:  You have exceeded the number of allowable\n";
        cerr << "  attempts at data server authentication.\n\n";
        cerr << "  Please contact your ECCE administrator if you\n";
        cerr << "  have forgotten your data server password.\n\n";
        exit(1);
      } else {
        cerr << "\n\n  ERROR:  The ECCE application software cannot \n";
        cerr << "  establish a connection to the ECCE Server\n  '";
        cerr << mount.getUrl();
        cerr << "'\n  at this time.\n\n";
        cerr << "  Please contact your ECCE administrator to check\n";
        cerr << "  that the ECCE Server is running and that\n";
        cerr << "  the ECCE application software configuration file\n  '";
        cerr << p_mountFile; 
        cerr << "'\n  contains the correct URL for your ECCE Server.\n\n";
        exit(1);
      }
    }

    // Possible things to check...
    
    if (!connection->checkServer()) {
      cerr << "\n\n  ERROR:  The ECCE application software cannot \n";
      cerr << "  establish a connection to the ECCE Server\n  '";
      cerr << mount.getUrl();
      cerr << "'\n  because it is not a WebDAV server.\n\n";
      cerr << "  Please contact your ECCE administrator to check\n";
      cerr << "  that the ECCE Server is properly installed.\n\n";
      exit(1);
    }

    string failure = "";

    // user area
    EcceURL url = getDefaultUserHome().getParent();
    connection->setURL(url);
    if (ret && (!connection->exists() || 
          !connection->m_msgStack.getMessage().empty()))
        failure += "users ";

    ret = !(connection->m_msgStack.findKey("CANCELED") ||
           connection->m_msgStack.findKey("TOO_MANY_RETRIES"));

    // Structure Library 
    url = getDefaultStructureLibrary();
    connection->setURL(url);
    if (ret && (!connection->exists() || 
          !connection->m_msgStack.getMessage().empty())) {
      SFile stlibdir(url.toString());
      if (!stlibdir.exists()) {
        failure += "StructureLibrary ";

        if (ret)
        ret = !(connection->m_msgStack.findKey("CANCELED") ||
               connection->m_msgStack.findKey("TOO_MANY_RETRIES"));
      }
    }

    // GBSL
    url = getDefaultBasisSetLibrary();
    connection->setURL(url);
    if (ret && (!connection->exists() || 
          !connection->m_msgStack.getMessage().empty()))
      failure += "GaussianBasisSetLibrary ";

    if (ret)
    ret = !(connection->m_msgStack.findKey("CANCELED") ||
           connection->m_msgStack.findKey("TOO_MANY_RETRIES"));

    if (ret && !failure.empty()) {
      string msg = "A failure was detected in the ECCE server setup.  One "
        "or more collections or files is missing:\n";
      msg += failure;
      msg += "\n";
      throw EcceException(msg, WHERE);
    }


  } else {
    string msg = "Unable to interpret url:\n";
    msg += mount.getUrl();
    throw EcceException(msg, WHERE);
  }
  if (connection->m_msgStack.findKey("TOO_MANY_RETRIES"))
    throw RetryException(connection->m_msgStack.getMessage(), WHERE);

  if (connection) delete connection;
  return ret;
}

int EDSIServerCentral::getAccess(const string& projectUrl, string& access, string& errMessage)
{

  access = "";

  // retrieve access permissions via cgi-bin script
  EcceURL serverUrl(projectUrl);
  string serverPath = serverUrl.getPath();
  serverPath.erase(0,5);
  string urlstr = serverUrl.getRef();
  urlstr += "cgi-bin/ecce_htaccess?parama=retrieve&paramb=" + serverPath +
            "&paramc=Ecce-" + readRealm();
  EcceDAVClient edcAccess(urlstr);
  ostrstream ostr;
  int status = edcAccess.doPost(ostr);
  ostr << ends;
  char *cptr = ostr.str();
  if (status == 200) {
    access = cptr;
  }
  else {
    errMessage = cptr;
  }
  return status;
}

int EDSIServerCentral::setAccess(const string& projectUrl,
                                 const string& readOnlyStr,
                                 const string& readWriteStr,
                                 string& errMessage)
{
  EcceURL serverUrl(projectUrl);
  EDSI *target = EDSIFactory::getEDSI(serverUrl);
  vector<MetaDataRequest> requests;
  ResourceMetaDataResult result;
  int status;

  if (target->exists()) {
    target->getMetaData(requests, result);
    if (result.resourcetype == ResourceDescriptor::RT_VIRTUAL_DOCUMENT) {
      status = 500;
      errMessage = "Can only edit access preferences for a Project folder.";
    }
    // Not a virtual document - go ahead and set access privileges
    else {
      string serverPath = serverUrl.getPath();
      serverPath.erase(0,5);
      string urlstr = serverUrl.getRef();
      urlstr += "cgi-bin/ecce_htaccess?parama=modify&paramb=" + serverPath +
                "&paramc=Ecce-" + readRealm() + "&paramd=" + readOnlyStr +
                "&parame=" + readWriteStr;
      EcceDAVClient edcAccess(urlstr);
      ostrstream ostr;
      status = edcAccess.doPost(ostr);
      if (status != 200) {
        ostr << ends;
        errMessage = ostr.str();
      }
    }
  }
  else {
    status = 500;
    errMessage = "Modify location not found.";
  }

  delete target;
  return status;
}

///////////////////////////////////////////////////////////////////////////////
// Currently we read the list of service providers from a system config
// file.
///////////////////////////////////////////////////////////////////////////////
vector<Bookmark> *EDSIServerCentral::getEDSIProviders()
{
  loadMountPointInfo();
  vector<Bookmark> *ret = new vector<Bookmark>;
  *ret = p_mountPoints;
  return ret;
}

string EDSIServerCentral::mapURLtoFullName(const EcceURL& url)
{
  string ret = url.toString();

  loadMountPointInfo();
  int cnt = p_mountPoints.size();
  string mounturl;
  for (int idx=0; idx<cnt; idx++) {
    mounturl = p_mountPoints[idx].getUrl();
    if (ret.find(mounturl) == 0) {
      ret.replace(0, mounturl.size(), p_mountPoints[idx].getName());
      break;
    }
  }

  // Remove the ending '/'
  if (ret[ret.size()-1] == '/') ret.erase(ret.size()-1);

  return ret;
}

string EDSIServerCentral::mapURLtoName(const EcceURL& url)
{
  string ret = url.toString();

  // Start by abbreviating the urlname if it's the user's home area
  string home = getDefaultUserHome().toString();
  if (ret.find(home) == 0) {
    ret.replace(0, home.size(), "(HOME)");
  } else {
    loadMountPointInfo();
    int cnt = p_mountPoints.size();
    string mounturl;
    for (int idx=0; idx<cnt; idx++) {
      mounturl = p_mountPoints[idx].getUrl();
      if (ret.find(mounturl) == 0) {
        ret.replace(0, mounturl.size(), p_mountPoints[idx].getName());
        break;
      }
    }
  }

  // Remove the ending '/'
  if (ret[ret.size()-1] == '/') ret.erase(ret.size()-1);

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// Currently just returns the first one.
//////////////////////////////////////////////////////////////////////////////
Bookmark EDSIServerCentral::getDefaultProvider()
{
  loadMountPointInfo();
  return p_mountPoints[0];
}


//////////////////////////////////////////////////////////////////////////////
// Currently reads GBSL Url from the DataServers file
//////////////////////////////////////////////////////////////////////////////
EcceURL EDSIServerCentral::getDefaultBasisSetLibrary()
{
  XMLCh *tmpStr = XMLString::transcode(p_BasisSetKey.c_str());
  DOMNodeList *GbslList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  DOMNode *Gbsl = 0;
  int numGbsl = GbslList->getLength();
  string GbslValue;

  if (numGbsl > 0) {
    Gbsl = GbslList->item(0);
    GbslValue = getNodeValue(Gbsl);
  }

  EcceURL ret = GbslValue;
  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// Returns true if the default GBSL can be accessed.  This means that
// the appropriate GBSL directory must exist on the DAV server with
// valid permissions and meta data.
//////////////////////////////////////////////////////////////////////////////
bool EDSIServerCentral::checkDefaultGBSL()
{
  EcceURL gbslUrl = getDefaultBasisSetLibrary();
  
  EDSIGaussianBasisSetLibrary gbsl(gbslUrl);

  return gbsl.checkStatus();
}

//////////////////////////////////////////////////////////////////////////////
// Get a default path for user home.  
// Currently defined as the first mount point with /users/<USER> appended
// @returns URL of user home area.
//////////////////////////////////////////////////////////////////////////////
EcceURL EDSIServerCentral::getDefaultUserHome()
{
  loadMountPointInfo();
  EcceURL defaultServer = getDefaultProvider().getEcceUrl();

  return getUserHome(defaultServer);
}


//////////////////////////////////////////////////////////////////////////////
// Get a default path for user home.
// Currently defined as the first mount point with /users/<USER> appended
// @returns URL of user home area.
//////////////////////////////////////////////////////////////////////////////
EcceURL EDSIServerCentral::getUserHome(const EcceURL& rootUrl)
{
  EcceURL ret = rootUrl;

  if (ret.getProtocol().find("http") == 0) {
    string path = ret.getFile();
    if (path[path.length()-1] != '/') path.append("/");
    path.append("users/");
    path.append(Ecce::serverUser());
    ret.set(ret.getProtocol().c_str(), ret.getHost().c_str(),
            ret.getPort(), path.c_str());
  } else if (ret.getProtocol().find("file") == 0 ||
             ret.getProtocol().empty()) {
    // HACK tree control only works if root is double slash
    string path = "/";
    path = path + Ecce::realUserHome();
    ret.set(ret.getProtocol().c_str(), ret.getHost().c_str(),
            ret.getPort(), path.c_str());
  }

  return ret;
}


//////////////////////////////////////////////////////////////////////////////
// Currently reads Structure Library Url from the DataServers file
//////////////////////////////////////////////////////////////////////////////
EcceURL EDSIServerCentral::getDefaultStructureLibrary()
{
  XMLCh *tmpStr = XMLString::transcode(p_StructureLibKey.c_str());
  DOMNodeList *structLibList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  DOMNode *structLib = 0;
  int numStructLib = structLibList->getLength();
  string structLibValue;

  if (numStructLib > 0) {
    structLib = structLibList->item(0);
    structLibValue = getNodeValue(structLib);
  }

  EcceURL ret = structLibValue;

  if (getenv("ECCE_NO_MESSAGING")) {
    // do this check every time for the standalone builder
    loadMountPointInfo();
    Bookmark mount = getDefaultProvider();
    EDSI* connection = EDSIFactory::getEDSI(mount.getEcceUrl());

    string errMessage;
    if (connection) {
      connection->setURL(ret);
      if (!connection->exists() ||
          !connection->m_msgStack.getMessage().empty()) {
        SFile stlibdir(ret.toString());
        if (!stlibdir.exists()) {
          string msg = "Structure Library cannot be found: ";
          msg += ret.toString();
          msg += "\n";
          throw EcceException(msg, WHERE);
        }
      }
    }
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////////////
// Currently reads default paramater files from the DataServers file
//////////////////////////////////////////////////////////////////////////////
vector<string> *EDSIServerCentral::getDefaultParamFiles(string forceFieldName)
{
  vector<string> *ret = new vector<string>;
  DOMNode *forceField = 0;
  DOMNodeList *children = 0;
  DOMNode *paramFiles = 0;
  DOMNode *paramFile = 0;
  int numChildren;
  int i;
  char *nodeName = (char *) 0;
  bool foundParamFiles;
  
  foundParamFiles = false;
  if (getDefaultForceField(forceField, forceFieldName)) {
    if (forceField->hasChildNodes()) {
      children = forceField->getChildNodes();
      i = 0;
      numChildren = children->getLength();
      while (i<numChildren && !foundParamFiles) {
        paramFiles = children->item(i);
        if (paramFiles->getNodeType() == DOMNode::ELEMENT_NODE) {
          nodeName = XMLString::transcode(paramFiles->getNodeName());
          if (strcmp(nodeName, p_paramFilesKey.c_str()) == 0) {
            foundParamFiles = true;
          }
          delete [] nodeName;
        }
        i++;
      }
    }
  }
  // Get Param Files
  if (foundParamFiles) {
    if (paramFiles->hasChildNodes()) {
      children = paramFiles->getChildNodes();
      i = 0;
      numChildren = children->getLength();
      while (i<numChildren) {
        paramFile = children->item(i);
        if (paramFile->getNodeType() == DOMNode::ELEMENT_NODE) {
          nodeName = XMLString::transcode(paramFile->getNodeName());
          if (strcmp(nodeName, p_paramFileKey.c_str()) == 0) {
            ret->push_back(getNodeValue(paramFile));
          }
          delete [] nodeName;
        }
        i++;
      }
    }
  }
  return ret;
}


//////////////////////////////////////////////////////////////////////////////
// Currently reads default segment directories from the DataServers file
//////////////////////////////////////////////////////////////////////////////
vector<string> *EDSIServerCentral::getDefaultSegmentDirs(string forceFieldName)
{
  vector<string> *ret = new vector<string>;
  DOMNode *forceField = 0;
  DOMNodeList *children = 0;
  DOMNode *segmentDirs = 0;
  DOMNode *segmentDir = 0;
  int numChildren;
  int i;
  char *nodeName = (char*)0;
  bool foundSegmentDirs;


  foundSegmentDirs = false;
  if (getDefaultForceField(forceField, forceFieldName)) {
    if (forceField->hasChildNodes()) {
      children = forceField->getChildNodes();
      i = 0;
      numChildren = children->getLength();
      while (i<numChildren && !foundSegmentDirs) {
        segmentDirs = children->item(i);
        if (segmentDirs->getNodeType() == DOMNode::ELEMENT_NODE) {
          nodeName = XMLString::transcode(segmentDirs->getNodeName());
          if (strcmp(nodeName, p_segmentDirsKey.c_str()) == 0) {
            foundSegmentDirs = true;
          }
          delete [] nodeName;
        }
        i++;
      }
    }
  }
  // Get Segment Dirs
  if (foundSegmentDirs) {
    if (segmentDirs->hasChildNodes()) {
      children = segmentDirs->getChildNodes();
      i = 0;
      numChildren = children->getLength();
      while (i<numChildren) {
        segmentDir = children->item(i);
        if (segmentDir->getNodeType() == DOMNode::ELEMENT_NODE) {
          nodeName = XMLString::transcode(segmentDir->getNodeName());
          if (strcmp(nodeName, p_segmentDirKey.c_str()) == 0) {
            ret->push_back(getNodeValue(segmentDir));
          }
          delete [] nodeName;
        }
        i++;
      }
    }
  }
  return ret;
}


EcceURL EDSIServerCentral::getConfigLocation()
{
  EcceURL url = Ecce::ecceDataPrefPath();
  return url;
}

//////////////////////////////////////////////////////////////////////////////
// Load from system config file (DataServers) if not already loaded.
//////////////////////////////////////////////////////////////////////////////
void EDSIServerCentral::loadMountPointInfo()
{
  string errMessage;
  if (p_mountPoints.size() == 0) {

    // Verify existence of configuration file
    errMessage = "\n\n";
    errMessage += "  ERROR:  The ECCE application software is missing a config-\n";
    errMessage += "  uration file '";
    errMessage += p_mountFile;
    errMessage += "'.\n";
    errMessage += "  The ECCE application software uses this file to determine";
    errMessage += "\n";
    errMessage +=  "  the location of your ECCE Server(s).";
    errMessage += "\n\n";
    errMessage +=  "  Please contact your ECCE administrator to restore this\n";
    errMessage +=  "  file.\n";
    SFile configFile(p_mountFile.c_str());
    EE_RT_ASSERT(configFile.exists() ,EE_FATAL, errMessage); 


    // Retrieve application server mount points
    errMessage = "\n\n";
    errMessage += "  ERROR: The ECCE application software configuration file \n  '";
    errMessage += p_mountFile + "'\n";
    errMessage += "  is empty or corrupt.  The ECCE application software uses";
    errMessage += " this \n";
    errMessage +=  "  file to determine the location of your ECCE Server";
    errMessage +=  "(s). \n\n";
    errMessage +=  "  Please contact your ECCE administrator to restore this\n";
    errMessage +=  "  file.\n";

    Bookmark mp;
    XMLCh *tmpStr = XMLString::transcode(p_UrlKey.c_str());
    DOMNodeList *serverUrlList = p_doc->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    tmpStr = XMLString::transcode(p_DescKey.c_str());
    DOMNodeList *serverDescList = p_doc->getElementsByTagName(tmpStr);
    delete [] tmpStr;

    DOMNode *serverUrl = 0;
    DOMNode *serverDesc = 0;
    int numUrl = serverUrlList->getLength();
    int numDesc = serverDescList->getLength();
    string urlValue;
    string descValue;
    int i;

    EE_RT_ASSERT(numUrl == numDesc ,EE_FATAL, errMessage); 

    for (i=0; i<numUrl; i++) {
      serverUrl = serverUrlList->item(i);
      serverDesc = serverDescList->item(i);
      urlValue = getNodeValue(serverUrl);
      descValue = getNodeValue(serverDesc);
      if (!urlValue.empty() || !descValue.empty()) {
        mp.setName(descValue);
        mp.setUrl(urlValue);
        p_mountPoints.push_back(mp);
      }
      else {
        EE_RT_ASSERT(i != 0 ,EE_FATAL, errMessage); 
      }
    }
  }
  EE_RT_ASSERT(p_mountPoints.size() != 0 ,EE_FATAL, errMessage); 
}


//////////////////////////////////////////////////////////////////////////////
// Returns value of a DOMNode
//////////////////////////////////////////////////////////////////////////////
string EDSIServerCentral::getNodeValue(DOMNode *domNode)
{
  string ret;

  DOMNode *child = 0;
  char *value;

  DOMElement *ele = (DOMElement *) domNode;
  if (ele->hasChildNodes()) {
    child = ele->getFirstChild();
    value = XMLString::transcode(child->getNodeValue());
    ret = value;
    delete [] value;
  }

  if (!ret.empty()) {
    int start = ret.find_first_not_of(" \t");
    int end = ret.find_last_not_of(" \t");
    if (start != 0 || end != 0)
      ret = ret.substr(start,end-start+1);
  }
  return ret;
}


//////////////////////////////////////////////////////////////////////////////
// Returns name (value of "name" attribute) of Force Field
//////////////////////////////////////////////////////////////////////////////
string EDSIServerCentral::getForceFieldName(DOMNode *forceField)
{
  string ret;
  DOMNamedNodeMap  *attributes = 0; 
  DOMNode *attribute = 0;
  int numAttr; 
  int i;
  char *nodeName = (char*)0;
  char *attrName = (char*)0;
  char *attrValue = (char*)0;

  nodeName = XMLString::transcode(forceField->getNodeName());
  // Ensure it is a Force Field Node
  if (strcmp(nodeName, p_forceFieldKey.c_str()) == 0) {
    attributes = forceField->getAttributes();
    numAttr = attributes->getLength();
    for (i=0; i<numAttr; i++) {
      attribute = attributes->item(i);
      attrName = XMLString::transcode(attribute->getNodeName());
      if (strcmp(attrName, "name") == 0) {
        attrValue = XMLString::transcode(attribute->getNodeValue());
        ret = attrValue;
        delete [] attrValue;
      }
      delete [] attrName;
    }
  }
  delete [] nodeName;
  return ret;
}


//////////////////////////////////////////////////////////////////////////////
// Determines default Force Field for a given force field name 
//////////////////////////////////////////////////////////////////////////////
bool EDSIServerCentral::getDefaultForceField(DOMNode*& forceField, string forceFieldName)
{
  int i;
  bool foundForceField;
  XMLCh *tmpStr = XMLString::transcode(p_forceFieldKey.c_str()); 
  DOMNodeList *FFList = p_doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  DOMNode *FF = 0;
  int numFF;

  foundForceField = false;
  numFF = FFList->getLength(); 
  if (numFF > 0) {
    i = 0;
    while (!foundForceField && i<numFF) {
      FF = FFList->item(i);
      if (forceFieldName == getForceFieldName(FF)) {
        foundForceField = true;
        forceField = FF; 
      }
      i++;
    }
  }
  return foundForceField;
}

