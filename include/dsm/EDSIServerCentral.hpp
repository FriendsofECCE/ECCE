/////////////////////////////////////////////////////////////////////////////
// Design
//   This class is a placeholder for methods to help manage whatever
//   strategy we ultimitely arrive at for load balancing and/or
//   distributing users data storage services.
//    
//   Initially we have only one server but we anticipate more.  This
//   class provides methods to find the servers that are available, what
//   server a user is assigned to or at least where their default session
//   and preference information is stored...
//
//   Currently the info is loaded once since its the same for all users.
//   In the future, perhaps should not be static.
/////////////////////////////////////////////////////////////////////////////
#ifndef _EDSISERVERCENTRAL_H
#define _EDSISERVERCENTRAL_H

#include <fstream>
 using std::ifstream;

#include <string>
#include <vector>
 using std::string;
 using std::vector;


#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
 using namespace xercesc;

#include "util/EcceURL.H"
#include "util/Bookmark.H"

#include "util/EcceException.H"
#include "util/RetryException.H"


class EDSIServerCentral 
{

  public: 
    EDSIServerCentral();
    virtual ~EDSIServerCentral();

    bool checkServer() throw (RetryException,EcceException);
    bool checkServerSetup() throw (RetryException,EcceException);
    bool checkDefaultGBSL();

         /**
          * Get list of available server mount points.
          * Currently this is system-wide information.
          */
    vector<Bookmark> *getEDSIProviders();

         /**
          * For a given calculation URL, return the mount point
          * name to make it more readable to users.  One method
          * returns the full name and one abbreviated if it's
          * under the user's home area.
          */
    string mapURLtoFullName(const EcceURL& url);
    string mapURLtoName(const EcceURL& url);

         /**
          * Get/set list of read-only and read-write users for the
          * given url
          */
    int getAccess(const string& projectUrl, string& access, string& errMessage);
    int setAccess(const string& projectUrl,
                  const string& readOnlyStr, const string& readWriteStr, string& errMessage);

          /**
           * Get mount point for the primary server.  This is a location where
           * users preferences can be stored much like a home directory.
           * It need not be the same as any of the urls returned from
           * getEDSIProviders.
           */
    Bookmark getDefaultProvider();

          /**
           * Get URLs for location of default libraries.
           */
    EcceURL   getDefaultBasisSetLibrary();
          /**
           * Structure Library path.
           */
    EcceURL    getDefaultStructureLibrary();

          /**
           * Path to system configuration info - local data
           */
    EcceURL    getConfigLocation();

          /**
           * Location of user home area for any system files or for auto
           * opening their view.
           */
    EcceURL    getDefaultUserHome();


          /**
           * Location of user home area for any system files or for auto
           * opening their view.
           */
    static EcceURL getUserHome(const EcceURL& rootUrl);

          /**
           * Location of Force Field parameter files
           */
    vector<string> *getDefaultParamFiles(string forceFieldName);

          /**
           * Location of Force Field segment directories
           */ 
    vector<string> *getDefaultSegmentDirs(string forceFieldName);
    static string readRealm();

  protected:
    static vector<Bookmark> p_mountPoints;
    string p_mountFile;

    void loadMountPointInfo();
    bool getline(ifstream& is, string& key, string& value);

  private:

    string getNodeValue(DOMNode *domNode);
    string getForceFieldName(DOMNode *forceField);
    bool   getDefaultForceField(DOMNode*& forceField, string forceFieldName);

    DOMDocument    *p_doc;
    string         p_serverKey;
    string         p_UrlKey;
    string         p_DescKey;
    string         p_BasisSetKey;
    string         p_StructureLibKey;
    string         p_forceFieldKey;
    string         p_paramFileKey;
    string         p_paramFilesKey;
    string         p_segmentDirKey;
    string         p_segmentDirsKey;
};

#endif
