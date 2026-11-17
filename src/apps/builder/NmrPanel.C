#include <cmath>
#include <sstream>
#include <stdexcept>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/choicdlg.h>
#include <wx/dcmemory.h>
#include <wx/file.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/link.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/wrapsizer.h>
#include "dsm/EDSIFactory.H"
#include "dsm/IPropCalculation.H"
#include "dsm/Resource.H"
#include "tdat/Fragment.H"
#include "tdat/PropVector.H"
#include "tdat/TAtm.H"
#include "wxgui/ewxGrid.H"
#include "wxviz/WxVizToolFW.H"
#include "NmrPanel.H"

wxFORCE_LINK_THIS_MODULE(NmrPanel)
IMPLEMENT_DYNAMIC_CLASS(NmrPanel, VizPropertyPanel)

namespace {
NmrSpectrum readShielding(IPropCalculation *calc)
{
  auto *iso = dynamic_cast<PropVector*>(calc->getProperty("ISOSHIELD"));
  Fragment frag;
  if (!iso || !calc->getFragment(frag) || iso->rows() != int(frag.numAtoms()))
    throw std::invalid_argument("Shieldings do not match the stored structure.");
  std::vector<std::string> elements;
  std::vector<double> values;
  for (int i = 0; i < iso->rows(); ++i) {
    elements.push_back(frag.atomRef(i)->atomicSymbol());
    values.push_back(iso->value(i));
  }
  NmrSpectrum data;
  data.set(elements, values);
  return data;
}
}

bool NmrPanel::Create(IPropCalculation *calc, wxWindow *parent, wxWindowID id,
                      const wxPoint& pos, const wxSize& size, long style,
                      const wxString& name)
{
  if (!VizPropertyPanel::Create(calc, parent, id, pos, size, style, name)) return false;
  auto *col = new wxBoxSizer(wxVERTICAL);
  auto *row = new wxWrapSizer(wxHORIZONTAL);
  row->Add(new wxStaticText(this, wxID_ANY, "Nucleus:"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 3);
  p_element = new wxChoice(this, wxID_ANY);
  row->Add(p_element, 0, wxALL, 3);
  row->Add(new wxStaticText(this, wxID_ANY, "Reference shielding (ppm):"), 0,
           wxALL | wxALIGN_CENTER_VERTICAL, 3);
  p_reference = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(85, -1), wxTE_PROCESS_ENTER);
  p_reference->SetToolTip("Reference shielding for this element. Leave empty for no spectrum.\n"
                          "Use the same method, basis and solvent as this calculation.");
  row->Add(p_reference, 0, wxALL, 3);
  auto *reference = new wxButton(this, wxID_ANY, "Reference calculation...");
  row->Add(reference, 0, wxALL, 3);
  col->Add(row, 0, wxEXPAND);
  p_source = new wxStaticText(this, wxID_ANY, "Enter a reference shielding.",
                               wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
  col->Add(p_source, 0, wxEXPAND | wxALL, 3);
  p_canvas = new SpectrumCanvas(this);
  p_canvas->setClickHandler(this);
  p_canvas->setCompactBelow(0);
  p_canvas->setReversed(true);
  p_canvas->setFwhm(0);
  p_canvas->setLineShape(LINE_LORENTZIAN);
  col->Add(p_canvas, 3, wxEXPAND);
  row = new wxWrapSizer(wxHORIZONTAL);
  row->Add(new wxStaticText(this, wxID_ANY, "Lorentzian FWHM (ppm):"), 0,
           wxALL | wxALIGN_CENTER_VERTICAL, 3);
  p_width = new wxTextCtrl(this, wxID_ANY, "0", wxDefaultPosition, wxSize(65, -1), wxTE_PROCESS_ENTER);
  p_width->SetToolTip("0 shows sticks only. A positive width adds an area-normalised Lorentzian curve.");
  row->Add(p_width, 0, wxALL, 3);
  auto *reset = new wxButton(this, wxID_ANY, "Reset zoom");
  auto *csv = new wxButton(this, wxID_ANY, "Export CSV...");
  auto *png = new wxButton(this, wxID_ANY, "Export PNG...");
  for (auto *button : {reset, csv, png}) row->Add(button, 0, wxALL, 3);
  col->Add(row, 0, wxEXPAND);
  p_grid = new ewxGrid(this, wxID_ANY);
  p_grid->CreateGrid(0, 3, wxGrid::wxGridSelectRows);
  p_grid->SetColLabelValue(0, "Shift (ppm)");
  p_grid->SetColLabelValue(1, "Count");
  p_grid->SetColLabelValue(2, "Atoms (1-based)");
  p_grid->SetRowLabelSize(0);
  p_grid->EnableEditing(false);
  p_grid->SetMinSize(wxSize(200, 90));
  col->Add(p_grid, 1, wxEXPAND | wxALL, 3);
  SetSizer(col);
  p_element->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { showReference(); update(); });
  auto applyRef = [this]() {
    if (!setReference(p_reference->GetValue().ToStdString())) {
      showReference();
      p_source->SetLabel("Enter a finite reference shielding in ppm.");
    }
  };
  p_reference->Bind(wxEVT_TEXT_ENTER, [applyRef](wxCommandEvent&) { applyRef(); });
  p_reference->Bind(wxEVT_KILL_FOCUS, [applyRef](wxFocusEvent& e) { applyRef(); e.Skip(); });
  auto applyWidth = [this]() {
    double v;
    if (!p_width->GetValue().ToDouble(&v) || !setWidth(v))
      p_width->ChangeValue(wxString::Format("%g", p_fwhm));
  };
  p_width->Bind(wxEVT_TEXT_ENTER, [applyWidth](wxCommandEvent&) { applyWidth(); });
  p_width->Bind(wxEVT_KILL_FOCUS, [applyWidth](wxFocusEvent& e) { applyWidth(); e.Skip(); });
  reference->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { referenceDialog(); });
  reset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { p_canvas->resetZoom(); });
  csv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportDialog(false); });
  png->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportDialog(true); });
  p_grid->Bind(wxEVT_GRID_SELECT_CELL, [this](wxGridEvent& e) {
    if (!p_filling) modeClicked(e.GetRow());
    e.Skip();
  });
  return true;
}

