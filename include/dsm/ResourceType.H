/**
* @file
*
*
*/
#ifndef RESOURCETYPE_HH
#define RESOURCETYPE_HH

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <map>
  using std::map;
  using std::multimap;


#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceTool.H"
#include "dsm/ResourceRmTask.H"
#include "dsm/ResourceProperty.H"


/**
 * Represents a type of resource managed by Ecce.  These 
 * resources are initialized with data from the resource
 * descriptor file.  Only ResourceDescriptor can create
 * a Resource object.
 */
class ResourceType {

  public:
    enum {
      LOWEST_PRIORITY = 10000
    };

    friend class ResourceDescriptor;

    virtual ~ResourceType();

    bool operator==(const ResourceType& rhs) const;

    string getName() const;
    int getId();
    int getSortPriority();
    string getResourceType() const;
    string getContentType() const;
    string getApplicationType() const;
    string getIcon(ResourceDescriptor::IconType iconType = 
                   ResourceDescriptor::IT_NORMAL);
    map<ResourceDescriptor::IconType, string> getIcons();
    string getLabel();
    string getNameRule();
    bool getRootTask();
    bool getStateBased();
    string getMenuItem();
    string getDescriptorFile();
    string getFactoryCategory();
    bool getIsExpandable();
    string getContextPanel();
    bool getIsSessionBased();
    vector<ResourceType*> getContains();
    vector<ResourceType*> getContainedBy();
    ResourceTool* getDefaultTool();
    ResourceTool* getDefaultTool(ResourceDescriptor::RUNSTATE runstate);
    vector<ResourceTool*> getTools();
    vector<ResourceTool*> getTools(ResourceDescriptor::RUNSTATE runstate);
    vector<ResourceRmTask*> getRmTasks();
    vector<ResourceRmTask*> getRmTasks(ResourceDescriptor::RUNSTATE runstate);

    void setName(const string& name);
    void setId(int id);
    void setSortPriority(int priority);
    void setResourceType(const string& rt);
    void setContentType(const string& ct);
    void setApplicationType(const string& at);
    void setIcon(ResourceDescriptor::IconType iconType, const string& icon);
    void setIcons(map<ResourceDescriptor::IconType, string> icons);
    void setLabel(const string& label);
    void setNameRule(const string& rule);
    void setRootTask(bool rootTask);
    void setStateBased(bool isStateBased);
    void setMenuItem(const string& menuItem);
    void setDescriptorFile(const string& descriptorFile);
    void setFactoryCategory(const string& factoryCategory);
    void setIsExpandable(bool isExpandable);
    void setContextPanel(const string& contextPanel);
    void setIsSessionBased(bool isSessionBased);
    void setContains(vector<ResourceType*> resources);
    void setContainedBy(vector<ResourceType*> resources);
    void setDefaultTool(ResourceTool* defaultTool);
    void setDefaultTools(map<ResourceDescriptor::RUNSTATE, ResourceTool*> defaultTools);
    void setTools(vector<ResourceTool*> tools);
    void setTools(multimap<ResourceDescriptor::RUNSTATE, ResourceTool*> tools);
    void setRmTasks(vector<ResourceRmTask*> rmTasks);
    void setRmTasks(multimap<ResourceDescriptor::RUNSTATE, ResourceRmTask*> rmTasks);

    bool isValidResourceName(const string& name);

  protected:

    ResourceType();

    string p_name;                /**< Name of resource.  Identifies resource 
                                       described within the the resource descriptor 
                                       file. */
    int p_id;                     /**< Resource ID start from 0, same as the index of the
                                       Resource in ResourceDescriptor::p_resources */
    int p_sortPriority;           /**< Priority when sorted, 0 highest */
    string p_rt;                  /**< Resource Type */
    string p_ct;                  /**< Content Type */
    string p_at;                  /**< Application Type */

    map<ResourceDescriptor::IconType, string>
      p_icons;                    /**< Icon file names for a resource */

    string p_label;               /**< Default name of resource when created */
    string p_nameRule;            /**< Reg exp defining naming convention/rule
                                       for this resource */
    bool p_rootTask;              /**< Indicates if the resource can be the root task of a
                                       study (ie first task created in the study).  As an
                                       example, for MD, only Prepare tasks can be the 
                                       root task */
    bool p_isStateBased;          /**< Indicates if resource is state based */
    string p_menuItem;            /**< New menu item for resource */
    string p_descriptorFile;      /**< Name of resource's descriptor file */
    string p_factoryCategory;     /**< Categorization of resource, used by EDSIFactory
                                       to call the correct create/get methods. */
    bool p_isExpandable;          /**< Indicates that the resource can be expanded in the
                                       Calculation Manager's tree view, revealing
                                       children, if any. */
    string p_contextPanel;        /**< Identifies the name of the context panel class for
                                       the resource */
    bool p_isSessionBased;        /**< Indicates that the resource manages a set of tasks
                                       using a linkbase */
    vector<ResourceType*>  
      p_containsResources;        /**< Resources that this resource can contain */
    vector<ResourceType*> 
      p_containedByResources;     /** Resources that can contain this resource */
    ResourceTool* p_defaultTool;  /**< Default tool for a non state-based resource */
    map<ResourceDescriptor::RUNSTATE, ResourceTool*> 
      p_stateDefaultTools;        /**< Default tool(s) for a state-based resource */
    vector<ResourceTool*>         
      p_tools;                    /**< List of all available tools for a resource */
    multimap<ResourceDescriptor::RUNSTATE, ResourceTool*> 
      p_stateTools;               /**< The set of available tools for each RUNSTATE.  If the 
                                       resource is not state-based then this set will be
                                       empty. */
    vector<ResourceRmTask*> 
      p_rmTasks;                  /**< List of all available run management tasks for a
                                       resource. */
    multimap<ResourceDescriptor::RUNSTATE, ResourceRmTask*> 
      p_stateRmTasks;             /**< The set of available rm tasks for each RUNSTATE.  If the 
                                       resource is not state-based then this set will be
                                       empty. */
};


#endif

