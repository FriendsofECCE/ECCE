#include "wxgui/DiffractionDialog.H"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <sstream>

#include <wx/dcbuffer.h>
#include <wx/listctrl.h>
#include <wx/filedlg.h>
#include <wx/process.h>
#include <wx/progdlg.h>
#include <wx/slider.h>
#include <wx/stopwatch.h>
#include <wx/sstream.h>

#include "util/Ecce.H"
#include "util/Filesystem.H"

using std::string;
using std::vector;
namespace fs = ecce::fs;

namespace {
const int PIXELS = 400;

// Powder is for periodic structures: the Builder (and the results entry)
// write them as an .xyz whose comment line carries Lattice="...".
bool hasLattice(const string& path)
{
  std::ifstream f(path);
  string a, b;
  std::getline(f, a);
  std::getline(f, b);
  return path.size() > 4 && path.compare(path.size() - 4, 4, ".xyz") == 0 &&
         b.find("Lattice=") != string::npos;
}

class Child : public wxProcess
{
public:
  Child() : wxProcess(wxPROCESS_REDIRECT), done(false), code(-1) {}
  void OnTerminate(int, int status) override { done = true; code = status; }
  bool done;
  int code;
};

string drain(wxInputStream *in)
{
  string out;
  char buf[4096];
  while (in && in->CanRead()) {
    in->Read(buf, sizeof buf);
    size_t n = in->LastRead();
    if (!n) break;
    out.append(buf, n);
  }
  return out;
}

string num(double v)
{
  std::ostringstream o;
  o.imbue(std::locale::classic());
  o << v;
  return o.str();
}

double parse(const wxString& t)
{
  std::istringstream i(string(t.ToUTF8().data()));
  i.imbue(std::locale::classic());
  double v = 0;
  i >> v;
  return v;
}

string tool()
{
  const char *home = getenv("ECCE_HOME");
  return home && *home ? string(home) + "/bin/ecce-xrd" : string("ecce-xrd");
}
}  // namespace


// The image, scaled to the window, with the pointer position reported back.
class DiffractionPanel : public wxPanel
{
public:
  DiffractionPanel(wxWindow *parent, DiffractionDialog *d)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(480, 480)), p_d(d)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &DiffractionPanel::onPaint, this);
    Bind(wxEVT_MOTION, &DiffractionPanel::onMove, this);
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { p_d->p_pointer->SetLabel(""); });
  }
  void setImage(const wxImage& im) { p_bmp = im.IsOk() ? wxBitmap(im) : wxBitmap(); Refresh(); }
  /// Image pixel under a window position, or false.
  bool pixelAt(const wxPoint& p, int& x, int& y) const
  {
    if (!p_bmp.IsOk()) return false;
    const wxRect r = area();
    if (!r.Contains(p)) return false;
    x = (p.x - r.x) * p_bmp.GetWidth() / r.width;
    y = (p.y - r.y) * p_bmp.GetHeight() / r.height;
    return true;
  }

private:
  wxRect area() const
  {
    const wxSize s = GetClientSize();
    const int side = std::min(s.x, s.y);
    return wxRect((s.x - side) / 2, (s.y - side) / 2, side, side);
  }
  void onPaint(wxPaintEvent&)
  {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    if (!p_bmp.IsOk()) return;
    const wxRect r = area();
    wxImage im = p_bmp.ConvertToImage().Scale(r.width, r.height, wxIMAGE_QUALITY_NEAREST);
    dc.DrawBitmap(wxBitmap(im), r.x, r.y);
  }
  void onMove(wxMouseEvent& e)
  {
    int x, y;
    p_d->p_pointer->SetLabel(pixelAt(e.GetPosition(), x, y) ? p_d->pointerText(x, y) : wxString());
  }
  DiffractionDialog *p_d;
  wxBitmap p_bmp;
};


// The powder pattern as a line plot; the pointer reports the reflection nearest to it.
class PowderPanel : public wxPanel
{
public:
  PowderPanel(wxWindow *parent, DiffractionDialog *d)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(560, 340)), p_d(d)
  {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &PowderPanel::onPaint, this);
    Bind(wxEVT_MOTION, &PowderPanel::onMove, this);
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { p_d->p_pointer->SetLabel(""); });
  }
  // 2theta under a window x, false outside the plot.
  bool twoThetaAt(int x, double& tt) const
  {
    if (p_d->p_tt.size() < 2) return false;
    const wxRect r = plot();
    if (x < r.x || x > r.x + r.width) return false;
    tt = p_d->p_tt.front() + (x - r.x) * (p_d->p_tt.back() - p_d->p_tt.front()) / r.width;
    return true;
  }