std::string NmrPanel::element() const { return p_element->GetStringSelection().ToStdString(); }

void NmrPanel::refresh()
{
  if (!p_canvas) return;
  const std::string previous = element();
  p_error.clear();
  try { p_data = readShielding(getCalculation()); }
  catch (const std::exception& e) { p_data = NmrSpectrum(); p_error = e.what(); }
  p_element->Clear();
  for (const auto& e : p_data.elements())
    p_element->Append(e);
  if (!p_element->SetStringSelection(previous) && p_element->GetCount()) p_element->SetSelection(0);
  showReference();
  update();
}

bool NmrPanel::chooseElement(const std::string& value)
{
  if (!p_element->SetStringSelection(value)) return false;
  showReference();
  update();
  return true;
}

void NmrPanel::showReference()
{
  auto at = p_references.find(element());
  p_reference->ChangeValue(at == p_references.end() ? wxString() : wxString::Format("%.12g", at->second.value));
  p_source->SetLabel(at == p_references.end() ? "Enter a reference shielding." :
                     wxString::FromUTF8(at->second.source.c_str()));
  p_source->SetToolTip(p_source->GetLabel());
}

bool NmrPanel::setReference(const std::string& text)
{
  wxString value = wxString::FromUTF8(text.c_str());
  value.Trim().Trim(false);
  auto at = p_references.find(element());
  if (value.empty()) p_references.erase(element());
  else {
    double v;
    if (!value.ToDouble(&v) || !std::isfinite(v)) return false;
    // Losing focus must not replace a calculation's provenance with "Entered".
    if (at == p_references.end() || std::fabs(at->second.value - v) > 1e-9)
      p_references[element()] = {v, "Entered reference shielding"};
  }
  showReference();
  update();
  return true;
}

bool NmrPanel::setWidth(double ppm)
{
  if (!std::isfinite(ppm) || ppm < 0) return false;
  p_fwhm = ppm;
  p_width->ChangeValue(wxString::Format("%g", ppm));
  p_canvas->setFwhm(ppm);
  return true;
}

void NmrPanel::update()
{
  p_filling = true;
  auto at = p_references.find(element());
  std::optional<double> ref;
  if (at != p_references.end()) ref = at->second.value;
  p_groups = ref ? p_data.groups(element(), *ref) : std::vector<NmrGroup>();
  VibSpectrum spec = p_data.spectrum(element(), ref);
  spec.setLabel(element() + " NMR" + (ref ? " | reference " +
      std::to_string(*ref) + " ppm | " + at->second.source : ""));
  p_canvas->setSpectrum(spec);
  if (p_grid->GetNumberRows()) p_grid->DeleteRows(0, p_grid->GetNumberRows());
  if (!p_groups.empty()) p_grid->AppendRows(p_groups.size());
  for (size_t i = 0; i < p_groups.size(); ++i) {
    const auto& g = p_groups[i];
    p_grid->SetCellValue(i, 0, wxString::Format("%.4f", g.shift));
    p_grid->SetCellValue(i, 1, wxString::Format("%d", int(g.atoms.size())));
    wxString atoms;
    for (int a : g.atoms) atoms += wxString::Format("%s%d", atoms.empty() ? "" : " ", a + 1);
    p_grid->SetCellValue(i, 2, atoms);
  }
  p_grid->AutoSizeColumns();
  if (!p_error.empty()) p_source->SetLabel(wxString::FromUTF8(p_error.c_str()));
  p_filling = false;
  Layout();
}

void NmrPanel::modeClicked(int group)
{
  if (group < 0 || group >= int(p_groups.size())) return;
  SGFragment *frag = getFW().getSceneGraph().getFragment();
  Fragment stored;
  if (!frag || !getCalculation()->getFragment(stored) || frag->numAtoms() != stored.numAtoms()) return;
  for (size_t i = 0; i < stored.numAtoms(); ++i)
    if (stored.atomRef(i)->atomicSymbol() != frag->atomRef(i)->atomicSymbol()) return;
  p_canvas->setSelected(group);
  p_filling = true;
  p_grid->SelectRow(group);
  p_filling = false;
  selectCommand(p_groups[group].atoms, std::vector<int>());
}

