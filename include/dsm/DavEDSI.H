#ifndef _DAVEDSI_H
#define _DAVEDSI_H

#include <iostream>
 using std::istream;
 using std::ostream;

#include <vector>
#include <string>
  using std::string;
  using std::vector;

#include <xercesc/dom/DOMDocument.hpp>
  using namespace xercesc;

#include "dsm/EDSI.H"
#include "dsm/ResourceDescriptor.H"
#include "util/ProgressEvent.H"


class EcceDAVClient;
class DavNameSpaceAlias;
class ResourceResult;
class DavEDSI : public EDSI, public ProgressEventListener {
   public: 
     DavEDSI();
     DavEDSI(const EcceURL& url);
     DavEDSI(const DavEDSI& rhs);
     virtual ~DavEDSI();

     virtual void setURL(const EcceURL& url);

     virtual bool describeServerMetaData(vector<string>& metadata);
     virtual bool checkServer();

     // ProgressEventListener virtuals
     void progressNotice(ProgressEvent& event);

     void addAuthEventListener(AuthEventListener *l);
     void removeAuthEventListener(AuthEventListener *l);

     void addProgressEventListener(ProgressEventListener *l);
     void removeProgressEventListener(ProgressEventListener *l);

     virtual istream *getDataSet();
     virtual bool getDataSet(ostream& dest);
     virtual unsigned long getDataSetSize();  

     virtual istream* getDataSubSet(int start_position, 
                                    int length);

     virtual bool getDataSubSet(ostream& dest, 
                                int start_position, 
                                int length);

     virtual bool putDataSet(const char* putStream);
     virtual bool putDataSet(istream& putStream);
     virtual bool appendDataSet(const char* putStream, int bytesToOverwrite=0);
     virtual bool appendDataSet(istream& putStream, int bytesToOverwrite=0);

     virtual bool listCollection(vector<ResourceResult>& results);
     virtual bool listCollection(vector<ResourceMetaDataResult>& results);
     virtual bool listCollection(const vector<MetaDataRequest>& requests, 
                                 vector<ResourceMetaDataResult>& results);

     virtual bool getMetaData(const vector<MetaDataRequest>& requests, 
                              vector<MetaDataResult>& results,
                              bool getVDocMetaData = true);
     virtual bool getMetaData(const vector<MetaDataRequest>& requests, 
                              ResourceMetaDataResult& results,
                              bool getVDocMetaData = true);
     virtual bool putMetaData(const vector<MetaDataResult>& results);
     virtual bool removeMetaData(const vector<MetaDataRequest>& requests);
     virtual bool describeMetaData(vector<MetaDataResult>& metaDataTags);

     virtual bool moveResource(EcceURL& targetURL, 
         EDSIOverwrite overwrite=NO);
     virtual bool copyResource(EcceURL& targetURL, EDSIOverwrite overwrite=NO);
     virtual bool removeResource();
     virtual bool removeResources(const vector<EcceURL> urls);
     virtual bool efind(const string & key, const string& substring, 
                  const EcceURL& start, vector<EcceURL>& matches);
     virtual bool efindProp(const string& substring, const EcceURL& start, vector<EcceURL>& matches);
   /**
    *
    *  The isWritable() will return true if: 
    *    1)  The user has permission to use all "write" methods:  
    *        PUT, PROPPATCH, MKCOL, DELETE.
    *    2)   If the url is not locked. (future) 
    *
    *  This whole process hinges on the fact that the web server "owning" 
    *  all directories and files in the web space.  If someone logs into 
    *  the server and changes file permissions on the backend isWritable() 
    *  could return a "true" value even though the file permissions do not 
    *  allow writes to occur. 
    *
   **/
     virtual bool isWritable(); 
     virtual bool isLocked(string& locker);
     virtual bool exists(const bool& newUser=false);
     virtual EcceURL *makeCollection(const string& base, const string& pattern="-%d");
     virtual EcceURL *makeDataSet(const string& base);
     virtual string uniqueName(const string& base, const string& pattern="-%d");
     virtual string getClassName();
     virtual EcceURL uniqueName(const EcceURL& url, const string& pattern="-%d");

   protected:
    /**
     *  Uses a Propfind method on URL to determine if the URL is a virtual document. 
    **/
    bool   isVDoc(const vector<MetaDataResult>& properties);

    /**
     *  Protected function which attempts to return the resource Header to determine proof of existance. 
    **/
    bool   resourceExists(const bool& newUser=false);

    /**
     *  If MetaDataRequests exist this function guarentees that DAV and ecce properties
     *  also exist to help describe the returning ResourceMetaDataResutl. 
    **/
    void addVDocAttributes(vector<MetaDataRequest>&);
    /**
     *
     *  Appends the second vector to the first vector specified and returns a new vector. 
     *
    **/
    void appendMetaDataResults(vector<MetaDataResult>& results1, const vector<MetaDataResult>& results2); 

     /**
      *
      *  Uses an algorithm to extract virtual metadata throughout a virtual document structure.
      *
     **/
     bool getVirtualMetaData(const vector<MetaDataRequest>& requests, vector<MetaDataResult>& results); 

