/**
 * @file
 *
 *
 */
#ifndef RESOURCEDESCRIPTOR_HH
#define RESOURCEDESCRIPTOR_HH

#include <iostream>
using std::cout;
using std::endl;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <map>
using std::map;
using std::multimap;

#include <utility>
using std::pair;

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
using namespace xercesc;

#include "util/ParseException.H"


class TNode;
class ResourceType;
class ResourceTool;
class ResourceRmTask;
class PropertyView;
class ToolPropertyView;
class AtomView;


/**
 * ResourceTool names.
 * These names were moved from util/EcceAppStartInfo.H so that we can get rid
 * of the EcceAppStartInfo class. They correspond to the ResourceTool names
 * defined in the registry xml file.
 */
const string BASISTOOL = "Basistool";
const string BUILDER = "Builder";
const string CALCED = "CalculationEditor";
const string CALCVIEWER = "CalculationViewer";
const string GATEWAY = "Gateway";
const string LAUNCHER = "Launcher";
const string MACHBROWSER = "MachineBrowser";
const string MACHREGISTER = "MachineRegister";
const string ORGANIZER = "Organizer";
const string PERTAB = "PeriodicTable";
const string MDPREPARE = "MDPrepare";
const string MDOPTIMIZE = "MDOptimize";
const string MDENERGY = "MDEnergy";
const string MDDYNAMICS = "MDDynamics";
const string NWDIRDY = "NWDirdy";
const string POLYRATE = "Polyrate";
const string SOLVATE = "SolvationEditor";
const string METADYNAMICS = "MetadynamicsEditor";
const string VIZTHUMBNAIL = "VizThumbnail";
const string PARTIALCHARGE = "PartialCharge";
const string GEOMCONSTRAINTS = "GeomConstraints";
const string ATOMTYPES = "AtomTypes";

/**
 * RunManagementTask names.
 */
const string TAILOUTPUTFILE = "tailOutputFile";
const string SHELLINRUNDIR = "shellInRunDir";
const string MANAGEFILES = "manageFiles";
const string TERMINATECALC = "terminateCalc";
const string CHECKJOB = "checkJob";
const string RECONNECTJOB = "reconnectJob";
const string FORCEFAIL = "forceFail";
const string RESETFORRERUN = "resetForRerun";
const string RESETFORRESTART = "resetForRestart";
const string RESETCOMPLETED = "resetCompleted";
const string RESETINCOMPLETE = "resetIncomplete";
const string RESETIMPORTED = "resetImported";
const string RESETKILLED = "resetKilled";
const string MARKREVIEWED = "markReviewed";
const string MARKUNREVIEWED = "markUnreviewed";
const string CLEANRUNDIR = "cleanRunDir";
const string VIEWRUNLOG = "viewRunLog";
const string VIEWOUTPUTFILE = "viewOutputFile";
const string VIEWINPUTFILE = "viewInputFile";

const string DEFAULT_UNIT_FAMILY = "SI";

/**
 * Interface to Resource Descriptor data.  ResourceDescriptor is 
 * a singleton, and provides access to resource descriptor data
 * stored in a structured xml file.  Resources, tools (i.e. apps),
 * run management tasks, MD units, and property views are all 
 * described in the resource descriptor file.
 */
class ResourceDescriptor 
{
  public:

    static ResourceDescriptor& getResourceDescriptor();

    virtual ~ResourceDescriptor();

    enum RESOURCETYPE 
    {
      RT_DOCUMENT,      
      RT_COLLECTION,
      RT_VIRTUAL_DOCUMENT,
      RT_UNDEFINED,
      NUMBER_OF_RT
    };

    enum CONTENTTYPE
    {
      CT_PROJECT,     
      CT_SESSION,
      CT_CALCULATION,
      CT_MDPREPARE,
      CT_MDOPTIMIZE,
      CT_MDENERGY,
      CT_MDDYNAMICS,
      CT_CHEMICAL_MVM,
      CT_CHEMICAL_PDB,
      CT_UNDEFINED,
      NUMBER_OF_CT
    };

