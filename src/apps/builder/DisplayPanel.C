#include "DisplayPanel.H"

#include <sstream>

#include <wx/colordlg.h>

#include "SurfaceStyle.H"
#include "wxviz/VizStyleChooser.H"
#include "wxviz/ViewerEvtHandler.H"

namespace {
const char *SLIDER_KEYS[5] = {"sphscl", "cylrad", "sphrq", "cylrq", "opacity"};
const char *SLIDER_LABELS[5] = {"Ball and Stick Scale", "Cylinder Radius",
                                "Sphere Quality", "Cylinder Quality",
                                "Atom Opacity (%)"};
const char *ATOM_LABELS[7] = {"None", "Element with Number", "Atom Name",
    "Charge, 2 decimals", "Charge, 3 decimals", "Charge, 4 decimals", "Type"};
const char *PRESET_NAMES[4] = {"R/G", "R/B", "B/Y", "L/D"};
}

// A button showing a colour; a click asks for a new one.
class DisplayPanel::Swatch : public wxButton {
public:
  Swatch(wxWindow *parent, const wxString& text,
         std::function<void(const double[3])> onPick)
    : wxButton(parent, wxID_ANY, text), p_pick(onPick)
  {
    p_rgb[0] = p_rgb[1] = p_rgb[2] = 0.5;
    paint();
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
      wxColourData cd;
      cd.SetColour(wxColour(p_rgb[0]*255+.5, p_rgb[1]*255+.5, p_rgb[2]*255+.5));
      wxColourDialog dlg(this, &cd);
      if (dlg.ShowModal() != wxID_OK) return;
      wxColour c = dlg.GetColourData().GetColour();
      double v[3] = {c.Red()/255.0, c.Green()/255.0, c.Blue()/255.0};
      set(v);
      if (p_pick) p_pick(v);
    });
  }
  void set(const double rgb[3])
  {
    for (int i = 0; i < 3; i++) p_rgb[i] = rgb[i];
    paint();
  }
  const double *rgb() const { return p_rgb; }
private:
  void paint()
  {
    wxBitmap bmp(16, 16);
    wxMemoryDC dc(bmp);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    dc.SetBrush(wxBrush(wxColour(p_rgb[0]*255+.5, p_rgb[1]*255+.5,
                                 p_rgb[2]*255+.5)));
    dc.SetPen(*wxGREY_PEN);
    dc.DrawRectangle(1, 1, 14, 14);
    dc.SelectObject(wxNullBitmap);
    SetBitmap(bmp);
  }
  double p_rgb[3];
  std::function<void(const double[3])> p_pick;
};

DisplayPanel::DisplayPanel(wxWindow *parent, const DisplayHost& host)
  : wxScrolledWindow(parent, wxID_ANY), p_h(host)
{
  SetScrollRate(0, 10);
  build();
  p_building = false;
}

