/**
 * @file
 *
 * Preferences > Windows and tabs (#983 stage 2): a preset dropdown and a
 * Customize... table of one Tab/Window row per tool.  The setting itself
 * is util/WindowLayout; this page only shows and edits it.
 */
#include <map>
#include <sstream>

#include "wx/wx.h"
#include "wx/timer.h"

#include "util/MqttLink.H"
#include "util/WindowLayout.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxNotebook.H"
#include "wxgui/GlobalPrefs.H"
#include "wxgui/WindowShot.H"

namespace {
const int PAD = 5;

// Dropdown order; Custom is shown only as the result of editing the table.
const WindowLayout::Preset PRESETS[] = {
  WindowLayout::Normal, WindowLayout::Small, WindowLayout::Wide,
  WindowLayout::Custom
};


// The Customize... table: a Tab/Window choice per tool, filled from the
// current setting.
class LayoutTable : public wxDialog
{
public:
  LayoutTable(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, _("Windows and tabs"))
  {
    wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
    top->Add(new ewxStaticText(this, wxID_ANY,
        _("Each tool opens as a tab of the Organizer's window or in a window "
          "of its own. Changing a row makes the layout Custom.")),
        0, wxALL, 2*PAD);
    wxFlexGridSizer *grid = new wxFlexGridSizer(2, PAD, 4*PAD);
    std::map<std::string, bool> rows = WindowLayout::rows();
    for (const WindowLayout::Tool& t : WindowLayout::tools()) {
      grid->Add(new ewxStaticText(this, wxID_ANY, wxString::FromUTF8(t.label)),
                0, wxALIGN_CENTER_VERTICAL);
      ewxChoice *c = new ewxChoice(this, wxID_ANY);
      c->Append(_("Tab"));
      c->Append(_("Window"));
      c->SetSelection(rows[t.key] ? 0 : 1);
      grid->Add(c, 0, wxALIGN_CENTER_VERTICAL);
      p_choices[t.key] = c;
    }
    top->Add(grid, 0, wxLEFT|wxRIGHT|wxBOTTOM, 2*PAD);
    top->Add(CreateButtonSizer(wxOK|wxCANCEL), 0, wxGROW|wxALL, PAD);
    SetSizerAndFit(top);
    Centre();
  }

  bool setRow(const std::string& key, bool tab)
  {
    if (!p_choices.count(key)) return false;
    p_choices[key]->SetSelection(tab ? 0 : 1);
    return true;
  }

  std::map<std::string, bool> rows() const
  {
    std::map<std::string, bool> r;
    for (const auto& kv : p_choices) r[kv.first] = kv.second->GetSelection() == 0;
    return r;
  }

private:
  std::map<std::string, ewxChoice*> p_choices;
};

}


void GlobalPrefs::createWindowsPage(wxWindow* page)
{
  wxBoxSizer* outer = new wxBoxSizer(wxVERTICAL);
  wxStaticBoxSizer* box = new wxStaticBoxSizer(wxVERTICAL, page,
                                               _("Windows and tabs"));
  wxWindow* sb = box->GetStaticBox();
  wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
  row->Add(new ewxStaticText(sb, wxID_ANY, _("Layout:")), 0,
           wxALIGN_CENTER_VERTICAL|wxRIGHT, PAD);
  p_layout = new ewxChoice(sb, wxID_ANY);
  p_layout->Append(_("Normal"));
  p_layout->Append(_("Small"));
  p_layout->Append(_("Wide"));
  p_layout->Append(_("Custom"));
  p_layout->Bind(wxEVT_CHOICE, &GlobalPrefs::OnLayoutChoice, this);
  row->Add(p_layout, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, PAD);
  ewxButton* customize = new ewxButton(sb, wxID_ANY, _("Customize..."));
  customize->Bind(wxEVT_BUTTON, &GlobalPrefs::OnLayoutCustomize, this);
  row->Add(customize, 0, wxALIGN_CENTER_VERTICAL);
  box->Add(row, 0, wxALL, PAD);

  p_layoutRows = new ewxStaticText(sb, wxID_ANY, "");
  box->Add(p_layoutRows, 0, wxLEFT|wxRIGHT|wxBOTTOM, PAD);

  box->Add(new ewxStaticText(sb, wxID_ANY,
      _("Normal: tools as tabs (later with one combined Set-up tab; for now "
        "as Small).\nSmall: one tool per tab, full width (small screens, X "
        "forwarding).\nWide: every tool in a window of its own (large or "
        "several monitors).\nApplies to tools opened after the change.")),
      0, wxLEFT|wxRIGHT|wxBOTTOM, PAD);
  if (!MqttLink::localDelivery())
    box->Add(new ewxStaticText(sb, wxID_ANY,
        _("Tabs need the tools in one program (ECCE_ONE_PROCESS=0 is set); until "
          "then every tool opens in its own window.")),
        0, wxLEFT|wxRIGHT|wxBOTTOM, PAD);
  outer->Add(box, 0, wxGROW|wxALL, PAD);
  page->SetSizer(outer);
  updateWindowsPage();
}


