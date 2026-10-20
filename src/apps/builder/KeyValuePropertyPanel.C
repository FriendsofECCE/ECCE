#include "util/UnitDisplay.H"
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

#include "wx/dcclient.h"

namespace {
//  A group heading's fold arrow: pointing down when open, right when folded.
class FoldArrow : public wxWindow
{
  public:
    FoldArrow(wxWindow *parent, bool open, int height)
      : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxSize(height, height)),
        p_open(open)
    {
      SetMinSize(wxSize(height, height));
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      Bind(wxEVT_PAINT, [this](wxPaintEvent&) {
        wxPaintDC dc(this);
        dc.SetBackground(wxBrush(GetParent()->GetBackgroundColour()));
        dc.Clear();
        const wxSize s = GetClientSize();
        const double h = s.y * 0.45, c = s.x / 2.0, m = s.y / 2.0;
        wxPoint p[3];
        if (p_open) {
          p[0] = wxPoint(int(c - h / 2), int(m - h / 4));
          p[1] = wxPoint(int(c + h / 2), int(m - h / 4));
          p[2] = wxPoint(int(c), int(m + h / 4 + 1));
        } else {
          p[0] = wxPoint(int(c - h / 4), int(m - h / 2));
          p[1] = wxPoint(int(c - h / 4), int(m + h / 2));
          p[2] = wxPoint(int(c + h / 4 + 1), int(m));
        }
        const wxColour fg = GetParent()->GetForegroundColour();
        dc.SetPen(wxPen(fg));
        dc.SetBrush(wxBrush(fg));
        dc.DrawPolygon(3, p);
      });
    }
  private:
    bool p_open;
};
}


//  The least a label may shrink to: about ten characters, then "...".
static int labelMin(wxWindow *label)
{
  return 10 * label->GetCharWidth();
}


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
  //  The label column takes the slack and gives way first (ellipsis), so
  //  the value and its unit are never what a narrow pane cuts off.
  if (wxFlexGridSizer *grid = dynamic_cast<wxFlexGridSizer*>(GetSizer()))
    if (!grid->IsColGrowable(0)) grid->AddGrowableCol(0, 1);

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
  //  Clear(true), not DestroyChildren(): the headings' spacers are not
  //  windows, and left in the sizer they shift every later row by a cell.
  GetSizer()->Clear(true);
  p_rows.clear();
  p_rowWindows.clear();

  struct Row { string key, value, units, group; size_t order; };
  vector<Row> rows;
  string thermoTemp, thermoPres, solvModel;
  bool grouped = false;
  set<string> names = getPropertyNames();
  // MOPAC publishes its heat of formation as TE too; listing that number
  // again as "Total Energy" would call it something it is not.
  if (names.count("HFENERGY") && names.count("TE")) {
    TProperty *hof = getCalculation()->getProperty("HFENERGY");
    TProperty *te = getCalculation()->getProperty("TE");
    if (hof && te && hof->scalarize() == te->scalarize() &&
        hof->units() == te->units())
      names.erase("TE");
  }
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
    string units = tprop->units()=="NA"? "": unitLabel(tprop->units());
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
      //  The fold arrow is drawn, not a "▾" in the label: wxMSW measured
      //  that glyph narrower than the native control drew it (a fallback
      //  font), and the control wrapped the title out of sight.
      wxBoxSizer *headSizer = new wxBoxSizer(wxHORIZONTAL);
      const wxString headText = wxString::FromUTF8(title + (folded ? " (" +
              std::to_string(members.size()) + ")" : string()));
      ewxStaticText *head = new ewxStaticText(this, wxID_ANY, headText,
          wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
      head->SetToolTip(headText);
      head->SetFont(head->GetFont().Bold());
      FoldArrow *arrow = new FoldArrow(this, !folded, head->GetCharHeight());
      headSizer->Add(arrow, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 3);
      head->SetMinSize(wxSize(labelMin(head), -1));
      headSizer->Add(head, 1, wxALIGN_CENTER_VERTICAL);
      for (wxWindow *w : {static_cast<wxWindow*>(arrow),
                          static_cast<wxWindow*>(head)}) {
        w->SetCursor(wxCursor(wxCURSOR_HAND));
        w->Bind(wxEVT_LEFT_DOWN, [this, name](wxMouseEvent&) {
          CallAfter([this, name]() { toggleGroup(name); });
        });
      }
      sizer->Add(headSizer, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL);
      sizer->AddSpacer(0);
      sizer->AddSpacer(0);
      p_rows.push_back("H " + text);
    }
    if (!folded) {
      for (const Row *row : members) {
        ewxNonBoldLabel *key = new ewxNonBoldLabel(this, wxID_ANY,
            wxString::FromUTF8(row->key), wxDefaultPosition, wxDefaultSize,
            wxST_ELLIPSIZE_END);
        key->SetToolTip(wxString::FromUTF8(row->key));
        key->SetMinSize(wxSize(labelMin(key), -1));
        sizer->Add(key, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL);
        wxWindow *value = new ewxStaticText(this, wxID_ANY,
                                      wxString::FromUTF8(row->value));
        sizer->Add(value, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
        wxWindow *unit = new ewxNonBoldLabel(this, wxID_ANY,
                                         wxString::FromUTF8(row->units));
        sizer->Add(unit, 0, left);
        p_rowWindows.push_back({row->key, value, unit});
        p_rows.push_back("R " + row->key + " | " + row->value + " | " +
                         row->units);
      }
    }
    if (!grouped) break;
  }

  sizer->Fit(this);
  sizer->SetSizeHints(this);
}


std::vector<std::string> KeyValuePropertyPanel::rowFit() const
{
  std::vector<std::string> out;
  for (const RowWindows& r : p_rowWindows)
    out.push_back(r.key + " | " +
                  std::to_string(r.value->GetRect().GetRight()) + " | " +
                  std::to_string(r.unit->GetRect().GetRight()) + " | " +
                  std::to_string(GetClientSize().x));
  return out;
}


void KeyValuePropertyPanel::initialize()
{
  refresh();
}

