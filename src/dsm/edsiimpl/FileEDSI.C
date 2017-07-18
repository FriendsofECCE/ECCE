#include <iostream>
  using std::ends;

#include <fstream>
  using std::ifstream;
  using std::ofstream;

#include <strstream>
  using std::ostrstream;
  using std::strstreambuf;

#include <stdio.h> // sprintf
#include <string.h>

#include "util/SDirectory.H"
#include "util/ErrMsg.H"
#include "util/ProgressEvent.H"
#include "util/TDateTime.H"
#include "util/AuthEvent.H"

#include "dsm/FileEDSI.H"
#include "dsm/ResourceDescriptor.H"


FileEDSI::FileEDSI() : EDSI()
{
  p_authListener = 0;
  p_progressListener = 0;
}

FileEDSI::FileEDSI(const EcceURL& url) : EDSI(url)
{
  p_authListener = 0;
  p_progressListener = 0;

  // GDB 1/21/13  Not sure what happened that caused this logic to be needed
  // now while it seemed to be working fine before....But, there were
  // definitely file paths with $ECCE_HOME in regards to at least the
  // structure libraray (DNA builder and peptide builder) so expand the
  // path to the file name before attempting to open/read the file.
  SFile file(p_url);
  p_url = file.path(true);
}

FileEDSI::FileEDSI(const FileEDSI& rhs) : EDSI(rhs)
{
  p_authListener = rhs.p_authListener;
  p_progressListener = rhs.p_progressListener;
}

FileEDSI::~FileEDSI()
{
}

/**
 * Checks to see if server is operating and suitable for EDSI.
 *
 * For a File EDSI object, this currently means the following:
 * <li>protocol is file or nothing
 * <li>server and port are empty.
 * <li>file can be stat'd.
 */
