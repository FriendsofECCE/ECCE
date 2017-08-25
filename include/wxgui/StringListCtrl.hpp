#ifndef STRINGLISTCTRL_H_
#define STRINGLISTCTRL_H_

#include "wxgui/ewxListView.H"


class StringListCtrl : public ewxListView
{
  DECLARE_DYNAMIC_CLASS(StringListCtrl)

  public:
    StringListCtrl();
    StringListCtrl(wxWindow * parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxLC_SINGLE_SEL|wxLC_REPORT|wxLC_NO_HEADER,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = "StringListCtrl");
    virtual ~StringListCtrl();

    bool Create(wxWindow * parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxLC_SINGLE_SEL|wxLC_REPORT|wxLC_NO_HEADER,
            const wxValidator& validator = wxDefaultValidator,
            const wxString& name = "StringListCtrl");

    long Append(const wxString& label);
    long Insert(long index, const wxString& label);
    bool Delete(long index);
    long Find(const wxString& label);
    wxString GetSelection() const;
    void SetSelection(const wxString& label);
    void Change(long index, const wxString& label);
};

#endif // STRINGLISTCTRL_H_