    /**
     *
     *  Creates a PROPFIND XML request string based on the vector<MetaDataRequest>.
     *
    **/
    string createMetaDataRequestString(vector<MetaDataRequest>& requests);

    /**
     *
     *  Engine used for processing Propfind method requests for a general getMetaData request.
     *
    **/
    bool   requestMetaData(const vector<MetaDataRequest> requests,
                           vector<EcceURL>& results);

    /**
     *
     *  Engine used for processing Propfind method requests for a general getMetaData request.
     *
    **/
    bool   requestMetaData(const vector<MetaDataRequest> requests, 
                           vector<MetaDataResult>& results);

    /**
     *
     *  Engine used for processing Propfind method requests for a general getMetaData request.
     *
    **/
    bool   requestMetaData(const vector<MetaDataRequest> requests, 
                           ResourceMetaDataResult& result);

    /**
     *
     *  Engine used for processing Propfind method requests for a listCollection request.
     *
    **/
    bool   requestMetaData(const vector<MetaDataRequest> requests, 
                           vector<ResourceMetaDataResult>& results);

    /**
     *
     *  Engine used for processing Propfind method requests for a listCollection request.
     *
    **/
    bool   requestMetaData(const vector<MetaDataRequest> requests, 
                           vector<ResourceResult>& results);

    /**
     *
     *  Removes any invalid vector<MetaDataResult> entries in which a MetaDataResult.name
     *
    **/
    void   removeBadMetaDataRequests(vector<MetaDataResult>& requests); 

    /**
     *
     *  Removes any invalid vector<MetaDataRequest> entries in which a MetaDataRequest.name
     *  is empty.   
     *
    **/
    void   removeBadMetaDataRequests(vector<MetaDataRequest>& requests);

    /**
     *
     *  Returns the XML version 1.0 Document String header
     *
    **/
    string davXMLDocumentString() const;

    /**
     *
     *  Returns the <D:propfind> root element string.
     *
    **/
    string davPropfindHeader() const;

   /**
    *
    *  Returns the </D:propfind> root element closure tag string.
    *
   **/ 
   string davPropfindFooter() const;

  /**
   *
   *  Returns the <D:proppatch ....> root element string.
   *
   **/
   string davProppatchHeader(const string& namespaces = "") const;

  /**
   *
   *  Returns the </D:proppatch> root element closure tag string.
   *
  **/
  string davProppatchFooter() const;

  /**
   *
   *  Returns the <D:prop ...> child element string
   *
  **/
  string davPropElementOpenTag(const string& namespaces = "" ) const;

  /**
   *  Returns the </D:prop> child element closure tag string.
   *
  **/
  string davPropElementCloseTag() const;

    /**
     *
     *  Returns the <D:allprop/>
     *
    **/
     string davPropfindAllpropNullElement() const;

    /**
     *
     *  Returns the <D:propname/> null element string
     *
    **/
    string davPropfindPropnameNullElement() const;

    /**
     *
     *  Returns the <D:propname> element string.
     *
    **/
    string davPropfindPropnameElementHeader() const;


    /**
     *
     *  Returns the </D:propname> element closure string.
     *
     *
    **/
    string davPropfindPropnameElementFooter() const;

    /**
     *
     *  Returns the <D:remove> element string
     *
    **/
    string davProppatchRemoveElementHeader() const;

    /**
     *
     * Returns the </D:remove> element closure string
     *
    **/
    string davProppatchRemoveElementFooter() const;

   /**
    *
    *  Returns the <D:set> element string
    *
   **/
   string davProppatchSetElementHeader() const;

   /**
    *
    *  Returns the <D:set> element closure string
    *
   **/
   string davProppatchSetElementFooter() const;

   /**
    *
    *  Parse XML Document formatted string into DOM object  
    *
   **/
   bool parseXmlDocString(string& response, DOMDocument*& doc);


   /**
    *
    *  Engine to convert one or more DAV XML response to MetaDataResults  
    *
   **/
   bool processDAVResponses(string& response, vector<MetaDataResult>& results);

   /**
    *
    *  Engine to convert one or more DAV XML response to MetaDataResults  
    *
   **/
   bool processDescribeResponses(string& response, vector<MetaDataResult>& results);


   /**
    *
    *  Engine to convert one DAV XML response to ResourceMetaDataResults  
    *
   **/
   bool processDAVResponses(string& response, ResourceMetaDataResult& results);
 

   /**
    *
    *  Engine to convert list Collection DAV XML response to ResourceMetaDataResults
    *
   **/
   bool processFindResponses(vector<MetaDataRequest>& requests, string& response, vector<EcceURL>& results);


   /**
    *
    *  Engine to convert list Collection DAV XML response to ResourceMetaDataResults  
    *
   **/
   bool processListCollectionResponses(vector<MetaDataRequest>& requests, string& response, vector<ResourceMetaDataResult>& results);

   /**
    *
    *  Engine to convert list Collection DAV XML response to ResourceResults  
    *
   **/
   bool processListCollectionResponses(vector<MetaDataRequest>& requests, string& response, vector<ResourceResult>& results);