private:
  wxRect plot() const
  {
    const wxSize s = GetClientSize();
    return wxRect(52, 10, std::max(10, s.x - 66), std::max(10, s.y - 44));
  }
  void onPaint(wxPaintEvent&)
  {
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    const vector<double>& tt = p_d->p_tt;
    if (tt.size() < 2) return;
    const wxRect r = plot();
    dc.SetPen(*wxBLACK_PEN);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(r);
    const double x0 = tt.front(), x1 = tt.back();
    auto px = [&](double v) { return r.x + (int)((v - x0) / (x1 - x0) * r.width); };
    auto py = [&](double v) { return r.y + r.height - (int)(std::min(v, 105.0) / 105.0 * r.height); };
    double step = 1;
    for (double c : {1., 2., 5., 10., 20., 50.}) { step = c; if ((x1 - x0) / c <= 12) break; }
    for (double v = std::ceil(x0 / step) * step; v <= x1 + 1e-9; v += step) {
      dc.DrawLine(px(v), r.y + r.height, px(v), r.y + r.height + 4);
      dc.DrawText(wxString::Format("%g", v), px(v) - 8, r.y + r.height + 6);
    }
    dc.DrawText("2theta (degrees)", r.x + r.width / 2 - 45, r.y + r.height + 22);
    for (int v = 0; v <= 100; v += 25) {
      dc.DrawLine(r.x - 4, py(v), r.x, py(v));
      dc.DrawText(wxString::Format("%d", v), r.x - 30, py(v) - 8);
    }
    auto curve = [&](const vector<double>& y, const wxColour& c) {
      dc.SetPen(wxPen(c, 1));
      for (size_t i = 1; i < y.size() && i < tt.size(); i++)
        if (y[i] == y[i] && y[i - 1] == y[i - 1])
          dc.DrawLine(px(tt[i - 1]), py(y[i - 1]), px(tt[i]), py(y[i]));
    };
    curve(p_d->p_meas, wxColour(205, 40, 40));
    curve(p_d->p_y, wxColour(20, 60, 170));
  }
  void onMove(wxMouseEvent& e)
  {
    double tt;
    wxString t;
    if (twoThetaAt(e.GetX(), tt)) {
      const double perPx = (p_d->p_tt.back() - p_d->p_tt.front()) / plot().width;
      t = p_d->reflectionText(tt, 4 * perPx);
      if (t.empty()) t = wxString::Format("2theta = %.2f", tt);
    }
    p_d->p_pointer->SetLabel(t);
  }
  DiffractionDialog *p_d;
};


