#ifndef REACTIONTRACEPROPERTYPANEL_H_
#define REACTIONTRACEPROPERTYPANEL_H_

#include <map>
  using std::map;

#include "GeomTracePropertyPanel.H"

class ReactionTracePropertyPanel : public GeomTracePropertyPanel
{
  DECLARE_DYNAMIC_CLASS(ReactionTracePropertyPanel)
  DECLARE_EVENT_TABLE()

  public:
    ReactionTracePropertyPanel();
    ReactionTracePropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionTracePropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionTracePropertyPanel");
    virtual ~ReactionTracePropertyPanel();

    virtual void restoreSettings();

  protected:
    virtual void fillPlot();
    virtual void processStep(int step);

    size_t p_index;
};

#endif // REACTIONTRACEPROPERTYPANEL_H_