void DisplayPanel::build()
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  auto header = [&](const wxString& t) {
    wxStaticText *s = new wxStaticText(this, wxID_ANY, t);
    wxFont f = s->GetFont(); f.MakeBold(); s->SetFont(f);
    top->Add(s, 0, wxLEFT | wxRIGHT | wxTOP, 8);
  };
  auto grid = [&]() {
    wxFlexGridSizer *g = new wxFlexGridSizer(2, 4, 8);
    g->AddGrowableCol(1);
    top->Add(g, 0, wxEXPAND | wxALL, 6);
    return g;
  };

  header("Style");
  wxFlexGridSizer *g = grid();
  p_style = new wxChoice(this, wxID_ANY);
  p_scheme = new wxChoice(this, wxID_ANY);
  g->Add(new wxStaticText(this, wxID_ANY, "Display style"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(p_style, 1, wxEXPAND);
  g->Add(new wxStaticText(this, wxID_ANY, "Color scheme"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(p_scheme, 1, wxEXPAND);
  p_style->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { applyStyle(); });
  p_scheme->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { applyStyle(); });
  for (int i = 0; i < 5; i++) {
    p_sl[i] = new wxSlider(this, wxID_ANY, i == 4 ? 100 : 50, i == 4 ? 0 : 1, 100);
    p_val[i] = new wxStaticText(this, wxID_ANY, "50", wxDefaultPosition,
                                wxSize(30, -1));
    wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
    row->Add(p_sl[i], 1, wxEXPAND);
    row->Add(p_val[i], 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    wxBoxSizer *cell = new wxBoxSizer(wxVERTICAL);
    cell->Add(row, 0, wxEXPAND);
    if (i >= 2) {
      const int k = i - 2;
      p_auto[k] = new wxCheckBox(this, wxID_ANY, "Automatic (by atom count)");
      p_auto[k]->Bind(wxEVT_CHECKBOX, [this, k](wxCommandEvent& e) {
        if (!p_building) applyAutomatic(k == 0, e.IsChecked());
      });
      cell->Add(p_auto[k], 0);
    }
    g->Add(new wxStaticText(this, wxID_ANY, SLIDER_LABELS[i]), 0, wxALIGN_CENTER_VERTICAL);
    g->Add(cell, 1, wxEXPAND);
    p_sl[i]->Bind(wxEVT_SCROLL_CHANGED, [this, i](wxScrollEvent&) {
      if (!p_building) applySlider(SLIDER_KEYS[i], p_sl[i]->GetValue());
    });
    p_sl[i]->Bind(wxEVT_SLIDER, [this, i](wxCommandEvent&) {
      p_val[i]->SetLabel(wxString::Format("%d", p_sl[i]->GetValue()));
    });
  }
  wxButton *more = new wxButton(this, wxID_ANY, "More... (Custom Style)");
  more->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { if (p_h.openCustomStyle) p_h.openCustomStyle(); });
  top->Add(more, 0, wxLEFT | wxBOTTOM, 8);

  header("Colors");
  g = grid();
  p_bg = new Swatch(this, "Background...", [this](const double c[3]) {
    if (p_h.setBackground) p_h.setBackground(c[0], c[1], c[2]);
    refresh();
  });
  p_fg = new Swatch(this, "Foreground...", [this](const double c[3]) {
    if (p_h.setForeground) p_h.setForeground(c[0], c[1], c[2]);
  });
  g->Add(new wxStaticText(this, wxID_ANY, "Background"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(p_bg, 1, wxEXPAND);
  g->Add(new wxStaticText(this, wxID_ANY, "Foreground"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(p_fg, 1, wxEXPAND);
  header("Orbital / density surfaces");
  g = grid();
  p_pos = new Swatch(this, "Positive", [this](const double c[3]) {
    double pos[3] = {c[0], c[1], c[2]};
    applyLobes(pos, p_neg->rgb());
  });
  p_neg = new Swatch(this, "Negative", [this](const double c[3]) {
    double neg[3] = {c[0], c[1], c[2]};
    applyLobes(p_pos->rgb(), neg);
  });
  wxBoxSizer *lobes = new wxBoxSizer(wxHORIZONTAL);
  lobes->Add(p_pos, 1); lobes->Add(p_neg, 1, wxLEFT, 4);
  g->Add(new wxStaticText(this, wxID_ANY, "Lobe colors"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(lobes, 1, wxEXPAND);
  wxBoxSizer *presets = new wxBoxSizer(wxHORIZONTAL);
  for (int i = 0; i < 4; i++) {
    wxButton *b = new wxButton(this, wxID_ANY, PRESET_NAMES[i], wxDefaultPosition,
                               wxDefaultSize, wxBU_EXACTFIT);
    b->Bind(wxEVT_BUTTON, [this, i](wxCommandEvent&) { applyPreset(i); });
    presets->Add(b, 0, wxRIGHT, 3);
  }
  g->Add(new wxStaticText(this, wxID_ANY, "Presets"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(presets, 1);
  wxBoxSizer *surf = new wxBoxSizer(wxHORIZONTAL);
  const char *sn[3] = {"Solid", "Mesh", "Contour"};
  for (int i = 0; i < 3; i++) {
    p_surf[i] = new wxRadioButton(this, wxID_ANY, sn[i], wxDefaultPosition,
                                  wxDefaultSize, i == 0 ? wxRB_GROUP : 0);
    p_surf[i]->Bind(wxEVT_RADIOBUTTON, [this, i](wxCommandEvent&) { applySurfaceType(i); });
    surf->Add(p_surf[i], 0, wxRIGHT, 6);
  }
  g->Add(new wxStaticText(this, wxID_ANY, "Surface"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(surf, 1);
  p_opacity = new wxSlider(this, wxID_ANY, 50, 0, 100);
  p_opacityVal = new wxStaticText(this, wxID_ANY, "50%", wxDefaultPosition, wxSize(40, -1));
  wxBoxSizer *orow = new wxBoxSizer(wxHORIZONTAL);
  orow->Add(p_opacity, 1, wxEXPAND);
  orow->Add(p_opacityVal, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
  // The two settings the old right-click menu offered, one click each.
  for (int pct : {50, 100}) {
    wxButton *b = new wxButton(this, wxID_ANY, pct == 50 ? "Semi-transparent" : "Opaque",
                               wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    b->Bind(wxEVT_BUTTON, [this, pct](wxCommandEvent&) { applyOpacity(pct); refresh(); });
    orow->Add(b, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
  }
  p_opacity->Bind(wxEVT_SCROLL_CHANGED, [this](wxScrollEvent&) {
    if (!p_building) applyOpacity(p_opacity->GetValue());
  });
  p_opacity->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
    p_opacityVal->SetLabel(wxString::Format("%d%%", p_opacity->GetValue()));
  });
  g->Add(new wxStaticText(this, wxID_ANY, "Opacity"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(orow, 1, wxEXPAND);
  p_cell = new Swatch(this, "Cell edges", [this](const double c[3]) {
    applyCell(c);
  });
  g->Add(new wxStaticText(this, wxID_ANY, "Cell edges"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(p_cell, 1, wxEXPAND);

  header("Labels");
  g = grid();
  p_atomLabels = new wxChoice(this, wxID_ANY);
  for (const char *s : ATOM_LABELS) p_atomLabels->Append(s);
  p_atomLabels->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) {
    applyAtomLabel(p_atomLabels->GetSelection());
  });
  g->Add(new wxStaticText(this, wxID_ANY, "Atom Labels"), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(p_atomLabels, 1, wxEXPAND);
  auto check = [&](wxCheckBox *&c, const char *text, int id) {
    c = new wxCheckBox(this, wxID_ANY, text);
    c->Bind(wxEVT_CHECKBOX, [this, id](wxCommandEvent& e) {
      if (!p_building) applyToggle(id, e.IsChecked());
    });
    top->Add(c, 0, wxLEFT, 12);
  };
  check(p_bond, "Show Bond Labels", ViewerEvtHandler::ID_BOND_LABEL);
  check(p_res, "Show Residue Labels", ViewerEvtHandler::ID_RESIDUE_LABEL);
  check(p_sum, "Show Summary Text Labels", ViewerEvtHandler::ID_CORNER_LABELS);
  check(p_axes, "Show Axes", ViewerEvtHandler::ID_SHOW_AXES);
  p_cellLabels = new wxCheckBox(this, wxID_ANY, "Cell a/b/c labels");
  p_cellLabels->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
    if (!p_building) applyCellLabels(e.IsChecked());
  });
  top->Add(p_cellLabels, 0, wxLEFT, 12);

  wxButton *reset = new wxButton(this, wxID_ANY, "Reset to defaults");
  reset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { applyReset(); });
  top->Add(reset, 0, wxALL, 10);

  SetSizer(top);
  FitInside();
  Bind(wxEVT_SHOW, [this](wxShowEvent& e) { if (e.IsShown()) refresh(); e.Skip(); });
  Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& e) { refresh(); e.Skip(); });
}

void DisplayPanel::refresh()
{
  if (p_building && !p_style) return;
  const bool was = p_building;
  p_building = true;
  VizStyleChooser *c = p_h.chooser ? p_h.chooser() : 0;
  if (c) {
    c->syncFromScene();
    auto fill = [&](wxChoice *ch, bool scheme) {
      std::vector<std::string> items = c->choiceItems(scheme);
      ch->Clear();
      for (auto& s : items) ch->Append(s);
      ch->SetStringSelection(c->currentChoice(scheme));
    };
    fill(p_style, false);
    fill(p_scheme, true);
    for (int i = 0; i < 5; i++) {
      p_sl[i]->SetValue(c->sliderValue(SLIDER_KEYS[i]));
      p_val[i]->SetLabel(wxString::Format("%d", p_sl[i]->GetValue()));
    }
    p_auto[0]->SetValue(c->isQualityAutomatic(true));
    p_auto[1]->SetValue(c->isQualityAutomatic(false));
    // While Automatic is on the slider's number is not the quality in use.
    p_sl[2]->Enable(!p_auto[0]->GetValue());
    p_sl[3]->Enable(!p_auto[1]->GetValue());
  }
  double v[3] = {0, 0, 0};
  if (p_h.getBackground) { p_h.getBackground(v); p_bg->set(v); }
  if (p_h.getForeground) { p_h.getForeground(v); p_fg->set(v); }
  double pos[3], neg[3];
  SurfaceStyle::colors(pos, neg);
  p_pos->set(pos);
  p_neg->set(neg);
  for (int i = 0; i < 3; i++) p_surf[i]->SetValue(i == SurfaceStyle::surfaceType());
  const int op = SurfaceStyle::opacityPercent("MO/Transparency", true);
  p_opacity->SetValue(op);
  p_opacityVal->SetLabel(wxString::Format("%d%%", op));
  SurfaceStyle::cellColor(v);
  p_cell->set(v);
  p_cellLabels->SetValue(SurfaceStyle::cellLabels());
  if (p_h.checked) {
    p_bond->SetValue(p_h.checked(ViewerEvtHandler::ID_BOND_LABEL));
    p_res->SetValue(p_h.checked(ViewerEvtHandler::ID_RESIDUE_LABEL));
    p_sum->SetValue(p_h.checked(ViewerEvtHandler::ID_CORNER_LABELS));
    p_axes->SetValue(p_h.checked(ViewerEvtHandler::ID_SHOW_AXES));
  }
  p_building = was;
}

void DisplayPanel::applyStyle()
{
  VizStyleChooser *c = p_h.chooser ? p_h.chooser() : 0;
  if (!c) return;
  c->applyStyleAndScheme(p_style->GetStringSelection().ToStdString(),
                         p_scheme->GetStringSelection().ToStdString());
  refresh();
}

void DisplayPanel::applyAutomatic(bool sphere, bool on)
{
  if (VizStyleChooser *c = p_h.chooser ? p_h.chooser() : 0)
    c->setQualityAutomatic(sphere, on);
  p_auto[sphere ? 0 : 1]->SetValue(on);
  refresh();   // greys the slider while Automatic is on
}

void DisplayPanel::applySlider(const std::string& name, int v)
{
  VizStyleChooser *c = p_h.chooser ? p_h.chooser() : 0;
  if (!c) return;
  c->scriptSlider(name, v);
  if (name == "sphrq") p_auto[0]->SetValue(false);
  if (name == "cylrq") p_auto[1]->SetValue(false);
  for (int i = 0; i < 5; i++)
    if (name == SLIDER_KEYS[i]) {
      p_sl[i]->SetValue(v);
      p_val[i]->SetLabel(wxString::Format("%d", v));
    }
}

void DisplayPanel::surfaces() { if (p_h.surfacesChanged) p_h.surfacesChanged(); }

void DisplayPanel::applyLobes(const double pos[3], const double neg[3])
{
  SurfaceStyle::setColors(pos, neg);
  p_pos->set(pos);
  p_neg->set(neg);
  surfaces();
}

void DisplayPanel::applyPreset(int i)
{
  SurfaceStyle::setPreset(i);
  double pos[3], neg[3];
  SurfaceStyle::colors(pos, neg);
  p_pos->set(pos);
  p_neg->set(neg);
  surfaces();
}

void DisplayPanel::applySurfaceType(int t)
{
  SurfaceStyle::setSurfaceType(t);
  for (int i = 0; i < 3; i++) p_surf[i]->SetValue(i == t);
  surfaces();
}

void DisplayPanel::applyOpacity(int percent)
{
  SurfaceStyle::setOpacity(percent);
  p_opacity->SetValue(percent);
  p_opacityVal->SetLabel(wxString::Format("%d%%", percent));
  surfaces();
}

void DisplayPanel::applyCell(const double rgb[3])
{
  SurfaceStyle::setCellColor(rgb);
  p_cell->set(rgb);
  surfaces();
}

void DisplayPanel::applyCellLabels(bool on)
{
  SurfaceStyle::setCellLabels(on);
  p_cellLabels->SetValue(on);
  surfaces();
}

void DisplayPanel::applyToggle(int id, bool on)
{
  if (p_h.menuAction) p_h.menuAction(id, true, on);
}

void DisplayPanel::applyAtomLabel(int index)
{
  if (index < 0 || index > 6 || !p_h.menuAction) return;
  p_atomLabels->SetSelection(index);
  p_h.menuAction(ViewerEvtHandler::ID_LABEL_NONE + index, false, false);
}

void DisplayPanel::applyReset()
{
  SurfaceStyle::reset();
  if (p_h.resetAll) p_h.resetAll();
  surfaces();
  refresh();
}

static bool rgbArgs(const std::vector<std::string>& w, size_t at, double v[3])
{
  if (w.size() < at + 3) return false;
  for (int i = 0; i < 3; i++) v[i] = atof(w[at + i].c_str());
  return true;
}

bool DisplayPanel::script(const std::vector<std::string>& w, std::string& err)
{
  const std::string& c = w[0];
  double v[3], v2[3];
  auto on = [&]() { return w.size() > 1 && (w[1] == "on" || w[1] == "1"); };
  if (c == "style" && w.size() >= 2) {
    std::string name = w[1];
    for (size_t i = 2; i < w.size(); i++) name += " " + w[i];
    p_style->SetStringSelection(name);
    applyStyle();
  } else if (c == "scheme" && w.size() >= 2) {
    std::string name = w[1];
    for (size_t i = 2; i < w.size(); i++) name += " " + w[i];
    p_scheme->SetStringSelection(name);
    applyStyle();
  } else if (c == "slider" && w.size() == 3) {
    applySlider(w[1], atoi(w[2].c_str()));
  } else if (c == "auto" && w.size() == 3) {
    applyAutomatic(w[1] == "sphrq", w[2] == "on");
  } else if (c == "background" && rgbArgs(w, 1, v)) {
    if (p_h.setBackground) p_h.setBackground(v[0], v[1], v[2]);
    refresh();
  } else if (c == "foreground" && rgbArgs(w, 1, v)) {
    if (p_h.setForeground) p_h.setForeground(v[0], v[1], v[2]);
    refresh();
  } else if (c == "lobes" && rgbArgs(w, 1, v) && rgbArgs(w, 4, v2)) {
    applyLobes(v, v2);
  } else if (c == "preset" && w.size() == 2) {
    applyPreset(atoi(w[1].c_str()));
  } else if (c == "surface" && w.size() == 2) {
    applySurfaceType(w[1] == "mesh" ? 1 : w[1] == "contour" ? 2 : 0);
  } else if (c == "opacity" && w.size() == 2) {
    applyOpacity(atoi(w[1].c_str()));
  } else if (c == "cell" && rgbArgs(w, 1, v)) {
    applyCell(v);
  } else if (c == "celllabels") {
    applyCellLabels(on());
  } else if (c == "atomlabels" && w.size() == 2) {
    applyAtomLabel(atoi(w[1].c_str()));
  } else if (c == "bondlabels") {
    applyToggle(ViewerEvtHandler::ID_BOND_LABEL, on());
  } else if (c == "residuelabels") {
    applyToggle(ViewerEvtHandler::ID_RESIDUE_LABEL, on());
  } else if (c == "summary") {
    applyToggle(ViewerEvtHandler::ID_CORNER_LABELS, on());
  } else if (c == "axes") {
    applyToggle(ViewerEvtHandler::ID_SHOW_AXES, on());
  } else if (c == "reset") {
    applyReset();
  } else {
    err = "display: unknown form";
    return false;
  }
  refresh();
  return true;
}

std::string DisplayPanel::info()
{
  VizStyleChooser *c = p_h.chooser ? p_h.chooser() : 0;
  std::ostringstream o;
  double bg[3] = {0, 0, 0}, pos[3], neg[3], cell[3];
  if (p_h.getBackground) p_h.getBackground(bg);
  SurfaceStyle::colors(pos, neg);
  SurfaceStyle::cellColor(cell);
  if (c) {
    o << "style " << c->currentChoice(false) << "\n"
      << "scheme " << c->currentChoice(true) << "\n"
      << "sphrq " << c->sliderValue("sphrq") << " auto "
      << c->isQualityAutomatic(true) << "\n"
      << "cylrq " << c->sliderValue("cylrq") << " auto "
      << c->isQualityAutomatic(false) << "\n";
  }
  o << "background " << bg[0] << " " << bg[1] << " " << bg[2] << "\n"
    << "pos " << pos[0] << " " << pos[1] << " " << pos[2] << "\n"
    << "neg " << neg[0] << " " << neg[1] << " " << neg[2] << "\n"
    << "opacity " << SurfaceStyle::opacityPercent("MO/Transparency", true) << "\n"
    << "surface " << SurfaceStyle::surfaceType() << "\n"
    << "cell " << cell[0] << " " << cell[1] << " " << cell[2] << "\n"
    << "celllabels " << SurfaceStyle::cellLabels() << "\n";
  return o.str();
}
