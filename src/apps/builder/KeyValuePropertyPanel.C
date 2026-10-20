#include "util/UnitDisplay.H"
#include <algorithm>
#include <cmath>
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
#include "tdat/PropVector.H"

#include "dsm/IPropCalculation.H"
#include "dsm/PropFactory.H"

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxNonBoldLabel.H"
#include "wxgui/ewxStaticText.H"
#include <wx/clipbrd.h>
#include <wx/menu.h>

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
static const char *GROUPS[] = {"Final value", "Average over run (\xC2\xB1 s.d.)",
                               "Total", "Components", "Thermochemistry",
                               "Solvation", "Derived", ""};
static const char *INIFILE = "wxbuilder.ini";


namespace {

struct Row { string key, value, units, group; size_t order; };

//  n significant figures without an exponent, so a large energy reads
//  -64995, not -6.4995e+04.
string sigFigs(double v, int n)
{
  if (v == 0.0 || !std::isfinite(v)) return v == 0.0 ? "0" : "-";
  int dec = std::max(0, n - 1 - (int)std::floor(std::log10(std::fabs(v))));
  return string(wxString::Format("%.*f", dec, v).ToUTF8());
}

//  GROMACS logs no fluctuations, so the s.d. is that of the logged steps;
//  the average is the run's own (the log's AVERAGES block) when it has one.
string meanSd(double mean, double sd)
{
  if (!(sd > 0.0)) return sigFigs(mean, 5);
  int dec = std::max(0, 1 - (int)std::floor(std::log10(sd)));
  return string(wxString::Format("%.*f \xC2\xB1 %.*f", dec, mean, dec, sd).ToUTF8());
}

const PropVector *vectorOf(IPropCalculation *calc, const string& key)
{
  const PropVector *v = dynamic_cast<PropVector*>(calc->getProperty(key));
  return v && v->rows() > 0 ? v : 0;
}

//  The MD Summary rows (#334): a minimisation has only its end state; a
//  dynamics run adds the run average and s.d. of each quantity.
void mdSummaryRows(IPropCalculation *calc, vector<Row>& rows)
{
  struct Q { const char *label, *vec, *avg; };
  static const Q QS[] = {
    {"Potential Energy", "MDPEVEC", "MDPEAVG"},
    {"Kinetic Energy", "MDKEVEC", "MDKEAVG"},
    {"Total Energy", "TEVEC", "MDTEAVG"},
    {"Temperature", "TEMPVEC", "MDTEMPAVG"},
    {"Pressure", "PRESSVEC", "MDPRESAVG"},
    {"Volume", "MDVOLVEC", "MDVOLAVG"},
    {"Density", "MDDENSVEC", "MDDENSAVG"}};
  const bool dynamics = vectorOf(calc, "MDKEVEC") != 0;
  size_t order = 0;
  for (int pass = 0; pass < 2; pass++) {
    if (pass == 1 && !dynamics) break;
    for (const Q& q : QS) {
      if (!dynamics && string(q.vec) != "MDPEVEC") continue;
      const PropVector *vec = vectorOf(calc, q.vec);
      if (!vec) continue;
      const vector<double>& vals = vec->values();
      string units = vec->units() == "NA" ? "" : unitLabel(vec->units());
      if (string(q.vec) == "MDVOLVEC") units = "nm\xC2\xB3";
      if (string(q.vec) == "MDDENSVEC") units = "kg/m\xC2\xB3";
      Row row;
      row.key = q.label;
      row.units = units;
      row.order = order++;
      if (pass == 0) {
        row.group = "Final value";
        row.value = sigFigs(vals.back(), 5);
      } else {
        row.group = GROUPS[1];
        double mean = 0, sd = 0;
        const size_t n = vals.size();
        for (double x : vals) mean += x;
        mean /= n;
        if (n > 1) {
          for (double x : vals) sd += (x - mean) * (x - mean);
          sd = std::sqrt(sd / (n - 1));
        }
        if (TProperty *avg = calc->getProperty(q.avg)) mean = avg->scalarize();
        row.value = meanSd(mean, sd);
      }
      rows.push_back(row);
    }
    if (pass == 0 && !dynamics) {
      if (TProperty *f = calc->getProperty("FMAX")) {
        Row row;
        row.key = "Maximum Force";
        row.value = sigFigs(f->scalarize(), 5);
        row.units = "kJ/mol/nm";
        row.group = "Final value";
        row.order = order++;
        rows.push_back(row);
      }
    }
  }
}

}  // namespace


