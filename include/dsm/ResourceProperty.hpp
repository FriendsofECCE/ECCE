/**
* @file
*
*
*/
#ifndef RESOURCEPROPERTY_HH
#define RESOURCEPROPERTY_HH

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <xercesc/dom/DOMElement.hpp>
  using namespace xercesc;

#include "dsm/GridProperty.H"
#include "dsm/ResourceDescriptor.H"



/**
 * Represents a meta data property for a resource managed
 * by Ecce.
 */
class ResourceProperty : public GridProperty {

  public:

    ResourceProperty();
    ResourceProperty(DOMElement *propertyElement);
    bool operator==(const ResourceProperty& rhs);
    virtual ~ResourceProperty();

    string getNameSpace() const;
    string getQName() const;
    void setNameSpace(const string& nameSpace);

  protected:

    string loadNameSpace(DOMElement *propertyElement);

    string p_nameSpace; /**< Namespace of property */
};


#endif

