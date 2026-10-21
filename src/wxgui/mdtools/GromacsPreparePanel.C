// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <algorithm>
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
#include <wx/display.h>
#include <wx/scrolwin.h>
#include "dsm/GromacsPrepareModel.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"
#include "tdat/Fragment.H"
#include "tdat/InteractionModel.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxCheckBox.H"
#include "wxgui/ewxChoice.H"
#include "wxgui/ewxFileDialog.H"
#include "wxgui/ewxProgressDialog.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/GromacsForceFieldLibrary.H"
#include "wxgui/MDEdBase.H"
#include "wxgui/WxFeedback.H"
#include "wxgui/GromacsPreparePanel.H"

static string slurp(const string& path);

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
static const wxWindowID ID_FFIMPORT = wxNewId();
static const wxWindowID ID_WATER = wxNewId();
static const wxWindowID ID_STRUCT = wxNewId();
static const wxWindowID ID_BROWSE = wxNewId();
static const wxWindowID ID_IGNH = wxNewId();
static const wxWindowID ID_MERGE = wxNewId();
static const wxWindowID ID_NEUT = wxNewId();
static const wxWindowID ID_BOX = wxNewId();
static const wxWindowID ID_DIST = wxNewId();
static const wxWindowID ID_CONC = wxNewId();
static const wxWindowID ID_RMIN = wxNewId();
static const wxWindowID ID_CENTER = wxNewId();
static const wxWindowID ID_MIONS = wxNewId();
static const wxWindowID ID_MFF = wxNewId();
static const wxWindowID ID_MBEND = wxNewId();
static const wxWindowID ID_MRIGID = wxNewId();
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
// ClayFF (third_party/clayff): its ions.itp holds these, and its water is SPC.
static const IonInfo CLAYFF_POSITIVE[] = {
  { "Na", "Sodium (Na)" }, { "K", "Potassium (K)" }, { "Cs", "Caesium (Cs)" },
  { "Ca", "Calcium (Ca)" }, { "Ba", "Barium (Ba)" },
};
static const IonInfo CLAYFF_NEGATIVE[] = { { "Cl", "Chloride (Cl)" } };
static const WaterInfo CLAYFF_WATERS[] = { { "spc", "SPC (flexible; rigid below)" } };
// the id is what minff_topology.py --clayff-bend takes
static const WaterInfo CLAYFF_BENDS[] = {
  { "none", "None" }, { "moh", "Pouvreau 2017 (Al, Mg)" },
  { "moh2004", "Cygan 2004 generic" },
};
static const int NCLAYFF_BENDS = sizeof(CLAYFF_BENDS) / sizeof(CLAYFF_BENDS[0]);
static const int ANGLE_KS[] = { 0, 250, 500, 1500 };
// min.ff's ion parameter sets, in the order of the choice (the id is what
// minff_topology.py --ion-set takes).
static const WaterInfo MINFF_IONSETS[] = {
  { "HFE", "Hydration free energy (HFE)" }, { "IOD", "Ion-oxygen distance (IOD)" },
};
static const int NMINFF_IONSETS = sizeof(MINFF_IONSETS) / sizeof(MINFF_IONSETS[0]);

static string waterLabel(const string& id)
{
  for (size_t i = 0; i < sizeof(WATERS) / sizeof(WATERS[0]); i++)
    if (id == WATERS[i].id) return WATERS[i].label;
  return id;
}

