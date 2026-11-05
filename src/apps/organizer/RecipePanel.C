#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dcbuffer.h"
#include "wx/dcgraph.h"
#include "wx/listctrl.h"
#include "wx/scrolwin.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "dsm/PropertyTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Session.H"
#include "tdat/Fragment.H"
#include "tdat/TAtm.H"
#include "wxgui/WxHelpViewer.H"

#include "RecipePanel.H"
#include "WorkflowLayout.H"
#include "WorkflowPanel.H"

using std::string;
using std::vector;
using std::set;

namespace {
wxString U(const string& s) { return wxString::FromUTF8(s.c_str()); }
const char *const kLevels[] = {"B3LYP", "B3LYP-D3(BJ)", "PBE0", "PBE", "r2SCAN-3c",
                               "wB97X-D", "M06-2X", "HF", "GFN2-xTB", "none"};
const char *const kBases[] = {"6-31G(d)", "6-31+G(d)", "6-311+G(d,p)", "def2-SVP",
                              "def2-SVPD", "def2-TZVP", "aug-cc-pVDZ", "aug-cc-pVTZ", ""};
}


/// The boxes of the chosen variant, laid out as the workflow view lays them
/// out and drawn small enough to fit.
class RecipePreview : public wxPanel
{
public:
  RecipePreview(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN),
      p_variant(0), p_structure(true)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(420, 300));
    Bind(wxEVT_PAINT, &RecipePreview::OnPaint, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent &e) { Refresh(); e.Skip(); });
  }

  void show(const RecipeVariant *v, bool placeholders)
  {
    p_variant = v;
    p_structure = placeholders;
    Refresh();
  }

  /// Number of boxes and links drawn, and the label overlaps, for the report.
  string describe()
  {
    if (!p_variant) return "boxes=0 links=0 overlaps=0";
    wxClientDC dc(this);
    dc.SetFont(GetFont());
    WfLayoutResult r = layout(dc);
    std::ostringstream o;
    o << "boxes=" << r.boxes.size() << " links=" << r.links.size()
      << " overlaps=" << layoutOverlaps(r);
    return o.str();
  }

private:
  WfLayoutResult layout(wxDC &dc)
  {
    vector<WfLayoutNode> nodes(p_variant->steps.size());
    for (size_t i = 0; i < nodes.size(); i++) {
      for (const RecipeLink &l : p_variant->links) {
        if (l.to != p_variant->steps[i].id) continue;
        int from = -1;
        for (size_t k = 0; k < nodes.size(); k++)
          if (p_variant->steps[k].id == l.from) from = (int)k;
        nodes[i].in.push_back(std::make_pair(from, WorkflowSteps::linkLabel(l.name)));
      }
      nodes[i].source = nodes[i].in.size() == 1 ? nodes[i].in[0].first : -1;
      for (size_t k = 0; k < nodes[i].in.size(); k++)
        nodes[i].depth = std::max(nodes[i].depth, nodes[nodes[i].in[k].first].depth + 1);
    }
    return layoutWorkflow(dc, nodes);
  }

  void OnPaint(wxPaintEvent &)
  {
    wxAutoBufferedPaintDC pdc(this);
    pdc.SetBackground(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
    pdc.Clear();
    wxGCDC dc(pdc);
    const wxColour text = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    const wxColour dim = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
    dc.SetFont(GetFont());
    dc.SetTextForeground(dim);
    dc.DrawText("Preview", 8, 6);
    if (!p_variant) return;
    WfLayoutResult r = layout(dc);
    const wxSize avail = GetClientSize();
    const double sx = (avail.x - 16.0) / (r.maxX + kWfMargin);
    const double sy = (avail.y - 36.0) / (r.maxY + kWfMargin);
    const double s = std::min(1.0, std::min(sx, sy));
    dc.SetUserScale(s, s);
    dc.SetDeviceOrigin(8, 28);
    wxFont base = GetFont(), bold = base;
    bold.SetWeight(wxFONTWEIGHT_BOLD);
    dc.SetPen(wxPen(dim, 2));
    dc.SetBrush(wxBrush(dim));
    for (const WfLayoutLink &l : r.links) {
      dc.DrawLine(l.p0, l.p1);
      dc.DrawLine(l.p1, l.p2);
      dc.DrawLine(l.p2, wxPoint(l.p3.x - 8, l.p3.y));
      wxPoint head[3] = {l.p3, wxPoint(l.p3.x - 9, l.p3.y - 5), wxPoint(l.p3.x - 9, l.p3.y + 5)};
      dc.SetPen(*wxTRANSPARENT_PEN);
      dc.DrawPolygon(3, head);
      dc.SetPen(wxPen(dim, 2));
      dc.SetFont(base);
      dc.DrawText(U(l.text), l.label.GetPosition());
    }
    for (size_t i = 0; i < r.boxes.size(); i++) {
      const RecipeStep &st = p_variant->steps[i];
      const wxRect &b = r.boxes[i];
      bool root = true;
      for (const RecipeLink &l : p_variant->links) if (l.to == st.id) root = false;
      const bool needs = st.needsStructure && root && p_structure;
      dc.SetBrush(wxBrush(needs ? wxColour(0xff, 0xf3, 0xcf)
                                : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)));
      dc.SetPen(wxPen(dim, 2));
      dc.DrawRoundedRectangle(b, 8);
      dc.SetTextForeground(text);
      dc.SetFont(bold);
      dc.DrawText(wxControl::Ellipsize(U(st.id + "  (" + st.code + ")"), dc,
                                       wxELLIPSIZE_END, b.width - 20), b.x + 10, b.y + 6);
      dc.SetFont(base);
      dc.SetTextForeground(needs ? wxColour(0x9c, 0x5a, 0x00) : dim);
      dc.DrawText(wxControl::Ellipsize(U(needs ? "needs structure" : st.caption), dc,
                                       wxELLIPSIZE_END, b.width - 20),
                  b.x + 10, b.y + b.height - 8 - dc.GetTextExtent("Ag").y);
    }
  }

  const RecipeVariant *p_variant;
  bool p_structure;
};


