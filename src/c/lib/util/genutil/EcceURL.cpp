#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/Ecce.H"
#include "util/StringTokenizer.H"

#include "util/EcceURL.H"
#include "util/EcceMap.H"

using std::find;

const string EcceURL::URL_DELIMITER = "\t";


EcceURL::EcceURL() : p_port(-1), p_params(0)
{
}

EcceURL::EcceURL(const char *url) : p_port(-1), p_params(0)
{
  parse(url);
}

EcceURL::EcceURL(const string& url) : p_port(-1), p_params(0)
{
  parse(url.c_str());
}

EcceURL::EcceURL(const char* protocol, const char *host, int port, const char *file) : p_port(-1), p_params(0)
{
  set(protocol,host,port,file);
}

EcceURL::EcceURL(const EcceURL& rhs) : p_port(-1), p_params(0)
{
  *this = rhs;
}

EcceURL::~EcceURL()
{
}

bool EcceURL::operator==(const EcceURL& rhs) const
{
  return toDirString() == rhs.toDirString();
}

bool EcceURL::operator!=(const EcceURL& rhs) const
{ 
  return toString() != rhs.toString();
}

EcceURL& EcceURL::operator=(const EcceURL& rhs)
{
  if (this != &rhs) {
    p_protocol = rhs.getProtocol();
    p_host = rhs.getHost();
    p_port = rhs.getPort();
    p_file = rhs.getFile();
    clearParams();
    if (rhs.p_params != 0) {
      p_params = new EcceMap(*rhs.p_params);
    }
  }
  return *this;
}


/**
 * Resets the current url to the path.
 */
EcceURL& EcceURL::operator=(const string& path)
{
  parse(path.c_str());
  return *this;
}



/**
 * Resets the current url to the path.
 */
EcceURL& EcceURL::operator=(const char *path)
{
  parse(path);
  return *this;
}



/**
 * No need to call toString, this is the cast.
 */
EcceURL::operator string() const
{
  return toString();
}



/**
 * Returns path portion of url.
 * The returned string does NOT include params if any (see
 * getPathWithParams().
 */
string EcceURL::getPath() const
{ return p_file; }



/**
 * Returns path portion of url.
 * The returned string includes parameters if any.
 */
string EcceURL::getPathWithParams() const
{ 
  string ret = p_file;
  if (p_params && p_params->size() > 0) {
    ret.append("?" + paramsAsString());
  }
  return ret;
}

// depricate this in favor of getPath
string EcceURL::getFile() const
{ return p_file; }

/**
 * Returns the root part of the file path (that is all but the last
 * component which is assumed to be the name.
 */
string EcceURL::getFilePathRoot() const
{
  string ret = p_file;
  if (!ret.empty()) {
    // Start at 2nd to last character because with apache 2.0 we have the
    // silly trailing slash.  See if 00000 code.
    size_t pos = p_file.rfind('/',p_file.size()-2);
    if (pos == 0) {
      ret = p_file.substr(0,1);
    } else if (pos != string::npos) {
      ret = p_file.substr(0,pos);
    }
  }
  return ret;
}

/**
 * Returns the last part of the full file path.
 */
string EcceURL::getFilePathTail() const
{
  string ret;
  if (!p_file.empty()) {
    // Start at 2nd to last character because with apache 2.0 we have the
    // silly trailing slash.  See if 00000 code.
    size_t pos = p_file.rfind('/',p_file.size()-2);
    if (pos != string::npos) {
      ret = p_file.substr(pos+1);
    }
    if (ret[ret.length()-1] == '/') ret.erase(ret.length()-1);
  }
  return ret;
}
string EcceURL::getHost() const
{ return p_host; }
int EcceURL::getPort() const
{ return p_port; }
string EcceURL::getProtocol() const
{ return p_protocol; }


// This is the anchor or reference or "everything but the file"
string EcceURL::getRef() const
{
  string ret;
  if (p_protocol.empty()) {
  } else {
    ret = p_protocol + "://" + p_host;
    if (p_port > 0 ) {
      char buf[12]; // plenty big for port #
      sprintf(buf,":%d/",p_port);
      ret.append(buf);
    } else {
      ret += "/";
    }
  }
  return ret;
}


