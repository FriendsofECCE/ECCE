// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <fstream>
#include <sstream>
#include <algorithm>
  using std::ifstream;
  using std::istringstream;

#include "util/EcceURL.H"
#include "util/Ecce.H"
#include "util/Preferences.H"

#include "tdat/Fragment.H"

#include "dsm/EDSIFactory.H"
#include "dsm/GromacsStructure.H"
#include "dsm/JCode.H"
#include "dsm/MdTask.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/Session.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxFileDialog.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxMessageDialog.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/MDEdBase.H"
#include "wxgui/GromacsInputsPanel.H"

static const wxWindowID ID_ATTACH_TOP = wxNewId();
static const wxWindowID ID_REMOVE_TOP = wxNewId();
static const wxWindowID ID_ATTACH_GRO = wxNewId();
static const wxWindowID ID_REMOVE_GRO = wxNewId();
static const wxWindowID ID_ATTACH_INC = wxNewId();
static const wxWindowID ID_REMOVE_INC = wxNewId();
static const wxWindowID ID_LIST_INC   = wxNewId();

// The three kinds of file, as the JCode types GROMACS.edml declares them
static const int TOPOLOGY  = JCode::TOPOLOGY_OUTPUT;
static const int STRUCTURE = JCode::RESTART_OUTPUT;
static const int INCLUDE   = JCode::AUXILIARY_INPUT;

static string extensionless(const string& path)
{
  size_t slash = path.find_last_of("/\\");
  string tail = (slash == string::npos) ? path : path.substr(slash + 1);
  size_t dot = tail.find_last_of('.');
  return (dot == string::npos || dot == 0) ? tail : tail.substr(0, dot);
}

static string tailOf(const string& path)
{
  size_t slash = path.find_last_of("/\\");
  return (slash == string::npos) ? path : path.substr(slash + 1);
}


GromacsInputsPanel::GromacsInputsPanel(MDEdBase *owner, wxWindow *parent)
  : ewxPanel(parent, wxID_ANY)
{
  p_owner = owner;
  p_editable = true;

  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);

  ewxStaticText *intro = new ewxStaticText(this, wxID_ANY,
      _("GROMACS runs a system you have built elsewhere (pdb2gmx, CHARMM-GUI, "
        "a collaborator). Attach its topology and starting structure here; a "
        "task without its own uses those of an earlier task in the study. "
        "A topology is kept as .gmxtop so it cannot be mistaken for an "
        "NWChem topology; the job still sees it as topol.top."));
  intro->Wrap(520);
  top->Add(intro, 0, wxALL | wxEXPAND, 10);

  wxFlexGridSizer *grid = new wxFlexGridSizer(3, 3, 8, 8);
  grid->AddGrowableCol(1);

  grid->Add(new ewxStaticText(this, wxID_ANY, _("Topology:")), 0,
            wxALIGN_CENTER_VERTICAL);
  p_topology = new ewxStaticText(this, wxID_ANY, wxEmptyString);
  grid->Add(p_topology, 1, wxALIGN_CENTER_VERTICAL | wxEXPAND);
  wxBoxSizer *b1 = new wxBoxSizer(wxHORIZONTAL);
  p_attachTop = new ewxButton(this, ID_ATTACH_TOP, _("Attach..."));
  p_removeTop = new ewxButton(this, ID_REMOVE_TOP, _("Remove"));
  b1->Add(p_attachTop, 0, wxRIGHT, 4);
  b1->Add(p_removeTop, 0);
  grid->Add(b1);

  grid->Add(new ewxStaticText(this, wxID_ANY, _("Starting structure:")), 0,
            wxALIGN_CENTER_VERTICAL);
  p_structure = new ewxStaticText(this, wxID_ANY, wxEmptyString);
  grid->Add(p_structure, 1, wxALIGN_CENTER_VERTICAL | wxEXPAND);
  wxBoxSizer *b2 = new wxBoxSizer(wxHORIZONTAL);
  p_attachGro = new ewxButton(this, ID_ATTACH_GRO, _("Attach..."));
  p_removeGro = new ewxButton(this, ID_REMOVE_GRO, _("Remove"));
  b2->Add(p_attachGro, 0, wxRIGHT, 4);
  b2->Add(p_removeGro, 0);
  grid->Add(b2);

  grid->Add(new ewxStaticText(this, wxID_ANY, _("Include files:")), 0,
            wxALIGN_TOP);
  p_includes = new ewxListBox(this, ID_LIST_INC, wxDefaultPosition,
                              wxSize(-1, 90));
  grid->Add(p_includes, 1, wxEXPAND);
  wxBoxSizer *b3 = new wxBoxSizer(wxVERTICAL);
  p_attachInc = new ewxButton(this, ID_ATTACH_INC, _("Attach..."));
  p_removeInc = new ewxButton(this, ID_REMOVE_INC, _("Remove"));
  b3->Add(p_attachInc, 0, wxBOTTOM, 4);
  b3->Add(p_removeInc, 0);
  grid->Add(b3);

  top->Add(grid, 0, wxALL | wxEXPAND, 10);

  p_status = new ewxStaticText(this, wxID_ANY, wxEmptyString);
  top->Add(p_status, 0, wxALL | wxEXPAND, 10);

  SetSizer(top);

  Bind(wxEVT_BUTTON, &GromacsInputsPanel::OnAttachTopology, this, ID_ATTACH_TOP);
  Bind(wxEVT_BUTTON, &GromacsInputsPanel::OnRemoveTopology, this, ID_REMOVE_TOP);
  Bind(wxEVT_BUTTON, &GromacsInputsPanel::OnAttachStructure, this, ID_ATTACH_GRO);
  Bind(wxEVT_BUTTON, &GromacsInputsPanel::OnRemoveStructure, this, ID_REMOVE_GRO);
  Bind(wxEVT_BUTTON, &GromacsInputsPanel::OnAttachInclude, this, ID_ATTACH_INC);
  Bind(wxEVT_BUTTON, &GromacsInputsPanel::OnRemoveInclude, this, ID_REMOVE_INC);
  Bind(wxEVT_LISTBOX, &GromacsInputsPanel::OnIncludeSelected, this, ID_LIST_INC);
}

