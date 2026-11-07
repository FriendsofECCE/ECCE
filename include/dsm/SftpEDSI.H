#ifndef _SFTPEDSI_H
#define _SFTPEDSI_H

#include <iostream>
#include <string>
#include <vector>
  using std::istream;
  using std::ostream;
  using std::string;
  using std::vector;

#include "dsm/EDSI.H"
#include "util/EcceURL.H"

class FileEDSI;

// Storage in a folder on any machine with an ssh account:
// sftp://user@host/absolute/folder.  The folder has FileEDSI's layout
// (.ecce-meta per directory), so a folder copied either way works.  Every
// operation runs on the login RCommand makes for jobs (OpenSSH with ECCE's
// master, or libssh) as sh commands, and the metadata logic is FileEDSI's,
// run on a scratch mirror of the directories involved.  Metadata
// read-modify-write holds <dir>/.ecce-meta.lockdir on the server.
class SftpEDSI : public EDSI
{
  public:
    SftpEDSI();
    SftpEDSI(const EcceURL& url);
    virtual ~SftpEDSI();

    virtual bool describeServerMetaData(vector<string>& metadata);
    virtual bool checkServer();

    void addAuthEventListener(AuthEventListener *l) { p_authListener = l; }
    void removeAuthEventListener(AuthEventListener *) { p_authListener = 0; }
    void addProgressEventListener(ProgressEventListener *l) { p_progressListener = l; }
    void removeProgressEventListener(ProgressEventListener *) { p_progressListener = 0; }

    virtual bool exists(const bool& newUser=false);
    // Modification time (as text) of this folder: one stat on the login, no
    // listing; empty when it cannot be read.  For the Organizer's auto-refresh.
    string folderStamp();

    virtual istream *getDataSet();
    virtual unsigned long getDataSetSize();
    virtual bool getDataSet(ostream& dest);
    virtual istream* getDataSubSet(int start_position, int length);
    virtual bool getDataSubSet(ostream& dest, int start_position, int length);

    virtual bool putDataSet(const char *putStream);
    virtual bool putDataSet(istream& putStream);
    virtual bool appendDataSet(const char *putStream, int bytesToOverWrite=0);
    virtual bool appendDataSet(istream& putStream, int bytesToOverWrite=0);

    virtual bool listCollection(vector<ResourceResult>& results);
    virtual bool listCollection(vector<ResourceMetaDataResult>& results);
    virtual bool listCollection(const vector<MetaDataRequest>& requests,
                                vector<ResourceMetaDataResult>& results);

    virtual bool getMetaData(const vector<MetaDataRequest>& requests,
        vector<MetaDataResult>& results, bool getVDocMetaData = true);
    virtual bool getMetaData(const vector<MetaDataRequest>& requests,
        ResourceMetaDataResult& results, bool getVDocMetaData = true);
    virtual bool putMetaData(const vector<MetaDataResult>& results);
    virtual bool removeMetaData(const vector<MetaDataRequest>& requests);
    virtual bool describeMetaData(vector<MetaDataResult>& metaDataNames);

    virtual bool moveResource(EcceURL& targetURL, EDSIOverwrite overwrite=NO);
    virtual bool copyResource(EcceURL& targetURL, EDSIOverwrite overwrite=NO);
    virtual bool removeResource();
    virtual bool removeResources(const vector<EcceURL> urls);
    virtual bool efind(const string& key, const string& substring,
                       const EcceURL& start, vector<EcceURL>& matches);
    virtual bool efindProp(const string& substring, const EcceURL& start,
                           vector<EcceURL>& matches);
    virtual bool isWritable();
    virtual bool isLocked(string& locker);
    virtual EcceURL *makeCollection(const string& base, const string& pattern="-%d");
    virtual EcceURL *makeDataSet(const string& base);
    virtual string uniqueName(const string& base, const string& pattern="-%d");
    virtual string getClassName();

    // user, host and path of an sftp URL; false if it is not one.
    // Log in as user@host (with prompts, as jobs do) and look at the folder.
    // The login stays open for the operations that follow in this process.
    enum Probe { PROBE_OK, PROBE_NO_LOGIN, PROBE_MISSING, PROBE_NOT_WRITABLE };
    static Probe probe(const string& user, const string& host, const string& folder,
                       bool create, string& message);
    // Prompts are off except while probe() runs.
    static void setInteractive(bool on);
    static bool parseURL(const string& url, string& user, string& host,
                         string& path);

  private:
    SftpEDSI(const SftpEDSI&);
    SftpEDSI& operator=(const SftpEDSI&);
    AuthEventListener *p_authListener;
    ProgressEventListener *p_progressListener;
    friend class SftpOp;
};

#endif