DiffractionDialog::DiffractionDialog(wxWindow *parent, const string& structure,
                                     const vector<int>& selection)
  : wxDialog(parent, wxID_ANY, "Diffraction", wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_structure(structure), p_pixels(PIXELS), p_periodic(hasLattice(structure)),
    p_powder(false), p_single(false), p_frame(false), p_layerScale(1), p_layerSize(560)
{
  static int serial = 0;
  std::error_code ec;
  p_dir = (fs::temp_directory_path() / ("ecce-xrd-" + std::to_string(wxGetProcessId()) + "-" +
                                        std::to_string(serial++))).string();
  fs::create_directories(p_dir, ec);
  p_png = p_dir + "/pattern.png";
  p_csv = p_dir + "/pattern.csv";
  p_tab = p_dir + "/reflections.csv";

  // The selection as 1-40,57 for --select.
  std::ostringstream sel;
  for (size_t i = 0; i < selection.size(); ) {
    size_t j = i;
    while (j + 1 < selection.size() && selection[j + 1] == selection[j] + 1) j++;
    sel << (i ? "," : "") << selection[i];
    if (j > i) sel << "-" << selection[j];
    i = j + 1;
  }
  p_selectionText = sel.str();

  wxBoxSizer *top = new wxBoxSizer(wxHORIZONTAL);
  wxBoxSizer *left = new wxBoxSizer(wxVERTICAL);

  wxStaticBoxSizer *type = new wxStaticBoxSizer(wxVERTICAL, this, "Pattern");
  wxRadioButton *fibre = new wxRadioButton(this, wxID_ANY, "Fibre", wxDefaultPosition,
                                           wxDefaultSize, wxRB_GROUP);
  wxRadioButton *powder = new wxRadioButton(this, wxID_ANY, "Powder (periodic structures only)");
  wxRadioButton *single = new wxRadioButton(this, wxID_ANY, "Single crystal (periodic structures only)");
  wxRadioButton *frame = new wxRadioButton(this, wxID_ANY, "Detector frame, rotating crystal (periodic structures only)");
  powder->Enable(p_periodic);
  single->Enable(p_periodic);
  frame->Enable(p_periodic);
  p_rbFibre = fibre; p_rbPowder = powder; p_rbSingle = single;
  powder->SetValue(p_periodic);
  fibre->SetValue(!p_periodic);
  type->Add(fibre, 0, wxALL, 3);
  type->Add(powder, 0, wxALL, 3);
  type->Add(single, 0, wxALL, 3);
  type->Add(frame, 0, wxALL, 3);
  left->Add(type, 0, wxEXPAND | wxALL, 4);
  p_left = left;
  wxBoxSizer *fibreBox = new wxBoxSizer(wxVERTICAL);
  p_fibreItems = fibreBox;

  wxFlexGridSizer *grid = new wxFlexGridSizer(2, 4, 6);
  auto field = [&](const char *label, const char *value, const char *tip) {
    grid->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    grid->Add(t, 0);
    return t;
  };
  p_wavelength = field("Wavelength (A)", "1.5418", "Cu K-alpha is 1.5418");
  p_distance = field("Film distance (mm)", "30", "Sample to flat film");
  p_film = field("Film size (mm)", "40", "Edge of the square film");
  p_b = field("B (A^2)", "20", "Isotropic temperature factor");
  p_length = field("Fibre length (A)", "100", "Gaussian broadening along the layer lines; 0 = none");
  p_mis = field("Misalignment (deg)", "8", "Spread of the fibre axes: arcs");
  p_repeat = field("Repeat (A)", "0", "Axial repeat; the pointer then reports the layer line l = Z * repeat");
  p_gamma = field("Gamma", "1", "Display exponent");
  grid->Add(new wxStaticText(this, wxID_ANY, "Axis"), 0, wxALIGN_CENTER_VERTICAL);
  p_axis = new wxChoice(this, wxID_ANY);
  p_axis->Append("Auto (principal axis)");
  p_axis->Append("First to last selected atom");
  p_axis->SetSelection(0);
  grid->Add(p_axis, 0);
  grid->Add(new wxStaticText(this, wxID_ANY, "Scaling"), 0, wxALIGN_CENTER_VERTICAL);
  p_scale = new wxChoice(this, wxID_ANY);
  for (const char *s : {"log", "sqrt", "linear"}) p_scale->Append(s);
  p_scale->SetSelection(0);
  grid->Add(p_scale, 0);
  fibreBox->Add(grid, 0, wxALL, 6);
  p_invert = new wxCheckBox(this, wxID_ANY, "Dark spots on white");
  p_invert->SetValue(true);
  fibreBox->Add(p_invert, 0, wxLEFT | wxRIGHT, 6);
  p_selectedOnly = new wxCheckBox(this, wxID_ANY, "Selected atoms only");
  p_selectedOnly->SetValue(!selection.empty());
  p_selectedOnly->Enable(!selection.empty());
  fibreBox->Add(p_selectedOnly, 0, wxALL, 6);

  wxButton *back = new wxButton(this, wxID_ANY, "Load Backdrop...");
  fibreBox->Add(back, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  fibreBox->Add(new wxStaticText(this, wxID_ANY, "Calculated pattern opacity"), 0, wxLEFT | wxTOP, 6);
  p_opacity = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxSize(200, -1));
  fibreBox->Add(p_opacity, 0, wxLEFT | wxRIGHT, 6);
  left->Add(fibreBox, 0);

  wxBoxSizer *powderBox = new wxBoxSizer(wxVERTICAL);
  p_powderItems = powderBox;
  wxFlexGridSizer *pg = new wxFlexGridSizer(2, 4, 6);
  auto pfield = [&](const char *label, const char *value, const char *tip) {
    pg->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    pg->Add(t, 0);
    return t;
  };
  pg->Add(new wxStaticText(this, wxID_ANY, "Source"), 0, wxALIGN_CENTER_VERTICAL);
  p_source = new wxChoice(this, wxID_ANY);
  for (const char *c : {"Cu Ka (1.5406)", "Mo Ka (0.7093)", "Co Ka (1.7890)", "Other (type the wavelength)"})
    p_source->Append(c);
  p_source->SetSelection(0);
  pg->Add(p_source, 0);
  pg->AddSpacer(0);
  p_doublet = new wxCheckBox(this, wxID_ANY, "K\u03b11/K\u03b12 doublet");
  p_doublet->SetValue(true);
  p_doublet->SetToolTip("Draw every reflection at the K-alpha1 and K-alpha2 wavelengths (K-alpha2 at half the intensity); Cu, Mo and Co sources only");
  pg->Add(p_doublet, 0);
  p_pwave = pfield("Wavelength (A)", "1.5406", "Used for the pattern; the Source choice fills it");
  p_ptmin = pfield("2theta from", "5", "Degrees");
  p_ptmax = pfield("2theta to", "90", "Degrees");
  p_pfwhm = pfield("Peak width (deg)", "0.1", "FWHM in 2theta; pseudo-Voigt with equal Gauss and Lorentz parts");
  p_pb = pfield("B (A^2)", "0", "Isotropic temperature factor");
  powderBox->Add(pg, 0, wxALL, 6);
  wxButton *meas = new wxButton(this, wxID_ANY, "Load measured pattern...");
  powderBox->Add(meas, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  left->Add(powderBox, 0);

  wxBoxSizer *layerBox = new wxBoxSizer(wxVERTICAL);
  p_layerItems = layerBox;
  wxFlexGridSizer *lg = new wxFlexGridSizer(2, 4, 6);
  auto lfield = [&](const char *label, const char *value, const char *tip) {
    lg->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    lg->Add(t, 0);
    return t;
  };
  lg->Add(new wxStaticText(this, wxID_ANY, "Zone"), 0, wxALIGN_CENTER_VERTICAL);
  p_zone = new wxChoice(this, wxID_ANY);
  for (const char *c : {"hk(l): l is the layer", "h(k)l: k is the layer", "(h)kl: h is the layer"})
    p_zone->Append(c);
  p_zone->SetSelection(0);
  lg->Add(p_zone, 0);
  p_layerNo = lfield("Layer number", "0", "0 gives hk0, h0l or 0kl; 1, 2 ... the layers above it");
  p_lres = lfield("Resolution d_min (A)", "1", "Spots with d below this are left out");
  p_lb = lfield("B (A^2)", "0", "Isotropic temperature factor");
  lg->Add(new wxStaticText(this, wxID_ANY, "Spot area"), 0, wxALIGN_CENTER_VERTICAL);
  p_lscale = new wxChoice(this, wxID_ANY);
  for (const char *c : {"log", "sqrt", "linear"}) p_lscale->Append(c);
  p_lscale->SetSelection(0);
  lg->Add(p_lscale, 0);
  layerBox->Add(lg, 0, wxALL, 6);
  p_absent = new wxCheckBox(this, wxID_ANY, "Show absent reflections");
  p_absent->SetToolTip("Open circles where the structure factor is zero by symmetry");
  layerBox->Add(p_absent, 0, wxALL, 6);
  left->Add(layerBox, 0);

  // Detector frame (#985): a crystal turned through an oscillation range.
  wxBoxSizer *frameBox = new wxBoxSizer(wxVERTICAL);
  p_frameItems = frameBox;
  wxFlexGridSizer *fg = new wxFlexGridSizer(2, 4, 6);
  auto ffield = [&](const char *label, const char *value, const char *tip) {
    fg->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    wxTextCtrl *t = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxSize(80, -1));
    t->SetToolTip(tip);
    fg->Add(t, 0);
    return t;
  };
  fg->Add(new wxStaticText(this, wxID_ANY, "Start angle phi (deg)"), 0, wxALIGN_CENTER_VERTICAL);
  p_fphi = new wxSlider(this, wxID_ANY, 0, 0, 359, wxDefaultPosition, wxSize(160, -1),
                        wxSL_HORIZONTAL | wxSL_LABELS);
  p_fphi->SetToolTip("Where the oscillation starts, about the vertical axis");
  fg->Add(p_fphi, 0);
  p_fosc = ffield("Oscillation (deg)", "10", "Range turned during the exposure");
  p_fdist = ffield("Distance (mm)", "90", "Crystal to detector");
  p_fsource = new wxChoice(this, wxID_ANY);
  for (const char *c : {"Mo", "Cu", "Co"}) p_fsource->Append(c);
  p_fsource->SetSelection(0);
  fg->Add(new wxStaticText(this, wxID_ANY, "Source"), 0, wxALIGN_CENTER_VERTICAL);
  fg->Add(p_fsource, 0);
  frameBox->Add(fg, 0, wxALL, 6);
  p_fdark = new wxCheckBox(this, wxID_ANY, "Dark spots on light");
  p_fdark->SetValue(true);
  p_fnoise = new wxCheckBox(this, wxID_ANY, "Noise");
  p_fnoise->SetValue(true);
  p_flabel = new wxCheckBox(this, wxID_ANY, "Label spots");
  p_flabel->SetToolTip("hkl next to the 12 strongest spots");
  p_frings = new wxCheckBox(this, wxID_ANY, "Resolution rings");
  p_frings->SetToolTip("Circles at d = 3.0, 2.0, 1.5 and 1.0 A, labelled with d and 2 theta");
  for (wxCheckBox *c : {p_fdark, p_fnoise, p_flabel, p_frings}) frameBox->Add(c, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  left->Add(frameBox, 0);

  wxButton *calc = new wxButton(this, wxID_ANY, "Calculate");
  left->Add(calc, 0, wxALL, 6);
  wxButton *png = new wxButton(this, wxID_ANY, "Save PNG...");
  wxButton *csv = new wxButton(this, wxID_ANY, "Save CSV...");
  wxBoxSizer *save = new wxBoxSizer(wxHORIZONTAL);
  save->Add(png, 0, wxALL, 4);
  save->Add(csv, 0, wxALL, 4);
  left->Add(save, 0, wxALL, 2);
  left->AddStretchSpacer(1);
  left->Add(new wxButton(this, wxID_CLOSE, "Close"), 0, wxALIGN_RIGHT | wxALL, 6);
  top->Add(left, 0, wxEXPAND);

  wxBoxSizer *right = new wxBoxSizer(wxVERTICAL);
  p_panel = new DiffractionPanel(this, this);
  right->Add(p_panel, 1, wxEXPAND | wxALL, 4);
  p_plot = new PowderPanel(this, this);
  right->Add(p_plot, 2, wxEXPAND | wxALL, 4);
  p_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(560, 150), wxLC_REPORT | wxLC_SINGLE_SEL);
  const char *cols[] = {"h k l", "d (A)", "2theta", "|F|", "Mult.", "I rel"};
  for (int i = 0; i < 6; i++) p_list->InsertColumn(i, cols[i], wxLIST_FORMAT_LEFT, i ? 78 : 70);
  right->Add(p_list, 1, wxEXPAND | wxALL, 4);
  // The pointer and the status each have a panel of their own.
  wxPanel *pp = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
  wxBoxSizer *ps = new wxBoxSizer(wxVERTICAL);
  p_pointer = new wxStaticText(pp, wxID_ANY, "", wxDefaultPosition, wxSize(460, -1));
  ps->Add(p_pointer, 0, wxALL, 3);
  pp->SetSizer(ps);
  right->Add(pp, 0, wxEXPAND | wxALL, 4);
  wxPanel *sp = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
  wxBoxSizer *ss = new wxBoxSizer(wxVERTICAL);
  p_status = new wxStaticText(sp, wxID_ANY, "Press Calculate.", wxDefaultPosition, wxSize(460, -1));
  ss->Add(p_status, 0, wxALL, 3);
  sp->SetSizer(ss);
  right->Add(sp, 0, wxEXPAND | wxALL, 4);
  top->Add(right, 1, wxEXPAND);
  SetSizerAndFit(top);
  onPattern(p_periodic ? 1 : 0);
  powder->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { onPattern(1); });
  fibre->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { onPattern(0); });
  single->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { onPattern(2); });
  frame->Bind(wxEVT_RADIOBUTTON, [this](wxCommandEvent&) { onPattern(3); });
  p_rbFrame = frame;
  auto again = [this](wxCommandEvent&) { string why; if (p_frame && p_calc.IsOk()) calculate(why); };
  for (wxCheckBox *c : {p_fdark, p_fnoise, p_flabel, p_frings}) c->Bind(wxEVT_CHECKBOX, again);
  p_fsource->Bind(wxEVT_CHOICE, again);
  p_fphi->Bind(wxEVT_SCROLL_CHANGED, [this](wxScrollEvent&) { string why; if (p_frame) calculate(why); });
  meas->Bind(wxEVT_BUTTON, &DiffractionDialog::onMeasured, this);
  p_source->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { onSource(); });

  calc->Bind(wxEVT_BUTTON, &DiffractionDialog::onCalculate, this);
  back->Bind(wxEVT_BUTTON, &DiffractionDialog::onBackdrop, this);
  png->Bind(wxEVT_BUTTON, &DiffractionDialog::onSavePng, this);
  csv->Bind(wxEVT_BUTTON, &DiffractionDialog::onSaveCsv, this);
  p_opacity->Bind(wxEVT_SLIDER, &DiffractionDialog::onOpacity, this);
  FindWindow(wxID_CLOSE)->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Destroy(); });
  Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent&) { Destroy(); });
}

