#ifndef DIFFRACTIONDIALOG_H
#define DIFFRACTIONDIALOG_H

#include <string>
#include <vector>
#include "wx/wx.h"

class wxProcess;
class DiffractionPanel;
class PowderPanel;
class wxListCtrl;
class wxGraphicsContext;

// Tools > Diffraction... (#971): the calculated diffraction pattern of a
// structure file, drawn from `ecce-xrd`.  Only the file and an optional atom
// selection go in, so it opens the same way from the Builder (the structure
// being edited) and from a calculation's results (the final geometry).  Fibre
// and powder (#587, periodic structures: an .xyz whose comment line has
// Lattice="...") are drawn, and so are single-crystal layers (#972).
class DiffractionDialog : public wxDialog
{
public:
  // structure: an .xyz, .pdb or .mvm file; selection: atom numbers from 1,
  // empty for all atoms.  The file must outlive the dialog.
  DiffractionDialog(wxWindow *parent, const std::string& structure,
                    const std::vector<int>& selection = std::vector<int>());
  ~DiffractionDialog();

  // Runs ecce-xrd with the current fields; false and why on failure.
  bool calculate(std::string& why);
  // The pattern as shown (backdrop included) / as ecce-xrd wrote it.
  // Save PNG writes what the window draws (the system font, anti-aliased).
  bool savePng(const std::string& path, std::string& why) const;
  bool saveCsv(const std::string& path, std::string& why) const;
  bool setBackdrop(const std::string& path, std::string& why);
  // Powder: a measured pattern (two columns) drawn as a second curve.
  bool setMeasured(const std::string& path, std::string& why);
  // Powder source choice (0 Cu, 1 Mo, 2 Co, 3 other) and, if doublet >= 0,
  // the Ka1/Ka2 box; returns its state "doublet=0|1 enabled=0|1" (test hook).
  wxString setSource(int index, int doublet = -1);
  bool powder() const { return p_powder; }
  // Single crystal (#972): a reciprocal-lattice layer.  zone is "l" (hk0 for
  // layer 0), "k" (h0l) or "h" (0kl); false and why if the structure is not
  // periodic or the zone is unknown.  Switches the dialog to that pattern.
  bool setLayer(const std::string& zone, int layer, double dmin, bool showAbsent,
                std::string& why);
  bool singleCrystal() const { return p_single; }
  // Detector frame (#985): a rotating crystal, start angle and oscillation in
  // degrees, optional hkl labels and resolution rings.  Switches to that view.
  bool setFrame(double phi, double osc, bool labels, bool rings, std::string& why);
  bool detectorFrame() const { return p_frame; }
  // The spot (h k l) as hovering reports it; "" if it is not in the layer.
  wxString spotText(int h, int k, int l) const;
  // Display of the image views (#985): exposure in tenths of a stop (-20 to 20,
  // an intensity gain before saturation; the data are unchanged) and the
  // colour scheme (0 Film: dark on light, 1 Dark: black, blue, orange, white).
  void setDisplay(int exposure, int scheme);
  // Measuring (#985), as two clicks do it: the readout after picking the spots
  // h1 k1 l1 and then h2 k2 l2 of the detector frame (distance, angle) or the
  // single-crystal layer (reciprocal distance, row d, a*); "" if a spot is missing.
  wxString measure(int h1, int k1, int l1, int h2, int k2, int l2);
  // The same for the two strongest spots (frame or layer).
  wxString measureStrongest();
  // Powder cursor readout at a 2theta (degrees): 2theta, d, Q and I of the
  // curve, plus the reflection within tolerance if there is one.
  wxString cursorText(double twoTheta, double tolerance) const;
  // The reflection under a 2theta (degrees), as hovering shows it; "" if none.
  wxString reflectionText(double twoTheta, double tolerance) const;

private:
  friend class DiffractionPanel;
  friend class PowderPanel;
  void onSource();
  void paint(wxGraphicsContext *gc, double w, double h) const;
  void paintPowder(wxGraphicsContext *gc, double w, double h) const;
  void paintImage(wxGraphicsContext *gc, double w, double h) const;
  wxBitmap render(int w, int h, int scale) const;
  wxRect imageArea(const wxSize& s) const;
  int imagePixels() const;
  std::string run(const std::vector<std::string>& args, int& status);
  void status(const wxString& text);
  void onCalculate(wxCommandEvent&);
  void onBackdrop(wxCommandEvent&);
  void onSavePng(wxCommandEvent&);
  void onSaveCsv(wxCommandEvent&);
  void onOpacity(wxCommandEvent&);
  void onPattern(int mode);   // 0 fibre, 1 powder, 2 single crystal
  bool calculateLayer(std::string& why);
  bool calculateFrame(std::string& why);
  wxString framePointerText(int x, int y) const;
  wxString layerPointerText(int x, int y) const;
  void onMeasured(wxCommandEvent&);
  bool calculatePowder(std::string& why);
  double number(wxTextCtrl *c) const;
  wxString pointerText(int x, int y) const;
  void updateImage();
  void setLevels(const wxImage& im, bool inverted);
  void renderFilm();
  bool darkScheme() const;
  void onImageClick(int x, int y, bool second);

  std::string p_structure, p_dir, p_selectionText;
  wxTextCtrl *p_wavelength, *p_distance, *p_film, *p_b, *p_length, *p_mis,
             *p_repeat, *p_gamma;
  wxChoice *p_axis, *p_scale;
  wxCheckBox *p_selectedOnly;
  wxSlider *p_opacity;
  wxStaticText *p_status, *p_pointer;
  DiffractionPanel *p_panel;
  wxImage p_calc, p_back;
  std::vector<float> p_r, p_z, p_i;
  int p_pixels;
  std::string p_png, p_csv, p_tab, p_measured;
  bool p_periodic, p_powder, p_single, p_frame;
  wxTextCtrl *p_layerNo, *p_lres, *p_lb;
  wxChoice *p_zone;
  wxCheckBox *p_absent, *p_doublet;
  wxRadioButton *p_rbFibre, *p_rbPowder, *p_rbSingle;
  wxSizer *p_layerItems, *p_frameItems;
  wxSlider *p_fphi;
  wxTextCtrl *p_fosc, *p_fdist;
  wxChoice *p_fsource;
  wxCheckBox *p_fnoise, *p_flabel, *p_frings;
  wxRadioButton *p_rbFrame;
  double p_layerScale;
  int p_layerSize;
  struct Spot { double px, py; int h, k, l; double d, F, I; bool absent; double rel; };
  bool p_layerReady;
  double p_layerRmax, p_geomDistance, p_geomLambda, p_axes[3][2];
  std::vector<Spot> p_spots;
  wxString measureText(const Spot& a, const Spot* b) const;
  const Spot *spotNear(int x, int y) const;
  wxChoice *p_scheme;
  wxSlider *p_exposure;
  wxSizer *p_displayItems;
  wxCheckBox *p_lhkl, *p_laxes;
  wxStaticText *p_measure;
  std::vector<float> p_level;   // fibre, frame: display level 0..1; layer: optical density
  int p_levelN;
  const Spot *p_pick;           // first spot of a measurement
  wxTextCtrl *p_pwave, *p_ptmin, *p_ptmax, *p_pfwhm, *p_pb;
  wxChoice *p_source;
  PowderPanel *p_plot;
  wxListCtrl *p_list;
  wxSizer *p_left, *p_fibreItems, *p_powderItems;
  std::vector<double> p_tt, p_y, p_meas;
  struct Line { double tt, d, rel; wxString hkl; int h, k, l; bool a2; };
  std::vector<Line> p_lines;
};

#endif