RecipePanel::RecipePanel(wxWindow *parent, WorkflowPanel *owner, Session *wf,
                         Resource *after)
  : wxPanel(parent, wxID_ANY), p_owner(owner), p_session(wf), p_after(after),
    p_applyAfter(after != 0), p_recipe(-1), p_variant(0)
{
  // The user's folder exists from the first use, ready for hand-written recipes.
  if (system(("mkdir -p '" + WorkflowSteps::userRecipeDir() + "'").c_str()) != 0) {}
  p_recipes = loadRecipes(recipeDirs(), &p_problems);

  // Facts for the rules from the selected box, when it has a structure.
  PropertyTask *task = dynamic_cast<PropertyTask*>(after);
  Fragment *frag = task ? task->fragment() : 0;
  if (frag && frag->numAtoms() > 0) {
    p_facts.known = true;
    p_facts.charge = frag->charge();
    p_facts.atoms = (int)frag->numAtoms();
    vector<TAtm*> *atoms = frag->atoms();
    if (atoms) {
      for (size_t i = 0; i < atoms->size(); i++) {
        const int z = (*atoms)[i]->atomicNumber();
        if (std::find(p_facts.elements.begin(), p_facts.elements.end(), z) ==
            p_facts.elements.end()) p_facts.elements.push_back(z);
      }
      delete atoms;
    }
  }
  delete frag;

  wxBoxSizer *top = new wxBoxSizer(wxHORIZONTAL);
  p_settings = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                    wxVSCROLL | wxBORDER_SIMPLE);
  p_settings->SetScrollRate(0, 12);
  p_settings->SetMinSize(wxSize(540, -1));
  wxWindow *left = p_settings;
  wxBoxSizer *ls = new wxBoxSizer(wxVERTICAL);
  wxFont bold = GetFont(); bold.SetWeight(wxFONTWEIGHT_BOLD);
  auto heading = [&](const char *t) {
    wxStaticText *h = new wxStaticText(left, wxID_ANY, t);
    h->SetFont(bold);
    ls->Add(h, 0, wxLEFT | wxRIGHT | wxTOP, 10);
  };

  heading("1  Recipe");
  p_list = new wxListCtrl(left, wxID_ANY, wxDefaultPosition, wxSize(480, 112),
                          wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER);
  p_list->AppendColumn("Recipe", wxLIST_FORMAT_LEFT, 140);
  p_list->AppendColumn("Summary", wxLIST_FORMAT_LEFT, 330);
  for (size_t i = 0; i < p_recipes.size(); i++) {
    long row = p_list->InsertItem((long)i, U(p_recipes[i].title + (p_recipes[i].site ? " (site)" : "")));
    p_list->SetItem(row, 1, U(p_recipes[i].summary));
  }
  p_list->Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxListEvent &e) {
    if (e.GetIndex() != p_recipe) selectRecipe(p_recipes[e.GetIndex()].id);
  });
  ls->Add(p_list, 0, wxEXPAND | wxALL, 8);
  p_description = new wxStaticText(left, wxID_ANY, "");
  ls->Add(p_description, 0, wxLEFT | wxRIGHT, 10);

  p_variantBox = new wxBoxSizer(wxVERTICAL);
  ls->Add(p_variantBox, 0, wxALL, 10);

  heading("2  Level of theory  (recommended; change any row)");
  p_facts_text = new wxStaticText(left, wxID_ANY, "");
  ls->Add(p_facts_text, 0, wxLEFT | wxRIGHT, 10);
  p_rolePanel = new wxPanel(left, wxID_ANY);
  p_roleSizer = new wxBoxSizer(wxVERTICAL);
  p_rolePanel->SetSizer(p_roleSizer);
  ls->Add(p_rolePanel, 0, wxEXPAND | wxALL, 4);

  p_where = new wxPanel(left, wxID_ANY);
  wxBoxSizer *apply = new wxBoxSizer(wxVERTICAL);
  wxStaticText *where = new wxStaticText(p_where, wxID_ANY, "3  Where the new steps go");
  where->SetFont(bold);
  apply->Add(where, 0, wxBOTTOM, 6);
  p_new = new wxRadioButton(p_where, wxID_ANY, "Start a new workflow", wxDefaultPosition,
                            wxDefaultSize, wxRB_GROUP);
  p_afterButton = new wxRadioButton(p_where, wxID_ANY,
      after ? U("Continue after the selected box \"" + after->getName() + "\"")
            : wxString("Continue after the selected box"));
  (p_applyAfter ? p_afterButton : p_new)->SetValue(true);
  p_new->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent &) { setApplyAfter(false); });
  p_afterButton->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent &) { setApplyAfter(true); });
  apply->Add(p_new, 0, wxBOTTOM, 4);
  apply->Add(p_afterButton, 0);
  p_where->SetSizer(apply);
  ls->Add(p_where, 0, wxEXPAND | wxALL, 10);
  p_where->Show(after != 0);

  wxPanel *note = new wxPanel(left, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SIMPLE);
  note->SetBackgroundColour(wxColour(0xff, 0xf8, 0xe1));
  wxBoxSizer *ns = new wxBoxSizer(wxVERTICAL);
  wxStaticText *nt = new wxStaticText(note, wxID_ANY,
      "Structures are set in the boxes: click a box marked \"needs structure\"\n"
      "to build or import it. Everything else comes from this panel.");
  nt->SetForegroundColour(wxColour(0x6b, 0x4f, 0x10));
  ns->Add(nt, 0, wxALL, 8);
  note->SetSizer(ns);
  ls->Add(note, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);

  p_autorun = new wxCheckBox(left, wxID_ANY, "Run the remaining steps automatically");
  p_autorun->SetToolTip("Launch each step as soon as the step it starts from is "
                        "Complete. Works while the Organizer is open.");
  ls->Add(p_autorun, 0, wxLEFT | wxRIGHT | wxTOP, 10);

  p_problem = new wxStaticText(left, wxID_ANY, "");
  p_problem->SetForegroundColour(wxColour(0xc9, 0x2a, 0x2a));
  ls->Add(p_problem, 0, wxLEFT | wxRIGHT | wxTOP, 10);

  wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
  wxButton *help = new wxButton(left, wxID_ANY, "Help");
  wxButton *cancel = new wxButton(left, wxID_ANY, "Cancel");
  wxButton *create = new wxButton(left, wxID_ANY, "Create boxes");
  create->SetDefault();
  help->Bind(wxEVT_BUTTON, [](wxCommandEvent &) { WxHelpViewer::show("recipes.html"); });
  cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { cancelClicked(); });
  create->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { createClicked(); });
  buttons->AddStretchSpacer(1);
  buttons->Add(help, 0, wxRIGHT, 6);
  buttons->Add(cancel, 0, wxRIGHT, 6);
  buttons->Add(create, 0);
  ls->Add(buttons, 0, wxEXPAND | wxALL, 10);
  left->SetSizer(ls);

  p_preview = new RecipePreview(this);
  top->Add(left, 0, wxEXPAND);
  top->Add(p_preview, 1, wxEXPAND | wxLEFT, 4);
  SetSizer(top);

  if (!p_problems.empty())
    p_problem->SetLabel(U("Not read: " + p_problems[0] +
        (p_problems.size() > 1 ? " (and " + std::to_string(p_problems.size() - 1) + " more)" : "")));
  if (!p_recipes.empty()) selectRecipe(p_recipes[0].id);
}


