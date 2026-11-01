#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/dcbuffer.h"
#include "wx/dcgraph.h"
#include "wx/listctrl.h"

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
  wxPanel *left = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                              wxBORDER_SIMPLE);
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

  p_variantBox = new wxBoxSizer(wxHORIZONTAL);
  ls->Add(p_variantBox, 0, wxALL, 10);

  heading("2  Level of theory  (recommended; change any row)");
  p_facts_text = new wxStaticText(left, wxID_ANY, "");
  ls->Add(p_facts_text, 0, wxLEFT | wxRIGHT, 10);
  p_rolePanel = new wxPanel(left, wxID_ANY);
  p_roleSizer = new wxBoxSizer(wxVERTICAL);
  p_rolePanel->SetSizer(p_roleSizer);
  ls->Add(p_rolePanel, 0, wxEXPAND | wxALL, 4);

  heading("3  Where");
  wxBoxSizer *apply = new wxBoxSizer(wxHORIZONTAL);
  apply->Add(new wxStaticText(left, wxID_ANY, "Apply to:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  p_new = new wxRadioButton(left, wxID_ANY, "new workflow", wxDefaultPosition,
                            wxDefaultSize, wxRB_GROUP);
  p_afterButton = new wxRadioButton(left, wxID_ANY,
      after ? U("after the selected box \"" + after->getName() + "\"")
            : wxString("after the selected box (none selected)"));
  p_afterButton->Enable(after != 0);
  (p_applyAfter ? p_afterButton : p_new)->SetValue(true);
  p_new->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent &) { setApplyAfter(false); });
  p_afterButton->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent &) { setApplyAfter(true); });
  apply->Add(p_new, 0, wxRIGHT, 14);
  apply->Add(p_afterButton, 0);
  ls->Add(apply, 0, wxALL, 10);

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
  p_variantBox->Add(new wxStaticText(host, wxID_ANY, "Code:"), 0,
                    wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
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
      changed();
    });
    p_variantBox->Add(b, 0, wxRIGHT, 14);
    p_variantButtons.push_back(b);
  }
  p_variantBox->Layout();
}


bool RecipePanel::selectVariant(const string &id)
{
  if (!recipe()) return false;
  for (size_t i = 0; i < recipe()->variants.size(); i++)
    if (recipe()->variants[i].id == id && p_variantButtons[i]->IsEnabled()) {
      p_variant = (int)i;
      p_variantButtons[i]->SetValue(true);
      changed();
      return true;
    }
  return false;
}


void RecipePanel::fillRoles()
{
  p_roleSizer->Clear(true);
  p_roles.clear();
  for (const RecipeRole &role : recipe()->roles) {
    RoleChoice c = recommend(*recipe(), role, p_facts);
    wxPanel *row = new wxPanel(p_rolePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                               wxBORDER_SIMPLE);
    wxBoxSizer *rs = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *line = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText *label = new wxStaticText(row, wxID_ANY, U(role.label), wxDefaultPosition,
                                           wxSize(190, -1), wxST_ELLIPSIZE_END);
    wxComboBox *method = new wxComboBox(row, wxID_ANY, U(c.method), wxDefaultPosition,
                                        wxSize(130, -1));
    for (const char *m : kLevels) method->Append(m);
    wxComboBox *basis = new wxComboBox(row, wxID_ANY, U(c.basis), wxDefaultPosition,
                                       wxSize(130, -1));
    for (const char *b : kBases) basis->Append(b);
    line->Add(label, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    line->Add(method, 0, wxRIGHT, 8);
    line->Add(basis, 0);
    wxStaticText *why = new wxStaticText(row, wxID_ANY, U("Why: " + c.why));
    why->SetForegroundColour(wxColour(0x1e, 0x7e, 0x34));
    rs->Add(line, 0, wxALL, 4);
    rs->Add(why, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
    row->SetSizer(rs);
    p_roleSizer->Add(row, 0, wxEXPAND | wxBOTTOM, 4);
    RoleRow rr = {role.id, method, basis, why};
    p_roles.push_back(rr);
    auto edited = [this, &role, rr](wxCommandEvent &) {
      vector<string> w = checkLevel(*recipe(), string(rr.method->GetValue().ToUTF8()),
                                    string(rr.basis->GetValue().ToUTF8()), p_facts);
      string t = "Why: " + role.why;
      for (size_t i = 0; i < w.size(); i++) t += "\n" + w[i];
      rr.why->SetLabel(U(t));
      Layout();
    };
    method->Bind(wxEVT_TEXT, edited);
    basis->Bind(wxEVT_TEXT, edited);
    for (size_t i = 0; i < c.warnings.size(); i++) rr.why->SetLabel(U("Why: " + c.why + "\n" + c.warnings[i]));
  }
  p_facts_text->SetLabel(p_facts.known
      ? U("Rules applied to the selected box: charge " + std::to_string(p_facts.charge) +
          ", " + std::to_string(p_facts.atoms) + " atoms")
      : wxString("Defaults of the recipe; the rules apply again when a structure is set."));
}


bool RecipePanel::setLevel(const string &role, const string &method, const string &basis)
{
  for (size_t i = 0; i < p_roles.size(); i++)
    if (p_roles[i].id == role) {
      p_roles[i].method->SetValue(U(method));
      p_roles[i].basis->SetValue(U(basis));
      changed();
      return true;
    }
  return false;
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
    out[p_roles[i].id] = std::make_pair(string(p_roles[i].method->GetValue().ToUTF8()),
                                        string(p_roles[i].basis->GetValue().ToUTF8()));
  return out;
}


void RecipePanel::changed()
{
  p_preview->show(variant(), !p_applyAfter);
  Layout();
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
    o << string(p_roles[i].why->GetLabel().ToUTF8()).substr(0, 70) << ";";
  o << " variants-enabled=";
  for (size_t i = 0; i < p_variantButtons.size(); i++) o << (p_variantButtons[i]->IsEnabled() ? 1 : 0);
  return o.str();
}


void RecipePanel::createClicked()
{
  if (!variant()) return;
  string why = p_owner->recipeCreate(*recipe(), *variant(), levels(), after());
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
