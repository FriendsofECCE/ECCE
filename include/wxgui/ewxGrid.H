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

  protected:
    void OnRightClick(wxGridEvent& event);
    void OnExportClick(wxCommandEvent& event);

  private:
    bool p_contextMenuEnabled;
};

#endif // _EWXGRID_H
