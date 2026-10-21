#include <cstdio>
#include <cstdlib>

#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/grid.h>
#include <wx/link.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/wrapsizer.h>

#include "tdat/PropVecString.H"
#include "tdat/PropVector.H"

#include "dsm/ChemistryTask.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"
#include "util/EcceURL.H"

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxGrid.H"

#include "UvVisPanel.H"

static const char *INIFILE = "wxbuilder.ini";

wxFORCE_LINK_THIS_MODULE(UvVisPanel)

IMPLEMENT_DYNAMIC_CLASS(UvVisPanel, PropertyPanel)


UvVisPanel::UvVisPanel()
  : PropertyPanel(), p_canvas(0), p_grid(0), p_detail(0), p_width(0),
    p_axis(0), p_unit(X_NM), p_fwhm(0.3), p_selected(-1), p_dumped(false)
{
}

UvVisPanel::UvVisPanel(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
  : PropertyPanel(), p_canvas(0), p_grid(0), p_detail(0), p_width(0),
    p_axis(0), p_unit(X_NM), p_fwhm(0.3), p_selected(-1), p_dumped(false)
{
  Create(calculation, parent, id, pos, size, style, name);
}

UvVisPanel::~UvVisPanel()
{
}


bool UvVisPanel::Create(IPropCalculation *calculation, wxWindow *parent,
        wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
        const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style)) {
    wxFAIL_MSG(wxT("PropertyPanel creation failed"));
    return false;
  }

  ewxConfig *config = ewxConfig::getConfig(INIFILE);
  double fwhm = 0.3;
  bool nm = true, sticks = true;
  config->Read("UvVis/FWHM", &fwhm, 0.3);
  config->Read("UvVis/AxisNm", &nm, true);
  config->Read("UvVis/ShowSticks", &sticks, true);
  if (fwhm < 0 || fwhm > 2) fwhm = 0.3;
  p_fwhm = fwhm;
  p_unit = nm ? X_NM : X_EV;

  wxBoxSizer *col = new wxBoxSizer(wxVERTICAL);

  p_canvas = new SpectrumCanvas(this);
  p_canvas->setClickHandler(this);
  p_canvas->setCompactBelow(0);
  p_canvas->setReversed(false);
  p_canvas->setShowSticks(sticks);
  p_canvas->setFwhm(p_fwhm);
  col->Add(p_canvas, 3, wxEXPAND);

  wxWrapSizer *row = new wxWrapSizer(wxHORIZONTAL);
  const int gap = 6;
  row->Add(new wxStaticText(this, wxID_ANY, "Width (eV):"), 0,
           wxALIGN_CENTER_VERTICAL | wxLEFT, gap);
  p_width = new wxTextCtrl(this, wxID_ANY, wxString::Format("%g", p_fwhm),
                           wxDefaultPosition, wxSize(52, -1),
                           wxTE_PROCESS_ENTER);
  p_width->SetToolTip("Full width at half maximum of the Gaussian drawn "
                      "around each state, in eV (0 to 2).\nAbout 0.3 eV "
                      "resembles a solution spectrum.  0 shows sticks only.");
  row->Add(p_width, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 3);

  row->Add(new wxStaticText(this, wxID_ANY, "Axis:"), 0,
           wxALIGN_CENTER_VERTICAL | wxLEFT, gap * 2);
  p_axis = new wxChoice(this, wxID_ANY);
  p_axis->Append("Wavelength (nm)");
  p_axis->Append("Energy (eV)");
  p_axis->SetSelection(nm ? 0 : 1);
  row->Add(p_axis, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 3);

  wxCheckBox *stk = new wxCheckBox(this, wxID_ANY, "Sticks");
  stk->SetValue(sticks);
  row->Add(stk, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap * 2);

  wxButton *reset = new wxButton(this, wxID_ANY, "Reset");
  reset->SetToolTip("Show the whole spectrum (or double-click the plot).  "
                    "Drag to zoom, mouse wheel to zoom, right-drag to pan.");
  row->Add(reset, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, gap * 2);
  wxButton *exportCsv = new wxButton(this, wxID_ANY, "Export .csv...");
  exportCsv->SetToolTip("Save the states and the broadened curve as a "
                        "comma-separated file.");
  row->Add(exportCsv, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, gap);
  col->Add(row, 0, wxEXPAND | wxTOP | wxBOTTOM, 3);

  p_grid = new ewxGrid(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                       wxSUNKEN_BORDER | wxHSCROLL | wxVSCROLL);
  p_grid->CreateGrid(0, 5, wxGrid::wxGridSelectRows);
  p_grid->EnableEditing(false);
  p_grid->SetRowLabelSize(0);
  p_grid->SetColLabelValue(0, "State");
  p_grid->SetColLabelValue(1, "Energy (eV)");
  p_grid->SetColLabelValue(2, "Wavelength (nm)");
  p_grid->SetColLabelValue(3, "f");
  p_grid->SetColLabelValue(4, "Label");
  p_grid->SetMinSize(wxSize(200, 110));
  col->Add(p_grid, 2, wxEXPAND | wxALL, 3);

  p_detail = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                            wxSize(-1, 70),
                            wxTE_MULTILINE | wxTE_READONLY);
  p_detail->SetToolTip("The orbital pairs of the selected state with the "
                       "coefficient the code printed for each.  Orbitals "
                       "are numbered from 1.");
  col->Add(p_detail, 1, wxEXPAND | wxALL, 3);

  SetSizer(col);

  p_width->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) { applyWidth(); });
  p_width->Bind(wxEVT_KILL_FOCUS, [this](wxFocusEvent& e) {
    applyWidth(); e.Skip(); });
  p_axis->Bind(wxEVT_CHOICE, [this](wxCommandEvent& e) {
    setAxis(e.GetSelection() == 0 ? X_NM : X_EV);
  });
  stk->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) {
    p_canvas->setShowSticks(e.IsChecked());
    ewxConfig::getConfig(INIFILE)->Write("UvVis/ShowSticks", e.IsChecked());
  });
  reset->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { p_canvas->resetZoom(); });
  exportCsv->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { exportDialog(); });
  p_grid->Bind(wxEVT_GRID_SELECT_CELL, &UvVisPanel::onCell, this);

  load();
  CallAfter([this]() { dumpIfRequested(); });
  return true;
}