bool FileEDSI::checkServer()
{
  m_msgStack.clear();
  bool ret = true;
  string protocol = p_url.getProtocol();
  if (protocol != "file" && !protocol.empty()) {
    ret = false;
  } else if (!p_url.getHost().empty() || p_url.getPort() != -1) {
    ret = false;
  } else {
    SFile file(p_url.getFile().c_str());
    if (!file.exists()) {
      ret = false;
    }
  }
  if (!ret)  m_msgStack.add("SERVER_NOT_FOUND",p_url.toString().c_str());
  return ret;
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Return list of key attributes of file (see fstat).
//   List here should match method describeMetaData().
////////////////////////////////////////////////////////////////////////////
bool FileEDSI::describeServerMetaData(vector<string>& metadata)
{
  metadata.push_back("DAV:displayname");
  metadata.push_back("DAV:getcontentlength");
  metadata.push_back("DAV:getlastmodified");
  metadata.push_back("resourcetype");
  metadata.push_back("contenttype");
  metadata.push_back("application");
  // Others like ctime, atime, uid, gid also possible.
  return true;
}

// Listener stuff for getting passwords.  We currently don't have code to
// invoke the listener for this implementation.
void FileEDSI::addAuthEventListener(AuthEventListener *l)
{
  p_authListener = l;
}

void FileEDSI::removeAuthEventListener(AuthEventListener *l)
{
  if (p_authListener == l) p_authListener = 0;
}

void FileEDSI::addProgressEventListener(ProgressEventListener *l)
{
  p_progressListener = l;
}

void FileEDSI::removeProgressEventListener(ProgressEventListener *l)
{
  if (p_progressListener == l) p_progressListener = 0;
}


bool FileEDSI::exists(const bool& newUser)
{
  SFile test(getURL().getPath());
  return test.exists();
}

istream* FileEDSI::getDataSubSet(int start_position, int length)
{
  istream* ret = NULL;
  m_msgStack.add("NOT_IMPLEMENTED","getDataSubSet");
  return ret;
}

bool FileEDSI::getDataSubSet(ostream& dest, int start_position, int length)
{
  bool ret = false;
  m_msgStack.add("NOT_IMPLEMENTED","getDataSubSet");
  return ret;
}

unsigned long FileEDSI::getDataSetSize()
{
  unsigned long ret = 0;
  m_msgStack.add("NOT_IMPLEMENTED","getDataSetSize");
  return ret;
}


bool FileEDSI::getDataSet(ostream& dest)
{
  bool ret = false;
  m_msgStack.clear();
  SFile file(p_url.getPath().c_str());
  if (file.exists() && file.is_regular_file()) {
    static const int BUFSIZE=512;
    char buf[BUFSIZE];
    ifstream myfile(file.path().c_str());
    if (myfile) {
      while (myfile.getline(buf,BUFSIZE-1)) {
        dest << buf << "\n";
      }
      myfile.close();
      ret = true;
    }
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////
// Description
//   Open the file stream, read it into a memory stream and return that
//   stream.
//
//   TODO handle symbolic links?
////////////////////////////////////////////////////////////////////////////
istream *FileEDSI::getDataSet()
{
  istream *ret = NULL;
  m_msgStack.clear();

  static const int BUFSIZE=512;
  char buf[BUFSIZE];

  SFile file(p_url.getPath().c_str());
  bool exists = file.exists();
  if (exists && file.is_regular_file()) {
    ifstream myfile(file.path().c_str());
    if (myfile) {
      ostrstream mem;
      while (myfile.getline(buf,BUFSIZE-1)) {
        mem << buf << "\n";
      }
      myfile.close();
      mem << ends;
      string str = mem.str();

      strstreambuf *buffer = new strstreambuf(str.length());
      buffer->sputn(str.c_str(), str.length());
      ret = new istream(buffer);

    } else {
      m_msgStack.add("UNABLE_TO_READ",file.path().c_str());
    }
  } else if (exists) {
    m_msgStack.add("NOT_REGULAR_FILE",file.path().c_str());
  } else {
    m_msgStack.add("RESOURCE_NOT_FOUND",file.path().c_str());
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////////
// Description
////////////////////////////////////////////////////////////////////////////

bool FileEDSI::listCollection(vector<ResourceMetaDataResult>& result) {
  return false;
}

bool FileEDSI::listCollection(const vector<MetaDataRequest>& requests, vector<ResourceMetaDataResult>& result)
{
  bool ret = false;

  SDirectory dir(p_url.getPath().c_str());

  if (!dir.exists()) {
    m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
  } else if (dir.exists() && !dir.is_dir()) {
    m_msgStack.add("NOT_COLLECTION",dir.path().c_str());
  } else {
    vector<SFile> files = dir.get_files(false);
    int cnt = files.size();
    ret = true;

    ResourceMetaDataResult rmdr;
    vector<MetaDataResult> tmp;
    for (int idx=0; idx<cnt; idx++) {
      if (strncmp(files[idx].filename().c_str(), ".", 1) != 0) {
        rmdr.url = files[idx].path().c_str();
        rmdr.resourcetype = ResourceDescriptor::RT_COLLECTION;
        if (files[idx].is_regular_file()) 
          rmdr.resourcetype = ResourceDescriptor::RT_DOCUMENT;
        rmdr.contenttype = "";
        FileEDSI fe(files[idx].path().c_str());
        tmp.clear();
        // iterate through requested properties and add to actual result
        if (fe.describeMetaData(tmp)) {
          rmdr.metaData.clear();
          vector<MetaDataRequest>::const_iterator requestIt = requests.begin();
          while (requestIt != requests.end()) {
            vector<MetaDataResult>::iterator resultIt = tmp.begin();
            while (resultIt != tmp.end()) {
              if (rmdr.contenttype.empty() && resultIt->name == "contenttype") {
                rmdr.contenttype = resultIt->value;
              }
              if (resultIt->name.compare(requestIt->name) == 0) {
                rmdr.metaData.push_back(*resultIt);
                break;
  
              }
              resultIt++;
            }
            requestIt++;
          }
        }
        result.push_back(rmdr);
      }
    }
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////
// Description
////////////////////////////////////////////////////////////////////////////
bool FileEDSI::listCollection(vector<ResourceResult>& result)
{
  bool ret = false;
  m_msgStack.clear();

  SDirectory dir(p_url.getPath().c_str());
  bool exists = dir.exists();
  if (exists && dir.is_dir()) {
    ret = true;
    vector<SFile> files = dir.get_files(false);
    int cnt = files.size();
    for (int idx=0; idx<cnt; idx++) {
      if (strncmp(files[idx].filename().c_str(), ".", 1) != 0) {
        ResourceResult res;
        res.url = EcceURL(files[idx].path());
        res.resourcetype = ResourceDescriptor::RT_COLLECTION;
        res.contenttype = "httpd/unix-directory";
        if (files[idx].is_regular_file()) {
          res.resourcetype = ResourceDescriptor::RT_DOCUMENT;
          res.contenttype = files[idx].extension();
        }
        result.push_back(res);
      }
    }
  } else if (exists) {
    m_msgStack.add("NOT_COLLECTION",dir.path().c_str());
  } else {
    m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::putDataSet(const char *putStream)
{
  bool ret = false;
  m_msgStack.clear();

  SFile file(p_url.getPath().c_str());
  ofstream ofs(file.path().c_str());
  if (ofs) {
    ofs << putStream;
    ofs.close();
    ret = true;
  } else {
    m_msgStack.add("UNABLE_TO_WRITE",file.path().c_str());
  }
  return ret;
}

bool FileEDSI::putDataSet(istream& putStream)
{
  bool ret = false;
  m_msgStack.clear();

  SFile file(p_url.getPath().c_str());
  ofstream ofs(file.path().c_str());
  if (ofs) {
    const int bsz = 1024;
    char      buff[bsz];
    int nRead;
    do {
      putStream.read(buff, bsz);
      nRead = putStream.gcount();
      if (nRead > 0) {
        ofs.write(buff,nRead);
      }
   } while (putStream);
   ofs.close();
   ret = true;
  } else {
    m_msgStack.add("UNABLE_TO_WRITE",file.path().c_str());
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::appendDataSet(const char* putStream, int bytesToOverwrite)
{
  bool ret = false;
  m_msgStack.add("NOT_IMPLEMENTED","appendMetaData");
  return ret;
}


/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::appendDataSet(istream& putStream, int bytesToOverwrite)
{
  bool ret = false;
  m_msgStack.add("NOT_IMPLEMENTED","appendMetaData");
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::getMetaData(const vector<MetaDataRequest>& requests, 
    vector<MetaDataResult>& results, bool getVDocMetaData)
{
  bool ret = false;

  vector<MetaDataResult> tmp;
  if (describeMetaData(tmp)) {
    ret = true;
    int numRequests = requests.size();
    for (int idx=0; idx<numRequests; idx++) {
      vector<MetaDataResult>::iterator resultIt = tmp.begin();
      while (resultIt != tmp.end()) {
        if (resultIt->name.compare(requests[idx].name) == 0) {
          results.push_back(*resultIt);
          break;
        }
        resultIt++;
      }
    }
    tmp.clear();
  }

  return ret;
}

bool FileEDSI::getMetaData(const vector<MetaDataRequest>& requests,
    ResourceMetaDataResult& results, bool getVDocMetaData)
{
  bool ret = false;
  SFile file(p_url.getPath().c_str());

  vector<MetaDataResult> tmp;
  if (getMetaData(requests, tmp)) {
    ret = true;
    results.url = p_url;
    results.resourcetype = ResourceDescriptor::RT_COLLECTION;
    results.contenttype = "httpd/unix-directory";
    if (file.is_regular_file()) {
      results.resourcetype = ResourceDescriptor::RT_DOCUMENT;
      results.contenttype = file.extension();
    }
    for (int i = 0; i < tmp.size(); i++) {
      results.metaData.push_back(tmp[i]);
    }
  }
  return ret;
}


/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::putMetaData(const vector<MetaDataResult>& results)
{
  bool ret = false;
  m_msgStack.add("NOT_IMPLEMENTED","putMetaData");
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::removeMetaData(const vector<MetaDataRequest>& requests)
{
  bool ret = false;
  m_msgStack.add("NOT_IMPLEMENTED","removeMetaData");
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
//   Return a pre-defined set of file system properties.  Some obviously
//   missing stuff includes attributes like owner, creation date...
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::describeMetaData(vector<MetaDataResult>& metaDataNames)
{
  bool ret = false;
  SFile file(p_url.getPath().c_str());
  if (file.exists()) {
    ret = true;
    MetaDataResult mdr;
    char buf[128];

    // Name
    mdr.name = "DAV:displayname";
    mdr.type = "string";
    mdr.value = file.filename().c_str(); 
    metaDataNames.push_back(mdr);

    // Size
    mdr.name = "DAV:getcontentlength";
    mdr.type = "integer";
    unsigned int fileSize = file.size(); 
    sprintf(buf,"%u",fileSize);
    mdr.value = buf;
    metaDataNames.push_back(mdr);

    // resourcetype
    mdr.name = "resourcetype";
    mdr.type = "string";
    if (file.is_dir()) {
      mdr.value = "collection";
    } else if (file.is_link()) {
      mdr.value = "link";
    } else {
      mdr.value = "file";
    }
    metaDataNames.push_back(mdr);

    // contenttype
    mdr.name = "contenttype";
    mdr.type = "string";
    if (file.is_dir()) {
      mdr.value = "httpd/unix-directory";
    } else if (file.is_link()) {
      mdr.value = "link";
    } else {
      mdr.value = file.extension();
    }
    metaDataNames.push_back(mdr);

    // Application
    mdr.name = "application";
    mdr.type = "string";
    mdr.value = "";
    metaDataNames.push_back(mdr);

    // Last Modified
    mdr.name = "DAV:getlastmodified";
    mdr.type = "string";
    mdr.value = file.lastModified().toString();
    metaDataNames.push_back(mdr);
  } else {
    m_msgStack.add("RESOURCE_NOT_FOUND",file.path().c_str());
  }

  return ret;
}


/////////////////////////////////////////////////////////////////////////////
// Description
// TODO - overwrite
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::moveResource(EcceURL& targetURL, EDSIOverwrite overwrite)
{
  bool ret = false;
  m_msgStack.clear();
  SDirectory dir(p_url.getPath().c_str());
  if (dir.exists()) {
    if (dir.is_dir()) {
      ret = dir.move(targetURL.getPath().c_str());
    } else {
      SFile file(dir.path());
      ret = file.move(targetURL.getPath().c_str());
    }
  }
  return ret;
}

bool FileEDSI::copyResource(EcceURL& targetURL, EDSIOverwrite overwrite)
{
  bool ret = false;
  m_msgStack.clear();
  SDirectory dir(p_url.getPath().c_str());
  if (dir.exists()) {
    ErrMsg errs;
    errs.flush();
    if (dir.is_dir() || dir.is_link()) {
      SFile *newfile = dir.copy(targetURL.getPath().c_str());
      delete newfile;
    } else {
      SFile file(dir.path());
      SFile *newfile = file.copy(targetURL.getPath().c_str());
      delete newfile;
    }
    ret = (errs.count() == 0);
  }
  return ret;
}

bool FileEDSI::removeResource()
{
  bool ret = false;
  m_msgStack.clear();
  ret = removeHelper(p_url);
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::removeHelper(const EcceURL& url)
{
  bool ret = false;
  SDirectory dir(p_url.getPath().c_str());
  if (dir.exists()) {
    if (dir.is_dir()) {
      ret = dir.remove();
    } else {
      SFile file(dir.path());
      ret = file.remove();
    }
    if (!ret) {
      m_msgStack.add("UNABLE_TO_WRITE",dir.path().c_str());
    }
  } else {
    m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
EcceURL *FileEDSI::makeCollection(const string& base, const string& pattern)
{
  EcceURL *ret = NULL;
  m_msgStack.clear();

  // At a minimum pattern must include int
  if (pattern.find("%d") != string::npos) {
    // Get a unique name
    string newBase = uniqueName(base);

    string path = p_url.getPath().c_str();
    path += "/" + newBase;
    SDirectory dir(p_url.getPath().c_str());
    if (dir.exists() && dir.is_dir()) {
      if (SDirectory::create(path.c_str(),0744)) {
        ret = new EcceURL(path.c_str());
      } else {
        m_msgStack.add("UNABLE_TO_WRITE",dir.path().c_str());
      }
    } else {
      if (dir.exists()) {
        m_msgStack.add("NOT_COLLECTION",dir.path().c_str());
      } else {
        m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
      }
    }
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
EcceURL *FileEDSI::makeDataSet(const string& base)
{
  EcceURL *ret = NULL;
  m_msgStack.clear();

  // Get a unique name
  string newBase = uniqueName(base);

  string path = p_url.getPath().c_str();
  path += "/" + newBase;
  SDirectory dir(p_url.getPath().c_str());

  // Check the parent directory exists
  if (dir.exists() && dir.is_dir()) {
    if (SFile::create(path.c_str(),0644)) {
      ret = new EcceURL(path.c_str());
    } else {
      m_msgStack.add("UNABLE_TO_WRITE",dir.path().c_str());
    }
  } else {
    if (dir.exists()) {
      m_msgStack.add("NOT_COLLECTION",dir.path().c_str());
    } else {
      m_msgStack.add("RESOURCE_NOT_FOUND",dir.path().c_str());
    }
  }
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
string FileEDSI::uniqueName(const string& guess, const string& pattern)
{
  string base = guess;
  string ext;

  // Strip of the file extension if there is one.
  int pos = base.rfind('.');
  if (pos != string::npos) {
    ext = base.substr(pos);
    base = base.substr(0,pos);
  }

  // Get all the items in the current collection.
  // Maybe should use listCollection but this simpler
  SDirectory dir(p_url.getPath().c_str());
  if (dir.exists() && dir.is_dir()) {
    vector<SFile> files = dir.get_files(false);
    int cnt = files.size();
    int num = 1;
    char buf[128]; 
    strcpy(buf,base.c_str());
    for (int idx=0; idx<cnt; idx++) {
      if (strcmp(buf,files[idx].pathtail().c_str()) == 0) {
        string format = "%s"+pattern;
        sprintf(buf,format.c_str(),base.c_str(),num++);
        idx = 0;
      }
    }
    base = buf;
  }
  base += ext;
  return base;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
string FileEDSI::getClassName()
{
  return "FileEDSI";
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::isWritable()
{
  m_msgStack.add("NOT_IMPLEMENTED","isLocked");
  return false;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::isLocked(string& locker)
{
  m_msgStack.add("NOT_IMPLEMENTED","isLocked");
  return false;
}

/////////////////////////////////////////////////////////////////////////////
// Description
/////////////////////////////////////////////////////////////////////////////
bool FileEDSI::removeResources(const vector<EcceURL> urls) {
  bool ret = false;
  m_msgStack.clear();

  int size = urls.size();
  for (int idx=0; idx<size; idx++) {
    removeHelper(urls[idx]);
  }
  return ret;
}

bool FileEDSI::efind(const string& key, const string& substring, 
                     const EcceURL& start, vector<EcceURL>& matches)
{
  bool ret = false;
  m_msgStack.add("NOT_IMPLEMENTED","putMetaData");
  return ret;
}

bool FileEDSI::efindProp(const string& substring, const EcceURL& start, 
                         vector<EcceURL>& matches)
{
  bool ret = false;
  m_msgStack.add("NOT_IMPLEMENTED","putMetaData");
  return ret;
}