//  A GROMACS run is shown by the MD Summary panel; the one-value Energies
//  and Pressure panels would repeat its numbers.
bool KeyValuePropertyPanel::isRelevant(IPropCalculation *calc) const
{
  const string name = getName();
  if (name != "Energies" && name != "Pressure") return true;
  try {
    return calc->getProperty("MDPEVEC") == 0;
  } catch (...) {
    return true;
  }
}


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
  if (getName() == "MD Summary") {
    mdSummaryRows(getCalculation(), rows);
    grouped = true;
    names.clear();
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
    wxString valueStr = valueLabel(tprop->scalarize(), 10);
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
  //  The panel's own sizer has no border, so values touched the edge.
  const int PAD = 8;
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
      sizer->Add(headSizer, 0, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, PAD);
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
        sizer->Add(key, 1, wxEXPAND|wxALIGN_CENTER_VERTICAL|wxLEFT, PAD);
        wxWindow *value = new ewxStaticText(this, wxID_ANY,
                                      wxString::FromUTF8(row->value));
        sizer->Add(value, 0, wxALIGN_RIGHT|wxALIGN_CENTER_VERTICAL);
        wxWindow *unit = new ewxNonBoldLabel(this, wxID_ANY,
                                         wxString::FromUTF8(row->units));
        sizer->Add(unit, 0, left|wxRIGHT, PAD);
        p_rowWindows.push_back({row->key, value, unit, row->value, row->units});
        const string rowKey = row->key;
        for (wxWindow *w : {static_cast<wxWindow*>(key), value, unit})
          w->Bind(wxEVT_CONTEXT_MENU, [this, rowKey](wxContextMenuEvent&) {
            copyMenu(rowKey);
          });
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
  //  Right-click on the panel's background: copy every row.
  Bind(wxEVT_CONTEXT_MENU, [this](wxContextMenuEvent&) { copyMenu(""); });
}


wxString KeyValuePropertyPanel::copyText(const std::string& key, bool unit) const
{
  for (const RowWindows& r : p_rowWindows)
    if (r.key == key)
      return wxString::FromUTF8(r.valueText +
                                (unit && !r.unitText.empty() ? " " + r.unitText
                                                             : std::string()));
  return wxEmptyString;
}


wxString KeyValuePropertyPanel::copyAllText() const
{
  wxString out;
  for (const RowWindows& r : p_rowWindows)
    out += wxString::FromUTF8(r.key + "\t" + r.valueText + "\t" + r.unitText + "\n");
  return out;
}


void KeyValuePropertyPanel::copyMenu(const std::string& key)
{
  enum { ID_VALUE = wxID_HIGHEST + 1, ID_WITH_UNIT, ID_ALL };
  wxMenu menu;
  if (!key.empty()) {
    menu.Append(ID_VALUE, _("Copy value"));
    menu.Append(ID_WITH_UNIT, _("Copy value with unit"));
  }
  menu.Append(ID_ALL, _("Copy all"));
  const int id = GetPopupMenuSelectionFromUser(menu);
  wxString text = id == ID_VALUE ? copyText(key, false)
                : id == ID_WITH_UNIT ? copyText(key, true)
                : id == ID_ALL ? copyAllText() : wxString();
  if (text.empty() || !wxTheClipboard->Open()) return;
  wxTheClipboard->SetData(new wxTextDataObject(text));
  wxTheClipboard->Close();
}

