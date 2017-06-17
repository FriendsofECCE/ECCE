/**
* @file
*
*
*/

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
      

#include "dsm/ResourceType.H"

#include "util/EcceRegexp.H"
#include "util/StringConverter.H"




/**
 * Constructor.  Only ResourceDescriptor can
 * create ResourceType objects.
 */
ResourceType::ResourceType()
{
  p_name = "";
  p_id = -1;
  p_sortPriority = LOWEST_PRIORITY;  // 0 is highest
  p_rt = "";
  p_ct = "";
  p_at = "";
  p_label = "";
  p_nameRule = "";
  p_defaultTool = 0;
  p_rootTask = false;
  p_isStateBased = false;
  p_menuItem = "";
  p_descriptorFile = "";
  p_factoryCategory = "";
  p_isExpandable = false;
  p_isSessionBased = false;
}


/**
 * Destructor
 */
ResourceType::~ResourceType()
{}


bool ResourceType::operator==(const ResourceType& rhs) const
{
  return (this->getName() == rhs.getName());
}


string ResourceType::getName() const
{
  return p_name;
}


int ResourceType::getId()
{
  return p_id;
}


int ResourceType::getSortPriority()
{
  return p_sortPriority;
}


string ResourceType::getResourceType() const
{
  return p_rt;
}


string ResourceType::getContentType() const
{
  return p_ct;
}


string ResourceType::getApplicationType() const
{
  return p_at;
}


string ResourceType::getIcon(ResourceDescriptor::IconType iconType)
{
  string ret = "";
  map<ResourceDescriptor::IconType, string>::iterator mapIt;

  mapIt = p_icons.find(iconType);

  if (mapIt != p_icons.end()) {
    ret = mapIt->second;
  }
  
  return ret;
}


map<ResourceDescriptor::IconType, string> ResourceType::getIcons()
{
  return p_icons;
}


string ResourceType::getLabel()
{
  return p_label;
}


string ResourceType::getNameRule()
{
  return p_nameRule;
}


bool ResourceType::getRootTask()
{
  return p_rootTask;
}


bool ResourceType::getStateBased()
{
  return p_isStateBased;
}


string ResourceType::getMenuItem()
{
  return p_menuItem;
}


string ResourceType::getDescriptorFile()
{
  return p_descriptorFile;
}


string ResourceType::getFactoryCategory()
{
  return p_factoryCategory;
}


bool ResourceType::getIsExpandable()
{
  return p_isExpandable;
}


string ResourceType::getContextPanel()
{
  return p_contextPanel;
}


bool ResourceType::getIsSessionBased()
{
  return p_isSessionBased;
}


vector<ResourceType*> ResourceType::getContains()
{
  return p_containsResources;
}


vector<ResourceType*> ResourceType::getContainedBy()
{
  return p_containedByResources;
}


ResourceTool* ResourceType::getDefaultTool()
{
  return p_defaultTool;
}


ResourceTool* ResourceType::getDefaultTool(ResourceDescriptor::RUNSTATE runstate)
{
  ResourceTool *ret = 0;
  map<ResourceDescriptor::RUNSTATE, ResourceTool*>::iterator mapIt;
  mapIt = p_stateDefaultTools.find(runstate);
  if (mapIt != p_stateDefaultTools.end()) {
    ret = mapIt->second;
  }
  return ret;
}


vector<ResourceTool*> ResourceType::getTools()
{
  return p_tools;
}


vector<ResourceTool*> ResourceType::getTools(ResourceDescriptor::RUNSTATE runstate)
{
  vector<ResourceTool*> ret;
  multimap<ResourceDescriptor::RUNSTATE, ResourceTool*>::iterator mapIt;
  for (mapIt = p_stateTools.begin();
       mapIt != p_stateTools.end();
       mapIt++) {
    if (mapIt->first == runstate) {
      ret.push_back(mapIt->second);
    } 
  }
  return ret;
}


vector<ResourceRmTask*> ResourceType::getRmTasks()
{
  return p_rmTasks;
}


