#ifndef PLOTANYPROPERTYPANEL_H_
#define PLOTANYPROPERTYPANEL_H_

#include <map>
  using std::map;

#include "PropertyPanel.H"

class ewxPlotCtrl;

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
    void refreshAxisLabel(ewxChoice *axis);

  private:
    ewxPlotCtrl *p_plotCtrl;
    ewxChoice *p_xaxis;
    ewxChoice *p_yaxis;
};

#endif // PLOTANYPROPERTYPANEL_H_
