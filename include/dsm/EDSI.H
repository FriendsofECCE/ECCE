#ifndef _EDSI_H
#define _EDSI_H

#include <iostream>
  using std::istream;
  using std::ostream;

#include <string>
#include <vector>
  using std::string;
  using std::vector;

#include "util/EcceURL.H"
#include "dsm/EDSIMessage.H"
#include "dsm/ResourceDescriptor.H"

class MetaDataRequest;
class MetaDataResult;
class ResourceResult;
class ResourceMetaDataResult;
class AuthEventListener;
class ProgressEventListener;

class EDSI {
   public: 
     enum EDSIOverwrite { YES, NO, SORTOF };

     EDSI();
     EDSI(const EcceURL& url);
     EDSI(const EDSI& rhs);
     virtual ~EDSI();

     virtual EcceURL getURL() const;
     virtual void setURL(const EcceURL& url);

     /**
      * Add and remove listener for suppling of passwords.
      */
     virtual void addAuthEventListener(AuthEventListener *l) = 0;
     virtual void removeAuthEventListener(AuthEventListener *l) = 0;

     /**
      * Add and remove listener for monitoring progress of operations and
      * supporting interrupts.
      */
     virtual void addProgressEventListener(ProgressEventListener *l) = 0;
     virtual void removeProgressEventListener(ProgressEventListener *l) = 0;

     /**
      * Returns list of properties considered standard for this EDSI
      * implementation.  This information is not based on a URL at this
      * time and can be called without ever supplying a URL.
      */
     virtual bool describeServerMetaData(vector<string>& metadata) = 0;

     /**
       * Checks to see if the server associated with the EDSI object is
       * valid and operating.  If a problem exists, a message is returned
       * as a string.  Otherwise an empty string is returned.
       */
     virtual bool checkServer() = 0;

    /**
     *
     *  Inexpensive query to determine if the URL exists and is available.
     *
    **/
     virtual bool exists(const bool& newUser=false) = 0;

     /**
      * Returns a stream of data for accessing the url contents.  If the
      * url is a collection, this will return a null pointer.
      */
     virtual istream* getDataSet() = 0;
     virtual bool getDataSet(ostream& dest) = 0;

     /**
      * Returns the size of the data set contents.
      *
     **/
     virtual unsigned long getDataSetSize() = 0; 

     /**
      * Returns a stream of data for accessing a subset of url contents.  If
      * the url is a collection this will return a null pointer.
      */
     virtual istream* getDataSubSet(int start_position, 
                                    int length) = 0;

     virtual bool getDataSubSet(ostream& dest, 
                                int start_position, 
                                int length) = 0;
     /**
      * Returns a list of all children of the current url.  If the url is
      * exists but is not a directory, or does not exist false is returned.
      * Symbolic links should be followed.
      */
     virtual bool listCollection(vector<ResourceResult>& results) = 0;

     /**
      *
      */
     virtual bool listCollection(const vector<MetaDataRequest>& requests, 
         vector<ResourceMetaDataResult>& results) = 0;
     /**
      *
      */
     virtual bool listCollection(vector<ResourceMetaDataResult>& results) = 0;
     /**
      *  Write the string as the contents of the current url.  If the
      *  current url is a collection, false is returned.
      */
     virtual bool putDataSet(const char* putStream) = 0;

     /**
      *  Write the stream as the contents of the current url.  If the
      *  current url is a collection, false is returned.
      */
     virtual bool putDataSet(istream& putStream) = 0;

     /**
      *  Write the string as the contents of the current url.  If the
      *  current url is a collection, false is returned.
      */
     virtual bool appendDataSet(const char* putStream, int bytesToOverwrite=0) = 0;

     /**
      *  Write the stream as the contents of the current url.  If the
      *  current url is a collection, false is returned.
      */
     virtual bool appendDataSet(istream&  putStream, int bytesToOverwrite=0) = 0;

     /**
      *  Get the specified meta data properties.  If a chunk of requested
      *  meta data does not exist, a warning and NOT error will result.
      */
     virtual bool getMetaData(const vector<MetaDataRequest>& requests, 
         vector<MetaDataResult>& results, bool getVDocMetaData = true) = 0;

     /**
      *  Get the specified resource meta data properties.  
      *  If a chunk of requested meta data does not exist, a warning 
      *  and NOT error will result.  If the request vector is empty, all
      *  properties will be returned.
      */
     virtual bool getMetaData(const vector<MetaDataRequest>& requests, 
         ResourceMetaDataResult& result, bool getVDocMetaData = true) = 0;

