#ifndef CONTEXTPANEL_H_
#define CONTEXTPANEL_H_

#include <vector>

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

    int preferredHeight() const;
    wxString GetContext() const;
    wxString ContextAt(long index) const;
    bool SetContext(const wxString& context);
    bool RemoveContext(const wxString& context);
    bool RenameContext(const wxString& oldContext, const wxString& newContext);

  protected:
    static wxString label(const wxString& context);
    long indexOf(const wxString& context) const;
    void relabel();

    StringListCtrl * p_list;
    // The contexts (URLs) in list order; the list shows their names.
    std::vector<wxString> p_contexts;
};

#endif // CONTEXTPANEL_H_