/**
 * Recreate the url as a string.
 * Parameters if they exist will be appended with the normal
 * ?key=value&key=valyu syntax.
 */
string EcceURL::toString(bool includeParams) const
{
  string ret = "";
  if (p_protocol.empty()) {
    ret.append(p_file);
#if 00000
// GDB 4/26/02 Hack to support Apache 2
    if (ret.length() != 1 && ret[ret.length()-1] == '/') 
      ret.erase(ret.length()-1);
#endif
  } else {
    ret = p_protocol + "://" + p_host;
    if (p_port > 0 ) {
      char buf[12]; // plenty big for port #
      sprintf(buf,":%d",p_port);
      ret.append(buf);
    }
    if (!p_file.empty()) {
      ret.append(p_file);
    } else {
      ret.append("/");
    }
  }

  if (includeParams && p_params && p_params->size() > 0) {
    ret.append("?");
    ret.append(paramsAsString());
  }

  return ret;
}


/**
 * Recreate the url as a string with '/' at the end
 *
 * Notice that it may cause problem to urls with param
 */
string EcceURL::toDirString() const
{
  string ret = toString();
  if (ret[ret.size()-1] != '/')
    ret.append("/");
  return ret;
}


/**
 * Creates a string from the params.
 * Returns empty string if there are no parameters.  Otherwise,
 * returns string of form key1=value&key2=value
 */
string EcceURL::paramsAsString() const
{
  string ret;
  if (p_params != 0 && p_params->size() > 0) {
    for (EcceMapIterator it=p_params->begin(); it!=p_params->end(); it++) {
      ret.append((*it).first);
      ret.append("=");
      ret.append((*it).second);
      ret.append("&");
    }
    ret.erase(ret.size()-1);
  }
  return ret;
}

/**
 * Generates new url with child as the last part of the new path.
 * This method does not account for url parameters if any (ie if they 
 * exist they are not carried over to the new url).
 */
EcceURL EcceURL::getChild(const string & child) const
{
  string file = p_file;
  if (file == "") {
    if (child[0] != '/') file.append("/");
  } 
  else {
    if (file[file.length()-1] != '/') {
      if (child[0] != '/') {
        file.append("/");
      }
    }
  }
  file.append(child);

  EcceURL ret;
  ret.set(p_protocol.c_str(),p_host.c_str(),p_port,file.c_str());
  return ret;
}

/**
 * Generates new url that represents parent of current url.
 * This method does not account for url parameters if any (ie if they 
 * exist they are not carried over to the new url).
 */
EcceURL EcceURL::getParent() const
{
  EcceURL ret = *this;
  ret.clearParams();

  if (p_file != "/") {
    // Handle paths of form /path and /path/
    size_t pos = p_file.rfind('/');
    if (pos == p_file.length()-1) {
      pos = p_file.rfind('/',pos-1);
    }
    ret.p_file = p_file.substr(0,pos);  // Do not include the '/'

  }

  return ret;
}

/**
 * Clears all url params.
 * Parameter are typically used for POST commands with urls of the
 * form .../script?param1=value&param2=value
 */
void EcceURL::clearParams()
{
  if (p_params) {
    p_params->clear();
    delete p_params;
    p_params = 0;
  }
}

void EcceURL::addParam(const string& key, const string& value)
{
  if (p_params == 0) p_params = new EcceMap;
  (*p_params)[key] = value;
}

void EcceURL::removeParam(const string& key)
{
  if (p_params != 0) {
    p_params->erase(key);
  }
}

EcceMap *EcceURL::getParams() const
{
  EcceMap *ret = 0;
  if (p_params != 0) {
    ret = new EcceMap(*p_params);
  }
  return ret;
}

void 
EcceURL::set(const char* protocol,const char *host,int port,const char *file)
{
  p_protocol = protocol;
  p_host = host;
  p_port = port;
  p_file = file;

  int size = p_file.size();
  if (size > 2 && p_file[size-1] == '/') {
    p_file = p_file.substr(0, size-1);
  }
}