void GlobalPrefs::updateWindowsPage()
{
  if (!p_layout) return;
  WindowLayout::Preset p = WindowLayout::preset();
  for (int i = 0; i < 4; i++)
    if (PRESETS[i] == p) p_layout->SetSelection(i);
  wxString tabs, windows;
  std::map<std::string, bool> rows = WindowLayout::rows();
  for (const WindowLayout::Tool& t : WindowLayout::tools()) {
    wxString& list = rows[t.key] ? tabs : windows;
    list += (list.empty() ? "" : ", ") + wxString::FromUTF8(t.label);
  }
  p_layoutRows->SetLabel(_("Tabs: ") + (tabs.empty() ? _("none") : tabs) +
                         "\n" + _("Windows: ") +
                         (windows.empty() ? _("none") : windows));
  if (p_layoutRows->GetContainingSizer()) Layout();
}


void GlobalPrefs::OnLayoutChoice(wxCommandEvent& event)
{
  int i = p_layout->GetSelection();
  if (i < 0 || i > 3) return;
  if (PRESETS[i] == WindowLayout::Custom) {
    OnLayoutCustomize(event);
    return;
  }
  WindowLayout::setPreset(PRESETS[i]);
  updateWindowsPage();
  updateResetButton();
}


void GlobalPrefs::OnLayoutCustomize(wxCommandEvent&)
{
  LayoutTable table(this);
  if (table.ShowModal() == wxID_OK) WindowLayout::setRows(table.rows());
  updateWindowsPage();
  updateResetButton();
}


bool GlobalPrefs::Show(bool show)
{
  // A tab's "Always open in a window" changes the setting while this window
  // is hidden.
  if (show) updateWindowsPage();
  return ewxFrame::Show(show);
}


//  Test hook, "prefs windows show|preset NAME|row KEY tab|window [SNAP]|snap P":
//  drives the page and the Customize table as clicks do.
string GlobalPrefs::windowsTest(const string& line)
{
  std::istringstream in(line);
  string op, a, b, snap;
  in >> op >> a >> b >> snap;
  for (size_t i = 0; i < p_book->GetPageCount(); i++)
    if (p_book->GetPageText(i) == _("Windows and tabs")) p_book->SetSelection(i);
  updateWindowsPage();
  for (int i = 0; i < 5; i++) wxYield();
  auto shown = [this]() {
    return "choice=" + p_layout->GetStringSelection().Lower().ToStdString() +
           " " + WindowLayout::describe();
  };
  if (op == "show") return shown();
  if (op == "snap")
    return ecceClientShot(this, wxString::FromUTF8(a.c_str())) ? "saved" : "not saved";
  if (op == "preset") {
    WindowLayout::Preset p;
    if (!WindowLayout::presetFromName(a, p)) return "bad preset";
    for (int i = 0; i < 4; i++)
      if (PRESETS[i] == p) p_layout->SetSelection(i);
    if (p == WindowLayout::Custom) return "custom opens the table; use row";
    wxCommandEvent ev(wxEVT_CHOICE, p_layout->GetId());
    OnLayoutChoice(ev);
    return shown();
  }
  if (op == "row") {
    if (b != "tab" && b != "window") return "bad row";
    LayoutTable table(this);
    if (!table.setRow(a, b == "tab")) return "no such tool";
    wxTimer timer;
    timer.Bind(wxEVT_TIMER, [&](wxTimerEvent&) {
      for (int i = 0; i < 5; i++) wxYield();
      if (!snap.empty()) ecceClientShot(&table, wxString::FromUTF8(snap.c_str()));
      table.EndModal(wxID_OK);
    });
    timer.StartOnce(500);
    if (table.ShowModal() == wxID_OK) WindowLayout::setRows(table.rows());
    updateWindowsPage();
    updateResetButton();
    return shown();
  }
  return "unknown windows command";
}
