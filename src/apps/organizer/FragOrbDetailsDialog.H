#ifndef FRAGORBDETAILSDIALOG_HH
#define FRAGORBDETAILSDIALOG_HH

#include <string>
#include <vector>

#include <wx/dialog.h>

#include "util/FragOrbRun.H"

class wxChoice;
class wxNotebook;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;
class XYPlotCanvas;
class FragDiagramCanvas;

/**
 * The Fragment orbitals box's Details... window (#201): every complex MO
 * with its FO shares, and the partial and overlap-population densities of
 * states (PDOS, OPDOS) from `ecce-fragorb dos`, and the interaction diagram
 * from `ecce-fragorb diagram` (FragOrbDiagram.H).  Export CSV writes the
 * whole composition matrix.
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
  /**
   * For the test hook: shows the Diagram page with the given window (0 keeps
   * it), then the on-screen layout (FragDiagramLayout::dump), the text a
   * click on level `click` shows (-1: none) and the PNG when `png` is given.
   */
  std::string diagramReport(int occupied, int virtuals, int click, const std::string& png);
  /// For the test hook: size, minimum size and the controls outside the client area.
  std::string fitReport() const;

private:
  void exportCsv();
  void runDiagram();
  void showDiagram();
  void showLevel(int level);
  void saveDiagramPng();

  FragOrbResult p_result;
  FragDosResult p_dos;
  FragDiagramResult p_diagram;
  std::string p_complex;
  std::vector<std::string> p_fragments;
  std::vector<std::string> p_names;
  wxNotebook *p_book;
  FragDiagramCanvas *p_canvas;
  wxSpinCtrl *p_occupied, *p_virtual, *p_threshold;
  wxChoice *p_spin;
  wxTextCtrl *p_level;
  wxStaticText *p_status;
  wxTextCtrl *p_text;
  XYPlotCanvas *p_pdos, *p_opdos;
};

#endif
