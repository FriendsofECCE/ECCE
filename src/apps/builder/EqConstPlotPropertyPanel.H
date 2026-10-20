#ifndef EQUILIBRIUMCONSTANTPLOTPROPERTYPANEL_H_
#define EQUILIBRIUMCONSTANTPLOTPROPERTYPANEL_H_

#include "PropertyPanel.H"

class ewxChoice;
class XYPlotCanvas;

class EquilibriumConstantPlotPropertyPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(EquilibriumConstantPlotPropertyPanel)
  DECLARE_EVENT_TABLE()

  public:
    static const wxWindowID ID_BUTTON_REFERENCE;

    EquilibriumConstantPlotPropertyPanel();
    EquilibriumConstantPlotPropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="EquilibriumConstantPlotPropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="EquilibriumConstantPlotPropertyPanel");
    virtual ~EquilibriumConstantPlotPropertyPanel();

    virtual void refresh();

  protected:
    virtual void initialize();

  private:
    XYPlotCanvas *p_plotCtrl;
};

#endif // EQUILIBRIUMCONSTANTPLOTPROPERTYPANEL_H_