DiffractionDialog::~DiffractionDialog()
{
  std::error_code ec;
  if (!p_dir.empty()) fs::remove_all(p_dir, ec);
}

void DiffractionDialog::status(const wxString& text) { p_status->SetLabel(text); }

double DiffractionDialog::number(wxTextCtrl *c) const
{
  return parse(c->GetValue());
}

string DiffractionDialog::run(const vector<string>& args, int& status)
{
  string cmd;
#ifdef _WIN32
  fs::path py = fs::path(Ecce::ecceHome()) / "python" / "python.exe";
  cmd = "\"" + (fs::exists(py) ? py.string() : string("python.exe")) + "\" ";
#endif
  cmd += "\"" + tool() + "\"";
  for (const string& a : args) cmd += " \"" + a + "\"";

  Child *child = new Child;
  const long pid = wxExecute(wxString::FromUTF8(cmd.c_str()), wxEXEC_ASYNC, child);
  if (pid == 0) { delete child; status = -1; return "cannot start " + tool(); }
  string out;
  wxStopWatch watch;
  wxProgressDialog *pd = 0;
  bool aborted = false;
  while (!child->done) {
    out += drain(child->GetInputStream());
    out += drain(child->GetErrorStream());
    if (!pd && watch.Time() > 1000)
      pd = new wxProgressDialog("Diffraction", "Calculating the pattern...", 100, this,
                                wxPD_CAN_ABORT | wxPD_APP_MODAL);
    if (pd && !pd->Pulse() && !aborted) { aborted = true; wxProcess::Kill(pid, wxSIGTERM); }
    wxMilliSleep(20);
    wxTheApp->Yield(true);
  }
  out += drain(child->GetInputStream());
  out += drain(child->GetErrorStream());
  delete pd;
  status = aborted ? -2 : child->code;
  return out;
}

