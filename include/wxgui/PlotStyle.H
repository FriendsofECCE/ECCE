/**
 * @file
 *
 * The look every plot in ECCE shares: the palette of the vibrational
 * spectrum canvas (#214) and the fonts and line weights around it.  Header
 * only, so tools/spectrum/render can use it without linking the wx
 * widgets; ewxPlotCtrl::ApplyStyle() applies it to the older plot control.
 */

#ifndef PLOTSTYLE_H_
#define PLOTSTYLE_H_

#include <wx/colour.h>
#include <wx/settings.h>

/**
 * Colours for one theme, shared by every plot in ECCE so they all look
 * like the vibrational spectrum (#214, #210).  The series colours are
 * Okabe-Ito.
 */
struct PlotPalette
{
  wxColour background, text, axis, grid, ir, raman, imaginary, box;
  bool dark_;

  static PlotPalette make(bool dark, const wxColour& bg,
                              const wxColour& fg)
  {
    PlotPalette p;
    p.dark_ = dark;
    p.background = bg;
    p.text = fg;
    p.axis = mix(fg, bg, 0.25);
    p.grid = mix(fg, bg, 0.88);
    p.box = mix(fg, bg, 0.94);
    p.ir = dark ? wxColour(86, 180, 233) : wxColour(0, 114, 178);
    p.raman = dark ? wxColour(64, 208, 160) : wxColour(0, 128, 96);
    p.imaginary = dark ? wxColour(255, 138, 80) : wxColour(200, 80, 0);
    return p;
  }
  static PlotPalette light()
  {
    return make(false, wxColour(255, 255, 255), wxColour(30, 30, 30));
  }
  static PlotPalette dark()
  {
    return make(true, wxColour(36, 36, 36), wxColour(228, 228, 228));
  }
  /** The window's own colours, so the plot matches the GTK theme. */
  static PlotPalette system()
  {
    const wxColour bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    const wxColour fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    return make(bg.Red() + bg.Green() + bg.Blue() < 3 * 128, bg, fg);
  }

  /**
   * The i-th curve colour when a plot has several, Okabe-Ito and
   * distinguishable without hue.  The first is the spectrum's IR blue.
   */
  wxColour series(size_t i) const
  {
    static const unsigned char light[6][3] = {
      {0, 114, 178}, {213, 94, 0}, {0, 128, 96},
      {204, 121, 167}, {230, 159, 0}, {86, 180, 233} };
    static const unsigned char darkc[6][3] = {
      {86, 180, 233}, {255, 138, 80}, {64, 208, 160},
      {230, 160, 200}, {240, 190, 60}, {150, 200, 240} };
    const unsigned char *c = dark_ ? darkc[i % 6] : light[i % 6];
    return wxColour(c[0], c[1], c[2]);
  }

  /** `t` of the way from a to b. */
  static wxColour mix(const wxColour& a, const wxColour& b, double t)
  {
    return wxColour((unsigned char)(a.Red() + t * (b.Red() - a.Red())),
                    (unsigned char)(a.Green() + t * (b.Green() - a.Green())),
                    (unsigned char)(a.Blue() + t * (b.Blue() - a.Blue())));
  }
};

/** The spectrum code calls its palette by the name it had first. */
typedef PlotPalette SpectrumPalette;

#endif
