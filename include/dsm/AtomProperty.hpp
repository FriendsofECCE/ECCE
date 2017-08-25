/**
* @file
*
*
*/
#ifndef ATOMPROPERTY_HH
#define ATOMPROPERTY_HH

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <xercesc/dom/DOMElement.hpp>
  using namespace xercesc;

#include "dsm/GridProperty.H"



/**
 * Represents a property for an atom of a
 * chemical system.
 */
class AtomProperty : public GridProperty {

  public:

    AtomProperty();
    AtomProperty(DOMElement *propertyElement);
    virtual ~AtomProperty();

    /**
     * The different atom properties that may be represented 
     */
    enum Prop 
    {
      AP_ELEMENT,
      AP_NUM,
      AP_SUFFIX,
      AP_ATOM_NAME,
      AP_RES_NAME,
      AP_PDB_NUM,
      AP_ICODE,
      AP_CHAIN,
      AP_SEGMENT,
      AP_XCOORD, 
      AP_YCOORD,
      AP_ZCOORD,
      AP_MASS,
      AP_BEHAVIOR,
      AP_OCC,
      AP_BFACTOR,
      AP_CHARGE,
      AP_TYPE,
      AP_POL,
      AP_CG,
      AP_PG,
      AP_ENV,
      AP_LINK_NUM,
      AP_RES_EDIT,
      AP_RES_NUM,
      AP_COUPLING,
      AP_STATUS,
      AP_UNDEFINED
    };

    int getDefaultWidth() const;
    AtomProperty::Prop getProp() const;

    void setDefaultWidth(int width);
    void setProp(AtomProperty::Prop);

    static string propToStr(AtomProperty::Prop prop);
    static AtomProperty::Prop strToProp(const string& prop);

  protected:

    int loadDefaultWidth(DOMElement *propertyElement);

    int p_defaultWidth;         /**< Indicates width of column, in characters */
};


#endif

