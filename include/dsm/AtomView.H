/**
* @file
*
*
*/
#ifndef ATOMVIEW_HH
#define ATOMVIEW_HH

#include <string>
  using std::string;

#include <vector>
  using std::vector;


#include "dsm/AtomProperty.H"
#include "dsm/ResourceDescriptor.H"



/**
 * Represents a set of atom properties that may be viewed
 * for a chemical system.   
 */
class AtomView {

  public:

    AtomView(DOMElement *pvElement);
    virtual ~AtomView();

    string getName();
    int getId();
    string getMenuItem();
    string getToolTip();
    int getOrder();
    multimap<int, AtomProperty*> getProperties();
    multimap<int, AtomProperty*> getDisplayedProperties();

    void setName(const string& name);
    void setId(int id);
    void setMenuItem(const string& menuItem);
    void setToolTip(const string& toolTip);
    void setOrder(const int& order);
    void setProperties(multimap<int, AtomProperty*> properties);

  protected:
    AtomView(const AtomView& rhs);

    void cleanup();
    void flushProperties();

    string loadMenuItem(DOMElement *avElement);
    string loadToolTip(DOMElement *avElement);
    int loadOrder(DOMElement *avElement);
    multimap<int, AtomProperty*> loadProperties(DOMElement *avElement);


    string p_name;       /**< Name of property view. */
    int p_id;            /**< Id of property view, used to identify 
                              atom view menu items. */
    string p_menuItem;   /**< Menu item for atom view */
    string p_toolTip;    /**< Menu item for atom view */
    int    p_order;      /**< Order the atom view should 
                              appear in menu */
    multimap<int, AtomProperty*>
           p_properties; /**< List of properties included in 
                              this atom view. Multimap key is an int
                              which indicates the order of display
                              for these properties. */
};


#endif

