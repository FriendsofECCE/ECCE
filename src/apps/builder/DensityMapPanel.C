#include <algorithm>
#include <cmath>
#include <cstdio>

#include <wx/bitmap.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/dcbuffer.h>
#include <wx/dcmemory.h>
#include <wx/filedlg.h>
#include <wx/image.h>
#include <wx/link.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/stopwatch.h>

#include "dsm/IPropCalculation.H"

#include "DensityMapDraw.H"
#include "DensityMapPanel.H"

using namespace DensityPlane;

namespace {

const wxWindowID ID_SHOW = wxNewId(), ID_SPIN = wxNewId(), ID_MO = wxNewId(),
                 ID_PLANE = wxNewId(), ID_ATOM = wxNewId(),
                 ID_PNG = wxNewId(), ID_CSV = wxNewId();

}  // namespace


/** Draws a Map; knows nothing of the calculation. */
class DensityMapCanvas : public wxPanel
{
  public:
    DensityMapCanvas(wxWindow *parent)
      : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                wxFULL_REPAINT_ON_RESIZE),
        p_have(false)
    {
      SetBackgroundStyle(wxBG_STYLE_PAINT);
      SetMinSize(wxSize(420, 360));
      Bind(wxEVT_PAINT, &DensityMapCanvas::onPaint, this);
    }

    void show(const Map& m) { p_map = m; p_have = true; p_message.clear(); Refresh(); }
    void say(const wxString& msg) { p_have = false; p_message = msg; Refresh(); }

    void draw(wxDC& dc, const wxSize& size) const
    {
      paintDensityMap(dc, size, p_have, p_map, p_message);
    }

  private:
    void onPaint(wxPaintEvent&)
    {
      wxAutoBufferedPaintDC dc(this);
      draw(dc, GetClientSize());
    }

    Map p_map;
    bool p_have;
    wxString p_message;
};


IMPLEMENT_DYNAMIC_CLASS(DensityMapPanel, PropertyPanel)
wxFORCE_LINK_THIS_MODULE(DensityMapPanel)

DensityMapPanel::DensityMapPanel()
  : PropertyPanel(), p_haveMap(false), p_busy(false), p_show(0), p_spin(0),
    p_plane(0), p_atomA(0), p_atomB(0), p_atomC(0), p_mo(0), p_status(0),
    p_canvas(0)
{
}

