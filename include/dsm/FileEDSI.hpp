#ifndef _FILEEDSI_H
#define _FILEEDSI_H

#include <iostream>
  using std::istream;
  using std::ostream;

#include <string>
#include <vector>
  using std::string;
  using std::vector;


#include "dsm/EDSI.H"
#include "util/EcceURL.H"

class FileEDSI : public EDSI 
{

   public: 
     FileEDSI();
     FileEDSI(const EcceURL& url);
     FileEDSI(const FileEDSI& rhs);
     virtual ~FileEDSI();

     virtual bool describeServerMetaData(vector<string>& metadata);
     virtual bool checkServer();

     void addAuthEventListener(AuthEventListener *l);
     void removeAuthEventListener(AuthEventListener *l);

     void addProgressEventListener(ProgressEventListener *l);
     void removeProgressEventListener(ProgressEventListener *l);

     virtual bool exists(const bool& newUser=false);

     virtual istream *getDataSet();
     virtual unsigned long getDataSetSize();  
     virtual bool getDataSet(ostream& dest);
     virtual istream* getDataSubSet(int start_position, 
                                    int length);

     virtual bool getDataSubSet(ostream& dest, 
                                int start_position, 
                                int length);

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

     virtual bool moveResource(EcceURL& targetURL, 
         EDSIOverwrite overwrite=NO);
     virtual bool copyResource(EcceURL& targetURL, EDSIOverwrite overwrite=NO);
     virtual bool removeResource();
     virtual bool removeResources(const vector<EcceURL> urls);
     virtual bool efind(const string & key, const string& substring, 
                        const EcceURL& start, vector<EcceURL>& matches);
     virtual bool efindProp(const string& substring, const EcceURL& start, 
                            vector<EcceURL>& matches);
     virtual bool isWritable(); 
     virtual bool isLocked(string& locker);
     virtual EcceURL *makeCollection(const string& base, const string& pattern="-%d");
     virtual EcceURL *makeDataSet(const string& base);
     virtual string uniqueName(const string& base, const string& pattern="-%d");
     virtual string getClassName();

   protected:
     bool removeHelper(const EcceURL& url);
     AuthEventListener *p_authListener;
     ProgressEventListener *p_progressListener;
};

#endif
