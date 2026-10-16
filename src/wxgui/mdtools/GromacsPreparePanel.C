// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
  using std::vector;
  using std::ifstream;

#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/Preferences.H"

#include "dsm/EDSIFactory.H"
#include "dsm/GromacsPrepareModel.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxFileDialog.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/MDEdBase.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/GromacsPreparePanel.H"

static const wxWindowID ID_FF = wxNewId();
static const wxWindowID ID_OTHERFF = wxNewId();
static const wxWindowID ID_WATER = wxNewId();
static const wxWindowID ID_STRUCT = wxNewId();
static const wxWindowID ID_BROWSE = wxNewId();
static const wxWindowID ID_IGNH = wxNewId();
static const wxWindowID ID_MERGE = wxNewId();
static const wxWindowID ID_BOX = wxNewId();
static const wxWindowID ID_DIST = wxNewId();
static const wxWindowID ID_CONC = wxNewId();
static const wxWindowID ID_PION = wxNewId();
static const wxWindowID ID_NION = wxNewId();

// The force fields GROMACS 2025.2 ships and the water models each one's
// watermodels.dat offers, the recommended one first.  Static: the editor must
// not need a gmx on the machine it runs on.
struct WaterInfo { const char *id; const char *label; };
static const WaterInfo WATERS[] = {
  { "tip3p",   "TIP3P" },     { "tip4p",   "TIP4P" },
  { "tip4pew", "TIP4P-Ew" },  { "tip5p",   "TIP5P" },
  { "tip5pe",  "TIP5P (Ewald)" }, { "tips3p",  "TIPS3P" },
  { "spc",     "SPC" },       { "spce",    "SPC/E" },
};

struct ForceFieldInfo { const char *id; const char *label; const char *waters; };
static const ForceFieldInfo FORCEFIELDS[] = {
  { "amber99sb-ildn", "AMBER99SB-ILDN",   "tip3p tip4p tip4pew tip5p spc spce" },
  { "amber99sb",      "AMBER99SB",        "tip3p tip4p tip4pew tip5p spc spce" },
  { "amber99",        "AMBER99",          "tip3p tip4p tip4pew tip5p spc spce" },
  { "amber96",        "AMBER96",          "tip3p tip4p tip4pew tip5p spc spce" },
  { "amber94",        "AMBER94",          "tip3p tip4p tip4pew tip5p spc spce" },
  { "amber03",        "AMBER03",          "tip3p tip4p tip4pew tip5p spc spce" },
  { "amberGS",        "AMBERGS",          "tip3p tip4p tip4pew tip5p spc spce" },
  { "charmm27",       "CHARMM27",         "tip3p tip4p tips3p tip5p spc spce" },
  { "oplsaa",         "OPLS-AA/L",        "tip4p tip4pew tip3p tip5p tip5pe spc spce" },
  { "gromos43a1",     "GROMOS96 43a1",    "spc spce" },
  { "gromos43a2",     "GROMOS96 43a2",    "spc spce" },
  { "gromos45a3",     "GROMOS96 45a3",    "spc spce" },
  { "gromos53a5",     "GROMOS96 53a5",    "spc spce" },
  { "gromos53a6",     "GROMOS96 53a6",    "spc spce" },
  { "gromos54a7",     "GROMOS96 54a7",    "spc spce" },
};
static const int NFF = sizeof(FORCEFIELDS) / sizeof(FORCEFIELDS[0]);
static const int OTHER_FF = NFF;   // the last entry of the choice

static const char *BOXES[][2] = {
  { "dodecahedron", "Rhombic dodecahedron" }, { "cubic", "Cubic" },
  { "octahedron", "Truncated octahedron" },   { "triclinic", "Triclinic" },
};
static const int NBOX = 4;

static const char *POSITIVE[] = { "NA", "K" };
static const char *NEGATIVE[] = { "CL" };

static string waterLabel(const string& id)
{
  for (size_t i = 0; i < sizeof(WATERS) / sizeof(WATERS[0]); i++)
    if (id == WATERS[i].id) return WATERS[i].label;
  return id;
}