DensityMapPanel::DensityMapPanel(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
  : PropertyPanel(), p_haveMap(false), p_busy(false), p_show(0), p_spin(0),
    p_plane(0), p_atomA(0), p_atomB(0), p_atomC(0), p_mo(0), p_status(0),
    p_canvas(0)
{
  Create(calculation, parent, id, pos, size, style, name);
}

DensityMapPanel::~DensityMapPanel() {}

bool DensityMapPanel::Create(IPropCalculation *calculation,
        wxWindow *parent, wxWindowID id, const wxPoint& pos,
        const wxSize& size, long style, const wxString& name)
{
  if (!PropertyPanel::Create(calculation, parent, id, pos, size, style, name)) {
    wxFAIL_MSG( wxT("DensityMapPanel creation failed") );
    return false;
  }
  SetSizer(new wxBoxSizer(wxVERTICAL));
  return true;
}

bool DensityMapPanel::isRelevant(IPropCalculation *propCalc) const
{
  return propCalc != 0 && propCalc->getProperty("MO") != 0 &&
         propCalc->getProperty("ORBOCC") != 0;
}

void DensityMapPanel::initialize()
{
  build();
}

void DensityMapPanel::build()
{
  wxSizer *top = GetSizer();
  p_eval.reset(new Evaluator);
  const bool ok = p_eval->build(getCalculation());

  wxBoxSizer *row1 = new wxBoxSizer(wxHORIZONTAL);
  p_show = new wxChoice(this, ID_SHOW);
  p_show->Append("Total density");
  p_show->Append("One orbital");
  p_show->SetSelection(0);
  row1->Add(new wxStaticText(this, wxID_ANY, "Show"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  row1->Add(p_show, 0, wxRIGHT, 12);
  p_mo = new wxSpinCtrl(this, ID_MO, "1", wxDefaultPosition, wxSize(80, -1),
                        wxSP_ARROW_KEYS, 1, std::max(1, ok ? p_eval->orbitals(false) : 1),
                        std::max(1, ok ? p_eval->homo() : 1));
  row1->Add(new wxStaticText(this, wxID_ANY, "MO"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  row1->Add(p_mo, 0, wxRIGHT, 12);
  p_spin = new wxChoice(this, ID_SPIN);
  p_spin->Append("alpha");
  p_spin->Append("beta");
  p_spin->SetSelection(0);
  row1->Add(p_spin, 0);

  wxBoxSizer *row2 = new wxBoxSizer(wxHORIZONTAL);
  p_plane = new wxChoice(this, ID_PLANE);
  p_plane->Append("Molecular plane");
  p_plane->Append("Through three atoms");
  p_plane->Append(wxString::FromUTF8("Through two atoms, \xE2\x8A\xA5 molecular plane"));
  row2->Add(new wxStaticText(this, wxID_ANY, "Plane"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
  row2->Add(p_plane, 0, wxRIGHT, 12);
  p_atomA = new wxChoice(this, ID_ATOM);
  p_atomB = new wxChoice(this, ID_ATOM);
  p_atomC = new wxChoice(this, ID_ATOM);
  if (ok) {
    const vector<string> labels = p_eval->atomLabels();
    for (size_t i = 0; i < labels.size(); i++) {
      p_atomA->Append(wxString::FromUTF8(labels[i].c_str()));
      p_atomB->Append(wxString::FromUTF8(labels[i].c_str()));
      p_atomC->Append(wxString::FromUTF8(labels[i].c_str()));
    }
    const int n = (int)labels.size();
    p_atomA->SetSelection(0);
    p_atomB->SetSelection(std::min(1, n - 1));
    p_atomC->SetSelection(std::min(2, n - 1));
    //  Plane default: the molecular plane when there is one.
    p_plane->SetSelection(isPlanar(p_eval->coordsAng) ? 0 : 1);
  } else {
    p_plane->SetSelection(0);
  }
  row2->Add(p_atomA, 0, wxRIGHT, 4);
  row2->Add(p_atomB, 0, wxRIGHT, 4);
  row2->Add(p_atomC, 0);

  wxBoxSizer *row3 = new wxBoxSizer(wxHORIZONTAL);
  p_status = new wxStaticText(this, wxID_ANY, wxEmptyString);
  row3->Add(p_status, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
  row3->Add(new wxButton(this, ID_PNG, "Export .png..."), 0, wxRIGHT, 4);
  row3->Add(new wxButton(this, ID_CSV, "Export .csv..."), 0);

  p_canvas = new DensityMapCanvas(this);
  top->Add(row1, 0, wxEXPAND | wxALL, 6);
  top->Add(row2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
  top->Add(p_canvas, 1, wxEXPAND);
  top->Add(row3, 0, wxEXPAND | wxALL, 6);

  Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onChanged(); });
  Bind(wxEVT_SPINCTRL, [this](wxSpinEvent&) { onChanged(); });
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export map", wxEmptyString, "density-map.png",
                     "PNG files (*.png)|*.png", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !savePng(dlg.GetPath(), 1100, 820))
      wxMessageBox("The file could not be written.", "Export map", wxOK | wxICON_ERROR, this);
  }, ID_PNG);
  Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
    wxFileDialog dlg(this, "Export map values", wxEmptyString, "density-map.csv",
                     "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() == wxID_OK && !exportCsv(dlg.GetPath()))
      wxMessageBox("The file could not be written.", "Export map values", wxOK | wxICON_ERROR, this);
  }, ID_CSV);

  if (!ok) {
    p_canvas->say(wxString::FromUTF8(p_eval->complaint.c_str()));
    p_show->Enable(false);  p_mo->Enable(false);  p_spin->Enable(false);
    p_plane->Enable(false); p_atomA->Enable(false);
    p_atomB->Enable(false); p_atomC->Enable(false);
    return;
  }
  p_spin->Show(p_eval->unrestricted);
  syncControls();
  compute();
  Layout();
}

void DensityMapPanel::syncControls()
{
  const bool orbital = p_show->GetSelection() == 1;
  p_mo->Enable(orbital);
  p_spin->Enable(orbital && p_eval->unrestricted);
  const int plane = p_plane->GetSelection();
  p_atomA->Enable(plane != 0);
  p_atomB->Enable(plane != 0);
  p_atomC->Enable(plane == 1);
}

void DensityMapPanel::onChanged()
{
  syncControls();
  compute();
}

void DensityMapPanel::compute()
{
  if (p_busy || !p_eval) return;
  string why;
  const int orbital = p_show->GetSelection() == 1 ? p_mo->GetValue() : 0;
  const PlaneKind kind = p_plane->GetSelection() == 0 ? MOLECULAR
                       : p_plane->GetSelection() == 1 ? THREE_ATOMS : AXIAL;
  if (!evaluate(orbital, p_spin->GetSelection() == 1, kind,
                p_atomA->GetSelection(), p_atomB->GetSelection(),
                p_atomC->GetSelection(), why)) {
    if (!why.empty()) {
      p_canvas->say(wxString::FromUTF8(why.c_str()));
      p_status->SetLabel(wxEmptyString);
    }
    p_haveMap = false;
    return;
  }
  p_canvas->show(p_map);
  p_haveMap = true;
  p_status->SetLabel(p_map.quantity == DENSITY
      ? "Contours from 0.001 e/bohr^3, each level double the last"
      : "Contours from +-0.02, each level double the last; dashed is negative");
}

bool DensityMapPanel::evaluate(int orbital, bool beta, PlaneKind kind,
                               int a, int b, int c, string& why)
{
  Plane plane;
  if (!makePlane(kind, p_eval->coordsAng, p_eval->atomLabels(), a, b, c,
                 plane, why))
    return false;
  if (orbital > p_eval->orbitals(beta)) { why = "No such orbital."; return false; }
  //  Larger molecules get a coarser grid so a map stays interactive.
  const double step = p_eval->symbols.size() > 20 ? 0.1 : 0.05;

  //  Shown only if the map takes long enough to be noticed.
  wxStopWatch clock;
  std::unique_ptr<wxProgressDialog> progress;
  p_busy = true;
  Map out;
  const bool done = p_eval->map(plane, orbital ? ORBITAL : DENSITY, orbital,
      beta, step, 2.5, out, [&](double fraction) {
        if (!progress && clock.Time() > 500 && fraction < 0.7)
          progress.reset(new wxProgressDialog("Density map", "Evaluating...",
              100, this, wxPD_CAN_ABORT | wxPD_AUTO_HIDE | wxPD_APP_MODAL));
        return progress ? progress->Update((int)(100*fraction)) : true;
      });
  p_busy = false;
  if (!done) { why.clear(); return false; }
  p_map = out;
  return true;
}

bool DensityMapPanel::setView(int orbital, bool beta, int plane, int a, int b, int c)
{
  if (!p_eval) { ensureInitialized(); }
  if (!p_eval || !p_canvas || !p_show) return false;
  if (orbital > 0) { p_show->SetSelection(1); p_mo->SetValue(orbital); }
  else p_show->SetSelection(0);
  p_spin->SetSelection(beta ? 1 : 0);
  p_plane->SetSelection(plane);
  if (a > 0) p_atomA->SetSelection(a - 1);
  if (b > 0) p_atomB->SetSelection(b - 1);
  if (c > 0) p_atomC->SetSelection(c - 1);
  syncControls();
  compute();
  return p_haveMap;
}

bool DensityMapPanel::savePng(const wxString& path, int width, int height)
{
  if (!p_canvas) return false;
  wxInitAllImageHandlers();
  wxBitmap bmp(width, height);
  wxMemoryDC dc(bmp);
  p_canvas->draw(dc, wxSize(width, height));
  dc.SelectObject(wxNullBitmap);
  return bmp.SaveFile(path, wxBITMAP_TYPE_PNG);
}

bool DensityMapPanel::exportCsv(const wxString& path)
{
  if (!p_haveMap) return false;
  IPropCalculation *calc = getCalculation();
  const string text = csv(p_map, calc ? calc->getURL().getFilePathTail() : string());
  FILE *out = fopen(path.utf8_str(), "wb");
  if (out == 0) return false;
  const bool ok = fwrite(text.data(), 1, text.size(), out) == text.size();
  return fclose(out) == 0 && ok;
}