    enum APPLICATIONTYPE
    {
      AT_MDSTUDY,
      AT_NWCHEMMD,
      AT_REACTION_STUDY,
      AT_CONDENSED_REACTION_STUDY,
      AT_DRDVTST,
      AT_POLYRATE,
      AT_SOLVATE,
      AT_METADYNAMICS,
      AT_UNDEFINED,
      NUMBER_OF_AT
    };

    enum RUNSTATE
    {
      STATE_ILLEGAL,
      STATE_CREATED,
      STATE_READY,
      STATE_SUBMITTED,
      STATE_RUNNING,
      STATE_COMPLETED,
      STATE_LOADED,    
      STATE_KILLED,
      STATE_UNSUCCESSFUL,
      STATE_FAILED,
      STATE_SYSTEM_FAILURE,
      NUMBER_OF_STATES
    };

    enum IconType
    {
      IT_NORMAL,
      IT_SELECTED,
      IT_EXPANDED,
      IT_SELECTED_EXPANDED,
      IT_UNDEFINED
    };

    enum ResourceCategory
    {
      RC_DOCUMENT,
      RC_PROJECT,
      RC_ES_CALCULATION,
      RC_MD_SESSION,
      RC_REACTION_SESSION,
      RC_CONDENSED_REACTION_SESSION,
      RC_MD_TASK,
      RC_REACTION_TASK,
      RC_CONDENSED_REACTION_TASK,
      RC_UNDEFINED
    };


    bool isDocument(const string& resourceType);
    bool isCollection(const string& resourceType);
    bool isVirtualDocument(const string& resourceType);

    bool isEcceProject(const string& contentType);
    bool isEcceSession(const string& contentType);
    bool isEcceCalculation(const string& contentType);
    bool isEcceMdPrepare(const string& contentType);
    bool isEcceMdOptimize(const string& contentType);
    bool isEcceMdEnergy(const string& contentType);
    bool isEcceMdDynamics(const string& contentType);

    string getRtName(const RESOURCETYPE& rt);
    string getCtName(const CONTENTTYPE& ct);
    string getAtName(const APPLICATIONTYPE& at);

    vector<ResourceType*> getResourceTypes();
    ResourceType *getResourceType(TNode *tnode);
    ResourceType *getResourceType(ResourceDescriptor::RESOURCETYPE resourceRt,
                                  ResourceDescriptor::CONTENTTYPE resourceCt,
                                  ResourceDescriptor::APPLICATIONTYPE resourceAt);
    ResourceType *getResourceType(string resourceRt,
                                  string resourceCt,
                                  string resourceAt);
    ResourceType *getResourceType(int id);

    vector<ResourceTool*> getTools();
    ResourceTool *getTool(const string& toolName);
    ResourceTool *getTool(int id);
    ResourceTool *getGatewayTool(int pos);

    vector<ResourceRmTask*> getRmTasks();
    ResourceRmTask *getRmTask(const string& rmTaskName);
    ResourceRmTask *getRmTask(int id);

    multimap<int, PropertyView*> getPropertyViews();
    PropertyView *getPropertyView(const string& pvName);
    PropertyView *getPropertyView(int id);

    multimap<int, ToolPropertyView*> getToolPropertyViews();
    ToolPropertyView *getToolPropertyView(const string& tool, 
                                          const string& contentType);
    multimap<int, pair<ResourceTool*,ToolPropertyView*> > 
        getToolPropertyViews(ResourceType*);

    multimap<int, AtomView*> getAtomViews();
    AtomView *getAtomView(const string& avName);
    AtomView *getAtomView(int id);

    multimap<int, AtomView*> getResidueViews();
    AtomView *getResidueView(const string& rvName);
    AtomView *getResidueView(int id);

    vector<string> getMdUnits();

    void getResourceIdRange(int & low, int & high);
    void getToolIdRange(int & low, int & high);
    void getRmTaskIdRange(int & low, int & high);
    void getPropertyViewIdRange(int & low, int & high);
    void getAtomViewIdRange(int & low, int & high);
    void getResidueViewIdRange(int & low, int & high);

