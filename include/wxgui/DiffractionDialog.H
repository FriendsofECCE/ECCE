#ifndef DIFFRACTIONDIALOG_H
#define DIFFRACTIONDIALOG_H

#include <string>
#include <vector>
#include "wx/wx.h"

class wxProcess;
class DiffractionPanel;

// Tools > Diffraction... (#971): the calculated diffraction pattern of a
// structure file, drawn from `ecce-xrd`.  Only the file and an optional atom
// selection go in, so it opens the same way from the Builder (the structure
// being edited) and from a calculation's results (the final geometry).  Fibre
// is the only pattern type so far; powder (#587) and single crystal (#972)
// are shown disabled.
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

private:
  friend class DiffractionPanel;
  std::string run(const std::vector<std::string>& args, int& status);
  void status(const wxString& text);
  void onCalculate(wxCommandEvent&);
  void onBackdrop(wxCommandEvent&);
  void onSavePng(wxCommandEvent&);
  void onSaveCsv(wxCommandEvent&);
  void onOpacity(wxCommandEvent&);
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
  std::string p_png, p_csv;
};

#endif