void UvVisPanel::refresh()
{
  load();
}


/** The four properties into the model, the plot and the table. */
void UvVisPanel::load()
{
  IPropCalculation *calc = getCalculation();
  if (calc == 0) return;
  PropVector *ev = dynamic_cast<PropVector*>(calc->getProperty("EXCIENERGY"));
  PropVector *f = dynamic_cast<PropVector*>(calc->getProperty("EXCIOSC"));
  PropVecString *sym =
      dynamic_cast<PropVecString*>(calc->getProperty("EXCISYM"));
  PropVecString *trans =
      dynamic_cast<PropVecString*>(calc->getProperty("EXCITRANS"));

  std::vector<double> e, osc;
  std::vector<std::string> labels, tr;
  if (ev != 0) {
    for (int i = 0; i < ev->rows(); i++) {
      e.push_back(ev->value(i));
      osc.push_back(f != 0 && i < f->rows() ? f->value(i) : 0.0);
      labels.push_back(sym != 0 && i < sym->rows() ? sym->value(i) : "");
      tr.push_back(trans != 0 && i < trans->rows() ? trans->value(i) : "");
    }
  }
  p_states.set(e, osc, labels, tr);
  p_selected = -1;
  setAxis(p_unit);
  fillTable();
  if (p_detail != 0) p_detail->Clear();
}


