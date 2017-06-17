/**
* @file
*
*
*/
#ifndef GRIDPROPERTY_HH
#define GRIDPROPERTY_HH

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <xercesc/dom/DOMElement.hpp>
  using namespace xercesc;

#include "dsm/ResourceDescriptor.H"



/**
 *  Represents a generic grid property
 */
class GridProperty {

  public:

    /**
     * Grid property types.  These are the possible
     * data types for a grid property value.
     */
    enum PropertyType
    {
      PT_CDATA,         /**< Character data, ie strings of text */
      PT_DATE_TIME,     /**< A date time value */
      PT_CHECK,         /**< A "check mark" or some other visual cue indicating 
                             that a value for the property exists or that 
                             the property value is "true" */
      PT_ICON,          /**< Indicates that the property is represented
                             by an icon.  The property value will be the 
                             name of the icon file */
      PT_RESOURCE_ICON, /**< Indicates that the property is represented by
                             a resource icon (WxResourceImageList). */
      PT_RUNSTATE_ICON, /**< Indicates that the property is represented by a
                             runstate icon (WxState) */
      PT_NUMERIC,       /**< Numeric data */
      PT_FLOAT,         /**< Floating point data */
      PT_UNDEFINED      /**< An unrecognized data type */

    };


    GridProperty();
    GridProperty(GridProperty::PropertyType type);
    GridProperty(DOMElement *propertyElement);
    virtual bool operator==(const GridProperty& rhs);
    virtual ~GridProperty();

    string getName() const;
    int getOrder() const;
    string getLabel() const;
    bool getDisplay() const;
    bool getReadOnly() const;
    GridProperty::PropertyType getType() const;

    void setName(const string& name);
    void setOrder(const int& order);
    void setLabel(const string& label);
    void setDisplay(const bool& isDisplayed);
    void setReadOnly(const bool& isReadOnly);
    void setType(GridProperty::PropertyType type); 

    static string propertyTypeToStr(GridProperty::PropertyType type);
    static GridProperty::PropertyType strToPropertyType(const string& pt);

  protected:

    string loadLabel(DOMElement *propertyElement);
    string loadName(DOMElement *propertyElement);
    int loadOrder(DOMElement *propertyElement);
    bool loadDisplay(DOMElement *propertyElement);
    bool loadReadOnly(DOMElement *propertyElement);
    GridProperty::PropertyType loadType(DOMElement *propertyElement);

    string p_name;      /**< Name of property. */
    int p_order;        /**< Order that property is displayed */
    string p_label;     /**< Property label, used as the column header */
    bool p_display;     /**< Indicates if property is displayed */
    bool p_isReadOnly;  /**< Indicates the property is not editable */
    GridProperty::PropertyType 
         p_type;         /**< Data type of property value */
};


#endif

