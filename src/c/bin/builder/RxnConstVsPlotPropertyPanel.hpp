#ifndef REACTIONCONSTANTVSPLOTPROPERTYPANEL_H_
#define REACTIONCONSTANTVSPLOTPROPERTYPANEL_H_

#include "PropertyPanel.H"

class ewxChoice;
class ewxPlotCtrl;

class ReactionConstantVsPlotPropertyPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(ReactionConstantVsPlotPropertyPanel)
  DECLARE_EVENT_TABLE()

  public:
    ReactionConstantVsPlotPropertyPanel();
    ReactionConstantVsPlotPropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionConstantVsPlotPropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionConstantVsPlotPropertyPanel");
    virtual ~ReactionConstantVsPlotPropertyPanel();

    virtual void refresh();
    virtual bool isRelevant(IPropCalculation *propCalc) const;

  protected:
    virtual void initialize();
    virtual bool hasError();

  private:
    ewxPlotCtrl *p_plotCtrl;
};

#endif // REACTIONCONSTANTVSPLOTPROPERTYPANEL_H_
