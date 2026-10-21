#ifndef _EWXGRID_H
#define _EWXGRID_H

#include <wx/grid.h>

#include "wxgui/ewxStyledWindow.H"

class ewxGrid : public wxGrid
              , public ewxStyledWindow
{
  DECLARE_CLASS(ewxGrid)
  DECLARE_EVENT_TABLE()

  public:
    static const wxWindowID ID_EXPORT;
    static const wxWindowID ID_COPY;
    static const wxWindowID ID_COPY_ALL;

    ewxGrid(wxWindow * parent, wxWindowID id=wxID_ANY,
	    const wxPoint& pos=wxDefaultPosition,
	    const wxSize& size=wxDefaultSize,
	    long style=wxWANTS_CHARS,
	    const wxString& name="ewxGrid");
  
    virtual ~ewxGrid();
  
    void SetReadOnly(bool readOnly);

    void enableContextMenu(bool enable=true);
    void getContextMenu(wxMenu& menu);
    void doExport();
    //  Tab-separated, as shown: every cell under its column labels, or the
    //  selection (else the cursor's cell).  What Copy/Copy all put on the
    //  clipboard (#408).
    wxString copyText(bool all) const;

  protected:
    void OnRightClick(wxGridEvent& event);
    void OnExportClick(wxCommandEvent& event);
    void OnCopyClick(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void toClipboard(const wxString& text);

  private:
    bool p_contextMenuEnabled;
    int p_clickRow, p_clickCol;   // the right-clicked cell, -1 on a label
};

#endif // _EWXGRID_H
