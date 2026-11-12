#include "wxgui/DiffractionDialog.H"

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <locale>
#include <sstream>

#include <wx/dcbuffer.h>
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


DiffractionDialog::DiffractionDialog(wxWindow *parent, const string& structure,
                                     const vector<int>& selection)
  : wxDialog(parent, wxID_ANY, "Diffraction", wxDefaultPosition, wxDefaultSize,
             wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    p_structure(structure), p_pixels(PIXELS)
{
  static int serial = 0;
  std::error_code ec;
  p_dir = (fs::temp_directory_path() / ("ecce-xrd-" + std::to_string(wxGetProcessId()) + "-" +
                                        std::to_string(serial++))).string();
  fs::create_directories(p_dir, ec);
  p_png = p_dir + "/pattern.png";
  p_csv = p_dir + "/pattern.csv";

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
  fibre->SetValue(true);
  wxRadioButton *powder = new wxRadioButton(this, wxID_ANY, "Powder (periodic structures only)");
  wxRadioButton *single = new wxRadioButton(this, wxID_ANY, "Single crystal (periodic structures only)");
  powder->Enable(false);
  single->Enable(false);
  type->Add(fibre, 0, wxALL, 3);
  type->Add(powder, 0, wxALL, 3);
  type->Add(single, 0, wxALL, 3);
  left->Add(type, 0, wxEXPAND | wxALL, 4);

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
  left->Add(grid, 0, wxALL, 6);
  p_invert = new wxCheckBox(this, wxID_ANY, "Dark spots on white");
  p_invert->SetValue(true);
  left->Add(p_invert, 0, wxLEFT | wxRIGHT, 6);
  p_selectedOnly = new wxCheckBox(this, wxID_ANY, "Selected atoms only");
  p_selectedOnly->SetValue(!selection.empty());
  p_selectedOnly->Enable(!selection.empty());
  left->Add(p_selectedOnly, 0, wxALL, 6);

  wxButton *calc = new wxButton(this, wxID_ANY, "Calculate");
  left->Add(calc, 0, wxALL, 6);
  wxButton *back = new wxButton(this, wxID_ANY, "Load Backdrop...");
  left->Add(back, 0, wxLEFT | wxRIGHT | wxTOP, 6);
  left->Add(new wxStaticText(this, wxID_ANY, "Calculated pattern opacity"), 0, wxLEFT | wxTOP, 6);
  p_opacity = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxSize(200, -1));
  left->Add(p_opacity, 0, wxLEFT | wxRIGHT, 6);
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

bool DiffractionDialog::calculate(string& why)
{
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
