#include <cstdio>
#include <set>

#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textdlg.h>

#include "util/InvalidException.H"

#include "tdat/Fragment.H"
#include "tdat/GeomConstraint.H"
#include "tdat/GeomConstraintModel.H"
#include "tdat/GeomConstraintPick.H"
#include "tdat/GeomRestraint.H"

#include "Builder.H"
#include "GeomConstraintsDialog.H"
#include <algorithm>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>

using std::string;
using std::vector;

namespace {

enum { ID_CONSTRAIN = wxID_HIGHEST + 700, ID_RESTRAIN, ID_FIX, ID_EDIT,
       ID_REMOVE, ID_CLEAR, ID_LIST, ID_SCAN, ID_SAVE };

const char *DASH = "\xE2\x80\x93";

string plural(int n, const char *one, const char *many)
{
  char buf[64];
  snprintf(buf, sizeof buf, "%d %s", n, n == 1 ? one : many);
  return buf;
}

// "Angle H2-N1-H3", "Fixed atom N1"
string kindOf(const GeomConstraint *c)
{
  const bool restraint = dynamic_cast<const GeomRestraint*>(c) != 0;
  const char *k[] = {"", "Fixed atom", "Bond", "Angle", "Torsion"};
  string s = k[c->getNumAtoms()];
  if (restraint) s += " (restraint)";
  return s;
}

string atomsOf(const Fragment& frag, const GeomConstraint *c)
{
  vector<TAtm*> *atoms = c->getAtoms();
  string s;
  for (size_t i = 0; i < atoms->size(); i++) {
    if (i) s += DASH;
    s += constraintAtomName(frag, (*atoms)[i]->index());
  }
  if (atoms->size() == 3)
    s += " (centre " + constraintAtomName(frag, (*atoms)[1]->index()) + ")";
  delete atoms;
  return s;
}

string valueOf(const GeomConstraint *c)
{
  char buf[64];
  if (c->getNumAtoms() == 1) return "";
  const bool length = c->getNumAtoms() == 2;
  if (length)
    snprintf(buf, sizeof buf, "%.4f \xC3\x85", c->getConstraintValue());
  else
    snprintf(buf, sizeof buf, "%.2f\xC2\xB0", c->getConstraintValue());
  string s = buf;
  if (c->getScanSteps() > 0) {
    snprintf(buf, sizeof buf, ", scan %d \xC3\x97 %g%s", c->getScanSteps(),
             c->getScanStep(), length ? " \xC3\x85" : "\xC2\xB0");
    s += buf;
  }
  return s;
}

string springOf(const GeomConstraint *c)
{
  const GeomRestraint *r = dynamic_cast<const GeomRestraint*>(c);
  if (!r) return "";
  char buf[64];
  snprintf(buf, sizeof buf, "%g", r->getRestraintValue());
  return buf;
}

}


