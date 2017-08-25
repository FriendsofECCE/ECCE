#ifndef REACTIONCONSTANTPLOTPROPERTYPANEL_H_
#define REACTIONCONSTANTPLOTPROPERTYPANEL_H_

#include "PropertyPanel.H"

class ewxChoice;
class ewxPlotCtrl;

class ReactionConstantPlotPropertyPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(ReactionConstantPlotPropertyPanel)
  DECLARE_EVENT_TABLE()

  public:
    ReactionConstantPlotPropertyPanel();
    ReactionConstantPlotPropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionConstantPlotPropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionConstantPlotPropertyPanel");
    virtual ~ReactionConstantPlotPropertyPanel();

    virtual void refresh();
    virtual bool isRelevant(IPropCalculation *propCalc) const;

  protected:
    virtual void initialize();
    virtual bool hasError();

  private:
    ewxPlotCtrl *p_plotCtrl;
};

#endif // REACTIONCONSTANTPLOTPROPERTYPANEL_H_