GromacsInputsPanel::~GromacsInputsPanel()
{
  p_owner = 0;
}

MdTask *GromacsInputsPanel::task() const
{
  if (p_url.empty()) return 0;
  try {
    return dynamic_cast<MdTask*>(EDSIFactory::getResource(EcceURL(p_url)));
  } catch (...) {
    return 0;
  }
}

void GromacsInputsPanel::setContext(const string& url)
{
  p_url = url;
  refresh();
}

void GromacsInputsPanel::setEditable(bool editable)
{
  p_editable = editable;
  refresh();
}

/**
 * Where a kind of file is found for the task: on the task itself, or on the
 * nearest earlier one that has it.  Names the file and where it is.
 */
bool GromacsInputsPanel::describe(int fileType, string& text, bool& own) const
{
  text = "";
  own = false;
  MdTask *t = task();
  if (t == 0) return false;
  Session *session = t->getSession();
  bool first = true;
  while (t != 0) {
    vector<string> names = t->getDataFileNames((JCode::CodeFileType)fileType);
    if (!names.empty()) {
      own = first;
      text = names[0];
      if (!first) text += "   (from " + t->getName() + ")";
      return true;
    }
    first = false;
    t = (session == 0) ? 0
        : dynamic_cast<MdTask*>(t->getInputProvider(session));
  }
  return false;
}

