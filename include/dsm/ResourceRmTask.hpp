/**
* @file
*
*
*/
#ifndef RESOURCERMTASK_HH
#define RESOURCERMTASK_HH

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <map>
  using std::map;

#include <xercesc/dom/DOMElement.hpp>
  using namespace xercesc;

#include "dsm/ResourceDescriptor.H"



/**
 * Represents an Ecce Run Management Task.  These tasks
 * are initialized with data from the resource 
 * descriptor file.  Only ResourceDescriptor can create
 * a ResourceRmTask object.
 */
class ResourceRmTask {

  public:

    friend class ResourceDescriptor;

    virtual ~ResourceRmTask();

    string getName();
    int getId();
    string getMenuItem();

    void setName(const string& name);
    void setId(int id);
    void setMenuItem(const string& menuItem);

  protected:

    ResourceRmTask(DOMElement *rmTaskElement);

    string loadMenuItem(DOMElement *rmTaskElement);


    string p_name;      /**< Name of Run Management Task.  Identifies task 
                             described within the the resource descriptor 
                             file.  Used to look up information for 
                             this task.  */
    int p_id;           /**< RmTask ID start from 0, same as the index of the
                             RmTask in ResourceDescriptor::p_rmTasks */
    string p_menuItem;  /**< Menu item for task */
};


#endif