static int forceFieldIndex(const string& id)
{
  for (int i = 0; i < NFF; i++) if (id == FORCEFIELDS[i].id) return i;
  return OTHER_FF;
}

static vector<string> watersOf(int ff)
{
  vector<string> out;
  std::istringstream in(ff < NFF ? FORCEFIELDS[ff].waters
                                 : "tip3p tip4p tip4pew tip5p tip5pe tips3p spc spce");
  string w;
  while (in >> w) out.push_back(w);
  return out;
}

static bool isNumber(const string& s, double& v, bool allowZero)
{
  char *end = 0;
  v = strtod(s.c_str(), &end);
  if (s.empty() || *end != 0 || end == s.c_str()) return false;
  return allowZero ? v >= 0 : v > 0;
}

static wxString U(const char *s) { return wxString::FromUTF8(s); }

static ewxStaticText *label(wxWindow *parent, const wxString& text)
{
  return new ewxStaticText(parent, wxID_ANY, text);
}

GromacsPreparePanel::GromacsPreparePanel(MDEdBase *owner, wxWindow *parent)
  : ewxPanel(parent, wxID_ANY)
{
  p_owner = owner;
  p_model = 0;
  p_editable = true;
  p_refreshing = false;

  wxBoxSizer *columns = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *right = new wxBoxSizer(wxVERTICAL);

  // ---- structure ----
  wxStaticBoxSizer *sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Structure"));
  wxWindow *box = sb->GetStaticBox();
  wxFlexGridSizer *g = new wxFlexGridSizer(2, 6, 8);
  g->AddGrowableCol(1);
  g->Add(label(box, _("Source:")), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString src;
  src.Add(_("Protein (pdb2gmx)"));
  p_source = new ewxChoice(box, wxID_ANY, wxDefaultPosition, wxDefaultSize, src);
  p_source->SetSelection(0);
  g->Add(p_source, 1, wxEXPAND);
  g->Add(label(box, _("Structure:")), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString st;
  st.Add(_("from the Builder"));
  st.Add(_("PDB file..."));
  p_structure = new ewxChoice(box, ID_STRUCT, wxDefaultPosition, wxDefaultSize, st);
  p_structure->SetSelection(0);
  g->Add(p_structure, 1, wxEXPAND);
  p_browse = new ewxButton(box, ID_BROWSE, _("Browse..."));
  g->AddSpacer(0);
  g->Add(p_browse, 0);
  g->AddSpacer(0);
  p_file = label(box, wxEmptyString);
  g->Add(p_file, 1, wxEXPAND);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  left->Add(sb, 0, wxALL | wxEXPAND, 4);

  // ---- force field ----
  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Force field"));
  box = sb->GetStaticBox();
  g = new wxFlexGridSizer(2, 6, 8);
  g->AddGrowableCol(1);
  g->Add(label(box, _("Force field:")), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString ffs;
  for (int i = 0; i < NFF; i++) ffs.Add(U(FORCEFIELDS[i].label));
  ffs.Add(_("Other (GMXLIB)"));
  p_ff = new ewxChoice(box, ID_FF, wxDefaultPosition, wxDefaultSize, ffs);
  g->Add(p_ff, 1, wxEXPAND);
  g->Add(label(box, _("Directory name:")), 0, wxALIGN_CENTER_VERTICAL);
  p_otherFf = new ewxTextCtrl(box, ID_OTHERFF, wxEmptyString);
  p_otherFf->SetToolTip(_("The name of a force field folder (without .ff) that "
                          "GROMACS finds in its library or in GMXLIB."));
  g->Add(p_otherFf, 1, wxEXPAND);
  g->Add(label(box, _("Water model:")), 0, wxALIGN_CENTER_VERTICAL);
  p_water = new ewxChoice(box, ID_WATER);
  g->Add(p_water, 1, wxEXPAND);
  g->Add(label(box, _("Histidines:")), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(label(box, _("automatic (names in the PDB are kept)")), 1,
         wxALIGN_CENTER_VERTICAL);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  p_ignh = new ewxCheckBox(box, ID_IGNH, _("Ignore hydrogens in the PDB (-ignh)"));
  sb->Add(p_ignh, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  p_merge = new ewxCheckBox(box, ID_MERGE, _("Merge chains into one molecule"));
  sb->Add(p_merge, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  left->Add(sb, 0, wxALL | wxEXPAND, 4);

  // ---- box and ions ----
  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Box and water"));
  box = sb->GetStaticBox();
  g = new wxFlexGridSizer(3, 6, 8);
  g->AddGrowableCol(1);
  g->Add(label(box, _("Box type:")), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString boxes;
  for (int i = 0; i < NBOX; i++) boxes.Add(U(BOXES[i][1]));
  p_box = new ewxChoice(box, ID_BOX, wxDefaultPosition, wxDefaultSize, boxes);
  g->Add(p_box, 1, wxEXPAND);
  g->AddSpacer(0);
  g->Add(label(box, _("Distance to edge:")), 0, wxALIGN_CENTER_VERTICAL);
  p_dist = new ewxTextCtrl(box, ID_DIST, wxEmptyString);
  g->Add(p_dist, 1, wxEXPAND);
  g->Add(label(box, _("nm")), 0, wxALIGN_CENTER_VERTICAL);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  right->Add(sb, 0, wxALL | wxEXPAND, 4);

  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Ions"));
  box = sb->GetStaticBox();
  ewxStaticText *neutral = label(box,
      _("The system is always neutralised. Salt is added on top."));
  sb->Add(neutral, 0, wxALL, 6);
  g = new wxFlexGridSizer(3, 6, 8);
  g->AddGrowableCol(1);
  g->Add(label(box, _("Salt concentration:")), 0, wxALIGN_CENTER_VERTICAL);
  p_conc = new ewxTextCtrl(box, ID_CONC, wxEmptyString);
  p_conc->SetToolTip(_("0 neutralises only."));
  g->Add(p_conc, 1, wxEXPAND);
  g->Add(label(box, _("mol/L")), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(label(box, _("Positive ion:")), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString pos;
  pos.Add(_("Sodium (NA)"));
  pos.Add(_("Potassium (K)"));
  p_pion = new ewxChoice(box, ID_PION, wxDefaultPosition, wxDefaultSize, pos);
  g->Add(p_pion, 1, wxEXPAND);
  g->AddSpacer(0);
  g->Add(label(box, _("Negative ion:")), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString neg;
  neg.Add(_("Chloride (CL)"));
  p_nion = new ewxChoice(box, ID_NION, wxDefaultPosition, wxDefaultSize, neg);
  g->Add(p_nion, 1, wxEXPAND);
  g->AddSpacer(0);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  right->Add(sb, 0, wxALL | wxEXPAND, 4);

  columns->Add(left, 1, wxEXPAND);
  columns->Add(right, 1, wxEXPAND);
  SetSizer(columns);

  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnForceField, this, ID_FF);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnOtherForceField, this, ID_OTHERFF);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnWater, this, ID_WATER);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnStructure, this, ID_STRUCT);
  Bind(wxEVT_BUTTON, &GromacsPreparePanel::OnBrowse, this, ID_BROWSE);
  Bind(wxEVT_CHECKBOX, &GromacsPreparePanel::OnCheck, this, ID_IGNH);
  Bind(wxEVT_CHECKBOX, &GromacsPreparePanel::OnCheck, this, ID_MERGE);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnBox, this, ID_BOX);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnNumber, this, ID_DIST);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnNumber, this, ID_CONC);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnIon, this, ID_PION);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnIon, this, ID_NION);
}

GromacsPreparePanel::~GromacsPreparePanel()
{
  p_owner = 0;
}

void GromacsPreparePanel::setModel(GromacsPrepareModel *model)
{
  p_model = model;
}

void GromacsPreparePanel::setContext(const string& url)
{
  p_url = url;
  showStructure();
}

void GromacsPreparePanel::setEditable(bool editable)
{
  p_editable = editable;
  showFields();
}

MdTask *GromacsPreparePanel::task() const
{
  if (p_url.empty()) return 0;
  try {
    return dynamic_cast<MdTask*>(EDSIFactory::getResource(EcceURL(p_url)));
  } catch (...) {
    return 0;
  }
}

void GromacsPreparePanel::changed()
{
  if (!p_refreshing && p_owner != 0) p_owner->setSaveState(true);
}

static bool endsWith(const string& s, const string& tail)
{
  return s.size() >= tail.size() &&
         s.compare(s.size() - tail.size(), tail.size(), tail) == 0;
}

string GromacsPreparePanel::sourceFile() const
{
  MdTask *t = task();
  if (t == 0) return "";
  vector<string> names = t->getDataFileNames(JCode::AUXILIARY_INPUT);
  for (size_t i = 0; i < names.size(); i++)
    if (endsWith(names[i], ".gmxpdb")) return names[i];
  return "";
}

void GromacsPreparePanel::showStructure()
{
  string name = sourceFile();
  p_refreshing = true;
  p_structure->SetSelection(name.empty() ? 0 : 1);
  p_file->SetLabel(name.empty()
      ? _("The Builder's structure is used when the job is launched.")
      : U(name.c_str()));
  p_refreshing = false;
  showFields();
}

void GromacsPreparePanel::showFields()
{
  bool other = p_ff->GetSelection() == OTHER_FF;
  p_source->Enable(p_editable);
  p_structure->Enable(p_editable);
  p_browse->Enable(p_editable && p_structure->GetSelection() == 1);
  p_ff->Enable(p_editable);
  p_otherFf->Enable(p_editable && other);
  p_water->Enable(p_editable);
  p_ignh->Enable(p_editable);
  p_merge->Enable(p_editable);
  p_box->Enable(p_editable);
  p_dist->Enable(p_editable);
  p_conc->Enable(p_editable);
  p_pion->Enable(p_editable);
  p_nion->Enable(p_editable);
}

/** Offer the water models of the chosen force field; keep one if it is there. */
void GromacsPreparePanel::fillWaters(const string& preferred)
{
  vector<string> ids = watersOf(p_ff->GetSelection());
  p_water->Clear();
  int pick = 0;
  for (size_t i = 0; i < ids.size(); i++) {
    p_water->Append(U(waterLabel(ids[i]).c_str()),
                    new wxStringClientData(U(ids[i].c_str())));
    if (ids[i] == preferred) pick = (int)i;
  }
  p_water->SetSelection(pick);
  if (p_model != 0) {
    wxStringClientData *d =
      (wxStringClientData*)p_water->GetClientObject(pick);
    p_model->waterModel(string(d->GetData().ToUTF8()));
  }
}

void GromacsPreparePanel::refreshGUI()
{
  if (p_model == 0) return;
  p_refreshing = true;
  int ff = forceFieldIndex(p_model->forceField());
  p_ff->SetSelection(ff);
  p_otherFf->SetValue(ff == OTHER_FF ? U(p_model->forceField().c_str())
                                     : wxString());
  fillWaters(p_model->waterModel());
  p_ignh->SetValue(p_model->ignoreHydrogens());
  p_merge->SetValue(p_model->mergeChains());
  int b = 0;
  for (int i = 0; i < NBOX; i++) if (p_model->boxType() == BOXES[i][0]) b = i;
  p_box->SetSelection(b);
  std::ostringstream os;
  os << p_model->boxDistance();
  p_dist->SetValue(U(os.str().c_str()));
  std::ostringstream oc;
  oc << p_model->ionConcentration();
  p_conc->SetValue(U(oc.str().c_str()));
  p_pion->SetSelection(p_model->positiveIon() == "K" ? 1 : 0);
  p_nion->SetSelection(0);
  p_badDistance.clear();
  p_badConcentration.clear();
  p_refreshing = false;
  showStructure();
}

bool GromacsPreparePanel::validate(string& why) const
{
  if (!p_badDistance.empty()) { why = p_badDistance; return false; }
  if (!p_badConcentration.empty()) { why = p_badConcentration; return false; }
  if (p_ff->GetSelection() == OTHER_FF) {
    string n(p_otherFf->GetValue().ToUTF8());
    if (n.empty()) {
      why = "Give the directory name of the force field.";
      return false;
    }
    for (size_t i = 0; i < n.size(); i++) {
      unsigned char c = n[i];
      if (!(isalnum(c) || (i > 0 && (c == '_' || c == '.' || c == '+' || c == '-')))) {
        why = "The force field name may only hold letters, digits and _ . + -";
        return false;
      }
    }
  }
  return true;
}

void GromacsPreparePanel::OnForceField(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  int ff = p_ff->GetSelection();
  if (ff < NFF) {
    p_model->forceField(FORCEFIELDS[ff].id);
  } else {
    string n(p_otherFf->GetValue().ToUTF8());
    if (!n.empty()) p_model->forceField(n);
  }
  p_refreshing = true;
  fillWaters(p_model->waterModel());
  p_refreshing = false;
  showFields();
  changed();
}

void GromacsPreparePanel::OnOtherForceField(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0 || p_ff->GetSelection() != OTHER_FF) return;
  string n(p_otherFf->GetValue().ToUTF8());
  if (!n.empty()) p_model->forceField(n);
  changed();
}

