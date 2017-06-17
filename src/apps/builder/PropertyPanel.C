
#include "util/ErrMsg.H"

#include "dsm/IPropCalculation.H"

#include "PropertyPanel.H"
#include "PropertyPanelFactory.H"


IMPLEMENT_DYNAMIC_CLASS(PropertyPanel, ewxPanel)


set<PropertyPanel*> PropertyPanel::p_allPropertyPanels;
map<string, set<PropertyPanel*> > PropertyPanel::p_propertyPanels;


PropertyPanel::PropertyPanel()
  : ewxPanel(),
    p_description(NULL),
    p_calculation(NULL),
    p_needsInitialization(true),
    p_needsRefresh(false)
{
}


PropertyPanel::PropertyPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : ewxPanel(),
    p_description(NULL),
    p_calculation(NULL),
    p_needsInitialization(true),
    p_needsRefresh(false)
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool PropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!ewxPanel::Create(parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("PropertyPanel creation failed") );
    return false;
  }

  p_calculation = calculation;
  p_propertyPanels[calculation->getURL().toString()].insert(this);
  p_allPropertyPanels.insert(this);

  return true;
}


PropertyPanel::~PropertyPanel()
{
  // due to isRelevant needing a PropertyPanel instance, but otherwise
  // Create has not been called, we need to check here for a valid calc
  if (p_calculation) {
    p_propertyPanels[p_calculation->getURL().toString()].erase(this);
    p_allPropertyPanels.erase(this);
  }
}


bool PropertyPanel::Show(bool show)
{
  if (show) {
    ensureInitialized();
  }
  return ewxPanel::Show(show);
}


string PropertyPanel::getName() const
{
  return getDescription() ? getDescription()->name : "";
}


string PropertyPanel::getClass() const
{
  return getDescription() ? getDescription()->className : "";
}


bool PropertyPanel::isConsumer() const
{
  return getDescription() ? getDescription()->consumer : false;
}


bool PropertyPanel::allowPin() const
{
  return getDescription() ? getDescription()->allowPin : true;
}


set<string> PropertyPanel::getPropertyNames() const
{
  return getDescription() ? getDescription()->properties : set<string>();
}


/**
 * Last step in the 3-step creation process.
 * First, a PropertyPanel instance is created with wxCreateDynamicObject.
 * Second, it is modestly fleshed out with wxWidgets using Create().
 * Lastly, any real widgets are created using this function.
 * Should only call this function once.
 * @see p_needsInitialization
 */
void PropertyPanel::initialize()
{
}


void PropertyPanel::ensureInitialized()
{
  if (p_needsInitialization) {
    initialize();
    p_needsInitialization = false;
  }
  if (p_needsRefresh) {
    refresh();
    p_needsRefresh = false;
  }
}


/**
 * Update any widgets that already exist and create others as needed.
 * Less extreme than initialize, since initialize should only be called once.
 */
void PropertyPanel::refresh()
{
}


void PropertyPanel::stateChange(const ResourceDescriptor::RUNSTATE& state)
{
  if (IsShown()) {
    refresh();
  } else {
    p_needsRefresh = true;
  }
}


void PropertyPanel::propertyUpdate(const string &propKey)
{
  if (IsShown()) {
    refresh();
  } else {
    p_needsRefresh = true;
  }
}


void PropertyPanel::attributeUpdate(const string &attrKey)
{
  if (IsShown()) {
    refresh();
  } else {
    p_needsRefresh = true;
  }
}


bool PropertyPanel::isRelevant(IPropCalculation *propCalc) const
{
  return true;
}


PropertyPanelDescription* PropertyPanel::getDescription() const
{
  return p_description;
}


IPropCalculation* PropertyPanel::getCalculation() const
{
  return p_calculation;
}


bool PropertyPanel::needsInitialization() const
{
  return p_needsInitialization;
}


bool PropertyPanel::needsRefresh() const
{
  return p_needsRefresh;
}


/**
 * Retrieve all panels associated with the given url and with the given
 * name (if a panel was instanceable, there might be more than one.)
 * Either the name or both the url and name may be omitted.
 */
set<PropertyPanel*> PropertyPanel::getPanels(const string& url,
                                             const string& name)
{
  set<PropertyPanel*> ret;

  if (url.empty()) {
    ret = p_allPropertyPanels;
  } else {
    ret = p_propertyPanels[url];
  }

  if (!name.empty()) {
    set<PropertyPanel*>::iterator panel;
    for (panel = ret.begin(); panel != ret.end(); /*no inc*/) {
      if ((*panel)->getName() != name) {
        ret.erase(panel++);
      } else {
        ++panel;
      }
    }
  }

  return ret;
}


/**
 * Retrieve all panels associated with the given url and property key.
 * Either the url or both the url and key may be omitted.
 */
set<PropertyPanel*> PropertyPanel::getPanelsForPropertyName(const string& url,
                                                            const string& key)
{
  set<PropertyPanel*> ret;

  if (url.empty()) {
    ret = p_allPropertyPanels;
  } else {
    ret = p_propertyPanels[url];
  }

  if (!key.empty()) {
    set<PropertyPanel*>::iterator panel;
    for (panel = ret.begin(); panel != ret.end(); /*no inc*/) {
      set<string> keys = (*panel)->getPropertyNames();
      if (keys.find(key) == keys.end()) {
        ret.erase(panel++);
      } else {
        ++panel;
      }
    }
  }

  return ret;
}


/**
 * Destroy all panels associated with the given url and/or name.
 */
void PropertyPanel::removePanels(const string& url, const string& name)
{
  set<PropertyPanel*> panels = getPanels(url,name);
  set<PropertyPanel*>::iterator panelIt;
  for (panelIt = panels.begin(); panelIt != panels.end(); ++panelIt) {
    PropertyPanel *panel = *panelIt;
    string name = panel->getName();
    panel->Destroy();
  }
}


bool PropertyPanel::panelExists(const string& url, const string& name)
{
  set<PropertyPanel*> panels;

  if (url.empty()) {
    panels = p_allPropertyPanels;
  } else {
    panels = p_propertyPanels[url];
  }

  set<PropertyPanel*>::iterator panel;
  for (panel = panels.begin(); panel != panels.end(); ++panel) {
    if ((*panel)->getName() == name) {
      return true;
    }
  }

  return false;
}


void PropertyPanel::setDescription(PropertyPanelDescription *description)
{
  p_description = description;
}

