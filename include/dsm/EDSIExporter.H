#ifndef EDSIEXPORTER_HH
#define EDSIEXPORTER_HH

  using std::string;
  using std::vector;

#include "util/EcceURL.H"
#include "util/IOException.H"
#include "dsm/EDSI.H"


class EDSIExporter {

  public: 

    EDSIExporter();
    EDSIExporter(const EcceURL &sourceURL, const EcceURL &targetURL);
    virtual ~EDSIExporter();

    EcceURL getSourceURL() const;
    EcceURL getTargetURL() const;
    void setSourceURL(const EcceURL& sourceURL);
    void setTargetURL(const EcceURL& targetURL);
    

    EcceURL xport() throw (IOException);
    void logExport();

  private:

    bool exportResource(EDSI &source, EDSI &target, bool checkName = false) 
       throw(IOException);
    bool exportCollection(EDSI &source, EDSI &target) throw(IOException);
    void exportMetaData(EDSI &source, EDSI &target) throw(IOException);
    bool isResource(EDSI &edsi);
    bool isCollection(EDSI &edsi);
    bool isDAVExport(EDSI &source, EDSI &target);
    void removeLiveProperties(vector<MetaDataResult>& source);
    void initializeExport();

    EcceURL                 p_sourceURL;
    EcceURL                 p_targetURL;
    int                     p_collectionCount;
    int                     p_resourceCount;
    int                     p_propertyCount;
    int                     p_exportSize;
    unsigned long           p_startTime;
    unsigned long           p_endTime;

  protected:

};

#endif