std::vector<IPropCalculation*> NmrPanel::referenceCalculations() const
{
  std::vector<IPropCalculation*> out;
  Resource *folder = EDSIFactory::getResource(getCalculation()->getURL().getParent());
  auto *kids = folder ? folder->getChildren() : nullptr;
  if (kids) for (auto *kid : *kids) {
    auto *calc = dynamic_cast<IPropCalculation*>(kid);
    if (!calc || calc->getURL() == getCalculation()->getURL()) continue;
    try {
      if (readShielding(calc).meanShielding(element())) out.push_back(calc);
    } catch (const std::exception&) { }
  }
  return out;
}

bool NmrPanel::referenceCalculation(const std::string& name)
{
  for (auto *calc : referenceCalculations()) {
    if (calc->getURL().getFilePathTail() != name) continue;
    const auto mean = readShielding(calc).meanShielding(element());
    if (!mean) return false;
    p_references[element()] = {*mean, "Mean shielding: " + name};
    showReference();
    update();
    return true;
  }
  return false;
}

void NmrPanel::referenceDialog()
{
  const auto calcs = referenceCalculations();
  wxArrayString names;
  for (auto *calc : calcs) names.Add(wxString::FromUTF8(calc->getURL().getFilePathTail().c_str()));
  if (names.empty()) {
    wxMessageBox("No other calculation in this project folder has shielding data for this element.",
                 "Reference calculation", wxOK | wxICON_INFORMATION, this);
    return;
  }
  int at = wxGetSingleChoiceIndex("Use the mean shielding of this element.\n"
      "Choose a reference at the same method, basis and solvent.", "Reference calculation", names, this);
  if (at >= 0) referenceCalculation(names[at].ToStdString());
}

bool NmrPanel::exportCsv(const wxString& path) const
{
  auto at = p_references.find(element());
  if (at == p_references.end() || p_groups.empty()) return false;
  wxFile file(path, wxFile::write);
  const auto text = "# calculation: " + getCalculation()->getURL().getFilePathTail() + "\n" +
      p_data.csv(element(), at->second.value, at->second.source, p_fwhm);
  return file.IsOpened() && file.Write(text.data(), text.size()) == text.size() && file.Close();
}

bool NmrPanel::exportPng(const wxString& path)
{
  if (p_groups.empty()) return false;
  wxBitmap bitmap(1200, 760, 24);
  wxMemoryDC dc(bitmap);
  dc.SetBackground(wxBrush(*wxWHITE));
  dc.Clear();
  dc.SetTextForeground(*wxBLACK);
  dc.SetFont(*wxNORMAL_FONT);
  const auto ref = p_references.find(element());
  const std::string heading = getCalculation()->getURL().getFilePathTail() + " | " +
      element() + " NMR | reference " + std::to_string(ref->second.value) + " ppm | " + ref->second.source;
  dc.SetDeviceOrigin(0, 35);
  p_canvas->paintOnto(dc, wxSize(1200, 725));
  dc.SetDeviceOrigin(0, 0);
  dc.SetTextForeground(*wxBLACK);
  dc.SetFont(*wxNORMAL_FONT);
  dc.DrawText(wxString::FromUTF8(heading.c_str()), 12, 8);
  dc.SelectObject(wxNullBitmap);
  return bitmap.SaveFile(path, wxBITMAP_TYPE_PNG);
}

void NmrPanel::exportDialog(bool png)
{
  wxFileDialog dlg(this, "Export NMR spectrum", "", png ? "nmr.png" : "nmr.csv",
                    png ? "PNG files (*.png)|*.png" : "CSV files (*.csv)|*.csv",
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() == wxID_OK && !(png ? exportPng(dlg.GetPath()) : exportCsv(dlg.GetPath())))
    wxMessageBox("Supply a reference and check that the file can be written.", "Export NMR spectrum",
                 wxOK | wxICON_ERROR, this);
}

bool NmrPanel::clickGroup(int group)
{
  wxBitmap bitmap(1000, 650, 24);
  wxMemoryDC dc(bitmap);
  p_canvas->paintOnto(dc, wxSize(1000, 650));
  wxPoint top;
  bool ok = p_canvas->stickPosition(VIB_IR, group, &top) && p_canvas->clickAt(wxPoint(top.x, top.y + 2));
  dc.SelectObject(wxNullBitmap);
  return ok;
}

std::string NmrPanel::summary() const
{
  std::ostringstream out;
  out << "element=" << element() << " groups=" << p_groups.size()
      << " reversed=" << p_canvas->axis().reversed() << " width=" << p_fwhm;
  auto at = p_references.find(element());
  if (at != p_references.end()) out << " reference=" << at->second.value;
  for (const auto& g : p_groups) {
    out << "\nNMR group shift=" << g.shift << " count=" << g.atoms.size() << " atoms=";
    for (int a : g.atoms) out << a + 1 << ' ';
  }
  return out.str();
}