// Description
//   Doesn't do well with "//file"
void EcceURL::parse(const char *xurl)
{
  char *url = strdup(xurl);

  // reset everything
  p_protocol = "";
  p_host = "";
  p_file = "";
  p_port = -1;
  clearParams();

  if (url != (char*)0) {
    char *loc = 0;
    char *it = 0;

    char *end = (char*)url ;

#if 000000
// GDB 4/26/02 Hack to support Apache 2
    if ((end[strlen(end)-1] == '/') && (strlen(end) > 2)) {
      end[strlen(end)-1] = '\0'; 
    }
#endif

    // Pick off the protocol or "scheme" as defined in the URL recommendataion
    loc = strchr(url,':');
    if (loc != 0) {
      for (char *it=(char*)url; it<loc; it++) {
        p_protocol.append(1,*it);
      }
      loc++;  // increment past ':'
    } else {
      loc = (char*)url;
    }

    // Now ignore potential password stuff. Should be ://
    if (!p_protocol.empty() && 
        *loc == '/' && (*(loc+1) != '\0' && *(loc+1) == '/')) {
      loc+=2;
      char *endhost = strchr(loc,'/');
      if (endhost == 0) {
        endhost = (char*)&loc[strlen(loc)-1];
      }

      char *portloc = strchr(loc,':');
      if (portloc != 0) {
        // We have a port.  First get the internet domain name
        for (char *it=loc; it<portloc; it++) {
          p_host.append(1,*it);
        }
        loc = portloc+1;  // increment past ':'
        // Then dig out the port
        p_port = atoi(loc);
        char *tmp;
        p_port = (int) strtol(loc,&tmp,10);
        loc = tmp;
      } else {
        // no port - all internet domain
        char *tmpend = (*endhost == '/') ? endhost : endhost + 1;
        for (it=loc; it<tmpend; it++) {
          p_host.append(1,*it);
        }
        loc = tmpend;
      }
      // Now the rest should be the path
      if (loc != end) {
        p_file = loc;
      }
    } else {
      p_file = loc;
    }

    free(url);

    if (p_file.size() > 0) {
      // Now parse any parameters off the file url.  These are POST
      // parameters of the form ?key=value&key=value
      int pstart = p_file.find("?");
      if (pstart >= 0) { 

        if (p_params == 0) p_params = new EcceMap;

        string params = p_file.substr(pstart+1);
        p_file = p_file.substr(0,pstart);
        string kv;
        do {
          pstart = params.find("&");
          if (pstart >= 0) {
            kv = params.substr(0,pstart);
            params = params.substr(pstart+1);
          } else {
            kv = params;
            params = "";
          }
          pstart = kv.find("=");
          if (pstart >=0 ) {
            (*p_params)[kv.substr(0,pstart)] = kv.substr(pstart+1);
          }
        } while (params.size() > 0);

      }
      
      int size = p_file.size();
      if (size > 2 && p_file[size-1] == '/') {
        p_file = p_file.substr(0, size-1);
      }
    }
  }
}


/**
 * Check if parentUrl is a substring of this url.
 *
 * This function together with the following five are assuming that Ecce
 * is using hardwired path for the root and user directories. There might
 * be better ways to do it but at least by using these functions only this
 * file contains hardwired path and it makes it much easier to change if
 * needed in the future.
 */
bool EcceURL::isChildOf(const EcceURL & parentUrl) const
{
  string parentUrlStr = parentUrl.toDirString();
  string currentUrlStr = toString();
  return (currentUrlStr.find(parentUrlStr) == 0) &&
    parentUrlStr.length() < currentUrlStr.length();
}

/**
 * Check if current URL is either a child of the URL passed in or the same.
 * Used by applications when a remove message is received to determine
 * whether it applies to them or not.
 */
bool EcceURL::isChildOrMe(const string & parentUrlStr) const
{
  string parentDirStr;
  if (parentUrlStr[parentUrlStr.size()-1] == '/')
    parentDirStr = parentUrlStr;
  else
    parentDirStr = parentUrlStr + "/";

  string currentDirStr = toDirString();
  return (currentDirStr.find(parentDirStr) == 0);
}


