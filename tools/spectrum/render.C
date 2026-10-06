//  Paint a vibrational spectrum with the canvas builder itself uses.
//
//  WHY THIS EXISTS.  The layout of SpectrumCanvas can only be judged by
//  looking at it, and reaching it in builder takes a package, services
//  and a calculation.  This hands a model to the REAL SpectrumCanvas and
//  paints it onto a bitmap, so what comes out is what builder draws.
//  Needs a display (xvfb-run is enough).
//
//      render in.spec out.png [options]
//
//  in.spec, one mode per line after the unit lines:
//      ir_units KM/Mole
//      raman_units A^4/AMU
//      <frequency cm-1> <irrep or -> <IR or -> <Raman or ->
//
//  options:  --dark | --light       theme (default light)
//            --fwhm N               broadening, cm-1 (default 15; 0 = sticks)
//            --lorentzian           line shape
//            --scale S              frequency scaling factor
//            --zoom LO HI           show only this band
//            --forward              low wavenumbers on the left
//            --no-sticks            envelope only
//            --select M --hover M   highlight / hover mode M (1-based)
//            --hover-kind ir|raman  which pane the hover sits in
//            --compact-pane raman   in a short bitmap (< 320 px), show this pane
//            --size W H             bitmap size (default 1000 x 700)
//            --dump                 print the sticks the canvas holds:
//                                   stick <ir|raman> <mode> <cm-1> <intensity> <irrep> <x> <y>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/image.h>

#include "tdat/VibSpectrum.H"
#include "SpectrumCanvas.H"

class RenderApp : public wxApp
{
  public:
    virtual bool OnInit() { return true; }
};

IMPLEMENT_APP_NO_MAIN(RenderApp)


