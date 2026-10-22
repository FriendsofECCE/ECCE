// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <cstdlib>
#include <sstream>
#include <vector>

#include <wx/wrapsizer.h>

#include "dsm/MDCompositeModel.H"
#include "tdat/DynamicsModel.H"
#include "tdat/InteractionModel.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/GromacsInputsPanel.H"
#include "wxgui/GromacsRunPanel.H"
#include "wxgui/MDEdBase.H"

static const wxWindowID ID_RUN_FIELD = wxNewId();
static const wxWindowID ID_RUN_FILL  = wxNewId();
static const wxWindowID ID_RUN_FILLMOL = wxNewId();
static const wxWindowID ID_RUN_CHAIN = wxNewId();

static wxString W(const string& s) { return wxString::FromUTF8(s.c_str()); }
static string S(const wxString& w) { return string(w.ToUTF8()); }

static ewxChoice *makeChoice(wxWindow *parent, const char *a, const char *b,
                             const char *c = 0)
{
  wxArrayString items;
  items.Add(wxString::FromUTF8(a));
  items.Add(wxString::FromUTF8(b));
  if (c) items.Add(wxString::FromUTF8(c));
  ewxChoice *ch = new ewxChoice(parent, ID_RUN_FIELD, wxDefaultPosition,
                                wxDefaultSize, items);
  ch->SetSelection(0);
  return ch;
}

