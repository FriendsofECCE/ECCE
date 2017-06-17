#ifndef VECTORPLOTPROPERTYPANEL_H_
#define VECTORPLOTPROPERTYPANEL_H_

#include "PropertyPanel.H"

class wxPlotCtrl;

class VectorPlotPropertyPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(VectorPlotPropertyPanel)

  public:
    VectorPlotPropertyPanel();
    VectorPlotPropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="VectorPlotPropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="VectorPlotPropertyPanel");
    virtual ~VectorPlotPropertyPanel();

  private:
    wxPlotCtrl *p_plotCtrl;
};

#endif // VECTORPLOTPROPERTYPANEL_H_