void DiffractionDialog::onPattern(int mode)
{
  if (!p_periodic) mode = 0;
  p_powder = mode == 1;
  p_single = mode == 2;
  p_frame = mode == 3;
  p_left->Show(p_fibreItems, mode == 0, true);
  p_left->Show(p_powderItems, p_powder, true);
  p_left->Show(p_layerItems, p_single, true);
  p_left->Show(p_frameItems, p_frame, true);
  p_panel->Show(!p_powder);
  p_plot->Show(p_powder);
  p_list->Show(p_powder);
  p_pointer->SetLabel("");
  Layout();
  GetSizer()->Fit(this);
}

void DiffractionDialog::onSource()
{
  static const char *w[] = {"1.5406", "0.7093", "1.7890"};
  if (p_source->GetSelection() < 3) p_pwave->SetValue(w[p_source->GetSelection()]);
  p_doublet->Enable(p_source->GetSelection() < 3);
}

wxString DiffractionDialog::setSource(int index, int doublet)
{
  p_source->SetSelection(index);
  onSource();
  if (doublet >= 0) p_doublet->SetValue(doublet != 0);
  return wxString::Format("doublet=%d enabled=%d", p_doublet->GetValue() ? 1 : 0, p_doublet->IsEnabled() ? 1 : 0);
}

bool DiffractionDialog::setMeasured(const string& path, string& why)
{
  std::ifstream f(path);
  if (!f) { why = "cannot read " + path; return false; }
  p_measured = path;
  why.clear();
  return true;
}

void DiffractionDialog::onMeasured(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Measured pattern (two columns: 2theta, intensity)", "", "",
                   "Text (*.txt;*.csv;*.xy;*.dat)|*.txt;*.csv;*.xy;*.dat|All files|*",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  string why;
  if (dlg.ShowModal() != wxID_OK) return;
  if (!setMeasured(dlg.GetPath().ToStdString(wxConvUTF8), why)) { status(wxString::FromUTF8(why.c_str())); return; }
  string w2;
  if (!p_tt.empty() && !calculatePowder(w2)) status(wxString::FromUTF8(w2.c_str()));
}

wxString DiffractionDialog::reflectionText(double tt, double tol) const
{
  const Line *best = 0;
  for (const Line& l : p_lines)
    if (l.rel >= 0.5 && std::fabs(l.tt - tt) <= tol && (!best || l.rel > best->rel)) best = &l;
  if (!best) return "";
  return wxString::Format("(%s)   d = %.3f A   2theta = %.2f   I = %.1f", best->hkl, best->d, best->tt, best->rel);
}