static bool readSpec(const char *path, VibSpectrum& spec)
{
  std::ifstream in(path);
  if (!in) return false;
  std::vector<double> freq, ir, raman;
  std::vector<std::string> irrep;
  bool haveIr = false, haveRaman = false;
  std::string irUnits, ramanUnits, line;
  while (std::getline(in, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream ls(line);
    std::string a, b, c, d;
    ls >> a;
    if (a == "ir_units") { ls >> irUnits; continue; }
    if (a == "raman_units") { ls >> ramanUnits; continue; }
    ls >> b >> c >> d;
    freq.push_back(atof(a.c_str()));
    irrep.push_back(b == "-" ? std::string() : b);
    ir.push_back(c == "-" ? 0.0 : atof(c.c_str()));
    raman.push_back(d == "-" ? 0.0 : atof(d.c_str()));
    if (c != "-" && !c.empty()) haveIr = true;
    if (d != "-" && !d.empty()) haveRaman = true;
  }
  spec.setModes(freq, irrep);
  if (haveIr) spec.setIntensities(VIB_IR, ir, irUnits);
  if (haveRaman) spec.setIntensities(VIB_RAMAN, raman, ramanUnits);
  return !freq.empty();
}


int main(int argc, char** argv)
{
  if (argc < 3) {
    fprintf(stderr, "usage: render in.spec out.png [options]  (see source)\n");
    return 2;
  }
  int width = 1000, height = 700;
  bool dark = false, forward = false, noSticks = false, dump = false;
  bool lorentz = false, zoom = false;
  double fwhm = 15, scale = 1, zlo = 0, zhi = 0;
  int select = 0, hover = 0;
  VibKind hoverKind = VIB_IR, compactPane = VIB_IR;
  for (int i = 3; i < argc; i++) {
    const std::string o = argv[i];
    if (o == "--dark") dark = true;
    else if (o == "--light") dark = false;
    else if (o == "--forward") forward = true;
    else if (o == "--no-sticks") noSticks = true;
    else if (o == "--dump") dump = true;
    else if (o == "--lorentzian") lorentz = true;
    else if (o == "--fwhm" && i + 1 < argc) fwhm = atof(argv[++i]);
    else if (o == "--scale" && i + 1 < argc) scale = atof(argv[++i]);
    else if (o == "--select" && i + 1 < argc) select = atoi(argv[++i]);
    else if (o == "--hover" && i + 1 < argc) hover = atoi(argv[++i]);
    else if (o == "--compact-pane" && i + 1 < argc)
      compactPane = std::string(argv[++i]) == "raman" ? VIB_RAMAN : VIB_IR;
    else if (o == "--hover-kind" && i + 1 < argc)
      hoverKind = std::string(argv[++i]) == "raman" ? VIB_RAMAN : VIB_IR;
    else if (o == "--zoom" && i + 2 < argc) {
      zoom = true; zlo = atof(argv[i + 1]); zhi = atof(argv[i + 2]); i += 2;
    } else if (o == "--size" && i + 2 < argc) {
      width = atoi(argv[i + 1]); height = atoi(argv[i + 2]); i += 2;
    } else {
      fprintf(stderr, "render: unknown option %s\n", o.c_str());
      return 2;
    }
  }

  VibSpectrum spec;
  if (!readSpec(argv[1], spec)) {
    fprintf(stderr, "render: cannot read %s\n", argv[1]);
    return 1;
  }

  wxEntryStart(argc, argv);
  wxApp::GetInstance()->CallOnInit();
  wxInitAllImageHandlers();

  wxFrame *frame = new wxFrame(NULL, wxID_ANY, "render", wxDefaultPosition,
                               wxSize(width, height));
  SpectrumCanvas *canvas = new SpectrumCanvas(frame);
  canvas->setPalette(dark ? SpectrumPalette::dark() : SpectrumPalette::light());
  canvas->setSpectrum(spec);
  canvas->setLineShape(lorentz ? LINE_LORENTZIAN : LINE_GAUSSIAN);
  canvas->setScale(scale);
  canvas->setFwhm(fwhm);
  canvas->setReversed(!forward);
  canvas->setShowSticks(!noSticks);
  canvas->setCompactPane(compactPane);
  if (zoom) canvas->axis().zoomTo(zlo, zhi);
  if (select > 0) canvas->setSelected(select - 1);
  canvas->SetSize(wxSize(width, height));

  wxBitmap bitmap(width, height, 24);
  {
    wxMemoryDC dc(bitmap);
    canvas->paintOnto(dc, wxSize(width, height));   // lays the sticks out
    if (hover > 0) {
      wxPoint at;
      if (canvas->stickPosition(hoverKind, hover - 1, &at)) {
        canvas->setHover(hoverKind, hover - 1, at);
        canvas->paintOnto(dc, wxSize(width, height));
      } else {
        fprintf(stderr, "render: mode %d is not drawn\n", hover);
      }
    }
    dc.SelectObject(wxNullBitmap);
  }
  bitmap.SaveFile(wxString(argv[2], wxConvUTF8), wxBITMAP_TYPE_PNG);

  //  How much is not background: the one thing a machine can say about a
  //  picture without a person looking at it.
  wxImage image = bitmap.ConvertToImage();
  const unsigned char br = image.GetRed(0, 0), bg = image.GetGreen(0, 0),
                      bb = image.GetBlue(0, 0);
  int ink = 0;
  for (int y = 0; y < image.GetHeight(); y++)
    for (int x = 0; x < image.GetWidth(); x++)
      if (image.GetRed(x, y) != br || image.GetGreen(x, y) != bg ||
          image.GetBlue(x, y) != bb) ink++;

  if (dump) {
    const VibSpectrum& s = canvas->spectrum();
    for (int k = 0; k < 2; k++) {
      const std::vector<VibStick> st = s.sticks((VibKind)k);
      for (size_t i = 0; i < st.size(); i++) {
        wxPoint at(-1, -1);
        canvas->stickPosition((VibKind)k, st[i].mode, &at);
        printf("stick %s %d %.8f %.10g %s %d %d%s\n",
               k == VIB_IR ? "ir" : "raman", st[i].mode + 1,
               st[i].wavenumber, st[i].intensity,
               st[i].irrep.empty() ? "-" : st[i].irrep.c_str(), at.x, at.y,
               st[i].imaginary ? " imaginary" : "");
      }
    }
  }
  printf("%s: %d ink\n", argv[2], ink);

  frame->Destroy();
  wxEntryCleanup();
  return 0;
}