void UvVisPanel::fillTable()
{
  if (p_grid->GetNumberRows() > 0)
    p_grid->DeleteRows(0, p_grid->GetNumberRows());
  const int n = p_states.count();
  if (n > 0) p_grid->AppendRows(n);
  for (int i = 0; i < n; i++) {
    const ExcitedState& s = p_states.state(i);
    p_grid->SetCellValue(i, 0, wxString::Format("%d", i + 1));
    p_grid->SetCellValue(i, 1, wxString::Format("%.4f", s.ev));
    p_grid->SetCellValue(i, 2, wxString::Format("%.1f", ExcitedStates::nm(s.ev)));
    p_grid->SetCellValue(i, 3, wxString::Format("%.4f", s.f));
    p_grid->SetCellValue(i, 4, wxString::FromUTF8(s.label.c_str()));
    for (int c = 0; c < 4; c++)
      p_grid->SetCellAlignment(i, c, wxALIGN_RIGHT, wxALIGN_CENTRE);
  }
  p_grid->AutoSizeColumns();
}


void UvVisPanel::setAxis(SpectrumXUnit unit)
{
  p_unit = unit == X_EV ? X_EV : X_NM;
  VibSpectrum spec = p_states.spectrum(p_unit);
  spec.setLabel(getCalculation() ? getCalculation()->getURL().getFilePathTail()
                                 : std::string());
  p_canvas->setSpectrum(spec);          // keeps the width, drops the zoom
  if (p_axis != 0) p_axis->SetSelection(p_unit == X_NM ? 0 : 1);
  ewxConfig::getConfig(INIFILE)->Write("UvVis/AxisNm", p_unit == X_NM);
  if (p_selected >= 0) p_canvas->setSelected(p_selected);
}


void UvVisPanel::setWidth(double ev)
{
  if (ev < 0) ev = 0;
  if (ev > 2) ev = 2;
  if (ev > 0 && ev < 0.01) ev = 0.01;
  p_fwhm = ev;
  p_canvas->setFwhm(ev);
  if (p_width != 0) p_width->ChangeValue(wxString::Format("%g", ev));
  ewxConfig::getConfig(INIFILE)->Write("UvVis/FWHM", ev);
}


void UvVisPanel::applyWidth()
{
  double v;
  if (p_width->GetValue().ToDouble(&v) && v >= 0) setWidth(v);
  else p_width->ChangeValue(wxString::Format("%g", p_fwhm));
}


void UvVisPanel::modeClicked(int mode)
{
  selectState(mode);
}


void UvVisPanel::selectState(int index)
{
  if (index < 0 || index >= p_states.count()) return;
  p_selected = index;
  p_canvas->setSelected(index);
  p_grid->SelectRow(index);
  p_grid->MakeCellVisible(index, 0);
  showTransitions(index);
}


void UvVisPanel::onCell(wxGridEvent& event)
{
  const int row = event.GetRow();
  if (row >= 0 && row < p_states.count() && row != p_selected) {
    p_selected = row;
    p_canvas->setSelected(row);
    showTransitions(row);
  }
  event.Skip();
}


void UvVisPanel::showTransitions(int row)
{
  const ExcitedState& s = p_states.state(row);
  wxString text = wxString::Format("State %d", row + 1);
  if (!s.label.empty()) text += "  " + wxString::FromUTF8(s.label.c_str());
  text += wxString::Format(":  %.4f eV,  %.1f nm,  f = %.4f\n", s.ev,
                           ExcitedStates::nm(s.ev), s.f);
  const std::vector<std::string> lines = p_states.transitionLines(row);
  if (lines.empty()) text += "No transitions were printed for this state.";
  for (size_t i = 0; i < lines.size(); i++)
    text += wxString::FromUTF8(lines[i].c_str()) + "\n";
  p_detail->ChangeValue(text);
}


bool UvVisPanel::exportCsv(const wxString& path)
{
  IPropCalculation *calc = getCalculation();
  if (calc == 0 || p_states.count() == 0) return false;
  SpectrumCsvInfo info;
  info.calculation = calc->getURL().getFilePathTail();
  ChemistryTask *task = dynamic_cast<ChemistryTask*>(calc);
  const JCode *code = task ? task->application() : 0;
  info.code = code ? code->name() : std::string();
  //  1 nm or 0.01 eV steps over the plotted range.
  const std::string text = excitedSpectrumCsv(
      p_states, p_canvas->spectrum(), info, p_canvas->axis().fullLo(),
      p_canvas->axis().fullHi(), p_unit == X_NM ? 1.0 : 0.01);
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}