bool EcceURL::isLocal() const
{
  return (p_protocol == "file" || p_protocol == "");
}


bool EcceURL::isSystemFolder() const
{
  if (p_protocol == "http" || p_protocol == "https") {
    if (p_file == "/Ecce" ||
        p_file == "/Ecce/" ||
        p_file == "/Ecce/users" ||
        p_file == "/Ecce/users/" ||
        p_file == "/Ecce/system" ||
        p_file.find("/Ecce/system/") == 0 ||
        p_file == "/Ecce/share" ||
        p_file == "/Ecce/share/" ||
        p_file == "/Ecce/share/StructureLibrary" ||
        p_file.find("/Ecce/share/StructureLibrary/") == 0 ||
        getParent().isEcceUsers())
      return true;
  }
  return false;
}


/**
 * Check if current URL is the root directory of Ecce server
 */
bool EcceURL::isEcceRoot() const
{
  string target = "/Ecce";
  return (p_file == target || p_file == target + "/")
    && (p_protocol == "http" || p_protocol == "https");
}


/**
 * Check if current URL is the users directory
 */
bool EcceURL::isEcceUsers() const
{
  string target = "/Ecce/users";
  return (p_file == target || p_file == target + "/")
    && (p_protocol == "http" || p_protocol == "https");
}


/**
 * Check if current URL is the current user's home directory
 */
bool EcceURL::isEcceUserHome() const
{
  string target = "/Ecce/users/" + string(Ecce::serverUser());
  return (p_file == target || p_file == target + "/")
    && (p_protocol == "http" || p_protocol == "https");
}


/**
 * Return the user name part of the URL, empty if not inside a user directory.
 */
string EcceURL::getEcceUserName() const
{
  if (p_file.length() <= 12)
    return "";
  if (p_file.substr(0, 12) != "/Ecce/users/")
    return "";
  size_t pos = p_file.find("/", 12);
  if (pos == string::npos)
    return p_file.substr(12);
  return p_file.substr(12, pos - 12);
}


/**
 * Returns the root part of the URL, empty if not.
 * For example, http://foxnsox.emsl.pnl.gov:8080/Ecce.
 */
string EcceURL::getEcceRoot() const
{
  string ret = "";
  if (p_protocol == "http" || p_protocol == "https") {
    string target = "/Ecce";
    string url = toString();
    size_t idx = url.find(target);
    if (idx != string::npos) {
      ret = url.substr(0, idx + target.length());
    }
  }
  return ret;
}


/**
 * Serialize a vector of EcceURLs into a string
 */
string EcceURL::toString(const vector<EcceURL> & urls)
{
  string ret = "";
  vector<EcceURL>::const_iterator urlItor = urls.begin();
  for (; urlItor != urls.end(); urlItor++) {
    if (ret != "")
      ret.append(URL_DELIMITER);
    ret.append(urlItor->toString());
  }
  return ret;
}



/**
 * Deserialize a string into a vector of EcceURLs 
 */
vector<EcceURL> EcceURL::toVector(const string & str)
{
  vector<string> urlStrs;
  StringTokenizer tokenizer(str);
  urlStrs = tokenizer.tokenize(URL_DELIMITER);
  
  vector<EcceURL> ret;
  vector<string>::iterator urlItor = urlStrs.begin();
  for (; urlItor != urlStrs.end(); urlItor++) {
    ret.push_back(EcceURL(*urlItor));
  }
  return ret;
}


/**
 * Return true if two url are from same server and not accross different user
 * workspace.
 * If return false, the operations between two urls will require EDSIExporter.
 */
bool EcceURL::isSameDomain(const EcceURL & url1, const EcceURL & url2)
{
  return (url1.getHost() == url2.getHost() &&
          url1.getPort() == url2.getPort() &&
          (url1.getEcceUserName() == "" ||
           url2.getEcceUserName() == "" ||
           url1.getEcceUserName() == url2.getEcceUserName()));
}


ostream& operator <<(ostream& os, const EcceURL& url)
{
  os << url.toString();
  return os;
}

