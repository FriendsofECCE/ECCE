/**
* @file
*
*
*/
#ifndef _VDOC_HH
#define _VDOC_HH

#include <string>
#include <vector>
  using std::string;
  using std::vector;
  using std::find;

#include "util/EcceURL.H"
#include "util/DavException.H"
#include "util/ParseException.H"
#include "util/InvalidException.H"
#include "dsm/JCode.H"
#include "dsm/EDSI.H"


/**
 * Provides methods for the storage, identification, removal, and
 * documentation of a calculations resources within the framework
 * of an Ecce Virtual Document (VDoc). 
 *
 * The VDoc manages configuration, input, output, property,  
 * and misc. file resources of a calculation.  The configuration, input,
 * output, and property resources are documented within DAV 
 * properties of the base URL.  Each property contains an XML
 * document that provides a listing of the resources.
 
 * The following properties are used to document a VDoc's 
 * configuration, input, output, and property resources:
 *
 *   :hasConfigurations
 *   :hasInputs
 *   :hasOutputs
 *   :hasProperties
 *
 * Calculations that were created prior to this VDoc implementation
 * will retain their original structure (i.e. not converted over 
 * to the new format/access mechanisms) in order to maintain 
 * backwards compatability with previous versions of Ecce. 
 */
class VDoc {

  public:

    // Only EDSIFactory can create a VDoc
    friend class EDSIFactory;

    ~VDoc();

    static string getEcceNamespace(void);
    static string getCmcsNamespace(void);
    static string getDcNamespace(void);
    static string getDcQualifierNamespace(void);

    void setURL(const EcceURL& url);
    EcceURL getBaseURL(void) const;

    string getMoleculeName() const;
    string getOriginalMoleculeName() const;
    string getInputMoleculeName() const;
    string getOutputMoleculeName() const;
    string getEspConstraintsName() const;
    string getGeometryConstraintsName() const;
    string getMetaPotentialsName() const;
    string getQMMMTermsName() const;
    string getBasisSetName() const;
    string getGeneralBasisSetName() const;
    string getSinglePointBasisSetName() const;
    string getRunLogName() const;
    string getSetupParametersName() const;
    string getGeneralSetupParametersName() const;
    string getSinglePointSetupParametersName() const;
    string getMdPrepareName() const;
    string getMdOptimizeName() const;
    string getMdEnergyName() const;
    string getMdDynamicsName() const;
    string getThumbnailName() const;
    string getNwDirdyName() const;
    string getPolyrateName() const;

    EcceURL         getConfiguration(const string& name);
    vector<EcceURL> getConfigurations();
    EcceURL         getInput(const string& name);
    vector<EcceURL> getInputs();
    EcceURL         getOutput(const string& name);
    vector<EcceURL> getOutputs();
    EcceURL         getFile(const string& name);
    vector<EcceURL> getFiles();
    EcceURL         getProperty(const string& name);
    vector<EcceURL> getProperties();

    EcceURL addConfiguration(
              const string& name, 
              istream* resource) 
;
    EcceURL addInput(
              const string& name, 
              istream* resource) 
;
    EcceURL addOutput(
              const string& name, 
              istream* resource) 
;
    EcceURL addFile(
              const string& name, 
              istream* resource) 
;
    EcceURL addProperty(
              const string& name, 
              istream* resource) 
;

    EcceURL moveConfiguration(
              const string& fromName, 
              const string& toName) 
;
    EcceURL moveInput(
              const string& fromName, 
              const string& toName) 
;
    EcceURL moveOutput(
            const string& fromName, 
            const string& toName) 
;
    EcceURL moveFile(
            const string& fromName, 
            const string& toName) 
;
    EcceURL moveProperty(
            const string& fromName, 
            const string& toName) 
;

