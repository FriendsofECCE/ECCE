#include <cstdio>

#include <wx/button.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "tdat/PropVector.H"
#include "dsm/IPropCalculation.H"
#include "XYPlotCanvas.H"

#include "PullPanel.H"

namespace {
const wxWindowID ID_EXPORT = wxNewId();

const std::vector<double> *valuesOf(IPropCalculation *calc, const char *key)
{
  PropVector *v = calc ? dynamic_cast<PropVector*>(calc->getProperty(key)) : 0;
  return v ? &v->values() : 0;
}
}

IMPLEMENT_DYNAMIC_CLASS(PullPanel, PropertyPanel)

PullPanel::PullPanel()
  : PropertyPanel(), p_force(NULL), p_dist(NULL), p_text(NULL)
{
}

PullPanel::PullPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(), p_force(NULL), p_dist(NULL), p_text(NULL)
{
  Create(calculation, parent, id, pos, size, style, name);
}

PullPanel::~PullPanel()
{
}

bool PullPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("PullPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));

  const std::vector<double> *t = valuesOf(calculation, "PULLTIME");
  const std::vector<double> *x = valuesOf(calculation, "PULLDIST");
  const std::vector<double> *f = valuesOf(calculation, "PULLFORCE");
  if (t && x && f) {
    p_series.time = *t;
    p_series.dist = *x;
    p_series.force = *f;
  }

  wxBoxSizer *bar = new wxBoxSizer(wxHORIZONTAL);
  p_text = new wxStaticText(this, wxID_ANY, wxEmptyString);
  bar->Add(p_text, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  bar->Add(new wxButton(this, ID_EXPORT, "Export .csv..."), 0);
  GetSizer()->Add(bar, 0, wxEXPAND | wxALL, 4);

  p_force = new XYPlotCanvas(this);
  p_force->SetMinSize(wxSize(300, 150));
  p_force->setXAxisLabel("Time (ps)");
  p_force->setYAxisLabel("Pull force (kJ/mol/nm)");
  GetSizer()->Add(p_force, 1, wxEXPAND);
  p_dist = new XYPlotCanvas(this);
  p_dist->SetMinSize(wxSize(300, 150));
  p_dist->setXAxisLabel("Time (ps)");
  p_dist->setYAxisLabel("COM distance (nm)");
  GetSizer()->Add(p_dist, 1, wxEXPAND);

  if (p_series.size() > 1) {
    p_force->addCurve(p_series.time, p_series.force);
    p_dist->addCurve(p_series.time, p_series.dist);
    const PullSummary s = pullSummary(p_series);
    p_text->SetLabel(wxString::Format(
        "%d points, distance rising at %.4g nm/ps\nmean force %.4g, peak %.4g "
        "kJ/mol/nm at %.4g ps", (int)s.points, s.rate, s.meanForce,
        s.peakForce, s.peakTime));
  } else {
    p_text->SetLabel("No pull output in this run.");
  }

  Bind(wxEVT_BUTTON, &PullPanel::OnExport, this, ID_EXPORT);
  return true;
}

bool PullPanel::exportCsv(const wxString& path)
{
  IPropCalculation *calc = getCalculation();
  const std::string text = pullCsv(p_series,
      calc ? calc->getURL().getFilePathTail() : std::string());
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}

void PullPanel::OnExport(wxCommandEvent&)
{
  IPropCalculation *calc = getCalculation();
  wxFileDialog dlg(this, "Export pull run", wxEmptyString,
                   wxString::FromUTF8(calc ? calc->getURL().getFilePathTail().c_str() : "pull") +
                   "-pull.csv", "CSV files (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath()))
    wxMessageBox("The file could not be written.", "Export pull run",
                 wxOK | wxICON_ERROR, this);
}