void GromacsInputsPanel::refresh()
{
  string text;
  bool own = false;
  bool havetop = describe(TOPOLOGY, text, own);
  p_topology->SetLabel(wxString::FromUTF8((havetop ? text :
                       string("none attached")).c_str()));
  p_removeTop->Enable(havetop && own && p_editable);
  p_attachTop->Enable(p_editable);

  bool havegro = describe(STRUCTURE, text, own);
  p_structure->SetLabel(wxString::FromUTF8((havegro ? text :
                        string("none attached")).c_str()));
  p_removeGro->Enable(havegro && own && p_editable);
  p_attachGro->Enable(p_editable);

  p_includes->Clear();
  MdTask *t = task();
  if (t != 0) {
    Session *session = t->getSession();
    bool first = true;
    vector<string> seen;
    while (t != 0) {
      vector<string> names = t->getDataFileNames(JCode::AUXILIARY_INPUT);
      for (size_t i = 0; i < names.size(); i++) {
        if (std::find(seen.begin(), seen.end(), names[i]) != seen.end())
          continue;
        seen.push_back(names[i]);
        string label = names[i] + (first ? "" : "   (from " + t->getName() + ")");
        p_includes->Append(wxString::FromUTF8(label.c_str()),
                           new wxStringClientData(
                             wxString::FromUTF8((first ? "own" : "other"))));
      }
      first = false;
      t = (session == 0) ? 0
          : dynamic_cast<MdTask*>(t->getInputProvider(session));
    }
  }
  p_attachInc->Enable(p_editable);
  p_removeInc->Enable(false);

  string why;
  if (inputsReady(why)) {
    p_status->SetLabel(_("Ready: a topology and a starting structure are attached."));
  } else {
    p_status->SetLabel(wxString::FromUTF8(("Not ready to run: " + why).c_str()));
  }
  Layout();
}

bool GromacsInputsPanel::inputsReady(string& why)
{
  string text;
  bool own;
  bool top = describe(TOPOLOGY, text, own);
  bool gro = describe(STRUCTURE, text, own);
  why = "";
  if (!top && !gro) why = "no topology and no starting structure are attached.";
  else if (!top)    why = "no topology is attached.";
  else if (!gro)    why = "no starting structure is attached.";
  return top && gro;
}

void GromacsInputsPanel::report(const string& message, bool error)
{
  if (error) {
    ewxMessageDialog dlg(this, wxString::FromUTF8(message.c_str()),
                         "ECCE GROMACS inputs", wxOK | wxICON_ERROR);
    dlg.ShowModal();
  } else {
    p_status->SetLabel(wxString::FromUTF8(message.c_str()));
    Layout();
  }
}

bool GromacsInputsPanel::pickFile(const wxString& title,
                                  const wxString& wildcard, string& path)
{
  Preferences prefs("GromacsInputs");
  string dir;
  prefs.getString("GromacsInputs.Dir", dir);
  if (dir.empty()) dir = Ecce::realUserHome();

  ewxFileDialog dlg(this, title, dir, "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST,
                    wxDefaultPosition);
  if (dlg.ShowModal() != wxID_OK) return false;
  path = dlg.GetPath().ToStdString();
  prefs.setString("GromacsInputs.Dir", dlg.GetDirectory().ToStdString());
  prefs.saveFile();
  return true;
}

