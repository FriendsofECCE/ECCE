// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/utils.h>
  using std::vector;
  using std::ifstream;

#include "util/Ecce.H"
#include "util/EcceURL.H"
#include "util/Preferences.H"

#include "dsm/EDSIFactory.H"
#include "dsm/GromacsIncludes.H"
#include "dsm/GromacsPrepareModel.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"
#include "tdat/InteractionModel.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxFileDialog.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/MDEdBase.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/GromacsPreparePanel.H"

static const wxWindowID ID_SOURCE = wxNewId();
static const wxWindowID ID_MREP = wxNewId();
static const wxWindowID ID_MSUBS = wxNewId();
static const wxWindowID ID_MMINERAL = wxNewId();
static const wxWindowID ID_MSEED = wxNewId();
static const wxWindowID ID_MVARIANT = wxNewId();
static const wxWindowID ID_MANGLE = wxNewId();
static const wxWindowID ID_MCELL = wxNewId();
static const wxWindowID ID_MBUILD = wxNewId();
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

struct IonInfo { const char *id; const char *label; };
static const IonInfo POSITIVE[] = { { "NA", "Sodium (NA)" }, { "K", "Potassium (K)" } };
static const IonInfo NEGATIVE[] = { { "CL", "Chloride (CL)" } };

// MINFF's ions.itp names its molecules in mixed case; the counter-ions a
// clay interlayer usually holds, then the halides.  md.gmxprep knows the
// charge of each and checks the name.
static const IonInfo MINFF_POSITIVE[] = {
  { "Na", "Sodium (Na)" }, { "K", "Potassium (K)" }, { "Li", "Lithium (Li)" },
  { "Cs", "Caesium (Cs)" }, { "Mg", "Magnesium (Mg)" }, { "Ca", "Calcium (Ca)" },
  { "Sr", "Strontium (Sr)" }, { "Ba", "Barium (Ba)" }, { "Zn", "Zinc (Zn)" },
  { "Cu2+", "Copper(II) (Cu2+)" }, { "Fe2+", "Iron(II) (Fe2+)" },
  { "Ni", "Nickel(II) (Ni)" }, { "Mn", "Manganese(II) (Mn)" },
  { "Al3+", "Aluminium (Al3+)" }, { "Fe3+", "Iron(III) (Fe3+)" },
};
static const IonInfo MINFF_NEGATIVE[] = {
  { "Cl", "Chloride (Cl)" }, { "Br", "Bromide (Br)" }, { "F", "Fluoride (F)" },
  { "I", "Iodide (I)" },
};

