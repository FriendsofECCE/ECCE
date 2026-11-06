#ifndef FRAGORBDETAILSDIALOG_HH
#define FRAGORBDETAILSDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

#include "util/FragOrbRun.H"

class wxStaticText;
class wxTextCtrl;
class XYPlotCanvas;

/**
 * The Fragment orbitals box's Details... window (#201): every complex MO
 * with its FO shares, and the partial and overlap-population densities of
 * states (PDOS, OPDOS) from `ecce-fragorb dos`.  Export CSV writes the whole
 * composition matrix.
 */
class FragOrbDetailsDialog : public wxDialog
{
public:
  FragOrbDetailsDialog(wxWindow *parent, const std::string& complex,
                       const std::vector<std::string>& fragments,
                       const std::vector<std::string>& names, const std::string& heading);

  /// For the test hook: the table, the plots' curves and the status line.
  std::string report() const;
  bool saveCsv(const std::string& file) const;

private:
  void exportCsv();

  FragOrbResult p_result;
  FragDosResult p_dos;
  std::vector<std::string> p_names;
  wxStaticText *p_status;
  wxTextCtrl *p_text;
  XYPlotCanvas *p_pdos, *p_opdos;
};

#endif