static bool readWhole(const string& path, string& contents, string& message)
{
  ifstream in(path.c_str(), std::ios::binary);
  if (!in) {
    message = "The file " + path + " could not be opened.";
    return false;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  contents = ss.str();
  if (contents.empty()) {
    message = "The file " + path + " is empty.";
    return false;
  }
  return true;
}

void GromacsInputsPanel::removeOwn(int fileType)
{
  MdTask *t = task();
  if (t == 0) return;
  vector<string> names = t->getDataFileNames((JCode::CodeFileType)fileType);
  for (size_t i = 0; i < names.size(); i++) {
    // A task's own files are in its input collection; those of an earlier
    // task are not listed here, because Remove is for this task's.
    t->putInputFile(names[i], 0);
  }
}

bool GromacsInputsPanel::attach(int fileType, const string& storedName,
                                const string& contents)
{
  MdTask *t = task();
  if (t == 0) return false;
  removeOwn(fileType);
  istringstream in(contents);
  return t->putInputFile(storedName, &in);
}

bool GromacsInputsPanel::attachFile(Kind kind, const string& path,
                                    string& message)
{
  string contents;
  if (!readWhole(path, contents, message)) return false;

  if (kind == TOPOLOGY_FILE) {
    if (contents.find("[") == string::npos &&
        contents.find("#include") == string::npos) {
      message = "This does not look like a GROMACS topology: it has no "
                "[ sections ] and includes no force field.";
      return false;
    }
    if (!attach(TOPOLOGY, extensionless(path) + ".gmxtop", contents)) {
      message = "The topology could not be stored.";
      return false;
    }
    message = "Attached " + tailOf(path) + " as the topology.";
  } else if (kind == STRUCTURE_FILE) {
    // Read here as well as by grompp: it is what lets the Builder show the
    // structure, and a file that is not a .gro is refused before it is
    // stored.
    string pdb, err;
    int natoms = GromacsStructure::groToPdb(contents, pdb, err);
    if (natoms < 0) {
      message = "This is not a usable .gro file: " + err;
      return false;
    }
    if (!attach(STRUCTURE, extensionless(path) + ".gro", contents)) {
      message = "The structure could not be stored.";
      return false;
    }
    MdTask *t = task();
    Fragment frag;
    istringstream pin(pdb);
    if (t != 0 && frag.restorePDB(pin, 1.0, true, 1, " ", true, false, " ")) {
      t->fragment(&frag);
    }
    char buf[200];
    snprintf(buf, sizeof(buf), "Attached %s: %d atoms.", tailOf(path).c_str(),
             natoms);
    message = buf;
  } else {
    MdTask *t = task();
    if (t == 0) {
      message = "The task could not be found.";
      return false;
    }
    string name = tailOf(path);
    istringstream in(contents);
    // The same name replaces: the topology refers to it by that name.
    t->putInputFile(name, 0);
    if (!t->putInputFile(name, &in)) {
      message = "The file could not be stored: " + t->messages();
      return false;
    }
    message = "Attached " + name + ".";
  }
  refresh();
  p_status->SetLabel(wxString::FromUTF8(message.c_str()));
  Layout();
  if (p_owner) p_owner->gromacsInputsChanged();
  return true;
}

void GromacsInputsPanel::OnAttachTopology(wxCommandEvent&)
{
  string path, message;
  if (pickFile(_("Attach a GROMACS topology"),
               _("GROMACS topologies (*.top;*.gmxtop)|*.top;*.gmxtop|All files|*"),
               path) &&
      !attachFile(TOPOLOGY_FILE, path, message))
    report(message, true);
}

void GromacsInputsPanel::OnAttachStructure(wxCommandEvent&)
{
  string path, message;
  if (pickFile(_("Attach a starting structure"),
               _("GROMACS structures (*.gro)|*.gro|All files|*"), path) &&
      !attachFile(STRUCTURE_FILE, path, message))
    report(message, true);
}

void GromacsInputsPanel::OnAttachInclude(wxCommandEvent&)
{
  string path, message;
  if (pickFile(_("Attach an include file"),
               _("GROMACS include files (*.itp;*.ndx)|*.itp;*.ndx|All files|*"),
               path) &&
      !attachFile(INCLUDE_FILE, path, message))
    report(message, true);
}

string GromacsInputsPanel::summary()
{
  string why;
  bool ready = inputsReady(why);
  string out = "topology: " + string(p_topology->GetLabel().ToUTF8()) +
               "; structure: " + string(p_structure->GetLabel().ToUTF8()) +
               "; includes: " + std::to_string(p_includes->GetCount()) +
               "; " + (ready ? string("Ready") : "Not ready: " + why);
  return out;
}

void GromacsInputsPanel::OnRemoveTopology(wxCommandEvent&)
{
  removeOwn(TOPOLOGY);
  refresh();
  if (p_owner) p_owner->gromacsInputsChanged();
}

void GromacsInputsPanel::OnRemoveStructure(wxCommandEvent&)
{
  removeOwn(STRUCTURE);
  refresh();
  if (p_owner) p_owner->gromacsInputsChanged();
}

void GromacsInputsPanel::OnIncludeSelected(wxCommandEvent&)
{
  int sel = p_includes->GetSelection();
  bool own = false;
  if (sel != wxNOT_FOUND) {
    wxStringClientData *d =
      dynamic_cast<wxStringClientData*>(p_includes->GetClientObject(sel));
    own = (d != 0 && d->GetData() == "own");
  }
  p_removeInc->Enable(own && p_editable);
}

void GromacsInputsPanel::OnRemoveInclude(wxCommandEvent&)
{
  int sel = p_includes->GetSelection();
  MdTask *t = task();
  if (sel == wxNOT_FOUND || t == 0) return;
  string label = p_includes->GetString(sel).ToStdString();
  size_t sp = label.find("   (from ");
  string name = (sp == string::npos) ? label : label.substr(0, sp);
  t->putInputFile(name, 0);
  refresh();
  if (p_owner) p_owner->gromacsInputsChanged();
}