     /**
      *  Writes the meta data properties for the current url.  If
      *  a given chunk of meta data exists, it will be overwritten.
      *  Implementations that are unable to support writing of meta data
      *  should return false.
      */
     virtual bool putMetaData(const vector<MetaDataResult>& results) = 0;

     /**
      *  Removes the named meta-data properties.  Locks are acquired and
      *  released if necessary.  Warnings, not errors are issued when 
      *  the request specifies the removal of meta data that does not exist.
      */
     virtual bool removeMetaData(const vector<MetaDataRequest>& requests) = 0;

     /**
      * Returns name of all properties associated with the current resource.
      */
     virtual bool describeMetaData(vector<MetaDataResult>& metaDataNames) = 0;

     /**
      *  Moves the current url to the specified target url.  If overwrite
      *  is NO, false is returned if the target url already exists.
      */
     virtual bool moveResource(EcceURL& targetURL, 
         EDSIOverwrite overwrite=NO) = 0;

     /**
      * Returns true if the resource is locked.  In this case the name of
      * the locker (if available) is returned as an argument.
      */
     virtual bool isLocked(string& locker) = 0;


     /**
      * Returns true if the resource is writable. 
      */
     virtual bool isWritable() = 0;

     /**
      * Acquires lock of the targetURL parent and any children locks 
      * if necessary.  If overwrite is NO and the targetURL exists, 
      * the copy fails.  If overwrite is YES and the targetURL exists, 
      * it will be overwritten.  If overwrite is SORTOF, a uniquename 
      * will be generated and the targetURL will be modified.  
      * There are many possible points of failure here.  Copies
      * across servers are not supported directly - the application
      * must do this as a multi-step process.
      */
     virtual bool copyResource(EcceURL& targetURL, EDSIOverwrite overwrite=NO) = 0;

     /**
      * Removes the resource specified by this instances url.  Attempts
      * to remove a non-existent url will result in warnings, not errors.
      */
     virtual bool removeResource() = 0;

     /**
      * This is a convenience method for removing multiple, probably
      * unrelated urls.  The operation is independent of the url
      * associated with the EDSI instance.  This design was chosen to
      * allow this to be a virtual function (could be static).  The
      * implementation should attempt to remove all resources, not
      * stopping because it encountered an error on one of the urls.
      * Deletion of a non-existent url should be a warning rather than an
      * error.
      *
      */
     virtual bool removeResources(const vector<EcceURL> urls) = 0;

     /**
      * Given an initial name for a new collection, this method will
      * create necessary locks, automatically alter the value of base to
      * guarantee a unique name, create the collection, and release the
      * locks.  The returned value is the url with the new collection
      * name.  The collection is created under the current url.
      */
     virtual EcceURL *makeCollection(const string& base, const string& pattern="-%d") = 0;

     /**
      * Same as makeCollection except creates an empty document.
      */
     virtual EcceURL *makeDataSet(const string& base) = 0;

     /**
      * Using base as the base portion of a name for a new object, look
      * at the names in the current urls collection and modify the base
      * name to ensure that it is unique.  This operation does not alter
      * the data server and is typically used internally to other
      * methods.  The unique name is generated on the part of the
      * filenames that don't include the file extension (everything
      * before the last '.').
      */
     virtual string uniqueName(const string& base, const string& pattern="-%d") = 0;

     /**
      * Returns the class name.
      */
     virtual string getClassName() = 0;

     /**
      * Starting at url "start", search for any document or collections
      * with the specified substring anywhere in the name.  The full urls
      * are returned in the matches vector.  false is only returned for
      * true errors like unable to contact server.  No matches is not an
      * error.
      */
     virtual bool efind(const string& key, const string& substring, 
                        const EcceURL& start, vector<EcceURL>& matches) = 0;
     virtual bool efindProp(const string& substring, const EcceURL& start, 
                            vector<EcceURL>& matches) = 0;

     /**
      * The message stack contains the possibly multiple messages from
      * the execution of the last method. If the last operation failed, 
      * this will be an erro message.  If the last operation succeeded, 
      * this string may contain a warning  message.
      */
     EDSIMessage m_msgStack;

   protected:
     EcceURL p_url;

   private:
};

class MetaDataRequest {
  public:
  string name;
  string filter;
};
class MetaDataResult {
  public:
  string name;
  string type;
  string value;
};
class ResourceResult {
  public: 
    EcceURL        url;
    ResourceDescriptor::RESOURCETYPE   resourcetype;
    string         contenttype;
};
class ResourceMetaDataResult : public ResourceResult {
  public:
    vector<MetaDataResult> metaData;
};
#endif