void GromacsPreparePanel::OnWater(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  int sel = p_water->GetSelection();
  if (sel == wxNOT_FOUND) return;
  wxStringClientData *d = (wxStringClientData*)p_water->GetClientObject(sel);
  p_model->waterModel(string(d->GetData().ToUTF8()));
  changed();
}

void GromacsPreparePanel::OnCheck(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  p_model->ignoreHydrogens(p_ignh->GetValue());
  p_model->mergeChains(p_merge->GetValue());
  changed();
}

void GromacsPreparePanel::OnBox(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  int b = p_box->GetSelection();
  if (b >= 0 && b < NBOX) p_model->boxType(BOXES[b][0]);
  changed();
}

void GromacsPreparePanel::OnIon(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  p_model->positiveIon(POSITIVE[p_pion->GetSelection() == 1 ? 1 : 0]);
  p_model->negativeIon(NEGATIVE[0]);
  changed();
}

void GromacsPreparePanel::OnNumber(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  double v;
  string d(p_dist->GetValue().ToUTF8());
  if (isNumber(d, v, false)) {
    p_model->boxDistance(v);
    p_badDistance.clear();
  } else {
    p_badDistance = "The distance to the box edge must be a number above 0 nm.";
  }
  string c(p_conc->GetValue().ToUTF8());
  if (isNumber(c, v, true)) {
    p_model->ionConcentration(v);
    p_badConcentration.clear();
  } else {
    p_badConcentration = "The salt concentration must be a number, 0 or more mol/L.";
  }
  if (p_owner != 0) {
    string why;
    if (validate(why)) p_owner->getFeedback().clearMessage();
    else p_owner->getFeedback().setMessage(why, WxFeedback::ERROR);
  }
  changed();
}