GeomConstraintsDialog::GeomConstraintsDialog(Builder *builder)
  : wxDialog(builder, wxID_ANY, "Geometry Constraints", wxDefaultPosition,
             wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_builder(builder)
{
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  const int pad = 8;

  wxStaticText *help = new wxStaticText(this, wxID_ANY, wxString::FromUTF8(
      "Select 2, 3 or 4 atoms in the viewer (Shift+click adds one), then "
      "press a button. Constrain holds the value; Restrain pulls towards it; "
      "Scan steps it, relaxing the rest; Fix Atoms holds atoms in place."));
  help->Wrap(440);
  top->Add(help, 0, wxALL | wxEXPAND, pad);

  p_pick = new wxStaticText(this, wxID_ANY, "");
  p_pick->SetFont(p_pick->GetFont().Bold());
  top->Add(p_pick, 0, wxLEFT | wxRIGHT | wxEXPAND, pad);

  wxBoxSizer *act = new wxBoxSizer(wxHORIZONTAL);
  p_constrain = new wxButton(this, ID_CONSTRAIN, "Constrain");
  p_restrain = new wxButton(this, ID_RESTRAIN, "Restrain");
  p_scan = new wxButton(this, ID_SCAN, "Scan...");
  p_fix = new wxButton(this, ID_FIX, "Fix Atoms");
  act->Add(p_constrain, 0, wxRIGHT, 4);
  act->Add(p_restrain, 0, wxRIGHT, 4);
  act->Add(p_scan, 0, wxRIGHT, 4);
  act->Add(p_fix, 0);
  top->Add(act, 0, wxALL, pad);

  p_status = new wxStaticText(this, wxID_ANY, "");
  top->Add(p_status, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, pad);

  p_list = new wxListView(this, ID_LIST, wxDefaultPosition, wxSize(440, 150),
                          wxLC_REPORT | wxBORDER_SUNKEN);
  p_list->AppendColumn("Constraint", wxLIST_FORMAT_LEFT, 110);
  p_list->AppendColumn("Atoms", wxLIST_FORMAT_LEFT, 170);
  p_list->AppendColumn("Value", wxLIST_FORMAT_LEFT, 80);
  p_list->AppendColumn("Spring", wxLIST_FORMAT_LEFT, 70);
  top->Add(p_list, 1, wxLEFT | wxRIGHT | wxEXPAND, pad);

  wxBoxSizer *edit = new wxBoxSizer(wxHORIZONTAL);
  p_edit = new wxButton(this, ID_EDIT, "Edit Value...");
  p_remove = new wxButton(this, ID_REMOVE, "Remove");
  p_clear = new wxButton(this, ID_CLEAR, "Clear All");
  edit->Add(p_edit, 0, wxRIGHT, 4);
  edit->Add(p_remove, 0, wxRIGHT, 4);
  edit->Add(p_clear, 0);
  top->Add(edit, 0, wxALL, pad);

  p_note = new wxStaticText(this, wxID_ANY, "");
  top->Add(p_note, 0, wxLEFT | wxRIGHT | wxEXPAND, pad);

  wxBoxSizer *bottom = new wxBoxSizer(wxHORIZONTAL);
  p_save = new wxButton(this, ID_SAVE, "Save");
  p_save->SetToolTip("Save the structure with its constraints");
  bottom->AddStretchSpacer(1);
  bottom->Add(p_save, 0, wxRIGHT, 4);
  bottom->Add(new wxButton(this, wxID_CLOSE, "Close"), 0);
  top->Add(bottom, 0, wxALL | wxEXPAND, pad);

  SetSizerAndFit(top);

  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnConstrain, this, ID_CONSTRAIN);
  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnRestrain, this, ID_RESTRAIN);
  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnFix, this, ID_FIX);
  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnScan, this, ID_SCAN);
  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnSave, this, ID_SAVE);
  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnEdit, this, ID_EDIT);
  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnRemove, this, ID_REMOVE);
  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnClearAll, this, ID_CLEAR);
  Bind(wxEVT_BUTTON, &GeomConstraintsDialog::OnClose, this, wxID_CLOSE);
  Bind(wxEVT_LIST_ITEM_ACTIVATED, &GeomConstraintsDialog::OnListActivated,
       this, ID_LIST);
  Bind(wxEVT_LIST_ITEM_SELECTED, &GeomConstraintsDialog::OnListSelected,
       this, ID_LIST);
  Bind(wxEVT_LIST_ITEM_DESELECTED, &GeomConstraintsDialog::OnListSelected,
       this, ID_LIST);
  Bind(wxEVT_CLOSE_WINDOW, &GeomConstraintsDialog::OnCloseWindow, this);

  refresh();
}


void GeomConstraintsDialog::refresh()
{
  GeomConstraintModel *model = p_builder->constraintModel(true);
  if (model) model->cleanUnmatchedAtoms();

  const vector<int> sel = p_builder->constraintSelection();
  const size_t n = sel.size();
  string text;
  if (!model) {
    text = "There is no structure to constrain.";
  } else if (n == 0) {
    text = "No atoms selected. Click an atom in the viewer.";
  } else if (n == 1) {
    text = "Selected " + constraintAtomName(model->getFragment(), sel[0]) +
           ". Add a second atom for a bond, a third for an angle, a fourth "
           "for a torsion, or press Fix Atoms.";
  } else if (n <= 4) {
    GeomConstraintPick p = orderConstraintAtoms(model->getFragment(), sel);
    text = "Press Constrain to add: " + p.description;
    if (p.needsConfirm) text += "\n" + p.warning + " You will be asked to confirm.";
  } else {
    text = plural((int)n, "atom", "atoms") + " selected. A bond, angle or "
           "torsion needs 2, 3 or 4 atoms; Fix Atoms accepts any number.";
  }
  p_pickText = text;
  p_pick->SetLabel(wxString::FromUTF8(text));
  p_pick->Wrap(440);

  fillList();
  updateButtons();
  p_note->SetLabel(wxString::FromUTF8(noteText()));
  p_note->Wrap(440);
  Layout();
  Fit();
}