GromacsRunPanel::GromacsRunPanel(MDEdBase *owner, wxWindow *parent)
  : ewxPanel(parent, wxID_ANY)
{
  p_owner = owner;
  p_loading = false;
  p_editable = true;

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);

  // ---- defines and restraints ----
  wxStaticBoxSizer *defs = new wxStaticBoxSizer(wxVERTICAL, this,
                                                _("Defines and restraints"));
  wxWindow *db = defs->GetStaticBox();
  wxFlexGridSizer *g1 = new wxFlexGridSizer(2, 4, 6);
  g1->AddGrowableCol(1);
  g1->Add(new ewxStaticText(db, wxID_ANY, _("Defines:")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_defines = new ewxTextCtrl(db, ID_RUN_FIELD);
  p_defines->SetToolTip(_("Flags for the topology's preprocessor, for example "
                          "-DFLEXIBLE -DOPC3_IOD_LM. Written as the define line."));
  g1->Add(p_defines, 1, wxEXPAND);
  p_posres = new ewxCheckBox(db, ID_RUN_FIELD, _("Restrain positions"));
  g1->Add(p_posres, 0, wxALIGN_CENTER_VERTICAL);
  wxBoxSizer *pr = new wxBoxSizer(wxHORIZONTAL);
  pr->Add(new ewxStaticText(db, wxID_ANY, _("named")), 0,
          wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_posresName = new ewxTextCtrl(db, ID_RUN_FIELD, "POSRES",
                                 wxDefaultPosition, wxSize(160, -1));
  p_posresName->SetToolTip(_("Adds -D<name> to the defines unless they already "
                             "have it. Several names may be given, for a "
                             "topology that restrains per chain; the topology must use this name in an "
                             "#ifdef around [ position_restraints ]."));
  pr->Add(p_posresName, 0);
  g1->Add(pr, 0);
  defs->Add(g1, 0, wxALL | wxEXPAND, 6);
  p_defsBox = db;
  p_chainRow = new wxWrapSizer(wxHORIZONTAL);
  defs->Add(p_chainRow, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 6);
  top->Add(defs, 0, wxALL | wxEXPAND, 6);

  // ---- groups by residue name (dynamics tasks) ----
  p_groupBox = new wxPanel(this);
  wxStaticBoxSizer *grp = new wxStaticBoxSizer(wxVERTICAL, p_groupBox,
                                               _("Groups by residue name"));
  wxWindow *gb = grp->GetStaticBox();
  ewxStaticText *gnote = new ewxStaticText(gb, wxID_ANY,
      _("One group per line: name = residue names, for example "
        "Water_and_ions = SOL Cu; mol:<name> takes all atoms of a molecule "
        "of the topology, such as a protein chain (chainA = "
        "mol:Protein_chain_A). ECCE writes the index file from the "
        "starting structure. With an .ndx file attached on the Inputs page "
        "the group names are yours and ECCE writes no index file."));
  gnote->Wrap(560);
  grp->Add(gnote, 0, wxALL | wxEXPAND, 4);
  p_groups = new ewxTextCtrl(gb, ID_RUN_FIELD, wxEmptyString,
                             wxDefaultPosition, wxSize(-1, 70),
                             wxTE_MULTILINE);
  grp->Add(p_groups, 1, wxALL | wxEXPAND, 4);
  ewxButton *fill = new ewxButton(gb, ID_RUN_FILL,
                                  _("One group per residue name"));
  ewxButton *fillMol = new ewxButton(gb, ID_RUN_FILLMOL,
                                     _("One group per molecule (chains)"));
  wxBoxSizer *fills = new wxBoxSizer(wxHORIZONTAL);
  fills->Add(fill, 0, wxRIGHT, 6);
  fills->Add(fillMol, 0);
  grp->Add(fills, 0, wxALL, 4);
  wxFlexGridSizer *g2 = new wxFlexGridSizer(2, 4, 6);
  g2->Add(new ewxStaticText(gb, wxID_ANY, _("Centre of mass removal groups:")),
          0, wxALIGN_CENTER_VERTICAL);
  p_comGroups = makeChoice(gb, "System", "Defined groups");
  g2->Add(p_comGroups);
  g2->Add(new ewxStaticText(gb, wxID_ANY, _("Thermostat groups:")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_tcGroups = makeChoice(gb, "System", "Defined groups");
  g2->Add(p_tcGroups);
  grp->Add(g2, 0, wxALL, 4);
  p_groupBox->SetSizer(grp);
  top->Add(p_groupBox, 0, wxALL | wxEXPAND, 6);

  // ---- pressure coupling (dynamics tasks) ----
  p_pressureBox = new wxPanel(this);
  wxStaticBoxSizer *prs = new wxStaticBoxSizer(wxVERTICAL, p_pressureBox,
                                               _("Pressure coupling"));
  wxWindow *pb = prs->GetStaticBox();
  wxFlexGridSizer *g3 = new wxFlexGridSizer(2, 4, 6);
  g3->Add(new ewxStaticText(pb, wxID_ANY, _("Barostat:")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_barostat = makeChoice(pb, "C-rescale", "Berendsen", "Parrinello-Rahman");
  g3->Add(p_barostat);
  g3->Add(new ewxStaticText(pb, wxID_ANY, _("Coupling type:")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_pcoupltype = makeChoice(pb, "isotropic", "semiisotropic", "anisotropic");
  g3->Add(p_pcoupltype);
  g3->Add(new ewxStaticText(pb, wxID_ANY, _("Coupling interval (steps):")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_nstpcouple = new ewxTextCtrl(pb, ID_RUN_FIELD, wxEmptyString,
                                 wxDefaultPosition, wxSize(80, -1));
  p_nstpcouple->SetToolTip(_("nstpcouple; leave blank for grompp's own choice."));
  g3->Add(p_nstpcouple);
  prs->Add(g3, 0, wxALL, 4);
  ewxStaticText *pnote = new ewxStaticText(pb, wxID_ANY,
      _("Used when constant pressure is switched on on the Pressure tab. The "
        "pressure and compressibility there are repeated for every direction "
        "the type has."));
  pnote->Wrap(560);
  prs->Add(pnote, 0, wxALL | wxEXPAND, 4);
  p_pressureBox->SetSizer(prs);
  top->Add(p_pressureBox, 0, wxALL | wxEXPAND, 6);

  // ---- centre-of-mass pulling (dynamics tasks) ----
  p_pullBox = new wxPanel(this);
  wxStaticBoxSizer *pul = new wxStaticBoxSizer(wxVERTICAL, p_pullBox,
                                               _("Centre-of-mass pulling"));
  wxWindow *ub = pul->GetStaticBox();
  p_pull = new ewxCheckBox(ub, ID_RUN_FIELD, _("Pull group 2 away from group 1"));
  p_pull->SetToolTip(_("Writes the pull code of the .mdp: a moving harmonic "
                       "(umbrella) potential on the distance between the "
                       "centres of mass of two groups. pullx.xvg and "
                       "pullf.xvg come back as the Pull Force plot."));
  pul->Add(p_pull, 0, wxALL, 4);
  wxFlexGridSizer *g4 = new wxFlexGridSizer(2, 4, 6);
  g4->Add(new ewxStaticText(ub, wxID_ANY, _("Group 1 (reference):")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_pullGroup1 = new ewxTextCtrl(ub, ID_RUN_FIELD, wxEmptyString,
                                 wxDefaultPosition, wxSize(160, -1));
  p_pullGroup1->SetToolTip(_("Name of a group defined above (or in the .ndx "
                             "file), for example Chain_A. To hold it in "
                             "place, tick Restrain positions with "
                             "POSRES_<chain>."));
  g4->Add(p_pullGroup1);
  g4->Add(new ewxStaticText(ub, wxID_ANY, _("Group 2 (pulled):")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_pullGroup2 = new ewxTextCtrl(ub, ID_RUN_FIELD, wxEmptyString,
                                 wxDefaultPosition, wxSize(160, -1));
  g4->Add(p_pullGroup2);
  g4->Add(new ewxStaticText(ub, wxID_ANY, _("Pull coordinate:")), 0,
          wxALIGN_CENTER_VERTICAL);
  wxBoxSizer *geo = new wxBoxSizer(wxHORIZONTAL);
  p_pullGeom = makeChoice(ub, "distance", "direction along axis");
  p_pullAxis = makeChoice(ub, "x", "y", "z");
  p_pullAxis->SetSelection(2);
  geo->Add(p_pullGeom, 0, wxRIGHT, 6);
  geo->Add(p_pullAxis, 0);
  g4->Add(geo);
  g4->Add(new ewxStaticText(ub, wxID_ANY, _("Distance measured on:")), 0,
          wxALIGN_CENTER_VERTICAL);
  wxBoxSizer *dim = new wxBoxSizer(wxHORIZONTAL);
  const char *axes[3] = {"x", "y", "z"};
  for (int i = 0; i < 3; i++) {
    p_pullDim[i] = new ewxCheckBox(ub, ID_RUN_FIELD, axes[i]);
    p_pullDim[i]->SetValue(true);
    dim->Add(p_pullDim[i], 0, wxRIGHT, 8);
  }
  g4->Add(dim);
  g4->Add(new ewxStaticText(ub, wxID_ANY, _("Rate (nm/ps):")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_pullRate = new ewxTextCtrl(ub, ID_RUN_FIELD, "0.01", wxDefaultPosition,
                               wxSize(100, -1));
  g4->Add(p_pullRate);
  g4->Add(new ewxStaticText(ub, wxID_ANY, _("Force constant (kJ/mol/nm^2):")), 0,
          wxALIGN_CENTER_VERTICAL);
  p_pullK = new ewxTextCtrl(ub, ID_RUN_FIELD, "1000", wxDefaultPosition,
                            wxSize(100, -1));
  g4->Add(p_pullK);
  pul->Add(g4, 0, wxALL, 4);
  p_pullBox->SetSizer(pul);
  top->Add(p_pullBox, 0, wxALL | wxEXPAND, 6);

  // ---- extra lines ----
  wxStaticBoxSizer *ext = new wxStaticBoxSizer(wxVERTICAL, this,
                                               _("Extra .mdp lines"));
  wxWindow *eb = ext->GetStaticBox();
  ewxStaticText *enote = new ewxStaticText(eb, wxID_ANY,
      _("Written at the end of the .mdp, for example DispCorr = EnerPres. A "
        "key written here replaces the line ECCE would write for it."));
  enote->Wrap(560);
  ext->Add(enote, 0, wxALL | wxEXPAND, 4);
  p_extra = new ewxTextCtrl(eb, ID_RUN_FIELD, wxEmptyString,
                            wxDefaultPosition, wxSize(-1, 90),
                            wxTE_MULTILINE);
  ext->Add(p_extra, 1, wxALL | wxEXPAND, 4);
  top->Add(ext, 1, wxALL | wxEXPAND, 6);

  p_warning = new ewxStaticText(this, wxID_ANY, wxEmptyString);
  top->Add(p_warning, 0, wxALL | wxEXPAND, 10);

  SetSizerAndFit(top);   // sizes the static boxes before GTK first allocates them

  Bind(wxEVT_TEXT, &GromacsRunPanel::OnChanged, this, ID_RUN_FIELD);
  Bind(wxEVT_CHECKBOX, &GromacsRunPanel::OnChanged, this, ID_RUN_FIELD);
  Bind(wxEVT_CHOICE, &GromacsRunPanel::OnChanged, this, ID_RUN_FIELD);
  Bind(wxEVT_CHECKBOX, &GromacsRunPanel::OnChain, this, ID_RUN_CHAIN);
  Bind(wxEVT_BUTTON, &GromacsRunPanel::OnFill, this, ID_RUN_FILL);
  Bind(wxEVT_BUTTON, &GromacsRunPanel::OnFillMolecules, this, ID_RUN_FILLMOL);
}

GromacsRunPanel::~GromacsRunPanel()
{
  p_owner = 0;
}

void GromacsRunPanel::setEditable(bool editable)
{
  p_editable = editable;
  Enable(editable);
}

void GromacsRunPanel::refresh()
{
  MDCompositeModel& m = p_owner->getMDModel();
  InteractionModel *im = m.getInteractionModel();
  if (im == 0) return;
  const GromacsRun& g = im->getGromacsRun();
  p_loading = true;
  p_defines->ChangeValue(W(g.defines));
  p_posres->SetValue(g.posres);
  p_posresName->ChangeValue(W(g.posresName));
  p_groups->ChangeValue(W(g.groups));
  p_comGroups->SetSelection(g.comGroups ? 1 : 0);
  p_tcGroups->SetSelection(g.tcGroups ? 1 : 0);
  p_barostat->SetSelection(g.barostat >= 0 && g.barostat <= 2 ? g.barostat : 0);
  p_pcoupltype->SetSelection(g.pcoupltype >= 0 && g.pcoupltype <= 2 ?
                             g.pcoupltype : 0);
  p_nstpcouple->ChangeValue(g.nstpcouple > 0 ?
      wxString::Format("%d", g.nstpcouple) : wxString());
  p_extra->ChangeValue(W(g.extraMdp));
  p_pull->SetValue(g.pull);
  p_pullGroup1->ChangeValue(W(g.pullGroup1));
  p_pullGroup2->ChangeValue(W(g.pullGroup2));
  p_pullGeom->SetSelection(g.pullGeom == 1 ? 1 : 0);
  p_pullAxis->SetSelection(g.pullAxis >= 0 && g.pullAxis <= 2 ? g.pullAxis : 2);
  for (int i = 0; i < 3; i++)
    p_pullDim[i]->SetValue(g.pullDim.size() > (size_t)(2 * i) &&
                           g.pullDim[2 * i] == 'Y');
  p_pullRate->ChangeValue(wxString::Format("%g", g.pullRate));
  p_pullK->ChangeValue(wxString::Format("%g", g.pullK));
  p_loading = false;

  bool dyn = m.getDynamicsModel() != 0;
  refreshChains();
  p_groupBox->Show(dyn);
  p_pressureBox->Show(dyn);
  p_pullBox->Show(dyn);
  warn();
  Layout();
}

void GromacsRunPanel::store()
{
  if (p_loading || p_owner == 0) return;
  InteractionModel *im = p_owner->getMDModel().getInteractionModel();
  if (im == 0) return;
  GromacsRun g = im->getGromacsRun();
  g.defines = S(p_defines->GetValue());
  g.posres = p_posres->GetValue();
  string name = S(p_posresName->GetValue());
  g.posresName = name.empty() ? GromacsRun().posresName : name;
  g.groups = S(p_groups->GetValue());
  g.comGroups = p_comGroups->GetSelection() == 1 ? 1 : 0;
  g.tcGroups = p_tcGroups->GetSelection() == 1 ? 1 : 0;
  g.barostat = p_barostat->GetSelection();
  g.pcoupltype = p_pcoupltype->GetSelection();
  int n = atoi(S(p_nstpcouple->GetValue()).c_str());
  g.nstpcouple = n > 0 ? n : 0;
  g.extraMdp = S(p_extra->GetValue());
  g.pull = p_pull->GetValue();
  g.pullGroup1 = S(p_pullGroup1->GetValue());
  g.pullGroup2 = S(p_pullGroup2->GetValue());
  g.pullGeom = p_pullGeom->GetSelection() == 1 ? 1 : 0;
  g.pullAxis = p_pullAxis->GetSelection();
  g.pullDim = string(p_pullDim[0]->GetValue() ? "Y" : "N") + " " +
              (p_pullDim[1]->GetValue() ? "Y" : "N") + " " +
              (p_pullDim[2]->GetValue() ? "Y" : "N");
  g.pullRate = atof(S(p_pullRate->GetValue()).c_str());
  g.pullK = atof(S(p_pullK->GetValue()).c_str());
  if (g != im->getGromacsRun()) {
    im->setGromacsRun(g);
    p_owner->setSaveState(true);
  }
  warn();
}

// Mistakes the user can still fix, said where they are made; none blocks.
void GromacsRunPanel::warn()
{
  MDCompositeModel& m = p_owner->getMDModel();
  InteractionModel *im = m.getInteractionModel();
  string msg;
  if (im != 0 && m.getDynamicsModel() != 0) {
    const GromacsRun& g = im->getGromacsRun();
    if (m.getDynamicsModel()->getUseNPT()) {
      msg = GromacsRun::pairProblem(g.barostat, g.pcoupltype);
    }
    if (g.pull && (g.pullGroup1.empty() || g.pullGroup2.empty())) {
      msg += (msg.empty() ? "" : "\n") + string(
          "Pulling is on: name the two groups to pull between.");
    }
    bool defined = g.groups.find_first_not_of(" \t\r\n") != string::npos;
    if ((g.comGroups || g.tcGroups) && !defined) {
      msg += (msg.empty() ? "" : "\n") + string(
          "Defined groups are chosen but no group is defined.");
    }
  }
  p_warning->SetLabel(W(msg));
  p_warning->Wrap(560);
  Layout();
}

// Chains that pdb2gmx left unmerged are molecules named <x>_chain_<id>; the
// prepared topology restrains each under POSRES_<id>.  Merged or unprepared
// topologies have none, and then no boxes are shown.
static vector<string> chainIds(const vector<string>& molecules)
{
  vector<string> ids;
  const string key = "_chain_";
  for (size_t i = 0; i < molecules.size(); i++) {
    size_t p = molecules[i].rfind(key);
    if (p == string::npos || p + key.size() >= molecules[i].size()) continue;
    string id = molecules[i].substr(p + key.size());
    bool seen = false;
    for (size_t j = 0; j < ids.size(); j++) if (ids[j] == id) seen = true;
    if (!seen) ids.push_back(id);
  }
  return ids;
}

static vector<string> words(const string& s)
{
  vector<string> w;
  std::istringstream in(s);
  string t;
  while (in >> t) w.push_back(t);
  return w;
}

void GromacsRunPanel::refreshChains()
{
  if (p_chainRow == 0) return;
  GromacsInputsPanel *in = p_owner ? p_owner->gromacsInputs() : 0;
  vector<string> ids = in ? chainIds(in->moleculeNames()) : vector<string>();
  if (ids != p_chainIds) {
    p_chainRow->Clear(true);
    p_chainBoxes.clear();
    p_chainIds = ids;
    for (size_t i = 0; i < ids.size(); i++) {
      ewxCheckBox *cb = new ewxCheckBox(p_defsBox, ID_RUN_CHAIN,
          wxString::Format(_("Restrain chain %s"), W(ids[i])));
      cb->SetToolTip(wxString::Format(_("Adds -DPOSRES_%s to the defines. "
          "The prepared topology restrains this chain alone under that name."),
          W(ids[i])));
      p_chainRow->Add(cb, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 12);
      p_chainBoxes.push_back(cb);
    }
  }
  vector<string> have = words(S(p_defines->GetValue()));
  for (size_t i = 0; i < p_chainIds.size(); i++) {
    string flag = "-DPOSRES_" + p_chainIds[i];
    bool on = false;
    for (size_t j = 0; j < have.size(); j++) if (have[j] == flag) on = true;
    p_chainBoxes[i]->SetValue(on);
  }
  Layout();
}

// A chain box adds or removes its flag in the Defines field, leaving the
// rest of what the user typed alone.
void GromacsRunPanel::OnChain(wxCommandEvent& event)
{
  setChain(event.GetEventObject());
  event.Skip();
}

void GromacsRunPanel::setChain(wxObject *box)
{
  vector<string> have = words(S(p_defines->GetValue()));
  string out;
  for (size_t i = 0; i < p_chainIds.size(); i++) {
    if (box != p_chainBoxes[i]) continue;
    string flag = "-DPOSRES_" + p_chainIds[i];
    bool on = p_chainBoxes[i]->GetValue();
    bool found = false;
    vector<string> kept;
    for (size_t j = 0; j < have.size(); j++) {
      if (have[j] == flag) { found = true; if (on) kept.push_back(have[j]); }
      else kept.push_back(have[j]);
    }
    if (on && !found) kept.push_back(flag);
    have = kept;
  }
  for (size_t j = 0; j < have.size(); j++) out += (j ? " " : "") + have[j];
  p_defines->ChangeValue(W(out));
  store();
}

void GromacsRunPanel::OnChanged(wxCommandEvent& event)
{
  if (!p_loading && event.GetEventObject() == p_defines) refreshChains();
  store();
  event.Skip();
}

void GromacsRunPanel::fillGroups(bool molecules)
{
  GromacsInputsPanel *in = p_owner->gromacsInputs();
  vector<string> names;
  if (in != 0) names = molecules ? in->moleculeNames() : in->residueNames();
  if (names.empty()) {
    p_warning->SetLabel(molecules
        ? _("No molecules found: attach a topology on the Inputs page first.")
        : _("No residue names found: attach a starting structure (.gro) on "
            "the Inputs page first."));
    p_warning->Wrap(560);
    Layout();
    return;
  }
  string text;
  for (size_t i = 0; i < names.size(); i++) {
    text += (i ? "\n" : "") + names[i] + " = " + (molecules ? "mol:" : "") + names[i];
  }
  p_groups->SetValue(W(text));
}

void GromacsRunPanel::OnFill(wxCommandEvent& event)
{
  fillGroups(false);
  event.Skip();
}

void GromacsRunPanel::OnFillMolecules(wxCommandEvent& event)
{
  fillGroups(true);
  event.Skip();
}

// One line per answer: the hook's output file is read by line.
static string oneLine(string s)
{
  size_t p;
  while ((p = s.find('\n')) != string::npos) s.replace(p, 1, "\\n");
  return s;
}

// "A:1,B:0" (ticked or not), "none" without chain boxes.
string GromacsRunPanel::chainSummary()
{
  string out;
  for (size_t i = 0; i < p_chainIds.size(); i++)
    out += (i ? "," : "") + p_chainIds[i] + ":" + (p_chainBoxes[i]->GetValue() ? "1" : "0");
  return out.empty() ? "none" : out;
}

string GromacsRunPanel::summary()
{
  InteractionModel *im = p_owner->getMDModel().getInteractionModel();
  if (im == 0) return "no model";
  const GromacsRun& g = im->getGromacsRun();
  std::ostringstream os;
  os << "defines=" << S(p_defines->GetValue())
     << " posres=" << (p_posres->GetValue() ? 1 : 0)
     << " posresname=" << S(p_posresName->GetValue())
     << " chains=" << chainSummary()
     << " groups=[" << oneLine(S(p_groups->GetValue())) << "]"
     << " comgroups=" << p_comGroups->GetSelection()
     << " tcgroups=" << p_tcGroups->GetSelection()
     << " barostat=" << p_barostat->GetSelection()
     << " pcoupltype=" << p_pcoupltype->GetSelection()
     << " nstpcouple=" << S(p_nstpcouple->GetValue())
     << " extra=[" << oneLine(S(p_extra->GetValue())) << "]"
     << " pull=" << (p_pull->GetValue() ? 1 : 0)
     << " pullgroups=" << S(p_pullGroup1->GetValue()) << ","
     << S(p_pullGroup2->GetValue())
     << " pullgeom=" << p_pullGeom->GetSelection()
     << " pullrate=" << S(p_pullRate->GetValue())
     << " pullk=" << S(p_pullK->GetValue())
     << " shown=" << (p_groupBox->IsShown() ? "groups+pressure" : "basic")
     << " model-defines=" << g.defines
     << " warning=[" << oneLine(S(p_warning->GetLabel())) << "]";
  return os.str();
}

string GromacsRunPanel::setField(const string& name, const string& value)
{
  wxString v = W(value);
  if (name == "defines") p_defines->SetValue(v);
  else if (name == "posres") p_posres->SetValue(value == "1");
  else if (name == "posresname") p_posresName->SetValue(v);
  else if (name.compare(0, 6, "chain:") == 0) {
    refreshChains();
    for (size_t i = 0; i < p_chainIds.size(); i++) {
      if (p_chainIds[i] != name.substr(6)) continue;
      p_chainBoxes[i]->SetValue(value == "1");
      setChain(p_chainBoxes[i]);
      return "ok";
    }
    return "no such chain";
  }
  else if (name == "groups") p_groups->SetValue(v);
  else if (name == "extra") p_extra->SetValue(v);
  else if (name == "nstpcouple") p_nstpcouple->SetValue(v);
  else if (name == "comgroups") p_comGroups->SetSelection(atoi(value.c_str()));
  else if (name == "tcgroups") p_tcGroups->SetSelection(atoi(value.c_str()));
  else if (name == "barostat") p_barostat->SetSelection(atoi(value.c_str()));
  else if (name == "pcoupltype") p_pcoupltype->SetSelection(atoi(value.c_str()));
  else if (name == "pull") p_pull->SetValue(value == "1");
  else if (name == "pullgroup1") p_pullGroup1->SetValue(v);
  else if (name == "pullgroup2") p_pullGroup2->SetValue(v);
  else if (name == "pullgeom") p_pullGeom->SetSelection(atoi(value.c_str()));
  else if (name == "pullaxis") p_pullAxis->SetSelection(atoi(value.c_str()));
  else if (name == "pullrate") p_pullRate->SetValue(v);
  else if (name == "pullk") p_pullK->SetValue(v);
  else if (name == "fill") { fillGroups(false); return "ok"; }
  else if (name == "fillmol") { fillGroups(true); return "ok"; }
  else return "unknown field";
  store();   // choices and check boxes raise no event when set in code
  return "ok";
}
