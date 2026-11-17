#ifndef ECCE_NMR_PANEL_H
#define ECCE_NMR_PANEL_H
#include <map>
#include "VizPropertyPanel.H"
#include "SpectrumCanvas.H"
#include "tdat/NmrSpectrum.H"

class wxChoice;
class wxTextCtrl;
class wxStaticText;
class wxGrid;

class NmrPanel : public VizPropertyPanel, public SpectrumClickTarget {
  DECLARE_DYNAMIC_CLASS(NmrPanel)
public:
  NmrPanel() = default;
  bool Create(IPropCalculation *calculation, wxWindow *parent,
              wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
              const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
              const wxString& name="NmrPanel") override;
  void refresh() override;
  bool drawsInViewer() const override { return false; }
  int minimumHeight() const override { return 350; }
  int openingHeight() const override { return 500; }
  void modeClicked(int group) override;
  bool chooseElement(const std::string& element);
  bool setReference(const std::string& text);
  bool referenceCalculation(const std::string& name);
  bool setWidth(double ppm);
  bool exportCsv(const wxString& path) const;
  bool exportPng(const wxString& path);
  bool clickGroup(int group);
  std::string summary() const;
protected:
  void initialize() override { refresh(); }
private:
  struct Reference { double value; std::string source; };
  std::vector<IPropCalculation*> referenceCalculations() const;
  void referenceDialog();
  void update();
  void showReference();
  void exportDialog(bool png);
  std::string element() const;
  SpectrumCanvas *p_canvas = nullptr;
  wxChoice *p_element = nullptr;
  wxTextCtrl *p_reference = nullptr;
  wxTextCtrl *p_width = nullptr;
  wxStaticText *p_source = nullptr;
  wxGrid *p_grid = nullptr;
  NmrSpectrum p_data;
  std::map<std::string, Reference> p_references;
  std::vector<NmrGroup> p_groups;
  double p_fwhm = 0;
  bool p_filling = false;
  std::string p_error;
};
#endif
