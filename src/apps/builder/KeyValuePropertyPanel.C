#include <algorithm>
#include <vector>
#include <iostream>
  using std::cout;
  using std::endl;

#include "wx/link.h"
#include "wx/log.h"
#include "wx/sizer.h"

#include "util/PreferenceLabels.H"

#include "tdat/TProperty.H"
#include "tdat/PropString.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxConfig.H"
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


//  Group names are the "group" column of data/client/config/properties, in
//  display order.  A property with no (or an unknown) group goes last.
static const char *GROUPS[] = {"Total", "Components", "Thermochemistry",
                               "Solvation", "Derived", ""};
static const char *INIFILE = "wxbuilder.ini";


bool KeyValuePropertyPanel::isFolded(const string& group) const
{
  bool folded = (group == "Components");
  ewxConfig::getConfig(INIFILE)->Read("Energies/Fold/" + group, &folded);
  return folded;
}


bool KeyValuePropertyPanel::toggleGroup(const string& group)
{
  bool folded = !isFolded(group);
  ewxConfig::getConfig(INIFILE)->Write("Energies/Fold/" + group, folded);
  refresh();
  if (GetParent()) GetParent()->Layout();
  return folded;
}


void KeyValuePropertyPanel::refresh()
{
  DestroyChildren();
  p_rows.clear();

  struct Row { string key, value, units, group; size_t order; };
  vector<Row> rows;
  string thermoTemp, thermoPres, solvModel;
  bool grouped = false;
  set<string> names = getPropertyNames();
  set<string>::iterator nameIt;
  for (nameIt = names.begin(); nameIt != names.end(); nameIt++) {
    TProperty *tprop = getCalculation()->getProperty(*nameIt);
    const Property_Ref *propRef = PropFactory::getPropRef(*nameIt);
    if (!tprop) continue;
    if (*nameIt == "SOLVMODEL") {
      // A string: shown in the Solvation heading, not as a row.
      if (PropString *ps = dynamic_cast<PropString*>(tprop)) solvModel = ps->value();
      continue;
    }
    string units = tprop->units()=="NA"? "": tprop->units();
    wxString valueStr = wxString::Format(PrefLabels::PRECISEDOUBLEFORMAT,
                                         tprop->scalarize());
    if (*nameIt == "THERMOTEMP") {
      // Shown in the Thermochemistry heading, not as a row.
      thermoTemp = string(wxString::Format("%g", tprop->scalarize()).c_str()) + " " +
                   (units == "Kelvin" ? string("K") : units);
      continue;
    }
    if (*nameIt == "THERMOPRES") {
      thermoPres = string(wxString::Format("%g", tprop->scalarize()).c_str()) + " " +
                   (units.empty() ? string("atm") : units);
      continue;
    }
    Row row;
    row.key = propRef ? propRef->long_name : *nameIt;
    row.value = valueStr.c_str();
    row.units = units;
    row.group = propRef ? propRef->group : "";
    if (std::find(std::begin(GROUPS), std::end(GROUPS), row.group) ==
        std::end(GROUPS))
      row.group = "";
    row.order = propRef ? propRef->order : 0;
    grouped = grouped || !row.group.empty();
    rows.push_back(row);
  }

  //  Without groups (the S**2 panel) the old order is kept: alphabetical,
  //  "Total..." last.  With them, each group lists in the file's order.
  if (!grouped) {
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
      bool ta = a.key.compare(0, 5, "Total") == 0;
      bool tb = b.key.compare(0, 5, "Total") == 0;
      return ta != tb ? tb : a.key < b.key;
    });
  } else {
    std::stable_sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
      return a.order < b.order;
    });
  }

  wxSizer *sizer = GetSizer();
  const long left = wxALIGN_LEFT|wxALIGN_CENTER_VERTICAL;
  for (const char *groupName : GROUPS) {
    const string name = groupName;
    vector<const Row*> members;
    for (const Row& row : rows)
      if (!grouped || row.group == name) members.push_back(&row);
    if (members.empty()) continue;

    bool folded = false;
    if (grouped) {
      folded = isFolded(name);
      string title = name.empty() ? "Other" : name;
      if (name == "Thermochemistry" && (!thermoTemp.empty() || !thermoPres.empty())) {
        string conds = thermoTemp;
        if (!thermoPres.empty()) conds += (conds.empty() ? "" : ", ") + thermoPres;
        title += " (" + conds + ")";
      }
      if (name == "Solvation" && !solvModel.empty())
        title += " (" + solvModel + ")";
      string text = (folded ? "\xE2\x96\xB8 " : "\xE2\x96\xBE ") + title;
      if (folded) text += " (" + std::to_string(members.size()) + ")";
      ewxStaticText *head = new ewxStaticText(this, wxID_ANY,
                                              wxString::FromUTF8(text));
      head->SetFont(head->GetFont().Bold());
      head->SetCursor(wxCursor(wxCURSOR_HAND));
      head->Bind(wxEVT_LEFT_DOWN, [this, name](wxMouseEvent&) {
        CallAfter([this, name]() { toggleGroup(name); });
      });
      sizer->Add(head, 0, left);
      sizer->AddSpacer(0);
      sizer->AddSpacer(0);
      p_rows.push_back("H " + text);
    }
    if (!folded) {
      for (const Row *row : members) {
        sizer->Add(new ewxNonBoldLabel(this, wxID_ANY, row->key), 0, left);
        sizer->Add(new ewxStaticText(this, wxID_ANY, row->value),
                0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
        sizer->Add(new ewxNonBoldLabel(this, wxID_ANY, row->units), 0, left);
        p_rows.push_back("R " + row->key + " | " + row->value + " | " +
                         row->units);
      }
    }
    if (!grouped) break;
  }

  sizer->Fit(this);
  sizer->SetSizeHints(this);
}


void KeyValuePropertyPanel::initialize()
{
  refresh();
}