// The water models of min.ff that minff_topology.py can write.
static const WaterInfo MINFF_WATERS[] = {
  { "opc3", "OPC3" }, { "opc", "OPC" }, { "spce", "SPC/E" }, { "tip3p", "TIP3P" },
  { "tip3p-fb", "TIP3P-FB" }, { "tip4p-fb", "TIP4P-FB" }, { "tip4pew", "TIP4P-Ew" },
};
static const int NMINFF_WATER = sizeof(MINFF_WATERS) / sizeof(MINFF_WATERS[0]);
static const int ANGLE_KS[] = { 0, 250, 500, 1500 };

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
  p_built = false;

  wxBoxSizer *columns = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);
  wxBoxSizer *right = new wxBoxSizer(wxVERTICAL);
  ewxStaticText *lab;

  // ---- structure ----
  wxStaticBoxSizer *sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Structure"));
  wxWindow *box = sb->GetStaticBox();
  wxFlexGridSizer *g = new wxFlexGridSizer(2, 6, 8);
  g->AddGrowableCol(1);
  g->Add(label(box, _("Source:")), 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString src;
  src.Add(_("Protein (pdb2gmx)"));
  src.Add(_("Mineral (MINFF)"));
  p_source = new ewxChoice(box, ID_SOURCE, wxDefaultPosition, wxDefaultSize, src);
  p_source->SetSelection(0);
  g->Add(p_source, 1, wxEXPAND);
  lab = label(box, _("Structure:"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString st;
  st.Add(_("from the Builder"));
  st.Add(_("PDB file..."));
  p_structure = new ewxChoice(box, ID_STRUCT, wxDefaultPosition, wxDefaultSize, st);
  p_structure->SetSelection(0);
  p_proteinOnly.push_back(p_structure);
  g->Add(p_structure, 1, wxEXPAND);
  p_browse = new ewxButton(box, ID_BROWSE, _("Browse..."));
  p_proteinOnly.push_back(p_browse);
  g->AddSpacer(0);
  g->Add(p_browse, 0);
  g->AddSpacer(0);
  p_file = label(box, wxEmptyString);
  p_proteinOnly.push_back(p_file);
  g->Add(p_file, 1, wxEXPAND);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  left->Add(sb, 0, wxALL | wxEXPAND, 4);

  // ---- mineral (MINFF) ----
  // minff_topology.py runs here, on this computer, when Build is pressed;
  // its structure, topology and includes are stored in the task.
  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Mineral (MINFF)"));
  box = sb->GetStaticBox();
  p_minffOnly.push_back(box);
  g = new wxFlexGridSizer(2, 6, 8);
  g->AddGrowableCol(1);
  lab = label(box, _("Unit cell:"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxBoxSizer *cellRow = new wxBoxSizer(wxHORIZONTAL);
  p_cellLabel = label(box, _("none chosen"));
  p_minffOnly.push_back(p_cellLabel);
  cellRow->Add(p_cellLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_mcellBtn = new ewxButton(box, ID_MCELL, _("Browse..."));
  p_minffOnly.push_back(p_mcellBtn);
  cellRow->Add(p_mcellBtn, 0);
  g->Add(cellRow, 1, wxEXPAND);
  lab = label(box, _("Repeat (a b c):"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  p_mrep = new ewxTextCtrl(box, ID_MREP, wxEmptyString);
  p_mrep->SetToolTip(_("How many times the unit cell is repeated along its a, b "
                       "and c axes, for example 4 3 3."));
  p_minffOnly.push_back(p_mrep);
  g->Add(p_mrep, 1, wxEXPAND);
  lab = label(box, _("Substitutions:"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  p_msubs = new ewxTextCtrl(box, ID_MSUBS, wxEmptyString);
  p_msubs->SetToolTip(_("Atoms replaced at random, as FROM,TO,COUNT separated by "
                        "semicolons: Al,Mgo,4 puts four Mgo in place of Al."));
  p_minffOnly.push_back(p_msubs);
  g->Add(p_msubs, 1, wxEXPAND);
  lab = label(box, _("Random seed:"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  p_mseed = new ewxTextCtrl(box, ID_MSEED, wxEmptyString);
  p_mseed->SetToolTip(_("A whole number makes the substitutions repeatable; "
                        "empty is a different draw every time."));
  p_minffOnly.push_back(p_mseed);
  g->Add(p_mseed, 1, wxEXPAND);
  lab = label(box, _("Parameters:"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString variants;
  variants.Add(_("General (GMINFF)"));
  variants.Add(_("Tailored to one mineral (TMINFF)"));
  p_mvariant = new ewxChoice(box, ID_MVARIANT, wxDefaultPosition, wxDefaultSize, variants);
  p_minffOnly.push_back(p_mvariant);
  g->Add(p_mvariant, 1, wxEXPAND);
  lab = label(box, _("Mineral (TMINFF):"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  p_mmineral = new ewxTextCtrl(box, ID_MMINERAL, wxEmptyString);
  p_mmineral->SetToolTip(_("The name of the mineral in MINFF's tailored set, for "
                           "example Montmorillonite or Kaolinite."));
  p_minffOnly.push_back(p_mmineral);
  g->Add(p_mmineral, 1, wxEXPAND);
  lab = label(box, _("O-M-O angle constant:"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString ks;
  for (size_t i = 0; i < sizeof(ANGLE_KS) / sizeof(ANGLE_KS[0]); i++)
    ks.Add(wxString::Format("%d kJ/mol/rad2", ANGLE_KS[i]));
  p_mangle = new ewxChoice(box, ID_MANGLE, wxDefaultPosition, wxDefaultSize, ks);
  p_minffOnly.push_back(p_mangle);
  g->Add(p_mangle, 1, wxEXPAND);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  p_mbuild = new ewxButton(box, ID_MBUILD, _("Build topology"));
  p_minffOnly.push_back(p_mbuild);
  sb->Add(p_mbuild, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  p_summaryLabel = label(box, wxEmptyString);
  p_minffOnly.push_back(p_summaryLabel);
  sb->Add(p_summaryLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  left->Add(sb, 0, wxALL | wxEXPAND, 4);
  p_leftSizer = left;
  p_minffSizer = sb;

  // ---- force field ----
  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Force field"));
  box = sb->GetStaticBox();
  p_ffBox = (wxStaticBox*)box;
  g = new wxFlexGridSizer(2, 6, 8);
  g->AddGrowableCol(1);
  lab = label(box, _("Force field:"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString ffs;
  for (int i = 0; i < NFF; i++) ffs.Add(U(FORCEFIELDS[i].label));
  ffs.Add(_("Other (GMXLIB)"));
  p_ff = new ewxChoice(box, ID_FF, wxDefaultPosition, wxDefaultSize, ffs);
  p_proteinOnly.push_back(p_ff);
  g->Add(p_ff, 1, wxEXPAND);
  lab = label(box, _("Directory name:"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  p_otherFf = new ewxTextCtrl(box, ID_OTHERFF, wxEmptyString);
  p_otherFf->SetToolTip(_("The name of a force field folder (without .ff) that "
                          "GROMACS finds in its library or in GMXLIB."));
  p_proteinOnly.push_back(p_otherFf);
  g->Add(p_otherFf, 1, wxEXPAND);
  g->Add(label(box, _("Water model:")), 0, wxALIGN_CENTER_VERTICAL);
  p_water = new ewxChoice(box, ID_WATER);
  g->Add(p_water, 1, wxEXPAND);
  lab = label(box, _("Histidines:"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  lab = label(box, _("automatic (names in the PDB are kept)"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 1, wxALIGN_CENTER_VERTICAL);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  p_ignh = new ewxCheckBox(box, ID_IGNH, _("Ignore hydrogens in the PDB (-ignh)"));
  p_proteinOnly.push_back(p_ignh);
  sb->Add(p_ignh, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  p_merge = new ewxCheckBox(box, ID_MERGE, _("Merge chains into one molecule"));
  p_proteinOnly.push_back(p_merge);
  sb->Add(p_merge, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  left->Add(sb, 0, wxALL | wxEXPAND, 4);

  // ---- box and ions ----
  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Box and water"));
  box = sb->GetStaticBox();
  p_waterBox = (wxStaticBox*)box;
  g = new wxFlexGridSizer(3, 6, 8);
  g->AddGrowableCol(1);
  lab = label(box, _("Box type:"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString boxes;
  for (int i = 0; i < NBOX; i++) boxes.Add(U(BOXES[i][1]));
  p_box = new ewxChoice(box, ID_BOX, wxDefaultPosition, wxDefaultSize, boxes);
  p_proteinOnly.push_back(p_box);
  g->Add(p_box, 1, wxEXPAND);
  lab = label(box, wxEmptyString);
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0);
  p_distLabel = label(box, _("Distance to edge:"));
  g->Add(p_distLabel, 0, wxALIGN_CENTER_VERTICAL);
  p_dist = new ewxTextCtrl(box, ID_DIST, wxEmptyString);
  g->Add(p_dist, 1, wxEXPAND);
  g->Add(label(box, _("nm")), 0, wxALIGN_CENTER_VERTICAL);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  right->Add(sb, 0, wxALL | wxEXPAND, 4);

  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Ions"));
  box = sb->GetStaticBox();
  p_ionNote = label(box,
      _("The system is always neutralised. Salt is added on top."));
  sb->Add(p_ionNote, 0, wxALL, 6);
  g = new wxFlexGridSizer(3, 6, 8);
  g->AddGrowableCol(1);
  g->Add(label(box, _("Salt concentration:")), 0, wxALIGN_CENTER_VERTICAL);
  p_conc = new ewxTextCtrl(box, ID_CONC, wxEmptyString);
  p_conc->SetToolTip(_("0 neutralises only."));
  g->Add(p_conc, 1, wxEXPAND);
  g->Add(label(box, _("mol/L")), 0, wxALIGN_CENTER_VERTICAL);
  g->Add(label(box, _("Positive ion:")), 0, wxALIGN_CENTER_VERTICAL);
  p_pion = new ewxChoice(box, ID_PION);
  g->Add(p_pion, 1, wxEXPAND);
  g->AddSpacer(0);
  g->Add(label(box, _("Negative ion:")), 0, wxALIGN_CENTER_VERTICAL);
  p_nion = new ewxChoice(box, ID_NION);
  g->Add(p_nion, 1, wxEXPAND);
  g->AddSpacer(0);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  right->Add(sb, 0, wxALL | wxEXPAND, 4);

  columns->Add(left, 1, wxEXPAND);
  columns->Add(right, 1, wxEXPAND);
  SetSizer(columns);
  fillIons();

  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnSource, this, ID_SOURCE);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnMinffText, this, ID_MREP);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnMinffText, this, ID_MSUBS);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnMinffText, this, ID_MMINERAL);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnMinffText, this, ID_MSEED);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnMinffChoice, this, ID_MVARIANT);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnMinffChoice, this, ID_MANGLE);
  Bind(wxEVT_BUTTON, &GromacsPreparePanel::OnMinffCell, this, ID_MCELL);
  Bind(wxEVT_BUTTON, &GromacsPreparePanel::OnMinffBuild, this, ID_MBUILD);
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

bool GromacsPreparePanel::isMinff() const
{
  return p_model != 0 && p_model->source() == "minff";
}

void GromacsPreparePanel::showFields()
{
  bool minff = isMinff();
  bool other = p_ff->GetSelection() == OTHER_FF;
  p_source->Enable(p_editable);
  p_source->SetSelection(minff ? 1 : 0);
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
  for (int i = 0; i < 2; i++) {
    wxWindow *w = i == 0 ? (wxWindow*)p_mrep : (wxWindow*)p_msubs;
    w->Enable(p_editable);
  }
  p_mseed->Enable(p_editable);
  p_mvariant->Enable(p_editable);
  p_mmineral->Enable(p_editable && p_mvariant->GetSelection() == 1);
  p_mangle->Enable(p_editable);
  p_mcellBtn->Enable(p_editable);
  p_mbuild->Enable(p_editable);

  // What does not apply to the source is hidden, not just greyed
  for (size_t i = 0; i < p_proteinOnly.size(); i++) p_proteinOnly[i]->Show(!minff);
  p_leftSizer->Show(p_minffSizer, minff, true);
  p_ffBox->SetLabel(minff ? _("Water model") : _("Force field"));
  p_waterBox->SetLabel(minff ? _("Water layer") : _("Box and water"));
  p_distLabel->SetLabel(minff ? _("Water layer above the mineral:")
                              : _("Distance to edge:"));
  p_dist->SetToolTip(minff
      ? _("Water added above the mineral, along the cell's c axis. 0 keeps the "
          "mineral's cell as it is and adds no water; a charged mineral is then "
          "not neutralised, because there is no water to replace by ions.")
      : _("The distance from the solute to the box edge."));
  p_ionNote->SetLabel(minff
      ? _("With a water layer the mineral's charge is neutralised by counter-ions.")
      : _("The system is always neutralised. Salt is added on top."));
  relayout();
}

// The mineral page is taller than the protein page; the scrolled window
// holding this panel (GmxPrepare) takes the new size, so the window stays put.
void GromacsPreparePanel::relayout()
{
  Layout();
  wxWindow *parent = GetParent();
  if (parent != 0) {
    parent->Layout();
    parent->FitInside();
  }
}

/** Offer the water models of the chosen force field; keep one if it is there. */
void GromacsPreparePanel::fillWaters(const string& preferred)
{
  vector<string> ids;
  if (isMinff()) {
    for (int i = 0; i < NMINFF_WATER; i++) ids.push_back(MINFF_WATERS[i].id);
  } else {
    ids = watersOf(p_ff->GetSelection());
  }
  p_water->Clear();
  int pick = 0;
  for (size_t i = 0; i < ids.size(); i++) {
    string lab = waterLabel(ids[i]);
    if (isMinff())
      for (int k = 0; k < NMINFF_WATER; k++)
        if (ids[i] == MINFF_WATERS[k].id) lab = MINFF_WATERS[k].label;
    p_water->Append(U(lab.c_str()), new wxStringClientData(U(ids[i].c_str())));
    if (ids[i] == preferred) pick = (int)i;
  }
  p_water->SetSelection(pick);
  if (p_model != 0) {
    wxStringClientData *d =
      (wxStringClientData*)p_water->GetClientObject(pick);
    p_model->waterModel(string(d->GetData().ToUTF8()));
  }
}

/** The ions of the source: GROMACS's NA/K/CL, or MINFF's ions.itp. */
void GromacsPreparePanel::fillIons()
{
  bool minff = isMinff();
  const IonInfo *pos = minff ? MINFF_POSITIVE : POSITIVE;
  int np = minff ? (int)(sizeof(MINFF_POSITIVE) / sizeof(IonInfo))
                 : (int)(sizeof(POSITIVE) / sizeof(IonInfo));
  const IonInfo *neg = minff ? MINFF_NEGATIVE : NEGATIVE;
  int nn = minff ? (int)(sizeof(MINFF_NEGATIVE) / sizeof(IonInfo))
                 : (int)(sizeof(NEGATIVE) / sizeof(IonInfo));
  bool was = p_refreshing;
  p_refreshing = true;
  p_pion->Clear();
  p_nion->Clear();
  int pp = 0, pn = 0;
  for (int i = 0; i < np; i++) {
    p_pion->Append(U(pos[i].label), new wxStringClientData(U(pos[i].id)));
    if (p_model != 0 && p_model->positiveIon() == pos[i].id) pp = i;
  }
  for (int i = 0; i < nn; i++) {
    p_nion->Append(U(neg[i].label), new wxStringClientData(U(neg[i].id)));
    if (p_model != 0 && p_model->negativeIon() == neg[i].id) pn = i;
  }
  p_pion->SetSelection(pp);
  p_nion->SetSelection(pn);
  if (p_model != 0) {
    p_model->positiveIon(pos[pp].id);
    p_model->negativeIon(neg[pn].id);
  }
  p_refreshing = was;
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
  fillIons();
  p_mrep->SetValue(U(p_model->minffReplicate().c_str()));
  p_msubs->SetValue(U(p_model->minffSubstitutions().c_str()));
  p_mseed->SetValue(U(p_model->minffSeed().c_str()));
  p_mmineral->SetValue(U(p_model->minffMineral().c_str()));
  p_mvariant->SetSelection(p_model->minffVariant() == "tminff" ? 1 : 0);
  int k = 2;
  for (size_t i = 0; i < sizeof(ANGLE_KS) / sizeof(ANGLE_KS[0]); i++)
    if (ANGLE_KS[i] == p_model->minffAngleK()) k = (int)i;
  p_mangle->SetSelection(k);
  p_badDistance.clear();
  p_badConcentration.clear();
  p_refreshing = false;
  showStructure();
  showMineral();
}

// What the stored mineral is, in words; the task's inputs say whether one is there.
void GromacsPreparePanel::showMineral()
{
  if (p_model == 0) return;
  MdTask *t = task();
  p_built = false;
  if (t != 0) {
    vector<string> names = t->getDataFileNames(JCode::AUXILIARY_INPUT);
    for (size_t i = 0; i < names.size(); i++)
      if (names[i] == "topol.gmxmin") p_built = true;
  }
  string cell = p_cellPath.empty() ? p_model->minffCell() : p_cellPath;
  string::size_type s = cell.find_last_of("/\\");
  if (s != string::npos) cell = cell.substr(s + 1);
  p_cellLabel->SetLabel(cell.empty() ? _("none chosen") : U(cell.c_str()));
  p_summaryLabel->SetLabel(p_built
      ? U(p_model->minffSummary().c_str())
      : _("No topology built yet. Choose a unit cell and press Build."));
  relayout();
}

// Options changed after a build: the stored files no longer match them.
void GromacsPreparePanel::minffStale()
{
  if (p_refreshing || !p_built) return;
  p_summaryLabel->SetLabel(_("The options changed since the topology was built; "
                             "press Build to apply them."));
  relayout();
}

// The cell the MD tasks after this Prepare get (the built mineral's box,
// its c lengthened by the water layer) against grompp's limit for the
// cut-off: half the shortest box vector and the smallest diagonal element.
// Empty when it holds the default cut-off of those tasks.
string GromacsPreparePanel::cellWarning() const
{
  double v[3][3];
  if (!cellVectors(v)) return "";
  double limit = 1e30;
  for (int i = 0; i < 3; i++) {
    limit = std::min(limit, 0.5 * std::sqrt(v[i][0]*v[i][0] + v[i][1]*v[i][1] + v[i][2]*v[i][2]));
    limit = std::min(limit, v[i][i]);
  }
  // grompp checks the pair list, which mdrun's buffer makes about 0.1 nm
  // longer than the cut-off.
  const double cut = InteractionModel::SHORTCUT;
  if (limit >= cut + 0.1) return "";
  char buf[400];
  snprintf(buf, sizeof buf,
           "The cell is too small for the %.1f nm cut-off the MD tasks use: "
           "with its pair-list buffer it must stay below %.2f nm here (half "
           "the shortest box vector). "
           "Repeat the cell more often, add water, or lower the cut-off in "
           "the tasks.", cut, limit);
  return buf;
}

// The built mineral's cell vectors (nm), c lengthened by the water layer.
bool GromacsPreparePanel::cellVectors(double v[3][3]) const
{
  if (p_model == 0 || !isMinff() || !p_built) return false;
  MdTask *t = task();
  istream *is = t ? t->getAnyFile(string("conf.gmxmin")) : 0;
  if (is == 0) return false;
  string line, last;
  while (std::getline(*is, line))
    if (line.find_first_not_of(" \t\r") != string::npos) last = line;
  delete is;
  double b[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
  std::istringstream in(last);
  int n = 0;
  while (n < 9 && in >> b[n]) n++;
  if (n != 3 && n != 9) return false;
  if (p_model->boxDistance() > 0) b[2] += p_model->boxDistance();
  const double m[3][3] = {{b[0], b[3], b[4]}, {b[5], b[1], b[6]}, {b[7], b[8], b[2]}};
  for (int i = 0; i < 3; i++)
    for (int k = 0; k < 3; k++) v[i][k] = m[i][k];
  return true;
}

// A warning only: the user may lower the cut-off in the tasks.
void GromacsPreparePanel::showCellWarning()
{
  if (p_owner == 0) return;
  string warn = cellWarning();
  string why;
  if (!warn.empty()) p_owner->getFeedback().setMessage(warn, WxFeedback::WARNING);
  else if (validate(why)) p_owner->getFeedback().clearMessage();
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
  minffStale();
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
  int ps = p_pion->GetSelection(), ns = p_nion->GetSelection();
  if (ps != wxNOT_FOUND)
    p_model->positiveIon(string(((wxStringClientData*)p_pion->GetClientObject(ps))
                                ->GetData().ToUTF8()));
  if (ns != wxNOT_FOUND)
    p_model->negativeIon(string(((wxStringClientData*)p_nion->GetClientObject(ns))
                                ->GetData().ToUTF8()));
  changed();
}

void GromacsPreparePanel::OnSource(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  bool minff = p_source->GetSelection() == 1;
  p_model->source(minff ? "minff" : "protein");
  // each source starts from its own defaults
  GromacsPrepareModel d;
  p_model->waterModel(minff ? "opc3" : d.waterModel());
  p_model->positiveIon(minff ? "Na" : d.positiveIon());
  p_model->negativeIon(minff ? "Cl" : d.negativeIon());
  p_model->boxDistance(minff ? 0.0 : d.boxDistance());
  p_model->ionConcentration(minff ? 0.0 : d.ionConcentration());
  refreshGUI();
  changed();
}

void GromacsPreparePanel::OnMinffText(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  p_model->minffReplicate(string(p_mrep->GetValue().ToUTF8()));
  p_model->minffSubstitutions(string(p_msubs->GetValue().ToUTF8()));
  p_model->minffSeed(string(p_mseed->GetValue().ToUTF8()));
  p_model->minffMineral(string(p_mmineral->GetValue().ToUTF8()));
  minffStale();
  changed();
}

void GromacsPreparePanel::OnMinffChoice(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  p_model->minffVariant(p_mvariant->GetSelection() == 1 ? "tminff" : "gminff");
  int k = p_mangle->GetSelection();
  if (k >= 0 && k < (int)(sizeof(ANGLE_KS) / sizeof(ANGLE_KS[0])))
    p_model->minffAngleK(ANGLE_KS[k]);
  p_mmineral->Enable(p_editable && p_mvariant->GetSelection() == 1);
  minffStale();
  changed();
}

void GromacsPreparePanel::OnMinffCell(wxCommandEvent&)
{
  Preferences prefs("GromacsPrepare");
  string dir;
  prefs.getString("GromacsPrepare.MineralDir", dir);
  if (dir.empty()) dir = Ecce::realUserHome();
  ewxFileDialog dlg(this, _("Choose the mineral unit cell"), dir, "",
                    "Structures (*.pdb;*.gro;*.cif)|*.pdb;*.gro;*.cif|All files|*",
                    wxFD_OPEN | wxFD_FILE_MUST_EXIST, wxDefaultPosition);
  if (dlg.ShowModal() != wxID_OK) return;
  prefs.setString("GromacsPrepare.MineralDir", dlg.GetDirectory().ToStdString());
  prefs.saveFile();
  p_cellPath = dlg.GetPath().ToStdString();
  showMineral();
  minffStale();
  changed();
}

void GromacsPreparePanel::OnMinffBuild(wxCommandEvent&)
{
  string message;
  if (!buildMineral(true, message))
    wxMessageBox(U(message.c_str()), "ECCE GROMACS Prepare", wxOK | wxICON_ERROR, this);
}

// Removes what an earlier build stored: the mineral's structure and
// topology and every include (min.itp, min.ff/...).
void GromacsPreparePanel::detachMineral()
{
  MdTask *t = task();
  if (t == 0) return;
  vector<string> names = t->getDataFileNames(JCode::AUXILIARY_INPUT);
  for (size_t i = 0; i < names.size(); i++)
    if (endsWith(names[i], ".gmxmin") || endsWith(names[i], ".itp"))
      t->putInputFile(names[i], 0);
}

static string slurp(const string& path)
{
  ifstream in(path.c_str(), std::ios::binary);
  std::ostringstream os;
  os << in.rdbuf();
  return os.str();
}

// Stores conf.gro and topol.top of a build as the task's inputs, with what
// the topology includes (as the Inputs tab does for an attached topology).
bool GromacsPreparePanel::storeMineral(const string& dir, string& message)
{
  MdTask *t = task();
  if (t == 0) { message = "No task is open."; return false; }
  string conf = slurp(dir + "/conf.gro"), top = slurp(dir + "/topol.top");
  if (conf.empty() || top.empty()) {
    message = "The build wrote no structure or topology.";
    return false;
  }
  vector<GromacsIncludes::File> found;
  vector<string> system, missing;
  GromacsIncludes::collect(dir + "/topol.top", found, system, missing);
  if (!missing.empty()) {
    message = "The topology includes " + missing[0] + ", which was not found.";
    return false;
  }
  detachMineral();
  std::istringstream c(conf), tp(top);
  if (!t->putInputFile("conf.gmxmin", &c) || !t->putInputFile("topol.gmxmin", &tp)) {
    message = "The mineral could not be stored in the task.";
    return false;
  }
  for (size_t i = 0; i < found.size(); i++) {
    std::istringstream in(slurp(found[i].path));
    if (!t->putInputFile(GromacsIncludes::encodeName(found[i].name), &in)) {
      message = "Could not store " + found[i].name + " in the task.";
      return false;
    }
  }
  return true;
}

static string jsonField(const string& text, const string& key)
{
  string::size_type p = text.find("\"" + key + "\":");
  if (p == string::npos) return "";
  p = text.find(':', p) + 1;
  while (p < text.size() && text[p] == ' ') p++;
  string::size_type e = text.find_first_of(",\n}", p);
  return text.substr(p, e - p);
}

class MinffProcess : public wxProcess
{
  public:
    MinffProcess() : wxProcess(wxPROCESS_REDIRECT), done(false), status(-1) {}
    virtual void OnTerminate(int, int st) { status = st; done = true; }
    bool done;
    int status;
};

// Removes the scratch directory of a build on every way out.
struct ScratchDir
{
  wxString path;
  ~ScratchDir() { if (!path.empty()) wxFileName::Rmdir(path, wxPATH_RMDIR_RECURSIVE); }
};

// Runs scripts/minff_topology.py here and stores its result.  Not under
// wxEXEC_SYNC: atomipy takes a while on a large cell and the window must
// stay alive, with a progress bar for as long as it does.
bool GromacsPreparePanel::buildMineral(bool withDialog, string& message)
{
  if (p_model == 0) { message = "No model."; return false; }
  if (p_cellPath.empty()) {
    message = "Choose the mineral's unit cell first (a .pdb, .gro or .cif file).";
    return false;
  }
  ScratchDir scratch;
  scratch.path = wxFileName::CreateTempFileName("ecce-minff");
  wxRemoveFile(scratch.path);
  if (scratch.path.empty() || !wxMkdir(scratch.path)) {
    scratch.path.clear();
    message = "No scratch directory could be made.";
    return false;
  }
  string out = string(scratch.path.ToUTF8());

  vector<string> argv;
  argv.push_back(Ecce::scriptCommand(string(Ecce::ecceHome()) + "/scripts/minff_topology.py"));
  argv.push_back("build");
  argv.push_back(p_cellPath);
  argv.push_back("-o");
  argv.push_back(out);
  std::istringstream rep(p_model->minffReplicate());
  int r[3] = { 1, 1, 1 };
  rep >> r[0] >> r[1] >> r[2];
  if (r[0] < 1 || r[1] < 1 || r[2] < 1) {
    message = "The repeat must be three whole numbers, 1 or more (for example 4 3 3).";
    return false;
  }
  argv.push_back("--replicate");
  for (int i = 0; i < 3; i++) argv.push_back(std::to_string(r[i]));
  // FROM,TO,COUNT;FROM,TO,COUNT
  std::istringstream subs(p_model->minffSubstitutions());
  string one;
  while (std::getline(subs, one, ';')) {
    for (size_t i = 0; i < one.size(); i++) if (one[i] == ',') one[i] = ' ';
    std::istringstream f(one);
    string a, b, n;
    if (!(f >> a)) continue;
    if (!(f >> b >> n) || atoi(n.c_str()) < 1) {
      message = "A substitution is FROM,TO,COUNT, for example Al,Mgo,4.";
      return false;
    }
    argv.push_back("--substitute");
    argv.push_back(a);
    argv.push_back(b);
    argv.push_back(n);
  }
  if (!p_model->minffSeed().empty()) {
    argv.push_back("--seed");
    argv.push_back(p_model->minffSeed());
  }
  argv.push_back("--variant");
  argv.push_back(p_model->minffVariant());
  if (p_model->minffVariant() == "tminff") {
    if (p_model->minffMineral().empty()) {
      message = "TMINFF needs the name of the mineral, for example Montmorillonite.";
      return false;
    }
    argv.push_back("--mineral");
    argv.push_back(p_model->minffMineral());
  }
  argv.push_back("--angle-k");
  argv.push_back(std::to_string(p_model->minffAngleK()));
  argv.push_back("--water");
  argv.push_back(p_model->waterModel());

  vector<const char*> cargv;
  for (size_t i = 0; i < argv.size(); i++) cargv.push_back(argv[i].c_str());
  cargv.push_back(0);

  MinffProcess *proc = new MinffProcess();
  proc->Redirect();
  long pid = wxExecute(&cargv[0], wxEXEC_ASYNC, proc);
  if (pid == 0) {
    delete proc;
    message = "minff_topology.py could not be started (is python3 installed?).";
    return false;
  }
  ewxProgressDialog *dlg = 0;
  string so, se;
  while (!proc->done) {
    while (proc->IsInputAvailable()) so += (char)proc->GetInputStream()->GetC();
    while (proc->IsErrorAvailable()) se += (char)proc->GetErrorStream()->GetC();
    if (withDialog) {
      if (dlg == 0)
        dlg = new ewxProgressDialog(_("MINFF"), _("Building the mineral topology..."),
                                    100, this, wxPD_APP_MODAL | wxPD_ELAPSED_TIME);
      dlg->Pulse();
    }
    wxMilliSleep(50);
    wxTheApp->Yield(true);
  }
  while (proc->IsInputAvailable()) so += (char)proc->GetInputStream()->GetC();
  while (proc->IsErrorAvailable()) se += (char)proc->GetErrorStream()->GetC();
  int status = proc->status;
  delete dlg;
  delete proc;

  if (status != 0) {
    string::size_type p = se.rfind("minff_topology:");
    message = p != string::npos ? se.substr(p) : se.substr(se.size() > 600 ? se.size() - 600 : 0);
    if (message.empty()) message = "minff_topology.py failed.";
    return false;
  }
  if (!storeMineral(out, message)) return false;

  // the result in words, from the script's own summary
  string atoms = jsonField(so, "n_atoms"), charge = jsonField(so, "total_charge");
  string untyped = so.find("\"untyped_atoms\": []") == string::npos ? " Some atoms have no MINFF type." : "";
  string cell = p_cellPath.substr(p_cellPath.find_last_of("/\\") == string::npos
                                  ? 0 : p_cellPath.find_last_of("/\\") + 1);
  string water = p_model->waterModel();
  p_model->minffCell(cell);
  p_model->minffSummary("Built from " + cell + ": " + atoms + " atoms, charge " + charge +
                        ", water " + water + "." + untyped);
  showMineral();
  showCellWarning();
  changed();
  return true;
}


void GromacsPreparePanel::OnNumber(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  double v;
  string d(p_dist->GetValue().ToUTF8());
  // A mineral may have no water layer (0), a protein needs a box around it.
  if (isNumber(d, v, isMinff())) {
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
    if (!validate(why)) p_owner->getFeedback().setMessage(why, WxFeedback::ERROR);
    else showCellWarning();
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
    vector<string> ids;
    for (unsigned k = 0; k < p_water->GetCount(); k++)
      ids.push_back(string(((wxStringClientData*)p_water->GetClientObject(k))
                           ->GetData().ToUTF8()));
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
  } else if (field == "pname" || field == "nname") {
    ewxChoice *c = field == "pname" ? p_pion : p_nion;
    for (unsigned i = 0; i < c->GetCount(); i++) {
      wxStringClientData *d = (wxStringClientData*)c->GetClientObject(i);
      if (string(d->GetData().ToUTF8()) == value) {
        c->SetSelection((int)i);
        OnIon(ev);
        return true;
      }
    }
    message = "not offered for this source";
    return false;
  } else if (field == "source") {
    p_source->SetSelection(value == "minff" ? 1 : 0);
    OnSource(ev);
  } else if (field == "mcell") {
    p_cellPath = value;
    showMineral();
    minffStale();
    changed();
  } else if (field == "mrep" || field == "msubs" || field == "mseed" ||
             field == "mmineral") {
    (field == "mrep" ? p_mrep : field == "msubs" ? p_msubs :
     field == "mseed" ? p_mseed : p_mmineral)->SetValue(U(value.c_str()));
    OnMinffText(ev);
  } else if (field == "mvariant") {
    p_mvariant->SetSelection(value == "tminff" ? 1 : 0);
    OnMinffChoice(ev);
  } else if (field == "mangle") {
    for (size_t i = 0; i < sizeof(ANGLE_KS) / sizeof(ANGLE_KS[0]); i++)
      if (std::to_string(ANGLE_KS[i]) == value) p_mangle->SetSelection((int)i);
    OnMinffChoice(ev);
  } else if (field == "mbuild") {
    return buildMineral(false, message);
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
     << " merge=" << p_model->mergeChains()
     << " source=" << p_model->source() << " built=" << (p_built ? 1 : 0)
     << " mineral=" << p_model->minffSummary()
     << " cellwarning=" << (cellWarning().empty() ? 0 : 1);
  return os.str();
}
