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
  string group;   // heading in the Properties list; "Other" if none
  set<string> properties;
};


class PropertyPanelFactory
{
  public:
    static PropertyPanelFactory& getPropertyPanelFactory();

    set<string> getPanelNamesForProperties(const vector<string>& keys);
    bool isRelevant(const string& key);

    PropertyPanel* createPropertyPanel(const string& name);

    // Position in PropertyPanelDescriptor.xml (-1 if unknown) and its group.
    int indexOf(const string& name) const;
    string groupOf(const string& name) const;

  protected:
    PropertyPanelFactory();
    void parsePropertyPanelDescriptorFile();
    void createPropertyPanelDescriptions();

  private:
    static PropertyPanelFactory *p_singleton;

    DOMDocument *p_doc;

    vector<PropertyPanelDescription*> p_panels; // indexed by doc order
    map<string, PropertyPanelDescription*> p_panelsByName; // indexed by name
};

#endif // PROPERTYPANELFACTORY_H_