string GeomConstraintsDialog::noteText() const
{
  GeomConstraintModel *model = p_builder->constraintModel(false);
  const int n = model ? model->size() : 0;
  if (n == 0) return "No constraints defined.";

  string s = plural(n, "constraint", "constraints") + " defined";
  const string code = p_builder->constraintCodeName();
  const string runtype = p_builder->constraintRunType();
  if (!code.empty() && !p_builder->constraintsSupportedByCode()) {
    s += ": " + code + " does not use geometry constraints; they are kept "
         "with the structure.";
  } else if (runtype.empty()) {
    s += ": they are saved with the calculation.";
  } else if (GeomConstraintModel::useBy(code, runtype) ==
             GeomConstraintModel::USED) {
    s += ": used by this calculation (run type " + runtype + ").";
  } else {
    s += ": " + GeomConstraintModel::notUsedText(code, runtype) +
         ". They are kept with the structure.";
  }
  return s;
}


vector<string> GeomConstraintsDialog::rowTexts() const
{
  vector<string> rows;
  GeomConstraintModel *model = p_builder->constraintModel(false);
  if (!model) return rows;
  const Fragment& frag = model->getFragment();
  for (size_t i = 0; i < p_rows.size(); i++)
    rows.push_back(kindOf(p_rows[i]) + " | " + atomsOf(frag, p_rows[i]) +
                   " | " + valueOf(p_rows[i]) + " | " + springOf(p_rows[i]));
  return rows;
}


void GeomConstraintsDialog::fillList()
{
  p_list->DeleteAllItems();
  p_rows.clear();
  GeomConstraintModel *model = p_builder->constraintModel(false);
  if (!model) return;
  const Fragment& frag = model->getFragment();

  vector<GeomConstraint*> *groups[4] = {
      model->getAtomConstraints(), model->getBondConstraints(),
      model->getAngleConstraints(), model->getTorsionConstraints()};
  for (int g = 0; g < 4; g++) {
    for (size_t i = 0; i < groups[g]->size(); i++) {
      GeomConstraint *c = (*groups[g])[i];
      const long row = p_list->GetItemCount();
      p_list->InsertItem(row, wxString::FromUTF8(kindOf(c)));
      p_list->SetItem(row, 1, wxString::FromUTF8(atomsOf(frag, c)));
      p_list->SetItem(row, 2, wxString::FromUTF8(valueOf(c)));
      p_list->SetItem(row, 3, wxString::FromUTF8(springOf(c)));
      p_rows.push_back(c);
    }
    delete groups[g];
  }
}


void GeomConstraintsDialog::updateButtons()
{
  const bool ro = p_builder->constraintsReadOnly();
  const size_t n = p_builder->constraintSelection().size();
  p_constrain->Enable(!ro && n >= 2 && n <= 4);
  p_restrain->Enable(!ro && n >= 2 && n <= 4);
  p_scan->Enable(!ro && n >= 2 && n <= 4);
  p_fix->Enable(!ro && n >= 1);
  p_save->Enable(!ro && p_builder->constraintsUnsaved());
  const bool picked = p_list->GetSelectedItemCount() > 0;
  p_edit->Enable(!ro && p_list->GetSelectedItemCount() == 1);
  p_remove->Enable(!ro && picked);
  p_clear->Enable(!ro && !p_rows.empty());
}


void GeomConstraintsDialog::done(const string& message)
{
  p_builder->constraintsEdited();
  p_status->SetLabel(wxString::FromUTF8(message));
  p_status->Wrap(440);
  refresh();
}


