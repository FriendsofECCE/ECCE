#include <iostream>
  using std::cout;
  using std::endl;

#include "wx/link.h"
#include "wx/log.h"
#include "wx/sizer.h"

#include "util/PreferenceLabels.H"

#include "tdat/TProperty.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"

#include "KeyValuePropertyPanel.H"


wxFORCE_LINK_THIS_MODULE(KeyValuePropertyPanel)


IMPLEMENT_DYNAMIC_CLASS(KeyValuePropertyPanel, PropertyPanel)


KeyValuePropertyPanel::KeyValuePropertyPanel()
  : PropertyPanel()
{
}


KeyValuePropertyPanel::KeyValuePropertyPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel()
{
  Create(calculation, parent, id, pos, size, style, name);
}


bool KeyValuePropertyPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("KeyValuePropertyPanel creation failed") );
    return false;
  }

  if (!GetSizer()) {
    SetSizer(new wxFlexGridSizer(3, 0, 5)); // three columns
  }

  return true;
}


KeyValuePropertyPanel::~KeyValuePropertyPanel()
{
}


void KeyValuePropertyPanel::refresh()
{
  DestroyChildren();

  // for each TProperty, if found, insert a key/value pair into a map
  // this automatically sorts the key/values by key
  set<string> names = getPropertyNames();
  set<string>::iterator nameIt;
  map<string,string> props, units;
  for (nameIt = names.begin(); nameIt != names.end(); nameIt++) {
    TProperty *tprop = getCalculation()->getProperty(*nameIt);
    const Property_Ref *propRef = PropFactory::getPropRef(*nameIt);
    if (tprop) {
      string key;
      if (propRef) {
        key = propRef->long_name;
      } else {
        key = nameIt->c_str();
      }
      wxString valueStr = 
          wxString::Format (_T(PrefLabels::PRECISEDOUBLEFORMAT), 
          tprop->scalarize());
      props[key] = valueStr.c_str();
      units[key] = tprop->units()=="NA"? "": tprop->units();
    }
  }

  // now iterate over the map and create the sizer
  map<string,string>::iterator propIt;
  for (propIt = props.begin(); propIt != props.end(); ++propIt) {
    string key = propIt->first;
    GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, key),
            0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
    GetSizer()->Add(new ewxStaticText(this, wxID_ANY, props[key]),
            0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
    GetSizer()->Add(new ewxNonBoldLabel(this, wxID_ANY, units[key]),
            0, wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL);
  }

  GetSizer()->Fit(this);
  GetSizer()->SetSizeHints(this);
}


void KeyValuePropertyPanel::initialize()
{
  refresh();
}

