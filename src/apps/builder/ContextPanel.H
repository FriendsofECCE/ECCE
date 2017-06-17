#ifndef CONTEXTPANEL_H_
#define CONTEXTPANEL_H_

#include "wxgui/ewxPanel.H"

class StringListCtrl;


class ContextPanel : public ewxPanel
{
  DECLARE_DYNAMIC_CLASS(ContextPanel)

  public:
    static const wxWindowID ID_LIST;

    ContextPanel();
    ContextPanel(wxWindow* parent, wxWindowID id = wxID_ANY,
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxTAB_TRAVERSAL,
             const wxString& name = "ContextPanel");
    virtual ~ContextPanel();
  
    virtual bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxTAB_TRAVERSAL,
                        const wxString& name = "ContextPanel");

    wxString GetContext() const;
    bool SetContext(const wxString& context);
    bool RemoveContext(const wxString& context);
    bool RenameContext(const wxString& oldContext, const wxString& newContext);

  protected:
    StringListCtrl * p_list;
};

#endif // CONTEXTPANEL_H_
