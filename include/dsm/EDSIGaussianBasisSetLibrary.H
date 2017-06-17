///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: EDSIGaussianBasisSetLibrary.H
//
//
// CLASS SYNOPSIS:
//    This class looks up basis sets and aggregate basis sets from the 
//    EMSL library files stored on the DAV server.
//
// SEE ALSO:
//    TGBSConfig.H, TGBSGroup.H, TGaussianBasisSet.H
//    
///////////////////////////////////////////////////////////////////////////////

#ifndef EDSIGAUSSIANBASISSETLIBRARY_H
#define EDSIGAUSSIANBASISSETLIBRARY_H

#include <istream>
    using std::istream;

// application includes:
   #include "util/EcceURL.H"

   #include "dsm/TGaussianBasisSet.H"
   #include "dsm/TGBSGroup.H"

class TGBSConfig;
class EDSI;
class MetaDataRequest;
class MetaDataResult;

// This class relates the name of a basis set to the files where its data
// is stored and the elements it supports
class gbs_alias {
  public:
    gbs_alias();
    gbs_alias(gbs_alias& alias);
    virtual ~gbs_alias();

    // basis set name (all lower case)
       char* name;       

    // nicely formatted name
       char* nicename;
      
    // one file name for each GBS in the group
    // plus one placeholder agg file if the gbs is aggregate
       vector<char*> files;       
       
    // a mapping of supported atoms for each basis set in the group
       vector< vector<char*> > atoms;
};

struct gbs_details {
  string reference;
  string info;
  string image_path;
};

typedef  map<TGaussianBasisSet::GBSType, vector<gbs_alias*>*, gtgbs>
           GBSMetaData;
         // note gtgbs is defined in TGaussianBasisSet.H
         //...it is used to compare two GBSType values

class EDSIGaussianBasisSetLibrary {

  public:

  // Constructors:
     // note the URL you pass in is to the default basis set library 
     // (you get this from EDSIServerCentral)
     EDSIGaussianBasisSetLibrary(const EcceURL& url);
     EDSIGaussianBasisSetLibrary(EDSIGaussianBasisSetLibrary& gbsl);

  // Destructor:
     virtual ~EDSIGaussianBasisSetLibrary(void);

  // Accessors:
     static bool           isOrbital(TGaussianBasisSet::GBSType type);

     bool checkStatus();  // indicates if library is up and accessible

     bool isSupported(const string& bsName, TGaussianBasisSet::GBSType type,
              const string& tag);

     string getNiceName(const string& bsName);

     vector<const char*>*  gbsNameList( TGaussianBasisSet::GBSType gbs_type,
                                        vector<const char*>* tags=0);

     vector<TGaussianBasisSet*> lookup( const char* gbs_name, 
     TGaussianBasisSet::GBSType gbs_type=TGaussianBasisSet::UnknownGBSType,
					const char* tag=0 );

     TGBSConfig*            simpleLookup( const char* gbs_name,
					  const char* tag );
  // Accessors for gbs meta data
     vector<const char*>*        atoms( const char* gbs_name,
                                        TGaussianBasisSet::GBSType gbs_type);

     gbs_details*              details( const char* gbs_name,
                                        TGaussianBasisSet::GBSType gbs_type);

     string                   comments( const char* gbs_name,
                                        TGaussianBasisSet::GBSType gbs_type);

  private:

  // parsing routines
     void                getBigProperty(EDSI* edsi, 
                                        vector<MetaDataRequest>& requests,
                                        vector<MetaDataResult>& results);

     void                setType( const char* s_type,
                                  TGaussianBasisSet::GBSType& type,
                                  TGaussianBasisSet::GBSCategory& cat);
  
     void                setAttributes( TGaussianBasisSet* gbs,
                                        const char* name,
                                        const char* type,
                                        const char* spherical=0,
                                        const char* contraction_type=0);

     void                parseGbsData( TGaussianBasisSet* gbs,
                                       istream *is, const char* tag=0 );
  
     void                parseEcpData( TGaussianBasisSet* ecp,
                                       istream *is, const char* tag=0 );

     const vector<gbs_alias*>* getAliasList( TGaussianBasisSet::GBSType type );

     const gbs_alias*    getGbsAlias( const char* gbs_name,
                                      TGaussianBasisSet::GBSType& type );
   
  // data
     GBSMetaData p_gbsMetaData;  // caches lookup params for each retrieved gbs
                                 
     static const char * p_ns;   // DAV namespace
     EcceURL p_gbsURLBase;       // URL to gbs directory
};
#endif
