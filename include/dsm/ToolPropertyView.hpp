/**
* @file
*
*
*/
#ifndef TOOLPROPERTYVIEW_HH
#define TOOLPROPERTYVIEW_HH

#include <string>
  using std::string;

#include <vector>
  using std::vector;


#include "dsm/ResourceProperty.H"
#include "dsm/ResourceDescriptor.H"



/**
 * Represents a set of properties that may be viewed
 * for a list of resources.    
 */
class ToolPropertyView {

  public:

    ToolPropertyView(DOMElement *pvElement);
    virtual ~ToolPropertyView();

    string getTool();
    string getContentType();
    string getMenuItem();
    int getOrder();
    multimap<int, ResourceProperty*> getProperties();
    multimap<int, ResourceProperty*> getDisplayedProperties();

    void setTool(const string& tool);
    void setContentType(const string& contentType);
    void setMenuItem(const string& menuItem);
    void setOrder(const int& order);
    void setProperties(multimap<int, ResourceProperty*> properties);

  protected:

    void cleanup();
    void flushProperties();

    string loadMenuItem(DOMElement *pvElement);
    int loadOrder(DOMElement *pvElement);
    multimap<int, ResourceProperty*> loadProperties(DOMElement *pvElement);


    string p_tool;       /**< Name of tool. */
    string p_contentType;/**< Name of content type. */
    string p_menuItem;   /**< Menu item for property view */
    int    p_order;      /**< Order the property view should 
                              appear in menu */
    multimap<int, ResourceProperty*>
           p_properties; /**< List of properties included in 
                              this property view. Multimap key is an int
                              which indicates the order of display
                              for these properties. */
};


#endif

