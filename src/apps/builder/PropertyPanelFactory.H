#ifndef PROPERTYPANELFACTORY_H_
#define PROPERTYPANELFACTORY_H_

#include <set>
  using std::set;
#include <map>
  using std::map;
#include <vector>
  using std::vector;

#include <xercesc/dom/DOMDocument.hpp>
  using namespace xercesc;

#include "util/ParseException.H"


class PropertyPanel;


struct PropertyPanelDescription {
  string name;
  string className;
  bool consumer;
  bool allowPin;
  set<string> properties;
};


class PropertyPanelFactory
{
  public:
    static PropertyPanelFactory& getPropertyPanelFactory();

    set<string> getPanelNamesForProperties(const vector<string>& keys);
    bool isRelevant(const string& key);

    PropertyPanel* createPropertyPanel(const string& name);

  protected:
    PropertyPanelFactory();
    void parsePropertyPanelDescriptorFile() throw (ParseException);
    void createPropertyPanelDescriptions();

  private:
    static PropertyPanelFactory *p_singleton;

    DOMDocument *p_doc;

    vector<PropertyPanelDescription*> p_panels; // indexed by doc order
    map<string, PropertyPanelDescription*> p_panelsByName; // indexed by name
};

#endif // PROPERTYPANELFACTORY_H_
