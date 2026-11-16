#ifndef DIFFRACTIONDIALOG_H
#define DIFFRACTIONDIALOG_H

#include <string>
#include <vector>
#include <functional>
#include <wx/timer.h>
#include <wx/stopwatch.h>
#include "wx/wx.h"

class wxProcess;
class wxProcessEvent;
class DiffractionPanel;
class PowderPanel;
class wxListCtrl;
class wxScrolledWindow;
class wxGraphicsContext;

// One calculation and settings owner, movable between the property pane and window.
class DiffractionView : public wxPanel
{
public:
  // structure: an .xyz, .pdb or .mvm file; selection: atom numbers from 1,
  // empty for all atoms.  The file must outlive the dialog.
  DiffractionView(wxWindow *parent, const std::string& structure,
                    const std::vector<int>& selection = std::vector<int>());
  ~DiffractionView();

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
  bool powder() const { return p_powder && !p_debye; }
  // The orientational average of all atoms (cell contents when periodic).
  void setDebye();
  bool debye() const { return p_debye; }
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
  // Exercise image navigation through mouse/key events and save view snapshots (test hook).
  bool testImageView(const std::string& prefix, std::string& why);
  // Window and display client-area size (test hook).
  wxString sizeText() const;
  // Powder cursor readout at a 2theta (degrees): 2theta, d, Q and I of the
  // curve, plus the reflection within tolerance if there is one.
  wxString cursorText(double twoTheta, double tolerance) const;
  // The reflection under a 2theta (degrees), as hovering shows it; "" if none.
  wxString reflectionText(double twoTheta, double tolerance) const;

  void requestCalculation();
  bool waitForResult(std::string& why);
  void showOptions();
  void setBreakoutHandler(const std::function<void(bool)>& handler);
  wxString script(const std::vector<std::string>& args);

private:
  void startCalculation();
  void cancelCalculation();
  bool launch(const std::vector<std::string>& args,
              const std::function<bool(std::string&)>& load, std::string& why);
  void processEnded(wxProcessEvent& event);
  void pollRun();
  bool prepareFibre(std::string& why);
  void restoreDefaults();
  wxTimer p_debounce, p_poll;
  wxStopWatch p_watch;
  wxProcess *p_process = nullptr;
  long p_pid = 0;
  unsigned p_generation = 0, p_completed = 0;
  bool p_dirty = true, p_manual = false, p_loading = true;
  std::string p_output, p_error, p_runDir;
  std::function<bool(std::string&)> p_load;
  std::function<void(bool)> p_breakout;
  wxDialog *p_options;
  std::vector<std::pair<wxSizer*, int>> p_optionRows;
  std::vector<std::pair<wxTextCtrl*, wxString>> p_defaults;
  wxChoice *p_view;
  class wxToggleButton *p_pattern;
  wxButton *p_update, *p_experimental;
  wxTextCtrl *p_step;
  class wxPopupWindow *p_tip = nullptr;
  wxString p_tipText;
  bool p_pinned = false;
  void showTip(wxWindow *canvas, const wxPoint& pos, const wxString& text, bool pin = false);
  void clearTip();
  void selectReflection(double tt, double tolerance);
  double p_inferredRepeat = 0;
  friend class DiffractionDialog;
  friend class DiffractionPanel;
  friend class PowderPanel;
  void onSource();
  void paint(wxGraphicsContext *gc, double w, double h) const;
  void paintPowder(wxGraphicsContext *gc, double w, double h) const;
  void paintImage(wxGraphicsContext *gc, double w, double h) const;
  wxBitmap render(int w, int h, int scale) const;
  wxRect imageArea(const wxSize& s) const;
  int imagePixels() const;
  void status(const wxString& text);
  void onCalculate(wxCommandEvent&);
  void onBackdrop(wxCommandEvent&);
  void onSavePng(wxCommandEvent&);
  void onSaveCsv(wxCommandEvent&);
  void onOpacity(wxCommandEvent&);
  void onPattern(int mode);   // 0 fibre, 1 powder, 2 single crystal, 3 frame, 4 Debye
  bool calculateLayer(std::string& why);
  bool calculateFrame(std::string& why);
  wxString framePointerText(double x, double y) const;
  wxString layerPointerText(double x, double y) const;
  void onMeasured(wxCommandEvent&);
  bool calculatePowder(std::string& why);
  double number(wxTextCtrl *c) const;
  wxString pointerText(double x, double y) const;
  void updateImage();
  void setLevels(const wxImage& im, bool inverted);
  void renderFilm();
  bool darkScheme() const;
  void onImageClick(double x, double y, bool second);

