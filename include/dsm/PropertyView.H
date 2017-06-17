/**
* @file
*
*
*/
#ifndef PROPERTYVIEW_HH
#define PROPERTYVIEW_HH

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
class PropertyView {

  public:

    PropertyView(DOMElement *pvElement);
    virtual ~PropertyView();

    string getName();
    int getId();
    string getMenuItem();
    int getOrder();
    multimap<int, ResourceProperty*> getProperties();
    multimap<int, ResourceProperty*> getDisplayedProperties();

    void setName(const string& name);
    void setId(int id);
    void setMenuItem(const string& menuItem);
    void setOrder(const int& order);
    void setProperties(multimap<int, ResourceProperty*> properties);

  protected:

    void cleanup();
    void flushProperties();

    string loadMenuItem(DOMElement *pvElement);
    int loadOrder(DOMElement *pvElement);
    multimap<int, ResourceProperty*> loadProperties(DOMElement *pvElement);


    string p_name;       /**< Name of property view. */
    int p_id;            /**< Id of property view, used to identify 
                              property view menu items. */
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