bool DiffractionDialog::calculatePowder(string& why)
{
  static const char *srcs[] = {"cu", "mo", "co"};
  const int src = p_source->GetSelection();
  const bool doublet = src >= 0 && src < 3 && p_doublet->GetValue();
  vector<string> a = {"powder", p_structure,
                      "--wavelength", num(number(p_pwave)),
                      "--tt-min", num(number(p_ptmin)), "--tt-max", num(number(p_ptmax)),
                      "--fwhm", num(number(p_pfwhm)), "--B", num(number(p_pb)),
                      "--out", p_png, "--out", p_csv, "--table", p_tab};
  if (!p_measured.empty()) { a.push_back("--measured"); a.push_back(p_measured); }
  if (doublet) { a.push_back("--source"); a.push_back(srcs[src]); }
  status("Calculating...");
  int st = 0;
  const string out = run(a, st);
  if (st != 0) {
    const size_t e = out.find("error: ");
    why = st == -2 ? "cancelled" :
        (e == string::npos ? "ecce-xrd failed: " + out.substr(0, 300)
                           : out.substr(e + 7, out.find('\n', e) - e - 7));
    status(wxString::FromUTF8(why.c_str()));
    return false;
  }
  p_tt.clear(); p_y.clear(); p_meas.clear(); p_lines.clear();
  string line;
  std::ifstream f(p_csv);
  std::getline(f, line);
  while (std::getline(f, line)) {
    double t, y, m = std::nan("");
    const int n = sscanf(line.c_str(), "%lf,%lf,%lf", &t, &y, &m);
    if (n < 2) continue;
    p_tt.push_back(t); p_y.push_back(y); p_meas.push_back(n == 3 ? m : std::nan(""));
  }
  if (p_measured.empty()) p_meas.clear();
  p_list->DeleteAllItems();
  std::ifstream g(p_tab);
  std::getline(g, line);
  while (std::getline(g, line)) {
    int h, k, l, mult;
    double d, tt, fa, rel;
    char which[8] = "";
    const int nf = sscanf(line.c_str(), "%d,%d,%d,%lf,%lf,%lf,%d,%lf,%7s", &h, &k, &l, &d, &tt, &fa, &mult, &rel, which);
    if (nf < 8) continue;
    Line ln = {tt, d, rel, wxString::Format("%d %d %d", h, k, l) + (nf == 9 ? wxString::Format("  %s", which) : wxString())};
    p_lines.push_back(ln);
    const long row = p_list->InsertItem(p_list->GetItemCount(), ln.hkl);
    p_list->SetItem(row, 1, wxString::Format("%.4f", d));
    p_list->SetItem(row, 2, wxString::Format("%.3f", tt));
    p_list->SetItem(row, 3, wxString::Format("%.2f", fa));
    p_list->SetItem(row, 4, wxString::Format("%d", mult));
    p_list->SetItem(row, 5, wxString::Format("%.2f", rel));
  }
  p_plot->Refresh();
  status(wxString::FromUTF8(out.substr(0, out.find('\n')).c_str()));
  return true;
}

bool DiffractionDialog::setLayer(const string& zone, int layer, double dmin, bool showAbsent,
                                 string& why)
{
  const int z = zone == "l" ? 0 : zone == "k" ? 1 : zone == "h" ? 2 : -1;
  if (!p_periodic) { why = "single crystal needs a periodic structure"; return false; }
  if (z < 0) { why = "zone is h, k or l"; return false; }
  p_zone->SetSelection(z);
  p_layerNo->SetValue(wxString::Format("%d", layer));
  p_lres->SetValue(wxString(num(dmin)));
  p_absent->SetValue(showAbsent);
  p_rbSingle->SetValue(true);
  onPattern(2);
  why.clear();
  return true;
}

wxString DiffractionDialog::spotText(int h, int k, int l) const
{
  for (const Spot& s : p_spots)
    if (s.h == h && s.k == k && s.l == l)
      return wxString::Format("(%d %d %d)   d = %.3f A   |F| = %.2f   I = %.4g%s", h, k, l, s.d, s.F,
                              s.I, s.absent ? "   (systematically absent)" : "");
  return "";
}

bool DiffractionDialog::setFrame(double phi, double osc, bool labels, bool rings, string& why)
{
  if (!p_periodic) { why = "detector frame needs a periodic structure"; return false; }
  p_fphi->SetValue((int)std::lround(phi));
  p_fosc->SetValue(wxString(num(osc)));
  p_flabel->SetValue(labels);
  p_frings->SetValue(rings);
  p_rbFrame->SetValue(true);
  onPattern(3);
  why.clear();
  return true;
}

wxString DiffractionDialog::framePointerText(int x, int y) const
{
  const Spot *best = 0;
  double bd = 10.0 * 10.0;
  for (const Spot& s : p_spots) {
    const double d2 = (s.px - x) * (s.px - x) + (s.py - y) * (s.py - y);
    if (d2 < bd) { bd = d2; best = &s; }
  }
  return best ? wxString::Format("(%d %d %d)   d = %.3f A   2theta = %.2f deg   I = %.4g", best->h,
                                 best->k, best->l, best->d, best->F, best->I)
              : wxString();
}