  std::string p_structure, p_dir, p_selectionText;
  wxTextCtrl *p_wavelength, *p_distance, *p_film, *p_b, *p_length, *p_mis,
             *p_repeat, *p_gamma;
  wxChoice *p_axis, *p_scale, *p_transform;
  wxCheckBox *p_selectedOnly;
  wxSlider *p_opacity;
  wxStaticText *p_status, *p_pointer;
  DiffractionPanel *p_panel;
  wxImage p_calc, p_back;
  std::vector<float> p_r, p_z, p_i;
  int p_pixels;
  std::string p_png, p_csv, p_tab, p_measured;
  bool p_periodic, p_powder, p_single, p_frame, p_debye;
  wxTextCtrl *p_layerNo, *p_lres, *p_lb;
  wxChoice *p_zone;
  wxCheckBox *p_absent, *p_doublet;
  wxSlider *p_fphi;
  wxTextCtrl *p_fosc, *p_fdist;
  wxCheckBox *p_fnoise, *p_flabel, *p_frings;
  double p_layerScale;
  int p_layerSize;
  struct Spot { double px, py; int h, k, l; double d, F, I; bool absent; double rel; };
  bool p_layerReady;
  double p_layerRmax, p_geomDistance, p_geomLambda, p_axes[3][2];
  std::vector<Spot> p_spots;
  wxString measureText(const Spot& a, const Spot* b) const;
  const Spot *spotNear(double x, double y) const;
  wxScrolledWindow *p_leftWin;
  class wxSpinCtrl *p_plabel;
  wxChoice *p_scheme;
  wxSlider *p_exposure;
  wxCheckBox *p_lhkl, *p_laxes;
  wxStaticText *p_measure;
  std::vector<float> p_level;   // fibre, frame: display level 0..1; layer: optical density
  int p_levelN;
  const Spot *p_pick;           // first spot of a measurement
  wxTextCtrl *p_pwave, *p_ptmin, *p_ptmax, *p_pfwhm, *p_pb;
  wxChoice *p_source;
  PowderPanel *p_plot;
  wxListCtrl *p_list;
  wxSizer *p_left;
  std::vector<double> p_tt, p_y, p_meas;
  struct Line { double tt, d, rel; wxString hkl; int h, k, l, mult; bool a2; };
  std::vector<Line> p_lines;
};

class DiffractionDialog : public wxDialog
{
public:
  DiffractionDialog(wxWindow *parent, const std::string& structure,
                    const std::vector<int>& selection = std::vector<int>());
  ~DiffractionDialog();
  void dockTo(wxWindow *host);
  void breakout(bool open);
  DiffractionView *view() const { return p_view; }
  bool calculate(std::string& why) { return p_view->calculate(why); }
  bool savePng(const std::string& path, std::string& why) const { return p_view->savePng(path, why); }
  bool saveCsv(const std::string& path, std::string& why) const { return p_view->saveCsv(path, why); }
  bool setBackdrop(const std::string& path, std::string& why) { return p_view->setBackdrop(path, why); }
  bool setMeasured(const std::string& path, std::string& why) { return p_view->setMeasured(path, why); }
  wxString setSource(int index, int doublet = -1) { return p_view->setSource(index, doublet); }
  bool powder() const { return p_view->powder(); }
  void setDebye() { p_view->setDebye(); }
  bool debye() const { return p_view->debye(); }
  bool setLayer(const std::string& zone, int layer, double dmin, bool showAbsent, std::string& why) { return p_view->setLayer(zone, layer, dmin, showAbsent, why); }
  bool singleCrystal() const { return p_view->singleCrystal(); }
  bool setFrame(double phi, double osc, bool labels, bool rings, std::string& why) { return p_view->setFrame(phi, osc, labels, rings, why); }
  bool detectorFrame() const { return p_view->detectorFrame(); }
  wxString spotText(int h, int k, int l) const { return p_view->spotText(h, k, l); }
  void setDisplay(int exposure, int scheme) { p_view->setDisplay(exposure, scheme); }
  wxString measure(int h1, int k1, int l1, int h2, int k2, int l2) { return p_view->measure(h1, k1, l1, h2, k2, l2); }
  wxString measureStrongest() { return p_view->measureStrongest(); }
  bool testImageView(const std::string& prefix, std::string& why) { return p_view->testImageView(prefix, why); }
  wxString sizeText() const { return p_view->sizeText(); }
  wxString cursorText(double twoTheta, double tolerance) const { return p_view->cursorText(twoTheta, tolerance); }
  wxString reflectionText(double twoTheta, double tolerance) const { return p_view->reflectionText(twoTheta, tolerance); }
private:
  DiffractionView *p_view;
  wxWindow *p_dock = nullptr;
};

#endif