    void removeConfiguration(const string& name);
    void removeConfigurations();
    void removeInput(const string& name);
    void removeInputs();
    void removeOutput(const string& name);
    void removeOutputs();
    void removeFile(const string& name);
    void removeFiles();
    void removeProperty(const string& name);
    void removeProperties();



  protected:

    // Only EDSIFactory can create a VDoc
    VDoc(const EcceURL& url, 
         bool createVdoc = false) 
;

  private:

    bool isVdoc();
    bool isCurrentVdoc();
    void setVdoc(bool createVdoc) 
;

    void loadMoleculeName();
    void loadOriginalMoleculeName();
    void loadEspConstraintsName();
    void loadGeometryConstraintsName();
    void loadMetaPotentialsName();
    void loadQMMMTermsName();
    void loadBasisSetName();
    void loadRunLogName();
    void loadSetupParametersName();
    void loadMdPrepareName();
    void loadMdOptimizeName();
    void loadMdEnergyName();
    void loadMdDynamicsName();
    void loadThumbnailName();
    void loadConfigurationCollection();
    void loadInputCollection();
    void loadOutputCollection();
    void loadFileCollection();
    void loadPropertyCollection();

    void createMoleculeName();
    void createOriginalMoleculeName();
    void createEspConstraintsName();
    void createGeometryConstraintsName();
    void createMetaPotentialsName();
    void createQMMMTermsName();
    void createBasisSetName();
    void createRunLogName();
    void createSetupParametersName();
    void createMdPrepareName();
    void createMdOptimizeName();
    void createMdEnergyName();
    void createMdDynamicsName();
    void createThumbnailName();
    void createConfigurationCollection();
    void createInputCollection();
    void createOutputCollection();
    void createPropertyCollection();

    vector<string> parseDoc(const string& document);
    vector<string> getDocItems(DOMDocument *domDocument);

    const JCode *getApplication();
    bool isInputFilename(const string& name);
    bool isInputType(const EcceURL& url);
    bool isOutputFilename(const string& name);
    bool isOutputType(const EcceURL& url);

    void saveConfigurations(const vector<EcceURL>& urls);
    void saveInputs(const vector<EcceURL>& urls);
    void saveOutputs(const vector<EcceURL>& urls);
    void saveProperties();
    void startDoc(string& document);
    void addDocItem(string& document, const string& path);
    void endDoc(string& document);

    bool resourceExists(const EcceURL& url);
    bool propertyExists(const EcceURL& url, const string& name) 
;
    string getVdocPath(
             const string& collection, 
             const string& resource = "") const;
    void trimSlash(string& value) const;
    EcceURL getVdocUrl(const string& path) const;
    vector<EcceURL> getVdocUrls(const vector<string>& paths) const;
    void createVdocCollection(const string& collectionName);

    EcceURL* findUrl(const vector<EcceURL>& urls, const string& name) const;
    EcceURL createConfigurationUrl(const string& name) const;
    EcceURL createInputUrl(const string& name) const;
    EcceURL createOutputUrl(const string& name) const;
    EcceURL createFileUrl(const string& name) const;
    EcceURL createPropertyUrl(const string& name) const;


    EcceURL p_base;                    /**< Virtual document's base URL */
    bool    p_currentVdoc;              /**< Indicates if the Vdoc is >= v4.0 */
    EDSI    *p_edsi;                   /**< DAV Server connection */