bool DiffractionDialog::calculateFrame(string& why)
{
  vector<string> a = {"frame", p_structure,
                      "--source", p_fsource->GetStringSelection().Lower().ToStdString(),
                      "--phi", std::to_string(p_fphi->GetValue()),
                      "--osc", num(number(p_fosc)), "--distance", num(number(p_fdist)),
                      "--pixels", std::to_string(p_layerSize),
                      "--out", p_png, "--out", p_csv, "--geometry", p_dir + "/geometry.txt"};
  if (!p_fnoise->IsChecked()) a.push_back("--no-noise");
  if (!p_fdark->IsChecked()) a.push_back("--dark");
  if (p_flabel->IsChecked()) { a.push_back("--label"); a.push_back("12"); }
  if (p_frings->IsChecked()) a.push_back("--rings");
  status("Calculating...");
  int st = 0;
  const string out = run(a, st);
  if (st != 0) {
    const size_t e = out.find("error: ");
    why = st == -2 ? "cancelled" :
        (e == string::npos ? "ecce-xrd failed: " + out.substr(0, 300)
                           : out.substr(e + 7, out.find('\n', e) - e - 7));
    status(wxString::FromUTF8(why.c_str()));
    return false;
  }
  if (!p_calc.LoadFile(wxString::FromUTF8(p_png.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot read the frame image";
    return false;
  }
  std::ifstream g(p_dir + "/geometry.txt");
  string key;
  double val;
  while (g >> key >> val) {
    if (key == "scale") p_layerScale = val;
    if (key == "size") p_layerSize = (int)val;
  }
  p_spots.clear();
  std::ifstream f(p_csv);
  string line;
  std::getline(f, line);
  while (std::getline(f, line)) {
    Spot s;
    double x, y, phi, tt;
    if (sscanf(line.c_str(), "%d,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf", &s.h, &s.k, &s.l, &x, &y, &phi,
               &tt, &s.d, &s.I) != 9) continue;
    s.px = p_layerSize / 2.0 + x * p_layerScale;
    s.py = p_layerSize / 2.0 - y * p_layerScale;
    s.F = tt;
    s.absent = false;
    p_spots.push_back(s);
  }
  p_back = wxImage();
  p_panel->setImage(p_calc);
  status(wxString::FromUTF8(out.substr(0, out.find('\n')).c_str()));
  return true;
}

wxString DiffractionDialog::layerPointerText(int x, int y) const
{
  const Spot *best = 0;
  double bd = 10.0 * 10.0;
  for (const Spot& s : p_spots) {
    const double d2 = (s.px - x) * (s.px - x) + (s.py - y) * (s.py - y);
    if (d2 < bd) { bd = d2; best = &s; }
  }
  return best ? spotText(best->h, best->k, best->l) : wxString();
}

bool DiffractionDialog::calculateLayer(string& why)
{
  static const char *zones[] = {"l", "k", "h"};
  const int layerNo = (int)std::lround(number(p_layerNo));
  vector<string> a = {"layer", p_structure,
                      "--zone", string(zones[std::max(0, p_zone->GetSelection())]) + "=" + std::to_string(layerNo),
                      "--resolution", num(number(p_lres)), "--B", num(number(p_lb)),
                      "--scale", p_lscale->GetStringSelection().ToStdString(),
                      "--size", std::to_string(p_layerSize),
                      "--out", p_png, "--out", p_csv, "--geometry", p_dir + "/geometry.txt"};
  if (p_absent->IsChecked()) a.push_back("--show-absent");
  status("Calculating...");
  int st = 0;
  const string out = run(a, st);
  if (st != 0) {
    const size_t e = out.find("error: ");
    why = st == -2 ? "cancelled" :
        (e == string::npos ? "ecce-xrd failed: " + out.substr(0, 300)
                           : out.substr(e + 7, out.find('\n', e) - e - 7));
    status(wxString::FromUTF8(why.c_str()));
    return false;
  }
  if (!p_calc.LoadFile(wxString::FromUTF8(p_png.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot read the layer image";
    return false;
  }
  std::ifstream g(p_dir + "/geometry.txt");
  string key;
  double val;
  while (g >> key >> val) {
    if (key == "scale") p_layerScale = val;
    if (key == "size") p_layerSize = (int)val;
  }
  p_spots.clear();
  std::ifstream f(p_csv);
  string line;
  std::getline(f, line);
  while (std::getline(f, line)) {
    Spot s;
    double x, y, irel;
    int ab;
    if (sscanf(line.c_str(), "%d,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%d", &s.h, &s.k, &s.l, &x, &y, &s.d,
               &s.F, &s.I, &irel, &ab) != 10) continue;
    s.px = p_layerSize / 2.0 + x * p_layerScale;
    s.py = p_layerSize / 2.0 - y * p_layerScale;
    s.absent = ab != 0;
    p_spots.push_back(s);
  }
  p_back = wxImage();
  p_panel->setImage(p_calc);
  status(wxString::FromUTF8(out.substr(0, out.find('\n')).c_str()));
  return true;
}

bool DiffractionDialog::calculate(string& why)
{
  if (p_single) return calculateLayer(why);
  if (p_frame) return calculateFrame(why);
  if (p_powder) return calculatePowder(why);
  vector<string> a = {"fibre", p_structure,
                      "--wavelength", num(number(p_wavelength)),
                      "--distance", num(number(p_distance)),
                      "--film-size", num(number(p_film)),
                      "--pixels", std::to_string(p_pixels),
                      "--B", num(number(p_b)),
                      "--length-broadening", num(number(p_length)),
                      "--misalignment", num(number(p_mis)),
                      "--scale", p_scale->GetStringSelection().ToStdString(),
                      "--decades", "2.5",
                      p_invert->IsChecked() ? "--invert" : "--no-invert",
                      "--gamma", num(number(p_gamma)),
                      "--out", p_png, "--out", p_csv};
  if (p_selectedOnly->IsChecked() && !p_selectionText.empty()) {
    a.push_back("--select");
    a.push_back(p_selectionText);
  }
  if (p_axis->GetSelection() == 1) {
    a.push_back("--axis");
    a.push_back("selection");
  }
  status("Calculating...");
  int st = 0;
  const string out = run(a, st);
  if (st != 0) {
    const size_t e = out.find("error: ");
    why = st == -2 ? "cancelled" :
        (e == string::npos ? "ecce-xrd failed: " + out.substr(0, 300)
                           : out.substr(e + 7, out.find('\n', e) - e - 7));
    status(wxString::FromUTF8(why.c_str()));
    return false;
  }
  if (!p_calc.LoadFile(wxString::FromUTF8(p_png.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot read the pattern image";
    return false;
  }
  p_r.clear(); p_z.clear(); p_i.clear();
  std::ifstream f(p_csv);
  string line;
  std::getline(f, line);
  while (std::getline(f, line)) {
    double x, y, r, z, i;
    if (sscanf(line.c_str(), "%lf,%lf,%lf,%lf,%lf", &x, &y, &r, &z, &i) != 5) continue;
    p_r.push_back((float)r); p_z.push_back((float)z); p_i.push_back((float)i);
  }
  wxCommandEvent none;
  onOpacity(none);
  status(wxString::FromUTF8(out.substr(0, out.find('\n')).c_str()));
  return true;
}

void DiffractionDialog::onOpacity(wxCommandEvent&)
{
  if (!p_calc.IsOk()) return;
  if (!p_back.IsOk()) { p_panel->setImage(p_calc); return; }
  const double a = p_opacity->GetValue() / 100.0;
  wxImage out(p_calc.GetWidth(), p_calc.GetHeight());
  const unsigned char *c = p_calc.GetData(), *b = p_back.GetData();
  unsigned char *o = out.GetData();
  for (int k = 0; k < p_calc.GetWidth() * p_calc.GetHeight() * 3; k++)
    o[k] = (unsigned char)(a * c[k] + (1 - a) * b[k] + 0.5);
  p_panel->setImage(out);
}

bool DiffractionDialog::setBackdrop(const string& path, string& why)
{
  wxImage im;
  if (!im.LoadFile(wxString::FromUTF8(path.c_str()))) { why = "cannot read " + path; return false; }
  p_back = im.Scale(p_pixels, p_pixels, wxIMAGE_QUALITY_BILINEAR);
  wxCommandEvent none;
  onOpacity(none);
  return true;
}

void DiffractionDialog::onBackdrop(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Backdrop image", "", "", "Images (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg",
                   wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  string why;
  if (dlg.ShowModal() == wxID_OK && !setBackdrop(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

wxString DiffractionDialog::pointerText(int x, int y) const
{
  if (p_single) return layerPointerText(x, y);
  if (p_frame) return framePointerText(x, y);
  const size_t k = (size_t)y * p_pixels + x;
  if (k >= p_i.size()) return "";
  const double r = p_r[k], z = p_z[k], s = std::hypot(r, z);
  const double rep = parse(p_repeat->GetValue());
  wxString t;
  if (rep > 0) t += wxString::Format("l = %.2f   ", z * rep);
  t += wxString::Format("R = %.4f   Z = %.4f /A", r, z);
  if (s > 0) t += wxString::Format("   d = %.2f A", 1.0 / s);
  t += wxString::Format("   I = %.3g", p_i[k]);
  return t;
}

bool DiffractionDialog::savePng(const string& path, string& why) const
{
  if (p_powder || p_single || p_frame) {
    std::error_code ec;
    if (!fs::exists(p_png)) { why = "nothing calculated yet"; return false; }
    fs::copy_file(p_png, path, fs::copy_options::overwrite_existing, ec);
    if (ec) { why = "cannot write " + path + ": " + ec.message(); return false; }
    return true;
  }
  if (!p_calc.IsOk()) { why = "nothing calculated yet"; return false; }
  wxImage out = p_calc;
  if (p_back.IsOk()) {
    const double a = p_opacity->GetValue() / 100.0;
    out = wxImage(p_calc.GetWidth(), p_calc.GetHeight());
    for (int k = 0; k < p_calc.GetWidth() * p_calc.GetHeight() * 3; k++)
      out.GetData()[k] = (unsigned char)(a * p_calc.GetData()[k] + (1 - a) * p_back.GetData()[k] + 0.5);
  }
  if (!out.SaveFile(wxString::FromUTF8(path.c_str()), wxBITMAP_TYPE_PNG)) {
    why = "cannot write " + path;
    return false;
  }
  return true;
}

bool DiffractionDialog::saveCsv(const string& path, string& why) const
{
  std::error_code ec;
  if (!fs::exists(p_csv)) { why = "nothing calculated yet"; return false; }
  fs::copy_file(p_csv, path, fs::copy_options::overwrite_existing, ec);
  if (ec) { why = "cannot write " + path + ": " + ec.message(); return false; }
  return true;
}

void DiffractionDialog::onSavePng(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Save the pattern", "", "diffraction.png", "PNG (*.png)|*.png",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  string why;
  if (dlg.ShowModal() == wxID_OK && !savePng(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

void DiffractionDialog::onSaveCsv(wxCommandEvent&)
{
  wxFileDialog dlg(this, "Save the pattern as CSV", "", "diffraction.csv", "CSV (*.csv)|*.csv",
                   wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
  string why;
  if (dlg.ShowModal() == wxID_OK && !saveCsv(dlg.GetPath().ToStdString(wxConvUTF8), why))
    status(wxString::FromUTF8(why.c_str()));
}

void DiffractionDialog::onCalculate(wxCommandEvent&)
{
  string why;
  if (!calculate(why))
    wxMessageBox(wxString::FromUTF8(why.c_str()), "Diffraction", wxOK | wxICON_ERROR, this);
}
