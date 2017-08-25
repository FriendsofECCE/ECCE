///////////////////////////////////////////////////////////////////////////////
//
// Synopsis: EcceURL encapsulates url components and parsing of string urls.
//   
// Description:
//   See http://www.w3.org/Addressing/URL
//   See http://www.w3.org/Addressing/URL/4_Recommentations.html
//
//   A general synopsis is:
//      A complete URL consists of a naming scheme specifier followed by a
//      string whose format is a function of the naming scheme.
//
//     Scheme:
//       Within the URL of a object, the first element is the name of the 
//       scheme,  separated from the rest of the object by a colon. The 
//       rest of the URL follows the colon in a format depending on the scheme. 
//
//     An optional user name
//       we are ignoring this....
//
//     The internet domain name
//       of the host in RFC1037 format (or, optionally and less advisably, the
//       IP address as a set of four decimal digits)
//
//     The port number
//       if it is no thte default number fo rthe protocol, is gieven in
//       decimal notation after a colon
//
//     Path
//        The rest of the locator is known as the path....
//        Paths may contain parameters (GET/POST) of the form
//        .../script?param1=value1&param2=value2...
//
///////////////////////////////////////////////////////////////////////////////
#ifndef ECCEURL_HH
#define ECCEURL_HH

#include <iostream>
  using std::ostream;
#include <string>
  using std::string;
#include <vector>
  using std::vector;

class EcceMap;


class EcceURL  {

  public:
    EcceURL(const char* url);
    EcceURL(const string& url);
    EcceURL();
    EcceURL(const char* protocol, const char *host, int port, const char *file);
    EcceURL(const EcceURL& rhs);
    virtual ~EcceURL();

    bool operator==(const EcceURL& rhs)  const;
    bool operator!=(const EcceURL& rhs)  const;
    EcceURL& operator=(const EcceURL& rhs);
    EcceURL& operator=(const char *);
    EcceURL& operator=(const string& );
    operator string() const;

    // getPath and getFile are equivelent.  use getPath
    string getPath() const;
    string getPathWithParams() const;
    string getFile() const;  // to be depricated

    string getFilePathRoot() const;
    string getFilePathTail() const;
    string getHost() const;
    int getPort() const;
    string getRef() const;   // anchor or "reference"
    string getProtocol() const;
    string toString(bool includeParams=true) const;
    string toDirString() const;

    // For POST style urls
    EcceMap *getParams() const;
    void addParam(const string& key, const string& value);
    void removeParam(const string& key);
    void clearParams();
    string paramsAsString() const;

    void set(const char* protocol,const char *host,int port,const char *file);

       // Generates child URL by appending to file path
    EcceURL getChild(const string & child) const;

       // Generates parent URL by removing last part of path if any.
    EcceURL getParent() const;
  
    bool isChildOf(const EcceURL & parentUrl) const;
    bool isChildOrMe(const string & parentUrlStr) const;

    bool isLocal() const;
    bool isSystemFolder() const;
    bool isEcceRoot() const;
    bool isEcceUsers() const;
    bool isEcceUserHome() const;
    string getEcceUserName() const;
    string getEcceRoot() const;

    static string toString(const vector<EcceURL> & urls);
    static vector<EcceURL> toVector(const string & str);
    const static string URL_DELIMITER;

    static bool isSameDomain(const EcceURL & url1, const EcceURL & url2);

    friend ostream& operator <<(ostream& os, const EcceURL& url);

 protected:
    void parse(const char *url);

    string   p_file;
    string   p_host;
    int      p_port;
    string   p_protocol;
    EcceMap *p_params;   // ?key=value&key=value... for POST
};

#endif
