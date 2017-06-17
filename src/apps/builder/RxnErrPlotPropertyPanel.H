#ifndef REACTIONERRORPLOTPROPERTYPANEL_H_
#define REACTIONERRORPLOTPROPERTYPANEL_H_

#include "PropertyPanel.H"

class ewxChoice;
class ewxPlotCtrl;

class ReactionErrorPlotPropertyPanel : public PropertyPanel
{
  DECLARE_DYNAMIC_CLASS(ReactionErrorPlotPropertyPanel)
  DECLARE_EVENT_TABLE()

  public:
    static const wxWindowID ID_BUTTON_REFERENCE;

    ReactionErrorPlotPropertyPanel();
    ReactionErrorPlotPropertyPanel(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionErrorPlotPropertyPanel");
    virtual bool Create(IPropCalculation *calculation,
            wxWindow *parent, wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxTAB_TRAVERSAL,
            const wxString& name="ReactionErrorPlotPropertyPanel");
    virtual ~ReactionErrorPlotPropertyPanel();

    virtual void refresh();
    virtual bool isRelevant(IPropCalculation *propCalc) const;

  protected:
    virtual void initialize();
    virtual bool hasError();

    void OnStdDevChoice(wxCommandEvent &event);
    void OnReferenceClick(wxCommandEvent &event);

  private:
    ewxPlotCtrl *p_plotCtrl;
    ewxChoice *p_choice;

    static map<wxString,wxString> s_key_to_name;
    static map<wxString,wxString> s_name_to_key;
};

#endif // REACTIONERRORPLOTPROPERTYPANEL_H_