  protected:

    ResourceDescriptor();


  private:

    void parseResourceDescriptorFile() throw (ParseException);
  
    void createResourceTypes();
    void loadResourceTypes();
    void loadResourceType(ResourceType *resource);
    ResourceType *getResourceType(const string& resourceName);
    DOMElement *getResourceElement(const string& resourceName);
    int getResourcePriority(DOMElement *resourceElement);
    map<ResourceDescriptor::IconType, string> getResourceIcons(DOMElement *resourceElement);
    string getResourceLabel(DOMElement *resourceElement);
    string getResourceNameRule(DOMElement *resourceElement);
    bool getResourceRootTask(DOMElement *resourceElement);
    bool getResourceStateBased(DOMElement *resourceElement);
    string getResourceMenuItem(DOMElement *resourceElement);
    string getResourceDescriptorFile(DOMElement *resourceElement);
    string getResourceFactoryCategory(DOMElement *resourceElement);
    bool getResourceIsExpandable(DOMElement *resourceElement);
    string getResourceContextPanel(DOMElement *resourceElement);
    bool getResourceIsSessionBased(DOMElement *resourceElement);
    vector<ResourceType*> getResourceContains(DOMElement *resourceElement);
    vector<ResourceType*> getResourceContainedBy(DOMElement *resourceElement);
    map<ResourceDescriptor::RUNSTATE, ResourceTool*> 
      getResourceStateDefaultTools(DOMElement *resourceElement);
    ResourceTool* getResourceDefaultTool(DOMElement *resourceElement);
    multimap<ResourceDescriptor::RUNSTATE, ResourceTool*> 
      getResourceStateEnabledTools(DOMElement *resourceElement);
    vector<ResourceTool*> getResourceEnabledTools(DOMElement *resourceElement);
    multimap<ResourceDescriptor::RUNSTATE, ResourceRmTask*> 
      getResourceStateEnabledRmTasks(DOMElement *resourceElement);
    vector<ResourceRmTask*> getResourceEnabledRmTasks(DOMElement *resourceElement);

    string getStateName(DOMElement *stateElement);

    void createTools();

    void createRmTasks();

    void createPropertyViews();

    void createToolPropertyViews();

    void createAtomViews();

    void createResidueViews();

    void loadMdUnits();


    static ResourceDescriptor 
      *p_singleton;     /**< Single resource descriptor instance */

    // ID Range 20000 - 21000 reserved
    static const int ID_RESOURCE_BASE = 20000;
    static const int ID_RMTASK_BASE = 20200;
    static const int ID_TOOL_BASE = 20400;
    static const int ID_PROPERTY_VIEW_BASE = 20600;
    static const int ID_ATOM_VIEW_BASE = 20800;
    static const int ID_RESIDUE_VIEW_BASE = 20900;

    DOMDocument *p_doc; /**< Parsed resource descriptor file */

    vector<ResourceType*> 
      p_resources;      /**< Resources contained in the resource
                             descriptor file */
    vector<ResourceTool*> 
      p_tools;          /**< Tools contained in the resource descriptor file */

    vector<ResourceRmTask*> 
      p_rmTasks;        /**< Run management tasks contained in the
                             resource descriptor file */
    multimap<int, PropertyView*>
      p_propertyViews;  /**< Property Views contained in the
                             resource descriptor file.  Integer
                             key represents the property view
                             order. */
    multimap<int, ToolPropertyView*>
      p_toolPropertyViews;  /**< Property Views contained in the
                             resource descriptor file.  Integer
                             key represents the property view
                             order. */
    multimap<int, AtomView*>
      p_atomViews;  /**< Atom Views contained in the
                             resource descriptor file.  Integer
                             key represents the atom view
                             order. */

    multimap<int, AtomView*>
      p_residueViews;  /**< Residue Views contained in the
                             resource descriptor file.  Integer
                             key represents the residue view
                             order. */
    vector<string> 
      p_mdUnits;        /**< Molecular dynamics units */

};

#endif 
