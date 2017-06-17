#ifndef _ECCEDAVCLIENT
#define _ECCEDAVCLIENT

#include <iostream>
  using std::ostream;
  using std::istream;

#include <string>
  using std::string;

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "dsm/CDAVHeader.H"
#include "dsm/CDAVRequest.H"
#include "dsm/CDAVResponse.H"
#include "dsm/CDAVServerInfo.H"
#include "dsm/CHTTPConnection.H"
#include "dsm/CHTTPUtils.H"

#include "util/EcceURL.H"


class ProgressEventListener;
class ProgressEvent;
class AuthEventListener;
class AuthEvent;

class EcceDAVClient 
{
  public:

    enum AuthStatus { 
      AUTHRETRY,              // new auth provided - try again
      AUTHCANCEL,             // listener says cancel
      AUTHOK,                 // no new authentication required
      AUTHMAX,                // listener says we've tried enough
      AUTHERROR               // authorization error - auth type not
                              // supported or ...
    };

    EcceDAVClient();
    EcceDAVClient(const EcceURL url);
    ~EcceDAVClient();

    // Manage single listerner for authentication prompting
    void addListener(AuthEventListener *l);
    void removeListener(AuthEventListener *l);

    void addProgressEventListener(ProgressEventListener *l);
    void removeProgressEventListener(ProgressEventListener *l);

    // Managemennt of current URL which DAV commands operate on
    EcceURL url() const; 
    void url(const string& url);
    void url(const EcceURL& url);

    // DAV commands that operate on current url
    int options();
    int trace();
    int head(const bool& newUser=false);
    int get_document();
    int get_document(ostream& dest);
    int get_document_subset(int start, int length);
    int get_document_subset(int start, int length, ostream& dest);
    int create_collection();
    int doPost(ostream& dest);
    int delete_document(void);
    int put_document(const char* putStream);
    int put_document(istream *putStream);
    int append_document(const char* putStream, int bytesToOverwrite=0);
    int append_document(istream *putStream, int bytesToOverwrite=0);
    int get_properties(const string& request);
    int put_properties(const string& request);
    int copy(const EcceURL& TargetURL);
    int move(const EcceURL& TargetURL);

    int lock(const string& request);
    int unlock(const string& LockToken);

    void reset_results() {p_results = "";}
    void setDepthHeader(const string&  depth);
    void setGetContentRangeHeader(CHTTPHeader&  header, int start, int length);
    void setPutContentRangeHeader(CHTTPHeader&  header, int bytesToOverwrite=0);
    string getDepthHeader() const;


    // Accessors for data from last dav command
    string getResultsText();
    string getHeaderText();
    string getStatusText();

  protected: 
    AuthStatus checkAuth(const CDAVResponse&, const CDAVHeader&, const bool& newUser=false);
    bool namePasswordCallback(string&,string&, const string& realm, const bool& newUser=false);
    void notifyAuthAccepted();
    AuthEventListener *p_authListener;

    ProgressEventListener *p_progressListener;

    // We have to keep one of these so we can notify the listener when an
    // authorization really worked.
    AuthEvent *p_lastAuthEvent;

    int execute(int method,istream *putdata=0, ostream *resultdata=0, bool content_range=false, int start=0, int length=0, int bytesToOverwrite=0, const bool& newUser=false);

    int  processStatus(const string& msg);
    bool canHaveRequestBody(int);
    void createRequest( int, CDAVRequest&, CDAVHeader&,
        const string& uri, istream *putbody =0);
    int  getStreamSize(istream *data);
    int  getContentLength(const CHTTPHeader&);

    bool getResponse(CDAVResponse&, CDAVHeader&, int sec=0, int microsec=0);
    bool getBody(CHTTPConnection&, int, const CDAVResponse&, const
        CDAVHeader&,ostream *dest);
    bool sendRequest(const CDAVRequest& request, const CDAVHeader& header);
    bool putBody(CHTTPConnection&, int, const CDAVHeader&, istream*);

    void notifyProgress();

    string p_results; //TODO get rid of this.
    string p_header; 
    string p_status;
    string p_length; 

    EcceURL p_url;
    static string p_gAuthStr;
    string p_depth;
    CHTTPConnection p_client;
    ProgressEvent *p_progressEvent;


  private:
    friend int getBodyCallback(http_body_tx_state, void *, unsigned, void *);

};

 int  getBodyCallback(http_body_tx_state, void *, unsigned,  void * = 0);
#endif
