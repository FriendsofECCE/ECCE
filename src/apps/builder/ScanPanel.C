#include <algorithm>
#include <cstdio>
#include <locale>
#include <sstream>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "tdat/PropVector.H"
#include "tdat/TProperty.H"
#include "dsm/IPropCalculation.H"
#include "wxgui/ewxPlotCtrl.H"

#include "ScanPanel.H"

namespace {
const double HARTREE_KJ = 2625.4996;
const double HARTREE_KCAL = 627.5095;
const wxWindowID ID_UNIT = wxNewId();
const wxWindowID ID_EXPORT = wxNewId();

double factor(bool kcal) { return kcal ? HARTREE_KCAL : HARTREE_KJ; }
}

std::string scanCsv(const std::vector<double>& coord,
                    const std::vector<double>& hartree,
                    const std::string& coordUnit, bool kcal,
                    const std::string& calculation)
{
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out.precision(10);
  out << "# ECCE relaxed scan\n";
  out << "# calculation: " << calculation << "\n";
  out << "# coordinate unit: " << coordUnit << "\n";
  out << "# relative energy is to the lowest scan point\n";
  out << "point,coordinate_" << coordUnit << ",energy_hartree,relative_energy_"
      << (kcal ? "kcal/mol" : "kJ/mol") << "\n";
  const size_t n = std::min(coord.size(), hartree.size());
  const double lo = n ? *std::min_element(hartree.begin(), hartree.begin() + n) : 0;
  for (size_t i = 0; i < n; i++)
    out << i + 1 << "," << coord[i] << "," << hartree[i] << ","
        << (hartree[i] - lo) * factor(kcal) << "\n";
  return out.str();
}

IMPLEMENT_DYNAMIC_CLASS(ScanPanel, PropertyPanel)

ScanPanel::ScanPanel()
  : PropertyPanel(), p_unit(NULL), p_plot(NULL), p_grid(NULL)
{
}

ScanPanel::ScanPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(), p_unit(NULL), p_plot(NULL), p_grid(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}

ScanPanel::~ScanPanel()
{
}

bool ScanPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("ScanPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));

  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  bar->Add(new wxStaticText(this, wxID_ANY, "Energy unit:"), 0,
           wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  p_unit = new wxChoice(this, ID_UNIT);
  p_unit->Append("kJ/mol");
  p_unit->Append("kcal/mol");
  p_unit->SetSelection(0);
  bar->Add(p_unit, 0, wxRIGHT, 12);
  bar->AddStretchSpacer();
  bar->Add(new wxButton(this, ID_EXPORT, "Export .csv..."), 0);
  GetSizer()->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_plot = new ewxPlotCtrl(this, wxID_ANY);
  p_plot->SetMinSize(wxSize(300, 180));
  GetSizer()->Add(p_plot, 3, wxEXPAND);

  p_grid = new wxGrid(this, wxID_ANY, wxDefaultPosition, wxSize(300, 110));
  p_grid->CreateGrid(0, 3, wxGrid::wxGridSelectRows);
  p_grid->EnableEditing(false);
  p_grid->SetRowLabelSize(40);
  p_grid->SetColLabelValue(0, "Coordinate");
  p_grid->SetColLabelValue(1, "Energy (Eh)");
  p_grid->SetColLabelValue(2, "Relative");
  GetSizer()->Add(p_grid, 2, wxEXPAND | wxALL, 3);

  Bind(wxEVT_CHOICE, &ScanPanel::OnUnit, this, ID_UNIT);
  Bind(wxEVT_BUTTON, &ScanPanel::OnExport, this, ID_EXPORT);

  fill();
  return true;
}

void ScanPanel::fill()
{
  p_coord.clear();
  p_hartree.clear();
  IPropCalculation *calc = getCalculation();
  PropVector *c = calc ? dynamic_cast<PropVector*>(calc->getProperty("SCANCOORD")) : 0;
  PropVector *e = calc ? dynamic_cast<PropVector*>(calc->getProperty("SCANENERGY")) : 0;
  if (!c || !e) return;
  const size_t n = std::min(c->values().size(), e->values().size());
  p_coord.assign(c->values().begin(), c->values().begin() + n);
  p_hartree.assign(e->values().begin(), e->values().begin() + n);
  p_coordUnit = c->units() == "Degree" ? "degree" : "angstrom";

  const bool kcal = p_unit->GetSelection() == 1;
  const wxString unit = kcal ? "kcal/mol" : "kJ/mol";
  const double lo = n ? *std::min_element(p_hartree.begin(), p_hartree.end()) : 0;

  p_plot->DeleteCurve(-1, false);
  if (n) {
    double *x = new double[n], *y = new double[n];
    for (size_t i = 0; i < n; i++) {
      x[i] = p_coord[i];
      y[i] = (p_hartree[i] - lo) * factor(kcal);
    }
    wxPlotData data(x, y, n);
    p_plot->AddCurve(data, true, true);
  }
  p_plot->SetXAxisLabel(p_coordUnit == "degree" ? "Scanned coordinate (degrees)"
                                                 : "Scanned coordinate (Angstrom)");
  p_plot->SetYAxisLabel("Relative energy (" + unit + ")");
  p_plot->SetShowXAxisLabel(true);
  p_plot->SetShowYAxisLabel(true);

  if (p_grid->GetNumberRows()) p_grid->DeleteRows(0, p_grid->GetNumberRows());
  p_grid->AppendRows((int)n);
  p_grid->SetColLabelValue(2, unit);
  for (size_t i = 0; i < n; i++) {
    p_grid->SetRowLabelValue((int)i, wxString::Format("%d", (int)i + 1));
    p_grid->SetCellValue((int)i, 0, wxString::Format("%.4f", p_coord[i]));
    p_grid->SetCellValue((int)i, 1, wxString::Format("%.6f", p_hartree[i]));
    p_grid->SetCellValue((int)i, 2,
        wxString::Format("%.3f", (p_hartree[i] - lo) * factor(kcal)));
  }
  p_grid->AutoSizeColumns();
}

void ScanPanel::OnUnit(wxCommandEvent&)
{
  fill();
}

bool ScanPanel::exportCsv(const wxString& path, bool kcal)
{
  IPropCalculation *calc = getCalculation();
  const std::string text = scanCsv(p_coord, p_hartree, p_coordUnit, kcal,
      calc ? calc->getURL().getFilePathTail() : std::string());
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}

void ScanPanel::OnExport(wxCommandEvent&)
{
  IPropCalculation *calc = getCalculation();
  wxFileDialog dlg(this, "Export scan", wxEmptyString,
                   wxString::FromUTF8(calc ? calc->getURL().getFilePathTail().c_str() : "scan") +
                   "-scan.csv", "CSV files (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath(), p_unit->GetSelection() == 1))
    wxMessageBox("The file could not be written.", "Export scan",
                 wxOK | wxICON_ERROR, this);
}
