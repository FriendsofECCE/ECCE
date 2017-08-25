#ifndef CALCINFOPANEL_H_
#define CALCINFOPANEL_H_

#include "PropertyPanel.H"

class CalcInfoPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(CalcInfoPanel)

  public:
    CalcInfoPanel();
    CalcInfoPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="CalcInfoPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="CalcInfoPanel");
    virtual ~CalcInfoPanel();

    virtual void refresh();

  protected:
    virtual void initialize();

    static wxString secondsToString(int sec);
};

#endif // CALCINFOPANEL_H_