bool GromacsPreparePanel::attachPdb(const string& path, string& message)
{
  MdTask *t = task();
  if (t == 0) { message = "No task is open."; return false; }
  ifstream in(path.c_str(), std::ios::binary);
  if (!in) { message = "The file " + path + " could not be opened."; return false; }
  string tail = path.substr(path.find_last_of("/\\") == string::npos
                            ? 0 : path.find_last_of("/\\") + 1);
  size_t dot = tail.find_last_of('.');
  if (dot != string::npos && dot > 0) tail = tail.substr(0, dot);
  if (tail.empty()) tail = "source";
  detachPdb();
  if (!t->putInputFile(tail + ".gmxpdb", &in)) {
    message = "The structure could not be stored in the task.";
    return false;
  }
  return true;
}

void GromacsPreparePanel::detachPdb()
{
  MdTask *t = task();
  if (t == 0) return;
  vector<string> names = t->getDataFileNames(JCode::AUXILIARY_INPUT);
  for (size_t i = 0; i < names.size(); i++)
    if (endsWith(names[i], ".gmxpdb")) t->putInputFile(names[i], 0);
}

bool GromacsPreparePanel::chooseFile()
{
  Preferences prefs("GromacsPrepare");
  string dir;
  prefs.getString("GromacsPrepare.Dir", dir);
  if (dir.empty()) dir = Ecce::realUserHome();
  ewxFileDialog dlg(this, _("Choose the protein structure"), dir, "",
                    "PDB files (*.pdb;*.ent)|*.pdb;*.ent|All files|*",
                    wxFD_OPEN | wxFD_FILE_MUST_EXIST, wxDefaultPosition);
  if (dlg.ShowModal() != wxID_OK) return false;
  prefs.setString("GromacsPrepare.Dir", dlg.GetDirectory().ToStdString());
  prefs.saveFile();
  string message;
  if (!attachPdb(dlg.GetPath().ToStdString(), message)) {
    wxMessageBox(U(message.c_str()), "ECCE GROMACS Prepare", wxOK | wxICON_ERROR, this);
    return false;
  }
  return true;
}

