//#include <iostream>
//using std::cout;
//using std::endl;

#include <algorithm>
#include <set>

#include <wx/link.h>
//#include <wx/log.h>

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLString.hpp>
  using namespace xercesc;

#include "util/Ecce.H"
#include "util/SFile.H"

#include "dsm/BasicDOMParser.H"
#include "dsm/PropFactory.H"

#include "PropertyPanel.H"
#include "PropertyPanelFactory.H"


wxFORCE_LINK_MODULE(KeyValuePropertyPanel)
wxFORCE_LINK_MODULE(MoPanel)
wxFORCE_LINK_MODULE(MullikenPanel)
wxFORCE_LINK_MODULE(PlotAnyPropertyPanel)
wxFORCE_LINK_MODULE(TrajectoryPanel)
wxFORCE_LINK_MODULE(VectorPlotPropertyPanel)


PropertyPanelFactory *PropertyPanelFactory::p_singleton = NULL;


PropertyPanelFactory& PropertyPanelFactory::getPropertyPanelFactory()
{
  /* optimized for single-threaded apps
  if (!p_singleton) {
    p_singleton = new PropertyPanelFactory();
  }
  return *p_singleton;
  */
  static PropertyPanelFactory instance;
  return instance;
}


set<string>
PropertyPanelFactory::getPanelNamesForProperties(const vector<string>& keys)
{
  set<string> ret;
  set<string> consumedKeys;

  vector<PropertyPanelDescription*>::iterator panel;
  for (panel = p_panels.begin(); panel != p_panels.end(); panel++) {
    set<string> &properties = (*panel)->properties;

    vector<string>::const_iterator key;
    for (key = keys.begin(); key != keys.end(); key++) {
      if (consumedKeys.find(*key) != consumedKeys.end()) {
        continue; // skip a consumed key
        // instead, we might consider a modifiable vector of query keys
        // where we remove keys known to be consumed, for performance reasons
      }

      if (properties.find(*key) != properties.end()) {
        ret.insert((*panel)->name);
        if ((*panel)->consumer) {
          consumedKeys.insert(properties.begin(), properties.end());
        }
        break; // all we wanted was to know to include this panel for any key
      }
    }
  }

  return ret;
}


bool PropertyPanelFactory::isRelevant(const string& key)
{
  return !getPanelNamesForProperties(vector<string>(1,key)).empty();
}


/**
 * Dynamically instantiates a PropertyPanel with the given name.
 *
 * @param name of the PropertyPanel (not its class name)
 * @return NULL if a PropertyPanel does not exist with the given name
 */
PropertyPanel* PropertyPanelFactory::createPropertyPanel(const string& name)
{
  if (p_panelsByName.find(name) == p_panelsByName.end()) {
    //cout << "PropertyPanel does not exists by the name " << name.c_str() << endl;
    return NULL;
  }

  PropertyPanel *panel = NULL;
  PropertyPanelDescription *description = p_panelsByName[name];

  wxObject *object = wxCreateDynamicObject(description->className.c_str());
  if (!object) {
    //cout << "Could not create %s" << description->className.c_str() << endl;
  } else {
    // NOTE: dynamic_cast<> did not work here
    panel = wxDynamicCast(object, PropertyPanel);
    if (!panel) {
      //cout << "Could not cast to PropertyPanel" << endl;
    } else {
      panel->setDescription(description);
    }
  }

  return panel;
}


PropertyPanelFactory::PropertyPanelFactory()
  : p_doc(NULL),
    p_panels(),
    p_panelsByName()
{
  try {
    parsePropertyPanelDescriptorFile();
    createPropertyPanelDescriptions();
  } catch (...) {
    //cout << "Unable to load PropertyPanelDescriptor file" << endl;
  }
}


void PropertyPanelFactory::parsePropertyPanelDescriptorFile()
throw (ParseException)
{
  string file = Ecce::ecceDataPath();
  file += "/client/config/PropertyPanelDescriptor.xml";
  BasicDOMParser parser;
  p_doc = parser.parse(SFile(file));
}


/**
 * Creates PropertyPanelDescription objects and saves in p_panels member.
 */
