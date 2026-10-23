#ifndef PLOTANYPROPERTYPANEL_H_
#define PLOTANYPROPERTYPANEL_H_

#include <map>
  using std::map;

#include "PropertyPanel.H"

class XYPlotCanvas;

class PlotAnyPropertyPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(PlotAnyPropertyPanel)
  DECLARE_EVENT_TABLE()

  public:
    PlotAnyPropertyPanel();
    PlotAnyPropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PlotAnyPropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="PlotAnyPropertyPanel");
    virtual ~PlotAnyPropertyPanel();

    virtual void refresh();

  protected:
    virtual void initialize();
    void refreshChoices(const wxString& lastX, const wxString& lastY);

    void OnAxisChoice(wxCommandEvent &event);
    void doAxisChoice();

     int getSelectionSize(ewxChoice *axis);
    void getSelectionData(ewxChoice *axis, double *values);
    std::string getSelectionKey(ewxChoice *axis);
    std::string getSelectionUnits(ewxChoice *axis);
    void refreshAxisLabel(ewxChoice *axis);

  private:
    XYPlotCanvas *p_plotCtrl;
    ewxChoice *p_xaxis;
    ewxChoice *p_yaxis;
    wxString p_timeUnit;   // set while the step axis is simulation time
};

#endif // PLOTANYPROPERTYPANEL_H_