void UvVisPanel::exportDialog()
{
  IPropCalculation *calc = getCalculation();
  if (calc == 0) return;
  wxFileDialog dlg(this, "Export spectrum", wxEmptyString,
                   wxString::FromUTF8(calc->getURL().getFilePathTail().c_str()) +
                   "-uvvis.csv", "CSV files (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  if (dlg.ShowModal() != wxID_OK) return;
  if (!exportCsv(dlg.GetPath()))
    wxMessageBox("The file could not be written.", "Export spectrum",
                 wxOK | wxICON_ERROR, this);
}


/**
 * ECCE_UVVIS_DUMP=<path>: what the panel holds, and the canvas painted to
 * <path>.png.  ECCE_UVVIS_AXIS=nm|eV, ECCE_UVVIS_WIDTH=<eV>,
 * ECCE_UVVIS_CLICK=<state number> (a click on its stick) and
 * ECCE_UVVIS_CSV=<path> (what Export would write) are applied first.
 * ECCE_EXIT_AFTER_DUMP=1 closes the Builder afterwards.
 */
void UvVisPanel::dumpIfRequested()
{
  const char *path = getenv("ECCE_UVVIS_DUMP");
  if (path == 0 || p_dumped || p_canvas == 0) return;
  p_dumped = true;

  const char *axis = getenv("ECCE_UVVIS_AXIS");
  if (axis != 0) setAxis(std::string(axis) == "eV" ? X_EV : X_NM);
  const char *width = getenv("ECCE_UVVIS_WIDTH");
  if (width != 0) setWidth(atof(width));

  const wxSize size(1000, 700);
  wxBitmap bitmap(size.x, size.y, 24);
  wxMemoryDC dc(bitmap);
  p_canvas->paintOnto(dc, size);

  FILE *out = fopen(path, "w");
  if (out != 0) {
    const VibSpectrum& s = p_canvas->spectrum();
    fprintf(out, "states %d fwhm %g axis %s\n", s.modeCount(), s.fwhm(),
            s.positionUnit().c_str());
    fprintf(out, "axis-title %s\n", s.xTitle().c_str());
    const std::vector<VibStick> st = s.sticks(VIB_IR);
    for (size_t i = 0; i < st.size(); i++) {
      wxPoint at(-1, -1);
      p_canvas->stickPosition(VIB_IR, st[i].mode, &at);
      fprintf(out, "stick %d %.8f %.10g %s %d %d\n", st[i].mode + 1,
              st[i].wavenumber, st[i].intensity,
              st[i].irrep.empty() ? "-" : st[i].irrep.c_str(), at.x, at.y);
    }
    const char *click = getenv("ECCE_UVVIS_CLICK");
    if (click != 0) {
      const int mode = atoi(click) - 1;
      wxPoint at;
      if (p_canvas->stickPosition(VIB_IR, mode, &at) &&
          p_canvas->clickAt(wxPoint(at.x, at.y + 3))) {
        fprintf(out, "click state %d: selected %d\n", mode + 1,
                p_selected + 1);
        const std::vector<std::string> lines = p_states.transitionLines(mode);
        for (size_t i = 0; i < lines.size(); i++)
          fprintf(out, "transition %s\n", lines[i].c_str());
      } else {
        fprintf(out, "click state %d: no stick\n", mode + 1);
      }
    }
    fclose(out);
  }
  dc.SelectObject(wxNullBitmap);
  bitmap.SaveFile(wxString(path) + ".png", wxBITMAP_TYPE_PNG);

  const char *csv = getenv("ECCE_UVVIS_CSV");
  if (csv != 0) exportCsv(csv);

  if (getenv("ECCE_EXIT_AFTER_DUMP") != 0) {
    wxTheApp->CallAfter([]() {
      wxWindow *top = wxTheApp->GetTopWindow();
      if (top != 0) top->Close(true);
    });
  }
}