static vector<string> watersOf(const string& id)
{
  vector<string> out;
  GromacsForceFieldLibrary::Entry e;
  if (GromacsForceFieldLibrary::find(id, e)) {
    out = GromacsForceFieldLibrary::waterModels(e);
    if (!out.empty()) return out;
  }
  const char *list = "tip3p tip4p tip4pew tip5p tip5pe tips3p spc spce";
  for (int i = 0; i < NFF; i++) if (id == FORCEFIELDS[i].id) list = FORCEFIELDS[i].waters;
  std::istringstream in(list);
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

// "" (centred) or three numbers, separated by spaces or commas; out is "x y z".
static bool parseCenter(string s, string& out)
{
  for (size_t i = 0; i < s.size(); i++) if (s[i] == ',') s[i] = ' ';
  vector<string> t;
  std::istringstream in(s);
  string w;
  while (in >> w) t.push_back(w);
  out.clear();
  if (t.empty()) return true;
  if (t.size() != 3) return false;
  for (size_t i = 0; i < 3; i++) {
    char *end = 0;
    double v = strtod(t[i].c_str(), &end);
    if (*end != 0 || end == t[i].c_str() || v != v) return false;
    char b[40];
    snprintf(b, sizeof b, "%g", v);
    out += (i ? " " : "") + string(b);
  }
  return true;
}

static wxString U(const char *s) { return wxString::FromUTF8(s); }

// A number field about eight characters wide, not stretched with the window.
static wxTextCtrl *shortField(wxTextCtrl *t)
{
  t->SetMinSize(wxSize(t->GetTextExtent("00000000").x + 24, -1));
  return t;
}

static ewxStaticText *label(wxWindow *parent, const wxString& text)
{
  return new ewxStaticText(parent, wxID_ANY, text);
}

// The width of about this many characters: where long notes wrap, so that a
// long summary or file name cannot make the whole page wider.
static int wrapPixels(wxWindow *w, int chars = 60)
{
  return w->GetTextExtent("0123456789").x * chars / 10;
}

// A label that is cut in the middle, not widened, when its text is long.
static ewxStaticText *clippedLabel(wxWindow *parent)
{
  ewxStaticText *t = new ewxStaticText(parent, wxID_ANY, wxEmptyString,
                                       wxDefaultPosition, wxDefaultSize,
                                       wxST_ELLIPSIZE_MIDDLE);
  t->SetMinSize(wxSize(wrapPixels(parent) / 3, -1));
  return t;
}

// The field with its unit right next to it.
static wxSizer *withUnit(wxWindow *parent, wxWindow *field, const wxString& unit)
{
  wxBoxSizer *row = new wxBoxSizer(wxHORIZONTAL);
  row->Add(field, 0, wxALIGN_CENTER_VERTICAL);
  row->Add(label(parent, unit), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);
  return row;
}

GromacsPreparePanel::GromacsPreparePanel(MDEdBase *owner, wxWindow *parent)
  : ewxPanel(parent, wxID_ANY)
{
  p_owner = owner;
  p_model = 0;
  p_editable = true;
  p_refreshing = false;
  p_built = false;
  p_staging = false;
  p_testClash = 2;

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
  src.Add(_("Mineral"));
  p_source = new ewxChoice(box, ID_SOURCE, wxDefaultPosition, wxDefaultSize, src);
  p_source->SetSelection(0);
  g->Add(p_source, 0);
  lab = label(box, _("Structure:"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString st;
  st.Add(_("from the Builder"));
  st.Add(_("PDB file..."));
  p_structure = new ewxChoice(box, ID_STRUCT, wxDefaultPosition, wxDefaultSize, st);
  p_structure->SetSelection(0);
  p_proteinOnly.push_back(p_structure);
  g->Add(p_structure, 0);
  p_browse = new ewxButton(box, ID_BROWSE, _("Browse..."));
  p_proteinOnly.push_back(p_browse);
  g->AddSpacer(0);
  g->Add(p_browse, 0);
  g->AddSpacer(0);
  p_file = clippedLabel(box);
  p_proteinOnly.push_back(p_file);
  g->Add(p_file, 1, wxEXPAND);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  left->Add(sb, 0, wxALL | wxEXPAND, 4);

  // ---- mineral (MINFF) ----
  // minff_topology.py runs here, on this computer, when Build is pressed;
  // its structure, topology and includes are stored in the task.
  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Mineral (MINFF)"));
  box = sb->GetStaticBox();
  p_mineralBox = (wxStaticBox*)box;
  p_minffOnly.push_back(box);
  g = new wxFlexGridSizer(2, 6, 8);
  g->AddGrowableCol(1);
  lab = label(box, _("Force field:"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString mffs;
  mffs.Add(_("MINFF"));
  mffs.Add(_("ClayFF"));
  p_mff = new ewxChoice(box, ID_MFF, wxDefaultPosition, wxDefaultSize, mffs);
  p_mff->SetToolTip(_("MINFF is the default. ClayFF (Cygan, Liang and Kalinichev "
                      "2004) has its own parameters, flexible SPC water and no "
                      "O-M-O angles."));
  p_minffOnly.push_back(p_mff);
  g->Add(p_mff, 0);
  lab = label(box, _("Unit cell:"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxBoxSizer *cellRow = new wxBoxSizer(wxHORIZONTAL);
  p_cellLabel = clippedLabel(box);
  p_cellLabel->SetLabel(_("none chosen"));
  p_minffOnly.push_back(p_cellLabel);
  cellRow->Add(p_cellLabel, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_mcellBtn = new ewxButton(box, ID_MCELL, _("Browse..."));
  p_minffOnly.push_back(p_mcellBtn);
  cellRow->Add(p_mcellBtn, 0);
  g->Add(cellRow, 1, wxEXPAND);
  lab = label(box, _("Repeat (a b c):"));
  p_minffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  // Three fields, not "1 1 1" in one: a proportional font draws the spaces
  // so narrow that the repeat reads as "111".
  wxBoxSizer *repRow = new wxBoxSizer(wxHORIZONTAL);
  for (int i = 0; i < 3; i++) {
    p_mrep[i] = new ewxTextCtrl(box, ID_MREP, wxEmptyString);
    p_mrep[i]->SetToolTip(_("How many times the unit cell is repeated along its "
                            "a, b and c axes, for example 4 3 3."));
    p_mrep[i]->SetMinSize(wxSize(p_mrep[i]->GetTextExtent("00000").x + 24, -1));
    p_minffOnly.push_back(p_mrep[i]);
    repRow->Add(p_mrep[i], 0, wxRIGHT, 8);
  }
  g->Add(repRow, 0);
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
  g->Add(shortField(p_mseed), 0);
  lab = label(box, _("Parameters:"));
  p_minffOnly.push_back(lab);
  p_minffFfOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString variants;
  variants.Add(_("General (GMINFF)"));
  variants.Add(_("Tailored to one mineral (TMINFF)"));
  p_mvariant = new ewxChoice(box, ID_MVARIANT, wxDefaultPosition, wxDefaultSize, variants);
  p_minffOnly.push_back(p_mvariant);
  p_minffFfOnly.push_back(p_mvariant);
  g->Add(p_mvariant, 0);
  lab = label(box, _("Mineral (TMINFF):"));
  p_minffOnly.push_back(lab);
  p_minffFfOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  p_mmineral = new ewxTextCtrl(box, ID_MMINERAL, wxEmptyString);
  p_mmineral->SetToolTip(_("The name of the mineral in MINFF's tailored set, for "
                           "example Montmorillonite or Kaolinite."));
  p_minffOnly.push_back(p_mmineral);
  p_minffFfOnly.push_back(p_mmineral);
  g->Add(p_mmineral, 1, wxEXPAND);
  lab = label(box, _("O-M-O angle constant:"));
  p_minffOnly.push_back(lab);
  p_minffFfOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString ks;
  for (size_t i = 0; i < sizeof(ANGLE_KS) / sizeof(ANGLE_KS[0]); i++)
    ks.Add(wxString::Format("%d kJ/mol/rad2", ANGLE_KS[i]));
  p_mangle = new ewxChoice(box, ID_MANGLE, wxDefaultPosition, wxDefaultSize, ks);
  p_minffOnly.push_back(p_mangle);
  p_minffFfOnly.push_back(p_mangle);
  g->Add(p_mangle, 0);
  lab = label(box, _("Ion parameters:"));
  p_minffOnly.push_back(lab);
  p_minffFfOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString is;
  for (int i = 0; i < NMINFF_IONSETS; i++) is.Add(U(MINFF_IONSETS[i].label));
  p_mions = new ewxChoice(box, ID_MIONS, wxDefaultPosition, wxDefaultSize, is);
  p_mions->SetToolTip(_("Which of min.ff's parameter sets the ions use, for the "
                        "chosen water model: fitted to the hydration free energy "
                        "or to the ion-oxygen distance."));
  p_minffOnly.push_back(p_mions);
  p_minffFfOnly.push_back(p_mions);
  g->Add(p_mions, 0);
  lab = label(box, _("M-O-H bend:"));
  p_clayffOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString bends;
  for (int i = 0; i < NCLAYFF_BENDS; i++) bends.Add(U(CLAYFF_BENDS[i].label));
  p_mbend = new ewxChoice(box, ID_MBEND, wxDefaultPosition, wxDefaultSize, bends);
  p_mbend->SetToolTip(_("Cygan 2004 includes a generic M-O-H bend for hydroxyl "
                        "groups, and Pouvreau 2017 fits one (with a Morse O-H bond) "
                        "to Al and Mg. None leaves it out, as the 2004 paper does "
                        "for the bulk phases."));
  p_clayffOnly.push_back(p_mbend);
  g->Add(p_mbend, 0);
  g->AddSpacer(0);
  p_mrigid = new ewxCheckBox(box, ID_MRIGID, _("Rigid water"));
  p_mrigid->SetToolTip(_("SETTLE-constrained SPC water (-DRIGID_WATER in the "
                         "topology) instead of ClayFF's flexible SPC."));
  p_clayffOnly.push_back(p_mrigid);
  g->Add(p_mrigid, 0);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  // Red frame while the topology is missing or stale: Launch needs it.
  p_mbuildFrame = new wxPanel(box);
  wxBoxSizer *fs = new wxBoxSizer(wxHORIZONTAL);
  p_mbuild = new ewxButton(p_mbuildFrame, ID_MBUILD, _("Build topology"));
  fs->Add(p_mbuild, 0, wxALL, 3);
  p_mbuildFrame->SetSizer(fs);
  p_minffOnly.push_back(p_mbuildFrame);
  sb->Add(p_mbuildFrame, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  p_summaryLabel = label(box, wxEmptyString);
  p_minffOnly.push_back(p_summaryLabel);
  sb->Add(p_summaryLabel, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  left->Add(sb, 0, wxALL | wxEXPAND, 4);
  p_leftSizer = left;
  p_rightSizer = right;
  p_minffSizer = sb;

  // ---- force field ----
  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Force field"));
  box = sb->GetStaticBox();
  p_ffBox = (wxStaticBox*)box;
  g = new wxFlexGridSizer(2, 6, 8);
  lab = label(box, _("Force field:"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  p_ff = new ewxChoice(box, ID_FF);
  fillForceFields();
  p_proteinOnly.push_back(p_ff);
  p_ffImport = new ewxButton(box, ID_FFIMPORT, _("Import..."));
  p_ffImport->SetToolTip(_("Add a GROMACS force field folder (<name>.ff) or a "
                           ".zip holding one to this user's library. The files "
                           "travel with the job, so a server needs nothing installed."));
  p_proteinOnly.push_back(p_ffImport);
  wxBoxSizer *ffRow = new wxBoxSizer(wxHORIZONTAL);
  ffRow->Add(p_ff, 0, wxALIGN_CENTER_VERTICAL);
  ffRow->Add(p_ffImport, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 6);
  g->Add(ffRow, 0);
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
  g->Add(p_water, 0);
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
  g = new wxFlexGridSizer(2, 6, 8);
  lab = label(box, _("Box type:"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  wxArrayString boxes;
  for (int i = 0; i < NBOX; i++) boxes.Add(U(BOXES[i][1]));
  p_box = new ewxChoice(box, ID_BOX, wxDefaultPosition, wxDefaultSize, boxes);
  p_proteinOnly.push_back(p_box);
  g->Add(p_box, 0);
  p_distLabel = label(box, _("Distance to edge:"));
  g->Add(p_distLabel, 0, wxALIGN_CENTER_VERTICAL);
  p_dist = new ewxTextCtrl(box, ID_DIST, wxEmptyString);
  g->Add(withUnit(box, shortField(p_dist), _("nm")), 0);
  lab = label(box, _("Solute centre (x y z):"));
  p_proteinOnly.push_back(lab);
  g->Add(lab, 0, wxALIGN_CENTER_VERTICAL);
  p_center = new ewxTextCtrl(box, ID_CENTER, wxEmptyString);
  p_center->SetToolTip(_("Where the solute's centre goes in the box, in nm, as three "
                         "numbers (for example 2 2 3). Empty keeps it in the middle "
                         "of the box. Off-centre placement leaves room to pull "
                         "along one axis; solvate fills the whole box."));
  p_proteinOnly.push_back(p_center);
  p_center->SetMinSize(wxSize(p_center->GetTextExtent("00.00 00.00 00.00").x + 24, -1));
  g->Add(withUnit(box, p_center, _("nm")), 0);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  right->Add(sb, 0, wxALL | wxEXPAND, 4);

  sb = new wxStaticBoxSizer(wxVERTICAL, this, _("Ions"));
  box = sb->GetStaticBox();
  p_neut = new ewxCheckBox(box, ID_NEUT, _("Neutralise with counter-ions"));
  p_neut->SetValue(true);
  p_neut->SetToolTip(_("genion -neutral. Unticked, only the salt below is added "
                       "and the system keeps the net charge of the structure."));
  sb->Add(p_neut, 0, wxALL, 6);
  p_ionNote = label(box, wxEmptyString);
  sb->Add(p_ionNote, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
  g = new wxFlexGridSizer(2, 6, 8);
  g->Add(label(box, _("Salt concentration:")), 0, wxALIGN_CENTER_VERTICAL);
  p_conc = new ewxTextCtrl(box, ID_CONC, wxEmptyString);
  p_conc->SetToolTip(_("0 neutralises only."));
  g->Add(withUnit(box, shortField(p_conc), _("mol/L")), 0);
  g->Add(label(box, _("Minimum ion distance:")), 0, wxALIGN_CENTER_VERTICAL);
  p_rmin = new ewxTextCtrl(box, ID_RMIN, wxEmptyString);
  p_rmin->SetToolTip(_("genion -rmin: how close an ion may be placed to the solute "
                       "and to other ions. 0.6 nm is GROMACS's default; lower it "
                       "when genion stops with \"No more replaceable solvent\", "
                       "as in a thin water layer."));
  g->Add(withUnit(box, shortField(p_rmin), _("nm")), 0);
  g->Add(label(box, _("Positive ion:")), 0, wxALIGN_CENTER_VERTICAL);
  p_pion = new ewxChoice(box, ID_PION);
  g->Add(p_pion, 0);
  g->Add(label(box, _("Negative ion:")), 0, wxALIGN_CENTER_VERTICAL);
  p_nion = new ewxChoice(box, ID_NION);
  g->Add(p_nion, 0);
  sb->Add(g, 0, wxALL | wxEXPAND, 6);
  right->Add(sb, 0, wxALL | wxEXPAND, 4);

  // Not equal proportions: those make the page twice as wide as its wider
  // column.  The right column takes what the window has over.
  columns->Add(left, 0, wxEXPAND);
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
  Bind(wxEVT_BUTTON, &GromacsPreparePanel::OnImportForceField, this, ID_FFIMPORT);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnOtherForceField, this, ID_OTHERFF);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnWater, this, ID_WATER);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnStructure, this, ID_STRUCT);
  Bind(wxEVT_BUTTON, &GromacsPreparePanel::OnBrowse, this, ID_BROWSE);
  Bind(wxEVT_CHECKBOX, &GromacsPreparePanel::OnCheck, this, ID_IGNH);
  Bind(wxEVT_CHECKBOX, &GromacsPreparePanel::OnCheck, this, ID_MERGE);
  Bind(wxEVT_CHECKBOX, &GromacsPreparePanel::OnCheck, this, ID_NEUT);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnBox, this, ID_BOX);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnNumber, this, ID_DIST);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnNumber, this, ID_CONC);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnNumber, this, ID_RMIN);
  Bind(wxEVT_TEXT, &GromacsPreparePanel::OnNumber, this, ID_CENTER);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnMinffChoice, this, ID_MIONS);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnMineralForceField, this, ID_MFF);
  Bind(wxEVT_CHOICE, &GromacsPreparePanel::OnMinffChoice, this, ID_MBEND);
  Bind(wxEVT_CHECKBOX, &GromacsPreparePanel::OnMinffChoice, this, ID_MRIGID);
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
  p_file->SetToolTip(p_file->GetLabel());
  p_refreshing = false;
  showFields();
}

bool GromacsPreparePanel::isMinff() const
{
  return p_model != 0 && p_model->source() == "minff";
}

bool GromacsPreparePanel::isClayff() const
{
  return isMinff() && p_model->mineralForceField() == "clayff";
}

void GromacsPreparePanel::showFields()
{
  bool minff = isMinff();
  bool other = p_ff->GetSelection() == otherIndex();
  p_source->Enable(p_editable);
  p_source->SetSelection(minff ? 1 : 0);
  p_structure->Enable(p_editable);
  p_browse->Enable(p_editable && p_structure->GetSelection() == 1);
  p_ff->Enable(p_editable);
  p_ffImport->Enable(p_editable);
  p_otherFf->Enable(p_editable && other);
  p_water->Enable(p_editable);
  p_ignh->Enable(p_editable);
  p_neut->Enable(p_editable);
  p_merge->Enable(p_editable);
  p_box->Enable(p_editable);
  p_dist->Enable(p_editable);
  p_center->Enable(p_editable);
  p_conc->Enable(p_editable);
  p_rmin->Enable(p_editable);
  p_pion->Enable(p_editable);
  p_nion->Enable(p_editable);
  for (int i = 0; i < 2; i++) {
    wxWindow *w = i == 0 ? (wxWindow*)p_mrep[0] : (wxWindow*)p_msubs;
    w->Enable(p_editable);
  }
  p_mrep[1]->Enable(p_editable);
  p_mrep[2]->Enable(p_editable);
  p_mseed->Enable(p_editable);
  p_mvariant->Enable(p_editable);
  p_mmineral->Enable(p_editable && p_mvariant->GetSelection() == 1);
  p_mangle->Enable(p_editable);
  p_mions->Enable(p_editable);
  p_mff->Enable(p_editable);
  p_mbend->Enable(p_editable);
  p_mrigid->Enable(p_editable);
  p_mcellBtn->Enable(p_editable);
  p_mbuild->Enable(p_editable);

  // What does not apply to the source is hidden, not just greyed
  for (size_t i = 0; i < p_proteinOnly.size(); i++) p_proteinOnly[i]->Show(!minff);
  p_leftSizer->Show(p_minffSizer, minff, true);
  // ClayFF has no variants, angle constants or ion sets; its own bend and
  // water choices show only for it
  bool clay = isClayff();
  for (size_t i = 0; i < p_minffFfOnly.size(); i++) p_minffFfOnly[i]->Show(minff && !clay);
  for (size_t i = 0; i < p_clayffOnly.size(); i++) p_clayffOnly[i]->Show(clay);
  // the box follows the force field chosen in it
  p_mineralBox->SetLabel(clay ? _("Mineral (ClayFF)") : _("Mineral (MINFF)"));
  p_ffBox->SetLabel(minff ? _("Water model") : _("Force field"));
  p_waterBox->SetLabel(minff ? _("Water layer") : _("Box and water"));
  p_distLabel->SetLabel(minff ? _("Water layer above the mineral:")
                              : _("Distance to edge:"));
  p_dist->SetToolTip(minff
      ? _("Water added above the mineral, along the cell's c axis. 0 keeps the "
          "mineral's cell as it is and adds no water; a charged mineral is then "
          "not neutralised, because there is no water to replace by ions.")
      : _("The distance from the solute to the box edge."));
  showIonNote();
  relayout();
}

// What the Ions box does, for the state of the tick box and the source.
void GromacsPreparePanel::showIonNote()
{
  bool neut = p_neut->GetValue();
  if (isMinff())
    p_ionNote->SetLabel(neut
        ? _("With a water layer the mineral's charge is neutralised by counter-ions.")
        : _("The mineral keeps its charge; only the salt below is added."));
  else
    p_ionNote->SetLabel(neut
        ? _("The system is neutralised first; salt is added on top.")
        : _("The system keeps its net charge; only the salt below is added."));
  p_ionNote->Wrap(wrapPixels(p_ionNote, 40));
}

// The page's natural size in the most demanding of its three states
// (protein, MINFF, ClayFF), for sizing the window once.  The model is put
// back as it was.
wxSize GromacsPreparePanel::bestSizeAllSources()
{
  if (p_model == 0 || GetSizer() == 0) return GetBestSize();
  const string src = p_model->source(), mff = p_model->mineralForceField();
  const char *states[3][2] = { { "protein", "minff" }, { "minff", "minff" },
                               { "minff", "clayff" } };
  bool was = p_refreshing;
  p_refreshing = true;
  wxSize best(0, 0);
  for (int i = 0; i < 3; i++) {
    p_model->source(states[i][0]);
    p_model->mineralForceField(states[i][1]);
    showFields();
    wxSize b = GetSizer()->CalcMin();
    best.x = std::max(best.x, b.x);
    best.y = std::max(best.y, b.y);
  }
  p_model->source(src);
  p_model->mineralForceField(mff);
  showFields();
  p_refreshing = was;
  return best;
}

// Test hook: the width each box asks for, "title=width ..." per column.
string GromacsPreparePanel::layoutReport() const
{
  std::ostringstream os;
  wxSizer *cols[2] = { p_leftSizer, p_rightSizer };
  for (int c = 0; c < 2; c++) {
    os << (c ? " right:" : "left:");
    for (wxSizerItemList::compatibility_iterator n = cols[c]->GetChildren().GetFirst();
         n; n = n->GetNext()) {
      wxStaticBoxSizer *sb = dynamic_cast<wxStaticBoxSizer*>(n->GetData()->GetSizer());
      if (sb == 0 || !n->GetData()->IsShown()) continue;
      os << " [" << sb->GetStaticBox()->GetLabel().ToUTF8() << "="
         << sb->CalcMin().x << "x" << sb->CalcMin().y << "]";
    }
  }
  return os.str();
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
  if (isClayff()) {
    ids.push_back(CLAYFF_WATERS[0].id);
  } else if (isMinff()) {
    for (int i = 0; i < NMINFF_WATER; i++) ids.push_back(MINFF_WATERS[i].id);
  } else {
    ids = watersOf(ffId(p_ff->GetSelection()));
  }
  p_water->Clear();
  int pick = 0;
  for (size_t i = 0; i < ids.size(); i++) {
    string lab = waterLabel(ids[i]);
    if (isClayff()) lab = CLAYFF_WATERS[0].label;
    else if (isMinff())
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
  bool clay = isClayff();
  const IonInfo *pos = clay ? CLAYFF_POSITIVE : minff ? MINFF_POSITIVE : POSITIVE;
  int np = clay ? (int)(sizeof(CLAYFF_POSITIVE) / sizeof(IonInfo))
         : minff ? (int)(sizeof(MINFF_POSITIVE) / sizeof(IonInfo))
                 : (int)(sizeof(POSITIVE) / sizeof(IonInfo));
  const IonInfo *neg = clay ? CLAYFF_NEGATIVE : minff ? MINFF_NEGATIVE : NEGATIVE;
  int nn = clay ? (int)(sizeof(CLAYFF_NEGATIVE) / sizeof(IonInfo))
         : minff ? (int)(sizeof(MINFF_NEGATIVE) / sizeof(IonInfo))
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
  fillForceFields();
  int ff = ffIndex(p_model->forceField());
  p_ff->SetSelection(ff);
  p_otherFf->SetValue(ff == otherIndex() ? U(p_model->forceField().c_str())
                                     : wxString());
  fillWaters(p_model->waterModel());
  p_ignh->SetValue(p_model->ignoreHydrogens());
  p_merge->SetValue(p_model->mergeChains());
  p_neut->SetValue(p_model->neutralise());
  showIonNote();
  int b = 0;
  for (int i = 0; i < NBOX; i++) if (p_model->boxType() == BOXES[i][0]) b = i;
  p_box->SetSelection(b);
  std::ostringstream os;
  os << p_model->boxDistance();
  p_dist->SetValue(U(os.str().c_str()));
  p_center->ChangeValue(U(p_model->boxCenter().c_str()));
  std::ostringstream oc;
  oc << p_model->ionConcentration();
  p_conc->SetValue(U(oc.str().c_str()));
  std::ostringstream orm;
  orm << p_model->ionMinDistance();
  p_rmin->SetValue(U(orm.str().c_str()));
  fillIons();
  setRepeat(p_model->minffReplicate());
  p_msubs->SetValue(U(p_model->minffSubstitutions().c_str()));
  p_mseed->SetValue(U(p_model->minffSeed().c_str()));
  p_mmineral->SetValue(U(p_model->minffMineral().c_str()));
  p_mvariant->SetSelection(p_model->minffVariant() == "tminff" ? 1 : 0);
  int k = 2;
  for (size_t i = 0; i < sizeof(ANGLE_KS) / sizeof(ANGLE_KS[0]); i++)
    if (ANGLE_KS[i] == p_model->minffAngleK()) k = (int)i;
  p_mangle->SetSelection(k);
  int is = 0;
  for (int i = 0; i < NMINFF_IONSETS; i++)
    if (p_model->minffIonSet() == MINFF_IONSETS[i].id) is = i;
  p_mions->SetSelection(is);
  p_mff->SetSelection(p_model->mineralForceField() == "clayff" ? 1 : 0);
  int bd = 0;
  for (int i = 0; i < NCLAYFF_BENDS; i++)
    if (p_model->clayffBend() == CLAYFF_BENDS[i].id) bd = i;
  p_mbend->SetSelection(bd);
  p_mrigid->SetValue(p_model->clayffRigidWater());
  p_badRmin.clear();
  p_buildWarnings.clear();
  p_badDistance.clear();
  p_badConcentration.clear();
  p_refreshing = false;
  showStructure();
  showMineral();
  stageForceField(false);
}

// The choice: built-ins (unless an imported folder of that name replaces
// one), the imported ones, then Other.
void GromacsPreparePanel::fillForceFields()
{
  vector<GromacsForceFieldLibrary::Entry> lib = GromacsForceFieldLibrary::list();
  p_ffItems.clear();
  for (int i = 0; i < NFF; i++) {
    bool replaced = false;
    for (size_t k = 0; k < lib.size(); k++) if (lib[k].name == FORCEFIELDS[i].id) replaced = true;
    if (!replaced) p_ffItems.push_back(std::make_pair(string(FORCEFIELDS[i].id),
                                                      string(FORCEFIELDS[i].label)));
  }
  for (size_t k = 0; k < lib.size(); k++)
    p_ffItems.push_back(std::make_pair(lib[k].name, lib[k].label + " (imported)"));
  p_ff->Clear();
  for (size_t i = 0; i < p_ffItems.size(); i++) p_ff->Append(U(p_ffItems[i].second.c_str()));
  p_ff->Append(_("Other (GMXLIB)"));
}

int GromacsPreparePanel::otherIndex() const { return (int)p_ffItems.size(); }

int GromacsPreparePanel::ffIndex(const string& id) const
{
  for (size_t i = 0; i < p_ffItems.size(); i++) if (p_ffItems[i].first == id) return (int)i;
  return otherIndex();
}

string GromacsPreparePanel::ffId(int index) const
{
  return index >= 0 && index < otherIndex() ? p_ffItems[index].first : string();
}

static const string FF_SEP = "__SLASH__";

// Names of the task's inputs that hold a force field folder other than min.ff.
static vector<string> stagedForceFieldFiles(MdTask *t, const string& only)
{
  vector<string> out;
  vector<string> names = t->getDataFileNames(JCode::AUXILIARY_INPUT);
  for (size_t i = 0; i < names.size(); i++) {
    size_t p = names[i].find(".ff" + FF_SEP);
    if (p == string::npos || !endsWith(names[i], ".gmxff") || names[i].compare(0, 6 + FF_SEP.size(), "min.ff" + FF_SEP) == 0)
      continue;
    if (only.empty() || names[i].compare(0, p, only) == 0) out.push_back(names[i]);
  }
  return out;
}

// The imported force field the model names travels with the task as flat
// inputs <name>.ff__SLASH__<file>.gmxff (the extension types any file name
// for the data server), like min.ff's includes, so every later
// task and a remote data server get it with no client path in the job.
void GromacsPreparePanel::stageForceField(bool force)
{
  MdTask *t = task();
  if (t == 0 || !p_editable || p_model == 0 || p_staging) return;
  p_staging = true;
  string ff = isMinff() ? "" : p_model->forceField();
  GromacsForceFieldLibrary::Entry e;
  bool inLibrary = !ff.empty() && GromacsForceFieldLibrary::find(ff, e);
  vector<string> have = stagedForceFieldFiles(t, "");
  vector<string> want;
  if (inLibrary) {
    vector<string> rel = GromacsForceFieldLibrary::files(e);
    for (size_t i = 0; i < rel.size(); i++)
      want.push_back(GromacsIncludes::encodeName(ff + ".ff/" + rel[i]) + ".gmxff");
  } else if (!ff.empty()) {
    // not in this library: keep the copy the task already holds
    want = stagedForceFieldFiles(t, ff);
    if (!want.empty() && !isMinff() && p_owner != 0)
      p_owner->getFeedback().setMessage("The imported force field " + ff +
          " is no longer in your library; the copy stored with this task is used.",
          WxFeedback::WARNING);
  }
  for (size_t i = 0; i < have.size(); i++)
    if (std::find(want.begin(), want.end(), have[i]) == want.end()) t->putInputFile(have[i], 0);
  if (inLibrary) {
    for (size_t i = 0; i < want.size(); i++) {
      if (!force && std::find(have.begin(), have.end(), want[i]) != have.end()) continue;
      string rel = GromacsIncludes::decodeName(want[i]).substr(ff.size() + 4);
      std::istringstream in(slurp(e.path + "/" + rel.substr(0, rel.size() - 6)));
      t->putInputFile(want[i], &in);
    }
  }
  p_staging = false;
}

void GromacsPreparePanel::OnImportForceField(wxCommandEvent&)
{
  wxMessageDialog ask(this, _("Import a force field from a folder (<name>.ff) or a zip file holding one?"),
                      _("Import force field"), wxYES_NO | wxCANCEL | wxICON_QUESTION);
  ask.SetYesNoCancelLabels(_("Folder..."), _("Zip file..."), _("Cancel"));
  int a = ask.ShowModal();
  if (a == wxID_CANCEL) return;
  wxString path;
  if (a == wxID_YES) {
    wxDirDialog dlg(this, _("Choose the <name>.ff folder"), wxEmptyString,
                    wxDD_DIR_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;
    path = dlg.GetPath();
  } else {
    wxFileDialog dlg(this, _("Choose the zip file"), wxEmptyString, wxEmptyString,
                     "Zip files (*.zip)|*.zip", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;
    path = dlg.GetPath();
  }
  string message;
  bool ok = importForceField(string(path.ToUTF8()), 0, message);
  if (!ok && !message.empty())
    wxMessageBox(U(message.c_str()), "ECCE GROMACS Prepare", wxOK | wxICON_ERROR, this);
}

// clash: 0 ask, 1 replace, 2 cancel (the test hook answers without a dialog).
bool GromacsPreparePanel::importForceField(const string& path, int clash, string& message)
{
  vector<string> builtIn;
  for (int i = 0; i < NFF; i++) builtIn.push_back(FORCEFIELDS[i].id);
  string name;
  GromacsForceFieldLibrary::Result r =
    GromacsForceFieldLibrary::import(path, clash == 1, builtIn, name, message);
  if (r == GromacsForceFieldLibrary::CLASH) {
    bool replace = clash == 1;
    if (clash == 0) {
      replace = wxMessageBox(U((message + " Replace it?").c_str()), _("Import force field"),
                             wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this) == wxYES;
    }
    if (!replace) { message = "cancelled: " + message; return false; }
    r = GromacsForceFieldLibrary::import(path, true, builtIn, name, message);
  }
  if (r != GromacsForceFieldLibrary::OK) return false;
  if (p_model != 0) {
    p_model->source("protein");
    p_refreshing = true;
    p_source->SetSelection(0);
    p_refreshing = false;
    p_model->forceField(name);
    refreshGUI();
    stageForceField(true);
    showFields();
    changed();
  }
  message = "imported " + name;
  return true;
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
  p_cellLabel->SetToolTip(U(cell.c_str()));
  p_summaryLabel->SetLabel(p_built
      ? U(p_model->minffSummary().c_str())
      : _("No topology built yet. Choose a unit cell and press Build topology."));
  markBuildNeeded(!p_built);
  p_summaryLabel->Wrap(wrapPixels(p_summaryLabel));
  relayout();
}

// Options changed after a build: the stored files no longer match them.
void GromacsPreparePanel::markBuildNeeded(bool needed)
{
  if (needed) p_mbuildFrame->SetBackgroundColour(wxColour(204, 0, 0));
  else p_mbuildFrame->SetBackgroundColour(p_mbuildFrame->GetParent()->GetBackgroundColour());
  p_mbuildFrame->Refresh();
}

void GromacsPreparePanel::minffStale()
{
  if (p_refreshing || !p_built) return;
  p_summaryLabel->SetLabel(_("The options changed since the topology was built; "
                             "press Build topology to apply them."));
  markBuildNeeded(true);
  p_summaryLabel->Wrap(wrapPixels(p_summaryLabel));
  relayout();
}

// The cell the MD tasks after this Prepare get (the built mineral's box,
// its c lengthened by the water layer) against grompp's limit for the
// cut-off: the pair-list cut-off must stay below half of each box vector and
// below each diagonal element, so an axis needs a width of
// min(|v|, 2 v_ii) of at least twice that.  Names every axis that falls
// short, with how often to repeat the unit cell or how much water to add.
// Empty when the cell holds the default cut-off of those tasks.
string GromacsPreparePanel::cellWarning() const
{
  double v[3][3];
  if (!cellVectors(v)) return "";
  // grompp checks the pair list, which mdrun's buffer makes about 0.1 nm
  // longer than the cut-off.
  const double cut = InteractionModel::SHORTCUT;
  const double need = 2.0 * (cut + 0.1);
  int rep[3] = { 1, 1, 1 };
  std::istringstream rin(p_model->minffReplicate());
  rin >> rep[0] >> rep[1] >> rep[2];
  const double water = p_model->boxDistance() > 0 ? p_model->boxDistance() : 0.0;
  const char axis[3] = { 'a', 'b', 'c' };
  string out;
  for (int i = 0; i < 3; i++) {
    double w = std::min(std::sqrt(v[i][0]*v[i][0] + v[i][1]*v[i][1] + v[i][2]*v[i][2]),
                        2.0 * v[i][i]);
    if (w >= need) continue;
    char buf[300];
    int n = std::max(rep[i], 1);
    double unit = (w - (i == 2 ? water : 0.0)) / n;
    int times = unit > 0 ? (int)std::ceil((need - (i == 2 ? water : 0.0)) / unit - 1e-9) : 0;
    snprintf(buf, sizeof buf, "%sThe box is %.2f nm along %c (%d unit cell%s) and "
             "needs %.2f nm: repeat the cell at least %d times along %c",
             out.empty() ? "" : " ", w, axis[i], n, n == 1 ? "" : "s", need,
             std::max(times, n + 1), axis[i]);
    out += buf;
    if (i == 2) {
      snprintf(buf, sizeof buf, ", or add at least %.1f nm of water",
               std::ceil((need - w) * 10.0 - 1e-9) / 10.0);
      out += buf;
    }
    out += ", or lower the cut-off in the tasks.";
  }
  if (out.empty()) return "";
  char head[120];
  snprintf(head, sizeof head, "The box is too small for the %.1f nm cut-off the MD tasks use. ", cut);
  return head + out;
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
  if (!p_buildWarnings.empty()) warn += (warn.empty() ? "" : "\n") + p_buildWarnings;
  string why;
  if (!warn.empty()) p_owner->getFeedback().setMessage(warn, WxFeedback::WARNING);
  else if (validate(why)) p_owner->getFeedback().clearMessage();
}

bool GromacsPreparePanel::validate(string& why) const
{
  if (!p_badDistance.empty()) { why = p_badDistance; return false; }
  if (!p_badCenter.empty()) { why = p_badCenter; return false; }
  if (!p_badConcentration.empty()) { why = p_badConcentration; return false; }
  if (!p_badRmin.empty()) { why = p_badRmin; return false; }
  if (p_ff->GetSelection() == otherIndex()) {
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
  if (ff >= 0 && ff < otherIndex()) {
    p_model->forceField(p_ffItems[ff].first);
  } else {
    string n(p_otherFf->GetValue().ToUTF8());
    if (!n.empty()) p_model->forceField(n);
  }
  p_refreshing = true;
  fillWaters(p_model->waterModel());
  p_refreshing = false;
  showFields();
  stageForceField(false);
  changed();
}

void GromacsPreparePanel::OnOtherForceField(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0 || p_ff->GetSelection() != otherIndex()) return;
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
  p_model->neutralise(p_neut->GetValue());
  showIonNote();
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
  p_model->mineralForceField(d.mineralForceField());
  p_model->waterModel(minff ? "opc3" : d.waterModel());
  p_model->positiveIon(minff ? "Na" : d.positiveIon());
  p_model->negativeIon(minff ? "Cl" : d.negativeIon());
  p_model->boxDistance(minff ? 0.0 : d.boxDistance());
  p_model->ionConcentration(minff ? 0.0 : d.ionConcentration());
  refreshGUI();
  changed();
  fitToSource();
}

// The sources differ in height: fit the page to this one, so the window has
// neither empty space nor a scrollbar, and keep the window on the screen.
void GromacsPreparePanel::fitToSource()
{
  wxScrolledWindow *page = dynamic_cast<wxScrolledWindow*>(GetParent());
  wxWindow *top = wxGetTopLevelParent(this);
  if (page == 0 || top == 0 || GetSizer() == 0) return;
  int idx = wxDisplay::GetFromWindow(top);
  wxRect area = wxDisplay(idx == wxNOT_FOUND ? 0 : idx).GetClientArea();
  int frameExtra = top->GetSize().y - page->GetSize().y;
  int want = std::min(GetSizer()->CalcMin().y + 8,
                      std::max(300, area.height - 60 - frameExtra));
  page->SetMinSize(wxSize(page->GetMinSize().x, want));
  top->SetMinSize(wxSize(420, 320));
  top->Fit();
}

// The model keeps the repeat as "a b c"; an empty field counts as missing.
string GromacsPreparePanel::repeat() const
{
  string out;
  for (int i = 0; i < 3; i++) {
    string v(p_mrep[i]->GetValue().ToUTF8());
    std::istringstream in(v);
    string word;
    while (in >> word) out += (out.empty() ? "" : " ") + word;
  }
  return out;
}

void GromacsPreparePanel::setRepeat(const string& value)
{
  std::istringstream in(value);
  for (int i = 0; i < 3; i++) {
    string word;
    in >> word;
    p_mrep[i]->SetValue(U(word.c_str()));
  }
}

void GromacsPreparePanel::OnMinffText(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  p_model->minffReplicate(repeat());
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
  int is = p_mions->GetSelection();
  if (is >= 0 && is < NMINFF_IONSETS) p_model->minffIonSet(MINFF_IONSETS[is].id);
  int bd = p_mbend->GetSelection();
  if (bd >= 0 && bd < NCLAYFF_BENDS) p_model->clayffBend(CLAYFF_BENDS[bd].id);
  p_model->clayffRigidWater(p_mrigid->GetValue());
  p_mmineral->Enable(p_editable && p_mvariant->GetSelection() == 1);
  minffStale();
  changed();
}

// MINFF or ClayFF: the water model and the ions are those of the force field.
void GromacsPreparePanel::OnMineralForceField(wxCommandEvent&)
{
  if (p_refreshing || p_model == 0) return;
  bool clay = p_mff->GetSelection() == 1;
  p_model->mineralForceField(clay ? "clayff" : "minff");
  p_refreshing = true;
  fillWaters(clay ? CLAYFF_WATERS[0].id : "opc3");
  fillIons();
  p_refreshing = false;
  showFields();
  minffStale();
  changed();
}

// Where Browse... for the unit cell starts: the folder last used, else the
// unit cells that ship with ECCE (data/minff/unit_cells; in a source tree
// third_party/), else the home directory.
string GromacsPreparePanel::cellStartDir() const
{
  Preferences prefs("GromacsPrepare");
  string dir;
  prefs.getString("GromacsPrepare.MineralDir", dir);
  if (!dir.empty() && wxDirExists(U(dir.c_str()))) return dir;
  const string home = Ecce::ecceHome();
  const char *sub[] = { "/data/minff/unit_cells", "/third_party/minff/unit_cells" };
  for (int i = 0; i < 2; i++)
    if (wxDirExists(U((home + sub[i]).c_str()))) return home + sub[i];
  return Ecce::realUserHome();
}

void GromacsPreparePanel::OnMinffCell(wxCommandEvent&)
{
  Preferences prefs("GromacsPrepare");
  string dir = cellStartDir();
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

// The strings of a JSON array of strings, one per line of the result.
static string jsonStrings(const string& text, const string& key)
{
  string::size_type p = text.find("\"" + key + "\": [");
  if (p == string::npos) return "";
  string::size_type e = text.find(']', p);
  string out, cur;
  bool in = false;
  for (string::size_type i = text.find('[', p) + 1; i < e && i < text.size(); i++) {
    char c = text[i];
    if (!in) { if (c == '"') { in = true; cur.clear(); } continue; }
    if (c == '\\' && i + 1 < text.size()) { cur += text[++i]; continue; }
    if (c == '"') { in = false; out += (out.empty() ? "" : "\n") + cur; continue; }
    cur += c;
  }
  return out;
}

// The keys of a JSON object whose values are lists, joined by spaces.
static string jsonObjectKeys(const string& text, const string& key)
{
  string::size_type p = text.find("\"" + key + "\": {");
  if (p == string::npos) return "";
  string::size_type i = text.find('{', p);
  if (text.compare(i, 2, "{}") == 0) return "";
  string::size_type e = text.find("\n  }", p);
  if (e == string::npos) return "";
  string out;
  while (i < e) {
    string::size_type a = text.find("\": [", i);
    if (a == string::npos || a > e) break;
    string::size_type b = text.rfind('"', a - 1);
    out += (out.empty() ? "" : " ") + text.substr(b + 1, a - b - 1);
    i = text.find(']', a);
  }
  return out;
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
  const bool clay = isClayff();
  if (clay) {
    // ClayFF takes its water (SPC) and has no variants, angles or ion sets
    argv.push_back("--forcefield");
    argv.push_back("clayff");
    argv.push_back("--clayff-bend");
    argv.push_back(p_model->clayffBend());
    if (p_model->clayffRigidWater()) argv.push_back("--clayff-rigid-water");
  } else {
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
    argv.push_back("--ion-set");
    argv.push_back(p_model->minffIonSet());
  }
  // a bare cell gets the vacuum MINFF needs, instead of a layer bonded to itself
  argv.push_back("--add-gap");
  argv.push_back("--gui");

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
  string gap = jsonField(so, "gap_added_nm");
  string gapText = gap.empty() ? "" : " The cell's c was lengthened by " + gap +
                   " nm so that the layer is not bonded to its own periodic image.";
  p_buildWarnings = jsonStrings(so, "warnings");
  string ffText = "";
  if (clay) {
    string missing = jsonStrings(so, "types_missing_from_ff");
    for (string::size_type i = 0; i < missing.size(); i++) if (missing[i] == '\n') missing[i] = ' ';
    string differing = jsonObjectKeys(so, "charges_differing_from_atomipy");
    if (!missing.empty())
      p_buildWarnings += (p_buildWarnings.empty() ? "" : "\n") +
          string("ClayFF has no parameters for the atom types ") + missing +
          "; grompp will stop on them.";
    if (!differing.empty())
      p_buildWarnings += (p_buildWarnings.empty() ? "" : "\n") +
          string("The charges of Table 1 of the ClayFF paper replace atomipy's for ") +
          differing + ".";
    ffText = "ClayFF, M-O-H bend " + p_model->clayffBend() +
             (p_model->clayffRigidWater() ? ", rigid water" : "") + ", ";
    untyped = "";
    water = "SPC";
  }
  p_model->minffCell(cell);
  p_model->minffSummary("Built from " + cell + ": " + atoms + " atoms, charge " + charge +
                        ", " + ffText + "water " + water + "." + untyped + gapText);
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
  string cen;
  if (parseCenter(string(p_center->GetValue().ToUTF8()), cen)) {
    p_model->boxCenter(cen);
    p_badCenter.clear();
  } else {
    p_badCenter = "The solute centre must be empty or three numbers (x y z) in nm.";
  }
  string c(p_conc->GetValue().ToUTF8());
  if (isNumber(c, v, true)) {
    p_model->ionConcentration(v);
    p_badConcentration.clear();
  } else {
    p_badConcentration = "The salt concentration must be a number, 0 or more mol/L.";
  }
  string r(p_rmin->GetValue().ToUTF8());
  if (isNumber(r, v, true)) {
    p_model->ionMinDistance(v);
    p_badRmin.clear();
  } else {
    p_badRmin = "The minimum ion distance must be a number, 0 or more nm.";
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
  // The file is also the task's structure, so the Builder and the Organizer
  // show it; the staged .gmxpdb alone is only an input file of the job.
  ifstream fin(path.c_str());
  Fragment frag;
  if (!fin || !frag.restorePDB(fin, 1.0, true, 1, " ", true, false, " ") ||
      !t->fragment(&frag)) {
    message = "The file " + path + " is not a PDB structure ECCE can read.";
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
    int ff = ffIndex(value);
    p_ff->SetSelection(ff);
    if (ff == otherIndex()) p_otherFf->SetValue(U(value.c_str()));
    OnForceField(ev);
  } else if (field == "ffclash") {
    p_testClash = value == "replace" ? 1 : 2;
  } else if (field == "ffimport") {
    return importForceField(value, p_testClash, message);
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
  } else if (field == "ignh" || field == "merge" || field == "neut") {
    (field == "ignh" ? p_ignh : field == "merge" ? p_merge : p_neut)->SetValue(value == "1");
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
  } else if (field == "center") {
    p_center->SetValue(value == "-" ? wxString() : U(value.c_str()));
    OnNumber(ev);
    if (!p_badCenter.empty()) { message = p_badCenter; return false; }
  } else if (field == "conc") {
    p_conc->SetValue(U(value.c_str()));
    OnNumber(ev);
  } else if (field == "rmin") {
    p_rmin->SetValue(U(value.c_str()));
    OnNumber(ev);
  } else if (field == "mions") {
    for (int i = 0; i < NMINFF_IONSETS; i++)
      if (value == MINFF_IONSETS[i].id) p_mions->SetSelection(i);
    OnMinffChoice(ev);
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
    if (field == "mrep") setRepeat(value);
    else (field == "msubs" ? p_msubs :
          field == "mseed" ? p_mseed : p_mmineral)->SetValue(U(value.c_str()));
    OnMinffText(ev);
  } else if (field == "mvariant") {
    p_mvariant->SetSelection(value == "tminff" ? 1 : 0);
    OnMinffChoice(ev);
  } else if (field == "mangle") {
    for (size_t i = 0; i < sizeof(ANGLE_KS) / sizeof(ANGLE_KS[0]); i++)
      if (std::to_string(ANGLE_KS[i]) == value) p_mangle->SetSelection((int)i);
    OnMinffChoice(ev);
  } else if (field == "mff") {
    p_mff->SetSelection(value == "clayff" ? 1 : 0);
    OnMineralForceField(ev);
  } else if (field == "mbend") {
    for (int i = 0; i < NCLAYFF_BENDS; i++)
      if (value == CLAYFF_BENDS[i].id) p_mbend->SetSelection(i);
    OnMinffChoice(ev);
  } else if (field == "mrigid") {
    p_mrigid->SetValue(value == "1");
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

string GromacsPreparePanel::forceFieldChoices() const
{
  string out;
  for (unsigned i = 0; i < p_ff->GetCount(); i++)
    out += (i ? "|" : "") + string(p_ff->GetString(i).ToUTF8());
  return out;
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
     << " center=" << (p_model->boxCenter().empty() ? "-" : p_model->boxCenter())
     << " conc=" << p_model->ionConcentration() << " ions="
     << p_model->positiveIon() << "," << p_model->negativeIon()
     << " ignh=" << p_model->ignoreHydrogens()
     << " merge=" << p_model->mergeChains()
     << " source=" << p_model->source() << " built=" << (p_built ? 1 : 0)
     << " mineral=" << p_model->minffSummary()
     << " rmin=" << p_model->ionMinDistance()
     << " ionset=" << p_model->minffIonSet()
     << " mff=" << p_model->mineralForceField()
     << " bend=" << p_model->clayffBend()
     << " rigid=" << (p_model->clayffRigidWater() ? 1 : 0)
     << " buildwarn=" << (p_buildWarnings.empty() ? 0 : 1)
     << " cellwarning=" << (cellWarning().empty() ? 0 : 1)
     << " repshown=" << repeat()
     << " repfields=" << p_mrep[0]->GetValue().ToUTF8() << "|"
     << p_mrep[1]->GetValue().ToUTF8() << "|" << p_mrep[2]->GetValue().ToUTF8()
     << " source_label=" << p_source->GetString(p_source->GetSelection()).ToUTF8()
     << " mineral_title=" << p_mineralBox->GetLabel().ToUTF8();
  return os.str();
}