void PropertyPanelFactory::createPropertyPanelDescriptions()
{
  DOMElement *panel = NULL, *element = NULL, *property = NULL;
  DOMNodeList *panels = NULL, *properties = NULL;
  int tmpStrLength = 100;
  XMLCh tmpStr[101];
  string boolStr;
  int numPanels, numProperties;

  // Get all <PropertyPanel> Elements
  XMLString::transcode("PropertyPanel", tmpStr, tmpStrLength);
  panels = p_doc->getElementsByTagName(tmpStr);

  numPanels = 0;
  if (panels) {
    numPanels = panels->getLength();
  }

  // Create PropertyPanelDescription objects
  for (int i = 0; i < numPanels; i++) {
    //cout << "<PropertyPanel>" << endl;
    // PropertyPanel Element
    panel = (DOMElement*)panels->item(i);

    PropertyPanelDescription *description = new PropertyPanelDescription;

    XMLString::transcode("name", tmpStr, tmpStrLength);
    element = (DOMElement*)panel->getElementsByTagName(tmpStr)->item(0);
    description->name =
            XMLString::transcode(element->getFirstChild()->getNodeValue());
    //cout << "\t<name>" << description->name.c_str() << "</name>" << endl;
    
    XMLString::transcode("class", tmpStr, tmpStrLength);
    element = (DOMElement*)panel->getElementsByTagName(tmpStr)->item(0);
    description->className =
            XMLString::transcode(element->getFirstChild()->getNodeValue());
    //cout << "\t<class>" << description->className.c_str() << "</class>" << endl;

    XMLString::transcode("consumer", tmpStr, tmpStrLength);
    element = (DOMElement*)panel->getElementsByTagName(tmpStr)->item(0);
    if (element) {
      boolStr = XMLString::transcode(element->getFirstChild()->getNodeValue());
      description->consumer = boolStr == "true" ? true : false;
    } else {
      description->consumer = false; // default
    }
    //cout << "\t<consumer>" << description->consumer << "</consumer>" << endl;

    XMLString::transcode("allowPin", tmpStr, tmpStrLength);
    element = (DOMElement*)panel->getElementsByTagName(tmpStr)->item(0);
    if (element) {
      boolStr = XMLString::transcode(element->getFirstChild()->getNodeValue());
      description->allowPin = boolStr == "true" ? true : false;
    } else {
      description->allowPin = true; // default
    }
    //cout << "\t<allowPin>" << description->allowPin << "</allowPin>" << endl;

    XMLString::transcode("property", tmpStr, tmpStrLength);
    properties = panel->getElementsByTagName(tmpStr);

    numProperties = 0;
    if (properties) {
      numProperties = properties->getLength();
    }

    // add property keys to set managed by this panel
    for (int j = 0; j < numProperties; j++) {
      //cout << "\tproperty set " << j+1 << "/" << numProperties << endl;
      property = (DOMElement*)properties->item(j);

      string name = "", classification = "", type = "", indexedBy = "";
      
      XMLString::transcode("name", tmpStr, tmpStrLength);
      if (property->hasAttribute(tmpStr)) {
        name = XMLString::transcode(property->getAttribute(tmpStr));
      }
      XMLString::transcode("class", tmpStr, tmpStrLength);
      if (property->hasAttribute(tmpStr)) {
        classification = XMLString::transcode(property->getAttribute(tmpStr));
      }
      XMLString::transcode("type", tmpStr, tmpStrLength);
      if (property->hasAttribute(tmpStr)) {
        type = XMLString::transcode(property->getAttribute(tmpStr));
      }
      XMLString::transcode("indexedBy", tmpStr, tmpStrLength);
      if (property->hasAttribute(tmpStr)) {
        indexedBy = XMLString::transcode(property->getAttribute(tmpStr));
      }

      // each "Property" element adds its own set of properties
      set<string> current_props;

      if (!name.empty()) {
        //cout << "\tNAME " << name << endl;
        current_props.insert(name);
      }

      if (!classification.empty()) {
        set<string> new_props;
        set<Property_Ref, less<Property_Ref> > fromClass;
        fromClass = PropFactory::ofClassification(classification);
        set<Property_Ref, less<Property_Ref> >::iterator it;
        for (it = fromClass.begin(); it != fromClass.end(); ++it) {
          //cout << "\tCLASS " << it->short_name << endl;
          new_props.insert(it->short_name);
        }

        if (name.empty()) {
          current_props = new_props;
        } else {
          vector<string> results(current_props.size() + new_props.size());
          vector<string>::iterator result, results_end;
          results_end = set_intersection(
                  new_props.begin(), new_props.end(),
                  current_props.begin(), current_props.end(),
                  results.begin());
          current_props.clear();
          for (result = results.begin(); result != results_end; ++result) {
            current_props.insert(*result);
          }
        }
      }

      if (!type.empty()) {
        set<string> new_props;
        set<Property_Ref, less<Property_Ref> > fromType;
        fromType = PropFactory::ofStorageType(type);
        set<Property_Ref, less<Property_Ref> >::iterator it;
        for (it = fromType.begin(); it != fromType.end(); ++it) {
          //cout << "\tTYPE " << it->short_name << endl;
          new_props.insert(it->short_name);
        }

        if (name.empty() && classification.empty()) {
          current_props = new_props;
        } else {
          vector<string> results(current_props.size() + new_props.size());
          vector<string>::iterator result, results_end;
          results_end = set_intersection(
                  new_props.begin(), new_props.end(),
                  current_props.begin(), current_props.end(),
                  results.begin());
          current_props.clear();
          for (result = results.begin(); result != results_end; ++result) {
            current_props.insert(*result);
          }
        }
      }

      if (!indexedBy.empty()) {
        set<string> new_props;
        set<Property_Ref, less<Property_Ref> > fromIndex;
        fromIndex = PropFactory::ofIndex(indexedBy);
        set<Property_Ref, less<Property_Ref> >::iterator it;
        for (it = fromIndex.begin(); it != fromIndex.end(); ++it) {
          //cout << "\tINDEXEDBY " << it->short_name << endl;
          new_props.insert(it->short_name);
        }

        if (name.empty() && classification.empty() && type.empty()) {
          current_props = new_props;
        } else {
          vector<string> results(current_props.size() + new_props.size());
          vector<string>::iterator result, results_end;
          results_end = set_intersection(
                  new_props.begin(), new_props.end(),
                  current_props.begin(), current_props.end(),
                  results.begin());
          current_props.clear();
          for (result = results.begin(); result != results_end; ++result) {
            current_props.insert(*result);
          }
        }
      }

      // by this time we've determined what prop keys are valid for
      // this Property element.  add them all to the description
      set<string>::iterator it;
      for (it = current_props.begin(); it != current_props.end(); ++it) {
        //cout << "\t\t" << *it << endl;
        description->properties.insert(*it);
      }
    }
    p_panels.push_back(description);
    p_panelsByName[description->name] = description;
  }
}