    string p_molecule;                 /**< Name of molecule resource */
    string p_originalMolecule;         /**< Name of original molecule resource */
    string p_inputMolecule;            /**< Name of input molecule resource */
    string p_outputMolecule;           /**< Name of output molecule resource */
    string p_espConstraints;           /**< Name of ESP Constraints resource */
    string p_geometryConstraints;      /**< Name of Geometry Constraints resource */
    string p_metaPotentials;           /**< Name of Meta-Potentials resource */
    string p_QMMMTerms;                /**< Name of QMMM terms resource */
    string p_basisSet;                 /**< Name of Gaussian Basis Set resource */
    string p_generalBasisSet;          /**< Name of DirDyVTST's general basis set resource */
    string p_singlePointBasisSet;      /**< Name of DirDyVTST's single point basis set resource */
    string p_runLog;                   /**< Name of Ecce's run log resource */
    string p_setupParameters;          /**< Name of Ecce's Setup Parameters resource */
    string p_generalSetupParameters;   /**< Name of DirDyVTST's general setup parameters resource */
    string p_singlePointSetupParameters;  /**< Name of DirDyVTST's single point setup parameters resource */
    string p_mdPrepare;                /**< Name of MD Prepare resource */
    string p_mdOptimize;               /**< Name of MD Optimize resource */
    string p_mdEnergy;                 /**< Name of MD Energy resource */
    string p_mdDynamics;               /**< Name of MD Dynamics resource */
    string p_thumbnail;                /**< Name of thumbnail resource */
    string p_nwDirdy;                  /**< Name of NWDirdy resource */
    string p_polyrate;                 /**< Name of polyrate resource */

    string p_configurationCollection;  /**< Configuration collection path, 
                                            relative to the VDoc's base URL */
    string p_inputCollection;          /**< Input collection path, relative 
                                            to the VDoc's base URL */
    string p_outputCollection;         /**< Output collection path, relative 
                                            to the VDoc's base URL */
    string p_fileCollection;           /**< File collection path, relative 
                                            to the VDoc's base URL */
    string p_propertyCollection;       /**< Property collection path, relative 
                                            to the VDoc's base URL */

    string p_moleculeProp;             /**< Name of dav property which documents 
                                            the molecule resource name */
    string p_originalMoleculeProp;     /**< Name of dav property which documents 
                                            the original molecule resource name */
    string p_espConstraintsProp;       /**< Name of dav property which documents 
                                            the ESP Constraints resource name */
    string p_geometryConstraintsProp;  /**< Name of dav property which documents 
                                            the Geometry Constraints 
                                            resource name */
    string p_metaPotentialsProp;       /**< Name of dav property which documents 
                                            the Meta-Potentials resource name */
    string p_QMMMTermsProp;            /**< Name of dav property which documents 
                                            the QMMM terms resource name */
    string p_basisSetProp;             /**< Name of dav property which documents 
                                            the Basis Set resource name */
    string p_runLogProp;               /**< Name of dav property which documents 
                                            the Run Log resource name */
    string p_setupParametersProp;      /**< Name of dav property which documents 
                                            the setup parameters resource name */
    string p_mdPrepareProp;            /**< Name of dav property which doucuments
                                            the MD Prepare resource name */
    string p_mdOptimizeProp;           /**< Name of dav property which doucuments
                                            the MD Optimize resource name */
    string p_mdEnergyProp;             /**< Name of dav property which doucuments
                                            the MD Energy resource name */
    string p_mdDynamicsProp;           /**< Name of dav property which doucuments
                                            the MD Dynamics resource name */
    string p_thumbnailProp;            /**< Name of dav property which documents
                                            the thumbnail resource name */
    string p_configurationCollectionProp;   /**< Name of dav property which 
                                                 documents the configuration 
                                                 collection */
    string p_inputCollectionProp;      /**< Name of dav property which documents 
                                            the input collection */
    string p_outputCollectionProp;     /**< Name of dav property which documents 
                                            the output collection */
    string p_fileCollectionProp;       /**< Name of dav property which documents 
                                            the file collection */
    string p_propertyCollectionProp;   /**< Name of dav property which documents 
                                            the property collection */
    string p_configurationsProp;       /**< Name of dav property which documents 
                                            the configuration resources */
    string p_inputsProp;               /**< Name of dav property which documents 
                                            the input resources */
    string p_outputsProp;              /**< Name of dav property which documents 
                                            the output resources */
    string p_propertiesProp;           /**< Name of dav property which documents 
                                            the property resources */
};

#endif
