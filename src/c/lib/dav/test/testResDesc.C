

#include <iostream>
  using std::cout;
  using std::endl;

#include <vector>
#include <string>
  using std::string;


#include "util/CmdLineAuthListener.H"
#include "dsm/ResourceType.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ITraversable.H"
#include "dsm/EDSITraversable.H"
#include "dsm/EDSI.H"
#include "dsm/DavEDSI.H"
#include "dsm/PropertyView.H"
#include "dsm/ResourceProperty.H"



int main(int argc, char** argv) {

  ResourceDescriptor rd = ResourceDescriptor::getResourceDescriptor();

  string tname = "test";

  //string tcontentType="ecceProject";
  string tcontentType="ecceSession";
  //string tcontentType="ecceCalculation";
  //string tcontentType="ecceMdPrepare";
  //string tcontentType="ecceMdOptimize";
  //string tcontentType="ecceMdEnergy";
  //string tcontentType="ecceMdDynamics";
  //string tcontentType="";

  //ResourceDescriptor::RESOURCETYPE tresourceType = ResourceDescriptor::RT_COLLECTION;
  //ResourceDescriptor::RESOURCETYPE tresourceType = ResourceDescriptor::RT_COLLECTION;
  ResourceDescriptor::RESOURCETYPE tresourceType = ResourceDescriptor::RT_VIRTUAL_DOCUMENT;
  //ResourceDescriptor::RESOURCETYPE tresourceType = ResourceDescriptor::RT_UNDEFINED;
  
  TNode tnode(tname, tcontentType, tresourceType, 
              "http://eccentric.pnl.gov:8080/Ecce/users/d3j766/MDStudy_7");

  ResourceDescriptor::RESOURCETYPE rt = ResourceDescriptor::RT_VIRTUAL_DOCUMENT;
  ResourceDescriptor::RESOURCETYPE ct = ResourceDescriptor::CT_SESSION;
  ResourceDescriptor::RESOURCETYPE at = ResourceDescriptor::AT_UNDEFINED;
  ResourceType *res = rd.getResource(rt, ct, at);

  if (res != 0) {
    cout << "Resource:: " << "::" << res->getName() << endl;
    cout << "\t Icon Normal :: " << res->getIcon(ResourceDescriptor::IT_NORMAL) << endl;
    cout << "\t Icon Selected :: " << res->getIcon(ResourceDescriptor::IT_SELECTED) << endl;
    cout << "\t Icon Expanded  :: " << res->getIcon(ResourceDescriptor::IT_EXPANDED) << endl;
    cout << "\t Icon Selected Expanded :: " << res->getIcon(ResourceDescriptor::IT_SELECTED_EXPANDED) << endl;
    cout << "\t Menu :: " << res->getMenuItem() << endl;
    cout << "\t StateBased :: " << res->getStateBased() << endl;
    cout << "\t Desc :: " << res->getDescriptorFile() << endl;
  }
  else {
    cout << "RESOURCE NOT FOUND!!!" << endl;
  }


  vector<ResourceType *> resources = rd.getResources();
  vector<ResourceTool *> tools = rd.getTools();
  vector<ResourceRmTask *> tasks = rd.getRmTasks();
  vector<string> units = rd.getMdUnits();


  for (int i = 0; i < units.size(); i++) {
    cout << "Units:: " << i << endl;
    cout << "\t Unit Item :: " << units[i] << endl;
  }

  for (int i = 0; i < tasks.size(); i++) {
    cout << "Task:: " << i << "::" << tasks[i]->getName() << endl;
    cout << "\t Menu Item :: " << tasks[i]->getMenuItem() << endl;
  }

  for (int i = 0; i < tools.size(); i++) {
    cout << "Tool:: " << i << "::" << tools[i]->getName() << endl;
    cout << "\t Menu Item :: " << tools[i]->getMenuItem() << endl;
    cout << "\t Label :: " << tools[i]->getLabel() << endl;
    cout << "\t Icon :: " << tools[i]->getIcon() << endl;
  }

  for (int i = 0; i < resources.size(); i++) {

cout <<  endl;
cout << "########################################################" << endl;
cout << "NEXT RESOURCE" << endl;
cout << "########################################################" << endl;
cout <<  endl;
    cout << "Resource:: " << i << "::" << resources[i]->getName() << endl;
    cout << "\t Icon Normal :: " << resources[i]->getIcon(ResourceDescriptor::IT_NORMAL) << endl;
    cout << "\t Icon Selected :: " << resources[i]->getIcon(ResourceDescriptor::IT_SELECTED) << endl;
    cout << "\t Icon Expanded :: " << resources[i]->getIcon(ResourceDescriptor::IT_EXPANDED) << endl;
    cout << "\t Icon Selected Expanded :: " << resources[i]->getIcon(ResourceDescriptor::IT_SELECTED_EXPANDED) << endl;
    cout << "\t Menu :: " << resources[i]->getMenuItem() << endl;
    cout << "\t StateBased :: " << resources[i]->getStateBased() << endl;
    cout << "\t Desc :: " << resources[i]->getDescriptorFile() << endl;



    cout << "CONTAINS ::" << endl;
    vector<ResourceType*> contains = resources[i]->getContains();
    for (int j = 0; j < contains.size(); j++) {
        cout << "Contains :: " << contains[j]->getName() << endl;
    }


    cout << "CONTAINED BY ::" << endl;
    vector<ResourceType*> containedBy = resources[i]->getContainedBy();
    for (int j = 0; j < containedBy.size(); j++) {
        cout << "ContainedBy :: " << containedBy[j]->getName() << endl;
    }


    cout << "NON STATE BASED DEFAULT TOOLS::" << endl;
    ResourceTool* defaultTool =  resources[i]->getDefaultTool();
    cout << "DEFAULT TOOL :: " << defaultTool << endl;
    if (defaultTool != 0) {
      cout << "\t TOOL :: " << defaultTool->getName() << endl;
      cout << "\t TOOL :: " << endl;
    }
    else {
      cout << "\tNO NON STATE BASED DEFAULT TOOL" << endl;;
    }


    cout << "STATE BASED DEFAULT TOOLS::" << endl;
    for (ResourceDescriptor::RUNSTATE z = ResourceDescriptor::STATE_ILLEGAL; 
         z < ResourceDescriptor::NUMBER_OF_STATES; 
         z = ResourceDescriptor::RUNSTATE(z + 1)) {
    ResourceTool* defaultTool =  resources[i]->getDefaultTool(z);
        if (defaultTool != 0) {
        cout << "DEFAULT TOOL FOR ::" << z << endl;
        cout << "\t TOOL :: " << defaultTool->getName() << endl;
        }
    }
    cout << "END DEFAULT TOOLS::" << endl;

    if (resources[i]->getStateBased()) {
      cout << "STATE BASED TOOLS::" << endl;
      for (ResourceDescriptor::RUNSTATE z = ResourceDescriptor::STATE_ILLEGAL; 
           z < ResourceDescriptor::NUMBER_OF_STATES; 
           z = ResourceDescriptor::RUNSTATE(z + 1)) {
        vector<ResourceTool*> enabledTools =  resources[i]->getTools(z);
        for (int y = 0; y < enabledTools.size(); y++) {
        cout << "\t STATE:: " << z << " TOOL :: " << enabledTools[y]->getName() << endl;
        }
      }
      cout << "END STATE BASED TOOLS::" << endl;
    }
    else {
    cout << "NON STATE BASED TOOLS::" << endl;
      vector<ResourceTool*> enabledTools =  resources[i]->getTools();
      for (int y = 0; y < enabledTools.size(); y++) {
      cout << "\t TOOL :: " << enabledTools[y]->getName() << endl;
      }
      cout << "END NON STATE BASED TOOLS::" << endl;
    }

    if (resources[i]->getStateBased()) {
      cout << "STATE BASED TASKS::" << endl;
      for (ResourceDescriptor::RUNSTATE z = ResourceDescriptor::STATE_ILLEGAL; 
           z < ResourceDescriptor::NUMBER_OF_STATES; 
           z = ResourceDescriptor::RUNSTATE(z + 1)) {
        vector<ResourceRmTask*> enabledTasks =  resources[i]->getRmTasks(z);
        for (int y = 0; y < enabledTasks.size(); y++) {
        cout << "\t STATE:: " << z << " TASK :: " << enabledTasks[y]->getName() << endl;
        }
      }
      cout << "END STATE BASED TASKS::" << endl;
    }
    else {
      cout << "NON STATE BASED TASKS::" << endl;
      vector<ResourceRmTask*> enabledTasks =  resources[i]->getRmTasks();
      for (int y = 0; y < enabledTasks.size(); y++) {
      cout << "\t TASK :: " << enabledTasks[y]->getName() << endl;
      }
      cout << "END NON STATE BASED TASKS::" << endl;
    }



  }

    cout << endl;
    cout << "PROPERTY VIEWS " << endl;
    multimap<int, PropertyView*> pvs = rd.getPropertyViews();
    multimap<int, PropertyView*>::iterator mapIt;
    for (mapIt = pvs.begin();
         mapIt != pvs.end();
         mapIt++) {
      cout << "\tORDER :: " << mapIt->first << endl;
      cout << "\tNAME :: " << mapIt->second->getName() << endl;
      cout << "\tMENU :: " << mapIt->second->getMenuItem() << endl;
      cout << "\tORDER :: " << mapIt->second->getOrder() << endl;
      multimap<int, ResourceProperty*> rps = mapIt->second->getProperties();
      multimap<int, ResourceProperty*>::iterator rpIt;
      for (rpIt = rps.begin();
           rpIt != rps.end();
           rpIt++) {
        cout << "##########################################" << endl;
        cout << "\tORDER :: " << rpIt->first << endl;
        cout << "\tNAME :: " << rpIt->second->getName() << endl;
        cout << "\tNAMESPACE :: " << rpIt->second->getNameSpace() << endl;
        cout << "\tLABEL :: " << rpIt->second->getLabel() << endl;
        cout << "\tDISPLAY :: " << rpIt->second->getDisplay() << endl;
        cout << "\tTYPE :: " << rpIt->second->propertyTypeToStr(rpIt->second->getType()) << endl;
        cout << "##########################################" << endl;
      }
        cout << "##########################################" << endl;
        cout << "##########################################" << endl;
    }

}
