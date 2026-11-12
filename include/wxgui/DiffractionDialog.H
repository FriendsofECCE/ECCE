#ifndef DIFFRACTIONDIALOG_H
#define DIFFRACTIONDIALOG_H

#include <string>
#include <vector>
#include "wx/wx.h"

class wxProcess;
class DiffractionPanel;
class PowderPanel;
class wxListCtrl;

// Tools > Diffraction... (#971): the calculated diffraction pattern of a
// structure file, drawn from `ecce-xrd`.  Only the file and an optional atom
// selection go in, so it opens the same way from the Builder (the structure
// being edited) and from a calculation's results (the final geometry).  Fibre
// and powder (#587, periodic structures: an .xyz whose comment line has
// Lattice="...") are drawn; single crystal (#972) is shown disabled.
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
  bool savePng(const std::string& path, std::string& why) const;
  bool saveCsv(const std::string& path, std::string& why) const;
  bool setBackdrop(const std::string& path, std::string& why);
  // Powder: a measured pattern (two columns) drawn as a second curve.
  bool setMeasured(const std::string& path, std::string& why);
  bool powder() const { return p_powder; }
  // The reflection under a 2theta (degrees), as hovering shows it; "" if none.
  wxString reflectionText(double twoTheta, double tolerance) const;

private:
  friend class DiffractionPanel;
  friend class PowderPanel;
  std::string run(const std::vector<std::string>& args, int& status);
  void status(const wxString& text);
  void onCalculate(wxCommandEvent&);
  void onBackdrop(wxCommandEvent&);
  void onSavePng(wxCommandEvent&);
  void onSaveCsv(wxCommandEvent&);
  void onOpacity(wxCommandEvent&);
  void onPattern(bool powder);
  void onMeasured(wxCommandEvent&);
  bool calculatePowder(std::string& why);
  double number(wxTextCtrl *c) const;
  wxString pointerText(int x, int y) const;

  std::string p_structure, p_dir, p_selectionText;
  wxTextCtrl *p_wavelength, *p_distance, *p_film, *p_b, *p_length, *p_mis,
             *p_repeat, *p_gamma;
  wxChoice *p_axis, *p_scale;
  wxCheckBox *p_selectedOnly, *p_invert;
  wxSlider *p_opacity;
  wxStaticText *p_status, *p_pointer;
  DiffractionPanel *p_panel;
  wxImage p_calc, p_back;
  std::vector<float> p_r, p_z, p_i;
  int p_pixels;
  std::string p_png, p_csv, p_tab, p_measured;
  bool p_periodic, p_powder;
  wxTextCtrl *p_pwave, *p_ptmin, *p_ptmax, *p_pfwhm, *p_pb;
  wxChoice *p_source;
  PowderPanel *p_plot;
  wxListCtrl *p_list;
  wxSizer *p_left, *p_fibreItems, *p_powderItems;
  std::vector<double> p_tt, p_y, p_meas;
  struct Line { double tt, d, rel; wxString hkl; };
  std::vector<Line> p_lines;
};

#endif