void GromacsPreparePanel::OnStructure(wxCommandEvent&)
{
  if (p_refreshing) return;
  if (p_structure->GetSelection() == 1) {
    if (sourceFile().empty() && !chooseFile()) {
      showStructure();
      return;
    }
  } else {
    detachPdb();
  }
  showStructure();
  changed();
}

void GromacsPreparePanel::OnBrowse(wxCommandEvent&)
{
  if (chooseFile()) {
    showStructure();
    changed();
  }
}

bool GromacsPreparePanel::testSet(const string& field, const string& value,
                                  string& message)
{
  if (p_model == 0) { message = "no model"; return false; }
  wxCommandEvent ev;
  if (field == "ff") {
    int ff = forceFieldIndex(value);
    p_ff->SetSelection(ff);
    if (ff == OTHER_FF) p_otherFf->SetValue(U(value.c_str()));
    OnForceField(ev);
  } else if (field == "water") {
    vector<string> ids = watersOf(p_ff->GetSelection());
    for (size_t i = 0; i < ids.size(); i++) {
      if (ids[i] == value) {
        p_water->SetSelection((int)i);
        OnWater(ev);
        return true;
      }
    }
    message = "not offered for this force field";
    return false;
  } else if (field == "ignh" || field == "merge") {
    (field == "ignh" ? p_ignh : p_merge)->SetValue(value == "1");
    OnCheck(ev);
  } else if (field == "box") {
    for (int i = 0; i < NBOX; i++) {
      if (value == BOXES[i][0]) {
        p_box->SetSelection(i);
        OnBox(ev);
        return true;
      }
    }
    message = "unknown box type";
    return false;
  } else if (field == "dist") {
    p_dist->SetValue(U(value.c_str()));
    OnNumber(ev);
  } else if (field == "conc") {
    p_conc->SetValue(U(value.c_str()));
    OnNumber(ev);
  } else if (field == "pname") {
    p_pion->SetSelection(value == "K" ? 1 : 0);
    OnIon(ev);
  } else if (field == "structure") {
    if (value == "builder") {
      p_structure->SetSelection(0);
      OnStructure(ev);
    } else {
      p_refreshing = true;
      bool ok = attachPdb(value, message);
      p_refreshing = false;
      if (!ok) return false;
      showStructure();
      changed();
    }
  } else {
    message = "unknown field";
    return false;
  }
  return true;
}

string GromacsPreparePanel::summary() const
{
  std::ostringstream os;
  string f = sourceFile();
  os << "structure=" << (f.empty() ? "builder" : f)
     << " ff=" << p_model->forceField() << " water=" << p_model->waterModel()
     << " waters=";
  for (unsigned i = 0; i < p_water->GetCount(); i++) {
    wxStringClientData *d = (wxStringClientData*)p_water->GetClientObject(i);
    os << (i ? "," : "") << d->GetData().ToUTF8();
  }
  os << " box=" << p_model->boxType() << " dist=" << p_model->boxDistance()
     << " conc=" << p_model->ionConcentration() << " ions="
     << p_model->positiveIon() << "," << p_model->negativeIon()
     << " ignh=" << p_model->ignoreHydrogens()
     << " merge=" << p_model->mergeChains();
  return os.str();
}
