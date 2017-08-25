/**
* @file
*
*
*/

#include <string>
  using std::string;

#include <vector>
  using std::vector;

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLString.hpp>
  using namespace xercesc;


#include "util/StringConverter.H"
#include "util/STLUtil.H"

#include "dsm/AtomProperty.H"


/**
 * Constructor.  
 */
AtomProperty::AtomProperty()
{
  p_defaultWidth = 1;
}


/**
 * Constructor.  
 */
AtomProperty::AtomProperty(DOMElement *propertyElement) :
              GridProperty(propertyElement) 
{
  p_defaultWidth = loadDefaultWidth(propertyElement);
}


/**
 * Destructor
 */
AtomProperty::~AtomProperty()
{}


int AtomProperty::getDefaultWidth() const
{
  return p_defaultWidth;
}


AtomProperty::Prop AtomProperty::getProp() const
{
  return strToProp(getName());
}


void AtomProperty::setDefaultWidth(int width)
{
  p_defaultWidth = width;
}


void AtomProperty::setProp(AtomProperty::Prop prop)
{
  setName(propToStr(prop));
}


/**
 * Converts an atom prop to its cooresponding string value
 */
string AtomProperty::propToStr(AtomProperty::Prop prop)
{
  string ret = "";

  switch (prop) {
    case AP_ELEMENT:
      ret = "ap_element";
      break;
    case AP_NUM:
      ret = "ap_num";
      break;
    case AP_SUFFIX:
      ret = "ap_suffix";
      break;
    case AP_ATOM_NAME:
      ret = "ap_atom_name";
      break;
    case AP_RES_NAME:
      ret = "ap_res_name";
      break;
    case AP_PDB_NUM:
      ret = "ap_pdb_num";
      break;
    case AP_ICODE:
      ret = "ap_icode";
      break;
    case AP_CHAIN:
      ret = "ap_chain";
      break;
    case AP_SEGMENT:
      ret = "ap_segment";
      break;
    case AP_XCOORD:
      ret = "ap_xcoord";
      break;
    case AP_YCOORD:
      ret = "ap_ycoord";
      break;
    case AP_ZCOORD:
      ret = "ap_zcoord";
      break;
    case AP_MASS:
      ret = "ap_mass";
      break;
    case AP_BEHAVIOR:
      ret = "ap_behavior";
      break;
    case AP_OCC:
      ret = "ap_occ";
      break;
    case AP_BFACTOR:
      ret = "ap_bfactor";
      break;
    case AP_CHARGE:
      ret = "ap_charge";
      break;
    case AP_TYPE:
      ret = "ap_type";
      break;
    case AP_POL:
      ret = "ap_pol";
      break;
    case AP_CG:
      ret = "ap_cg";
      break;
    case AP_PG:
      ret = "ap_pg";
      break;
    case AP_ENV:
      ret = "ap_env";
      break;
    case AP_LINK_NUM:
      ret = "ap_link_num";
      break;
    case AP_RES_EDIT:
      ret = "ap_res_edit";
      break;
    case AP_RES_NUM:
      ret = "ap_res_num";
      break;
    case AP_COUPLING:
      ret = "ap_coupling";
      break;
    case AP_STATUS:
      ret = "ap_status";
      break;
    case AP_UNDEFINED:
      ret = "ap_undefined";
      break;
  }
  return ret;
}


/**
 * Converts a string to an atom prop, if the string value is not recognized
 * then AP_UNDEFINED will be returned.
 */
AtomProperty::Prop AtomProperty::strToProp(const string& prop)
{
  AtomProperty::Prop ret;

  string lstr = "";
  if (StringConverter::toLower(prop, lstr)) {
    if (lstr == "ap_element") {
      ret = AtomProperty::AP_ELEMENT;
    }
    else if (lstr == "ap_num") {
      ret = AtomProperty::AP_NUM;
    }
    else if (lstr == "ap_suffix") {
      ret = AtomProperty::AP_SUFFIX;
    }
    else if (lstr == "ap_atom_name") {
      ret = AtomProperty::AP_ATOM_NAME;
    }
    else if (lstr == "ap_res_name") {
      ret = AtomProperty::AP_RES_NAME;
    }
    else if (lstr == "ap_pdb_num") {
      ret = AtomProperty::AP_PDB_NUM;
    }
    else if (lstr == "ap_icode") {
      ret = AtomProperty::AP_ICODE;
    }
    else if (lstr == "ap_chain") {
      ret = AtomProperty::AP_CHAIN;
    }
    else if (lstr == "ap_segment") {
      ret = AtomProperty::AP_SEGMENT;
    }
    else if (lstr == "ap_xcoord") {
      ret = AtomProperty::AP_XCOORD;
    }
    else if (lstr == "ap_ycoord") {
      ret = AtomProperty::AP_YCOORD;
    }
    else if (lstr == "ap_zcoord") {
      ret = AtomProperty::AP_ZCOORD;
    }
    else if (lstr == "ap_mass") {
      ret = AtomProperty::AP_MASS;
    }
    else if (lstr == "ap_behavior") {
      ret = AtomProperty::AP_BEHAVIOR;
    }
    else if (lstr == "ap_occ") {
      ret = AtomProperty::AP_OCC;
    }
    else if (lstr == "ap_bfactor") {
      ret = AtomProperty::AP_BFACTOR;
    }
    else if (lstr == "ap_charge") {
      ret = AtomProperty::AP_CHARGE;
    }
    else if (lstr == "ap_type") {
      ret = AtomProperty::AP_TYPE;
    }
    else if (lstr == "ap_pol") {
      ret = AtomProperty::AP_POL;
    }
    else if (lstr == "ap_cg") {
      ret = AtomProperty::AP_CG;
    }
    else if (lstr == "ap_pg") {
      ret = AtomProperty::AP_PG;
    }
    else if (lstr == "ap_env") {
      ret = AtomProperty::AP_ENV;
    }
    else if (lstr == "ap_link_num") {
      ret = AtomProperty::AP_LINK_NUM;
    }
    else if (lstr == "ap_res_edit") {
      ret = AtomProperty::AP_RES_EDIT;
    }
    else if (lstr == "ap_res_num") {
      ret = AtomProperty::AP_RES_NUM;
    }
    else if (lstr == "ap_coupling") {
      ret = AtomProperty::AP_COUPLING;
    }
    else if (lstr == "ap_status") {
      ret = AtomProperty::AP_STATUS;
    }
    else {
      ret = AtomProperty::AP_UNDEFINED;
    }
  }
  return ret;
}


/**
 * Get value of <Width> for an atom property.  Returns 1
 * if the width value is missing or cannot be evaluated.
 */
int AtomProperty::loadDefaultWidth(DOMElement *propertyElement)
{
  int ret = 0;
  string orderVal = "";

  XMLCh *tmpStr = XMLString::transcode("DefaultWidth");
  DOMNodeList *nodes = propertyElement->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  int numNodes = 0;
  if (nodes != 0) {
    numNodes = nodes->getLength();
  }

  if (numNodes > 0) {
    DOMElement* ele = (DOMElement*)nodes->item(0);
    if (ele->hasChildNodes()) {
      DOMNode *child = ele->getFirstChild();
      char *value = XMLString::transcode(child->getNodeValue());
      orderVal = value;
      delete [] value;
    }
  }

  STLUtil::stoi(orderVal, ret);
  return ret;
}