   /**
    *
    *    Extract All <prop> children values from XML. 
    *
   **/
   string getPropertyValue(const DOMNode& node) const;

   /**
    *
    *  Sends DAV request METHOD and request XML string.  Returns XML response String.     
    *
   **/
   bool sendMetaDataRequest(const int davMethod, const string davRequest, string& response);

   /**
    *
    *  Returns a list of vector<MetaDataRequest> that with xmlns alias's replacing the
    *  xmlns URI and returns a vector<DavNameSpaceAlias> which is essentially a list of
    *  alias/uri pairs.      
    *
   **/
   vector<DavNameSpaceAlias> parsePropertyNames (vector<MetaDataRequest>& requests);

   /**
    *
    *  Intializes the Xerces parser.     
    *
   **/
   bool initializeXMLParser();

   /**
    *
    *   Assembles chunk of XML properties for a D:propfind request.     
    *
   **/
   string assembleMetaDataRequestString(const vector<MetaDataRequest>& requests) const;

   /**
    *
    *  Assembles a chunk of XML properties for a D:proppatch request    
    *
   **/
   string assembleMetaDataRequestString(const vector<MetaDataResult>& requests) const;

   /**
    *
    *  Replaces an element's namespace URI with a namespace alias .     
    *
   **/
   void assignElementAliasNameSpace(string& element, vector<DavNameSpaceAlias>& nameSpaces);

   /**
    *
    *  Returns a list of vector<MetaDataResult> that with xmlns alias's replacing the
    *  xmlns URI and returns a vector<DavNameSpaceAlias> which is essentially a list of
    *  alias/uri pairs.      
    *
   **/
   vector<DavNameSpaceAlias> parsePropertyNames (vector<MetaDataResult>& requests);


   /**
    *
    *    Initalizes the DavNameSpaceVector and gets the namespace 
    *    attributes provided on the <response> element. 
    *
   **/
   void getResponseNameSpaces(DOMNode& response, vector<DavNameSpaceAlias>& nameSpaces);

   /**
    *
    *    Returns <href> string value. 
    *
   **/
   string getResponseHrefString(DOMNode& response);

   /**
    *
    *    Returns value of specified property key - must include namespace
    *
   **/
   string getResponseValue(DOMNode& response, const string& key);

   /**
    *
    *    Returns true if the current URL is = to the href being processed. 
    *
   **/
   bool isHrefCurrentURL(DOMNode& response); 


   /**
    *
    *    Returns URL in the context of the <href> string value 
    *    added to the present URL used in the DAV request. 
    *
   **/
   EcceURL getResponseURL(DOMNode& response );

   /**
    *
    *    Replaces an element's namespace alias with a namespace URI. 
    *
   **/
   void assignElementNameSpace(string& element, vector<DavNameSpaceAlias>& nameSpaces);

   /**
    *
    *    Returns List of Property elements from a <propstat> element with a <status> 
    *    element value = 200 (DAV Success status value). 
    *
   **/
   bool getResponsePropertyList(DOMNode& response, DOMNodeList*& validproperties);

   /**
    *
    *  Assembles xmlns attribute string for a PROPFIND or PROPPATCH request.      
    *
   **/
   string assembleNameSpaceString(const vector<DavNameSpaceAlias>& nameSpaces) const;

   /**
    *
    *  Extract MetaDataResult from XML elements and values.      
    *
   **/
   void getMetaDataResult(DOMElement& property, vector<DavNameSpaceAlias>& nameSpaces, MetaDataResult& result);

   /**
    *
    *  Determine Dav Content Type.      
    *
   **/
   string getDavContentType(ResourceDescriptor::RESOURCETYPE value, EcceURL& url) const;

   /**
    *
    *  Determine File Content Type.      
    *
   **/
   string getFileContentType(const EcceURL& url) const;


   /**
    *
    *  Extract ResourceMetaDataResult from XML elements and values.      
    *
   **/
   void getResourceMetaDataResult(DOMElement& property, 
                                  vector<DavNameSpaceAlias>& nameSpaces, 
                                  ResourceMetaDataResult& result);


   /**
    *
    *  get the Resource Type based on the D:resourcetype and ecce:resourcetype element      
    *
   **/
   ResourceDescriptor::RESOURCETYPE getDavResourceType(string& value) const;


   /**
    * get string from XMLCh* while not leaving memory leak.
    */
   string transcode(const XMLCh* str) const;
   /**
    *
    * Adds a formatted status message to the EDSIMessage stack
    * based on the HTTP status code.
    *
   **/
   void addStatusMessage(int status);


   private: 

     // Adds alias/namespace pair with no duplicates
     void addNamespaceAlias(vector<DavNameSpaceAlias>& nameSpaces,
                            const string& alias,
                            const string& value);

     //Client communications package for Dav server.  
     //Use pointer to remove need to include header file.
     EcceDAVClient         *p_EcceDAVClient;  
     bool                   p_VdUrl;

     // Methods/data associated with maintaining progress notification
     void initProgress();
     ProgressEvent *p_progressEvent;
     ProgressEventListener *p_progressListener;

};

class DavNameSpaceAlias {
 public:
    string NameSpace;
    string Alias;

};
#endif
