#ifndef GRIDDLG__H
#define GRIDDLG__H

#include "GridGUI.H"


/**
 */
class GridDlg : public GridGUI
{
  DECLARE_DYNAMIC_CLASS(GridDlg)

  public:
    GridDlg();
    GridDlg( wxWindow* parent, wxWindowID id = SYMBOL_GRIDGUI_IDNAME, const wxString& caption = SYMBOL_GRIDGUI_TITLE, const wxPoint& pos = SYMBOL_GRIDGUI_POSITION, const wxSize& size = SYMBOL_GRIDGUI_SIZE, long style = SYMBOL_GRIDGUI_STYLE );
    virtual ~GridDlg();

    void enableUniformGrid(bool enabled);

    void setResolution(int x, int y, int z, bool uniform);
    void getResolution(int& x, int&y, int& z, bool& uniform) const;
    void setExtent(float fromx, float tox, 
                   float fromy, float toy, 
                   float fromz, float toz);
    void getExtent(float& fromx, float& tox, 
                   float& fromy, float& toy, 
                   float& fromz, float& toz) const;


    virtual    void OnTextctrlGridUniformEnter( wxCommandEvent& event );
    virtual    void OnCheckboxGridUniformClick( wxCommandEvent& event );
    virtual    void OnOkClick( wxCommandEvent& event );
    virtual    void OnCloseWindow( wxCloseEvent& event );
    virtual    void OnApplyClick( wxCommandEvent& event );
    
    virtual    void Init();



   protected:

   private:



};

#endif // GRIDDLG__H

