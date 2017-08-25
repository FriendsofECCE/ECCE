/**
* @file
*
*
*/
#ifndef RESOURCETOOL_HH
#define RESOURCETOOL_HH

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
 * Represents an Ecce Tool.  These tools are initialized 
 * with data from the resource descriptor file.  Only  
 * ResourceDescriptor can create a ResourceTool object.
 */
class ResourceTool {

  public:

    friend class ResourceDescriptor;

    virtual ~ResourceTool();

    string getName();
    int getId();
    string getIcon();
    string getLabel();
    string getMenuItem();
    string getInvokeArg();
    int getGateway();

    void setName(const string& name);
    void setId(int id);
    void setIcon(const string& icon);
    void setLabel(const string& label);
    void setMenuItem(const string& menuItem);
    void setInvokeArg(const string& invokeArg);
    void setGateway(int gateway);

  protected:

    ResourceTool(DOMElement *toolElement);

    string loadMenuItem(DOMElement *toolElement);
    string loadLabel(DOMElement *toolElement);
    string loadIcon(DOMElement *toolElement);
    string loadInvokeArg(DOMElement *toolElement);
    int loadGateway(DOMElement *toolElement);

    string p_name;              /**< Name of Tool.  Identifies tool
                                     described within the the resource descriptor 
                                     file.  Used to look up information for 
                                     this tool.  */
    int p_id;                   /**< Tool ID start from 0, same as the index of the
                                     tool in ResourceDescriptor::p_tools */
    string p_icon;              /**< Icon file name */
    string p_label;             /**< Default name of tool */
    string p_menuItem;          /**< Menu item for starting tool */
    string p_invokeArg;         /**< Invocation argument for tool */
    int p_gateway;              /**< Position of current tool in gateway,
                                     -1 for hidden*/
};


#endif