vector<ResourceRmTask*> ResourceType::getRmTasks(ResourceDescriptor::RUNSTATE runstate)
{
  vector<ResourceRmTask*> ret;
  multimap<ResourceDescriptor::RUNSTATE, ResourceRmTask*>::iterator mapIt;
  for (mapIt = p_stateRmTasks.begin();
       mapIt != p_stateRmTasks.end();
       mapIt++) {
    if (mapIt->first == runstate) {
      ret.push_back(mapIt->second);
    } 
  }
  return ret;
}


void ResourceType::setName(const string& name)
{
  p_name = name;
}


void ResourceType::setId(int id)
{
  p_id = id;
}


void ResourceType::setSortPriority(int priority)
{
  p_sortPriority = priority;
}


void ResourceType::setResourceType(const string& rt)
{
  p_rt = rt;
}


void ResourceType::setContentType(const string& ct)
{
  p_ct = ct;
}


void ResourceType::setApplicationType(const string& at)
{
  p_at = at;
}


void ResourceType::setIcon(ResourceDescriptor::IconType iconType, const string& icon)
{
  p_icons[iconType] = icon;
}


void ResourceType::setIcons(map<ResourceDescriptor::IconType, string> icons)
{
  p_icons = icons;
}


void ResourceType::setLabel(const string& label)
{
  p_label = label;
}


void ResourceType::setNameRule(const string& rule)
{
  p_nameRule = rule;
}


void ResourceType::setRootTask(bool rootTask)
{
  p_rootTask = rootTask;
}


void ResourceType::setStateBased(bool isStateBased)
{
  p_isStateBased = isStateBased;
}


void ResourceType::setMenuItem(const string& menuItem)
{
  p_menuItem = menuItem;
}


void ResourceType::setDescriptorFile(const string& descriptorFile)
{
  p_descriptorFile = descriptorFile;
}


void ResourceType::setFactoryCategory(const string& factoryCategory)
{
  p_factoryCategory = factoryCategory;
}


void ResourceType::setIsExpandable(bool isExpandable)
{
  p_isExpandable = isExpandable;
}


void ResourceType::setContextPanel(const string& contextPanel)
{
  p_contextPanel = contextPanel;
}


void ResourceType::setIsSessionBased(bool isSessionBased)
{
  p_isSessionBased = isSessionBased;
}


void ResourceType::setContains(vector<ResourceType*> resources)
{
  p_containsResources = resources;
}


void ResourceType::setContainedBy(vector<ResourceType*> resources)
{
  p_containedByResources = resources;
}


void ResourceType::setDefaultTool(ResourceTool* defaultTool)
{
  p_defaultTool = defaultTool;
}


void ResourceType::setDefaultTools(map<ResourceDescriptor::RUNSTATE, ResourceTool*> defaultTools)
{
  p_stateDefaultTools = defaultTools;
}


void ResourceType::setTools(vector<ResourceTool*> tools)
{
  p_tools = tools;
}


void ResourceType::setTools(multimap<ResourceDescriptor::RUNSTATE, ResourceTool*> tools)
{
  p_stateTools = tools;

  // Refresh p_tools
  p_tools.clear(); 
  multimap<ResourceDescriptor::RUNSTATE, ResourceTool*>::iterator mapIt;
  for (mapIt = p_stateTools.begin();
       mapIt != p_stateTools.end();
       mapIt++) {
    p_tools.push_back(mapIt->second);
  }
}


void ResourceType::setRmTasks(vector<ResourceRmTask*> rmTasks)
{
  p_rmTasks = rmTasks;
}


void ResourceType::setRmTasks(multimap<ResourceDescriptor::RUNSTATE, ResourceRmTask*> rmTasks)
{
  p_stateRmTasks = rmTasks;

  // Refresh p_rmTasks
  p_rmTasks.clear(); 
  multimap<ResourceDescriptor::RUNSTATE, ResourceRmTask*>::iterator mapIt;
  for (mapIt = p_stateRmTasks.begin();
       mapIt != p_stateRmTasks.end();
       mapIt++) {
    p_rmTasks.push_back(mapIt->second);
  }
}


bool ResourceType::isValidResourceName(const string& name)
{
  bool ret = false;
  EcceRegexp regExp(getNameRule());

  int foundIdx;
  int length;
  string match = regExp.match(name, foundIdx, length);

  if (match == name) {
    ret = true;
  }

  return ret;
}