string GeomConstraintsDialog::addFromSelection(bool restraint, bool ask,
                                               bool confirm)
{
  GeomConstraintModel *model = p_builder->constraintModel(true);
  if (model) model->cleanUnmatchedAtoms();   // atoms may have been edited away
  const vector<int> sel = p_builder->constraintSelection();
  if (!model || sel.size() < 2 || sel.size() > 4)
    return "Select 2, 3 or 4 atoms in the viewer first.";

  GeomConstraintPick p = orderConstraintAtoms(model->getFragment(), sel);
  if (p.needsConfirm) {
    if (ask) {
      wxMessageDialog q(this, wxString::FromUTF8(p.warning + "\n\nAdd " +
                        p.description + " anyway?"), "Check the atoms",
                        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
      confirm = q.ShowModal() == wxID_YES;
    }
    if (!confirm) return "Not added: " + p.warning;
  }

  try {
    const bool duplicate = restraint ? model->addRestraint(p.atoms)
                                     : model->addConstraint(p.atoms);
    if (duplicate) return p.description + " is already defined.";
  } catch (InvalidException& ex) {
    return string("Not added: ") + ex.what();
  }
  return "Added " + p.description + (restraint ? " as a restraint." : ".");
}


string GeomConstraintsDialog::fixSelection()
{
  GeomConstraintModel *model = p_builder->constraintModel(true);
  if (model) model->cleanUnmatchedAtoms();
  const vector<int> sel = p_builder->constraintSelection();
  if (!model || sel.empty()) return "Select the atoms to fix in the viewer first.";
  try {
    const bool duplicate = model->fixAtoms(sel);
    string names;
    for (size_t i = 0; i < sel.size(); i++)
      names += (i ? ", " : "") + constraintAtomName(model->getFragment(), sel[i]);
    return "Fixed " + names + (duplicate ? " (some were already fixed)." : ".");
  } catch (InvalidException& ex) {
    return string("Not fixed: ") + ex.what();
  }
}


string GeomConstraintsDialog::press(const string& what, bool confirm)
{
  string msg;
  if (what == "fix") msg = fixSelection();
  else msg = addFromSelection(what == "restrain", false, confirm);
  done(msg);
  return msg;
}


void GeomConstraintsDialog::OnConstrain(wxCommandEvent&)
{
  done(addFromSelection(false, true, false));
}

void GeomConstraintsDialog::OnRestrain(wxCommandEvent&)
{
  done(addFromSelection(true, true, false));
}

void GeomConstraintsDialog::OnFix(wxCommandEvent&)
{
  done(fixSelection());
}

void GeomConstraintsDialog::OnScan(wxCommandEvent&)
{
  const vector<GeomConstraint*> before = p_rows;
  done(addFromSelection(false, true, false));
  for (size_t i = 0; i < p_rows.size(); i++)
    if (std::find(before.begin(), before.end(), p_rows[i]) == before.end()) {
      editRow((long)i, true);
      return;
    }
}

void GeomConstraintsDialog::OnSave(wxCommandEvent&)
{
  p_builder->saveFromConstraints();
  p_status->SetLabel("Saved.");
  updateButtons();
}


void GeomConstraintsDialog::OnRemove(wxCommandEvent&)
{
  GeomConstraintModel *model = p_builder->constraintModel(false);
  if (!model) return;
  vector<GeomConstraint*> doomed;
  for (long i = p_list->GetFirstSelected(); i != -1;
       i = p_list->GetNextSelected(i))
    if (i < (long)p_rows.size()) doomed.push_back(p_rows[i]);
  for (size_t i = 0; i < doomed.size(); i++) model->remove(doomed[i]);
  done(plural((int)doomed.size(), "constraint", "constraints") + " removed.");
}


void GeomConstraintsDialog::OnClearAll(wxCommandEvent&)
{
  GeomConstraintModel *model = p_builder->constraintModel(false);
  if (!model || model->size() == 0) return;
  wxMessageDialog q(this, "Remove all constraints from this structure?",
                    "Clear All", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
  if (q.ShowModal() != wxID_YES) return;
  model->clear();
  done("All constraints removed.");
}


void GeomConstraintsDialog::editRow(long row, bool scanOn)
{
  if (row < 0 || row >= (long)p_rows.size()) return;
  GeomConstraint *c = p_rows[row];
  if (c->getNumAtoms() == 1) return;
  GeomRestraint *r = dynamic_cast<GeomRestraint*>(c);

  const bool length = c->getNumAtoms() == 2;
  const wxString unit = length ? "Angstrom" : "degrees";
  wxDialog d(this, wxID_ANY, "Edit Value");
  wxBoxSizer *top = new wxBoxSizer(wxVERTICAL);
  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 5, 8);
  wxTextCtrl *val = new wxTextCtrl(&d, wxID_ANY,
                                   wxString::Format("%g", c->getConstraintValue()));
  grid->Add(new wxStaticText(&d, wxID_ANY, length ? "Bond length (Angstrom):"
                                          : "Angle or torsion (degrees):"),
            0, wxALIGN_CENTER_VERTICAL);
  grid->Add(val, 0, wxEXPAND);
  wxCheckBox *scan = nullptr;
  wxSpinCtrl *steps = nullptr;
  wxTextCtrl *step = nullptr;
  if (!r) {
    scan = new wxCheckBox(&d, wxID_ANY, "Scan: step this value, relaxing the rest");
    scan->SetValue(scanOn || c->getScanSteps() > 0);
    steps = new wxSpinCtrl(&d, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 1, 500,
                           c->getScanSteps() > 0 ? c->getScanSteps() : 10);
    step = new wxTextCtrl(&d, wxID_ANY,
                          wxString::Format("%g", c->getScanStep() != 0.0
                                           ? c->getScanStep() : (length ? 0.1 : 10.0)));
    grid->Add(scan, 0, wxALIGN_CENTER_VERTICAL);
    grid->AddSpacer(0);
    grid->Add(new wxStaticText(&d, wxID_ANY, "Steps:"), 0, wxALIGN_CENTER_VERTICAL);
    grid->Add(steps, 0, wxEXPAND);
    grid->Add(new wxStaticText(&d, wxID_ANY, "Step size (" + unit + "):"),
              0, wxALIGN_CENTER_VERTICAL);
    grid->Add(step, 0, wxEXPAND);
    auto enable = [=](wxCommandEvent&) {
      steps->Enable(scan->GetValue());
      step->Enable(scan->GetValue());
    };
    scan->Bind(wxEVT_CHECKBOX, enable);
    steps->Enable(scan->GetValue());
    step->Enable(scan->GetValue());
  }
  top->Add(grid, 0, wxALL | wxEXPAND, 10);
  top->Add(d.CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALL | wxEXPAND, 10);
  d.SetSizerAndFit(top);
  if (d.ShowModal() != wxID_OK) return;
  double v;
  if (!val->GetValue().ToDouble(&v)) return;
  c->setConstraintValue(v);
  if (scan) {
    double sv = 0.0;
    if (scan->GetValue() && step->GetValue().ToDouble(&sv) && sv != 0.0)
      c->setScan(steps->GetValue(), sv);
    else
      c->setScan(0, 0.0);
  }

  if (r) {
    wxTextEntryDialog k(this, "Spring constant:", "Edit Value",
                        wxString::Format("%g", r->getRestraintValue()));
    double kv;
    if (k.ShowModal() == wxID_OK && k.GetValue().ToDouble(&kv))
      r->setRestraintValue(kv);
  }
  done("Value changed.");
}

void GeomConstraintsDialog::OnEdit(wxCommandEvent&)
{
  editRow(p_list->GetFirstSelected());
}

void GeomConstraintsDialog::OnListActivated(wxListEvent& e)
{
  if (!p_builder->constraintsReadOnly()) editRow(e.GetIndex());
}

void GeomConstraintsDialog::OnListSelected(wxListEvent&)
{
  updateButtons();
}

void GeomConstraintsDialog::OnClose(wxCommandEvent&)
{
  Show(false);
}

void GeomConstraintsDialog::OnCloseWindow(wxCloseEvent&)
{
  Show(false);
}
