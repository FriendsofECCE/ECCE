#ifndef ECCE_STRESS_PANEL_H
#define ECCE_STRESS_PANEL_H

#include "VizPropertyPanel.H"
class wxGrid;
class wxStaticText;

class StressPanel : public VizPropertyPanel {
  DECLARE_DYNAMIC_CLASS(StressPanel)
public:
  StressPanel() = default;
  bool Create(IPropCalculation *calculation, wxWindow *parent,
              wxWindowID id=wxID_ANY, const wxPoint& pos=wxDefaultPosition,
              const wxSize& size=wxDefaultSize, long style=wxTAB_TRAVERSAL,
              const wxString& name="StressPanel") override;
  void refresh() override;
  string summaryText() const;
  int openingHeight() const override { return 440; }
protected:
  void initialize() override { refresh(); }
  void receiveFocus() override;
  void loseFocus() override;
private:
  void updateStress(bool draw);
  wxGrid *p_grid = nullptr;
  wxStaticText *p_summary = nullptr;
};
#endif