set<string> RecipePanel::registeredCodes() const
{
  // A code is usable when this installation has its resource type.
  set<string> out;
  vector<ResourceType*> types = ResourceDescriptor::getResourceDescriptor().getResourceTypes();
  for (const WorkflowSteps::Code &c : WorkflowSteps::codes())
    for (size_t i = 0; i < types.size(); i++)
      if (types[i]->getName() == c.typeName) out.insert(c.label);
  return out;
}


const Recipe *RecipePanel::recipe() const
{
  return p_recipe >= 0 ? &p_recipes[p_recipe] : 0;
}


const RecipeVariant *RecipePanel::variant() const
{
  return recipe() && p_variant < (int)recipe()->variants.size()
             ? &recipe()->variants[p_variant] : 0;
}


bool RecipePanel::selectRecipe(const string &id)
{
  for (size_t i = 0; i < p_recipes.size(); i++)
    if (p_recipes[i].id == id) {
      p_recipe = (int)i;
      p_variant = 0;
      p_list->SetItemState((long)i, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
      p_description->SetLabel(U(p_recipes[i].description));
      p_description->Wrap(470);
      fillVariants();
      fillRoles();
      changed();
      return true;
    }
  return false;
}


void RecipePanel::fillVariants()
{
  p_variantBox->Clear(true);
  p_variantButtons.clear();
  wxWindow *host = p_list->GetParent();
  p_variantBox->Add(new wxStaticText(host, wxID_ANY, "Code:"), 0, wxBOTTOM, 4);
  const set<string> registered = registeredCodes();
  bool chosen = false;
  for (size_t i = 0; i < recipe()->variants.size(); i++) {
    const RecipeVariant &v = recipe()->variants[i];
    wxRadioButton *b = new wxRadioButton(host, wxID_ANY, U(v.label), wxDefaultPosition,
                                         wxDefaultSize, i == 0 ? wxRB_GROUP : 0);
    const bool ok = codeAvailable(v.code, registered);
    b->Enable(ok);
    if (!ok) b->SetToolTip("not registered");
    if (ok && !chosen) { b->SetValue(true); p_variant = (int)i; chosen = true; }
    b->Bind(wxEVT_RADIOBUTTON, [this, i](wxCommandEvent &) {
      p_variant = (int)i;
      fillRoles(true);
      changed();
    });
    p_variantBox->Add(b, 0, wxRIGHT, 14);
    p_variantButtons.push_back(b);
  }
}


bool RecipePanel::selectVariant(const string &id)
{
  if (!recipe()) return false;
  for (size_t i = 0; i < recipe()->variants.size(); i++)
    if (recipe()->variants[i].id == id && p_variantButtons[i]->IsEnabled()) {
      p_variant = (int)i;
      p_variantButtons[i]->SetValue(true);
      fillRoles(true);
      changed();
      return true;
    }
  return false;
}


void RecipePanel::layoutSettings()
{
  // Rebuilding rows changes their best size as well as the parent's minimum.
  p_rolePanel->InvalidateBestSize();
  p_rolePanel->Layout();
  p_settings->Layout();
  p_settings->FitInside();
  Layout();
}


void RecipePanel::fillRoles(bool preserve)
{
  const WorkflowSteps::Levels previous = preserve ? levels() : WorkflowSteps::Levels();
#ifdef __WXMSW__
  p_settings->Freeze();
#endif
  p_roleSizer->Clear(true);
  p_roles.clear();
  wxBoxSizer *columns = new wxBoxSizer(wxHORIZONTAL);
  columns->Add(198, 1);
  columns->Add(new wxStaticText(p_rolePanel, wxID_ANY, "Method"), 0, wxRIGHT, 8);
  columns->GetItem((size_t)1)->SetMinSize(130, -1);
  columns->Add(new wxStaticText(p_rolePanel, wxID_ANY, "Basis set"), 0);
  p_roleSizer->Add(columns, 0, wxLEFT | wxBOTTOM, 5);
  for (const RecipeRole &role : recipe()->roles) {
    const bool solvent = role.id == "solvent";
    RoleChoice c = solvent ? RoleChoice{role.method, role.basis, role.why, {}}
                           : recommend(*recipe(), role, p_facts);
    auto old = previous.find(role.id);
    if (old != previous.end()) { c.method = old->second.first; c.basis = old->second.second; }
    wxPanel *row = new wxPanel(p_rolePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                               wxBORDER_SIMPLE);
    wxBoxSizer *rs = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *line = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *label = new wxStaticText(row, wxID_ANY, U(role.label));
    label->Wrap(190);
    label->SetMinSize(wxSize(190, -1));
    wxComboBox *method = nullptr;
    wxComboBox *basis = new wxComboBox(row, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                       wxSize(130, -1), wxArrayString(), solvent ? wxCB_READONLY : 0);
    line->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    string applies = "Applies to: ";
    for (const RecipeStep& st : variant()->steps)
      if (solvent || st.role == role.id) {
        if (applies != "Applies to: ") applies += ", ";
        applies += st.id;
      }
    if (solvent) {
      applies += " (applies to every step)";
      wxBoxSizer *pick = new wxBoxSizer(wxVERTICAL);
      pick->Add(new wxStaticText(row, wxID_ANY, "Solvent"), 0, wxBOTTOM, 2);
      const vector<string> names = recipeSolvents(*variant());
      for (const string& name : names) basis->Append(U(name));
      if (std::find(names.begin(), names.end(), c.basis) == names.end()) c.basis = "none";
      basis->SetValue(U(c.basis));
      pick->Add(basis);
      line->Add(pick);
      c.why = "none = gas phase. Solvent model: ";
      set<string> codes;
      for (const RecipeStep& st : variant()->steps) if (codes.insert(st.code).second) {
        if (codes.size() > 1) c.why += "; ";
        const string model = recipeSolventModel(st.code);
        c.why += st.code + ": " + (model.empty() ? "no recipe solvent mapping; set it in the box" : model);
      }
    } else {
      method = new wxComboBox(row, wxID_ANY, U(c.method), wxDefaultPosition, wxSize(130, -1));
      for (const char *m : kLevels) method->Append(m);
      for (const char *b : kBases) basis->Append(b);
      basis->SetValue(U(c.basis));
      line->Add(method, 0, wxRIGHT, 8);
      line->Add(basis, 0);
    }
    wxStaticText *targets = new wxStaticText(row, wxID_ANY, U(applies));
    targets->Wrap(490);
    string reason = "Why: " + c.why;
    for (const string& warning : c.warnings) reason += "\n" + warning;
    wxStaticText *why = new wxStaticText(row, wxID_ANY, U(reason));
    why->Wrap(490);
    why->SetForegroundColour(wxColour(0x1e, 0x7e, 0x34));
    rs->Add(line, 0, wxALL, 4);
    rs->Add(targets, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
    rs->Add(why, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
    row->SetSizer(rs);
    p_roleSizer->Add(row, 0, wxEXPAND | wxBOTTOM, 4);
    RoleRow rr = {role.id, method, basis, why, label, targets};
    p_roles.push_back(rr);
    if (method) {
      const string originalWhy = role.why;
      auto edited = [this, originalWhy, rr](wxCommandEvent &) {
        vector<string> warnings = checkLevel(*recipe(), string(rr.method->GetValue().ToUTF8()),
                                              string(rr.basis->GetValue().ToUTF8()), p_facts);
        string text = "Why: " + originalWhy;
        for (const string& warning : warnings) text += "\n" + warning;
        rr.why->SetLabel(U(text));
        rr.why->Wrap(490);
        layoutSettings();
      };
      method->Bind(wxEVT_TEXT, edited);
      basis->Bind(wxEVT_TEXT, edited);
    }
  }
  p_facts_text->SetLabel(p_facts.known
      ? U("Rules applied to the selected box: charge " + std::to_string(p_facts.charge) +
          ", " + std::to_string(p_facts.atoms) + " atoms")
      : wxString("Defaults of the recipe; the rules apply again when a structure is set."));
  p_facts_text->Wrap(490);
  layoutSettings();
#ifdef __WXMSW__
  p_settings->Thaw();
#endif
}


bool RecipePanel::setLevel(const string &role, const string &method, const string &basis)
{
  for (size_t i = 0; i < p_roles.size(); i++)
    if (p_roles[i].id == role) {
      if (p_roles[i].method) p_roles[i].method->SetValue(U(method));
      p_roles[i].basis->SetValue(U(basis));
      changed();
      return true;
    }
  return false;
}


void RecipePanel::setAutoRun(bool on)
{
  p_autorun->SetValue(on);
}


void RecipePanel::setApplyAfter(bool after)
{
  p_applyAfter = after && p_after;
  (p_applyAfter ? p_afterButton : p_new)->SetValue(true);
  changed();
}


WorkflowSteps::Levels RecipePanel::levels() const
{
  WorkflowSteps::Levels out;
  for (size_t i = 0; i < p_roles.size(); i++)
    out[p_roles[i].id] = std::make_pair(p_roles[i].method ? string(p_roles[i].method->GetValue().ToUTF8()) : string(),
                                        string(p_roles[i].basis->GetValue().ToUTF8()));
  return out;
}


void RecipePanel::changed()
{
  p_preview->show(variant(), !p_applyAfter);
  layoutSettings();
}


string RecipePanel::report() const
{
  std::ostringstream o;
  o << "recipe=" << (recipe() ? recipe()->id : "") << " variant="
    << (variant() ? variant()->id : "") << " " << p_preview->describe()
    << " recipes=" << p_recipes.size() << " apply=" << (p_applyAfter ? "after" : "new")
    << " roles=";
  const WorkflowSteps::Levels lv = levels();
  for (WorkflowSteps::Levels::const_iterator it = lv.begin(); it != lv.end(); ++it)
    o << it->first << ":" << it->second.first << "/" << it->second.second << ";";
  o << " why=";
  for (size_t i = 0; i < p_roles.size(); i++)
    o << string(p_roles[i].why->GetLabel().ToUTF8()) << ";";
  o << " variants-enabled=";
  for (size_t i = 0; i < p_variantButtons.size(); i++) o << (p_variantButtons[i]->IsEnabled() ? 1 : 0);
  o << " where-visible=" << p_where->IsShown()
    << " where-heading=" << p_where->GetChildren().GetFirst()->GetData()->GetLabel().ToStdString()
    << " new-label=" << p_new->GetLabel().ToStdString()
    << " after-label=" << p_afterButton->GetLabel().ToStdString();
  auto flat = [](wxStaticText *t) {
    string text = t->GetLabel().ToStdString();
    std::replace(text.begin(), text.end(), '\n', ' ');
    return text;
  };
  o << " headings=";
  for (wxWindow *child : p_rolePanel->GetChildren())
    if (auto text = dynamic_cast<wxStaticText*>(child)) o << flat(text) << ";";
  int overlaps = 0, clipped = 0;
  const auto& items = p_roleSizer->GetChildren();
  int bottom = 0;
  for (auto item : items) if (item->IsWindow()) {
    const wxRect rect = item->GetWindow()->GetRect();
    if (rect.y < bottom) overlaps++;
    bottom = rect.GetBottom() + 1;
  }
  for (const auto& row : p_roles) {
    o << " row[" << row.id << "]=" << flat(row.label) << ";" << flat(row.applies);
    if (row.label->GetSize().y < row.label->GetBestSize().y) clipped++;
  }
  if (p_rolePanel->GetSize().y < bottom) overlaps++;
  o << " role-overlaps=" << overlaps << " clipped-labels=" << clipped;
  string result = o.str();
  std::replace(result.begin(), result.end(), '\n', ' ');
  return result;
}


void RecipePanel::createClicked()
{
  if (!variant()) return;
  string why = p_owner->recipeCreate(*recipe(), *variant(), levels(), after(),
                                 p_autorun->GetValue());
  if (!why.empty()) {
    p_problem->SetLabel(U(why));
    Layout();
  }
}


string RecipePanel::problem() const
{
  return string(p_problem->GetLabel().ToUTF8());
}


void RecipePanel::cancelClicked()
{
  p_owner->closeRecipe();
}
