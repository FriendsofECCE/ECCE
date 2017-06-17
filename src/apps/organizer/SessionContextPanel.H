/**
 * @file
 *
 * @author Jeff Daily
 *
 *
 */

#ifndef SESSIONCONTEXTPANEL_HH
#define SESSIONCONTEXTPANEL_HH

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#define ID_ORIENTATION 6000
#define ID_COLOR_PRIMARY_PATH 6100
#define ID_COLOR_INPUT_PROVIDER 6200

#include <vector>
using std::vector;

#include "dsm/Session.H"

#include "wxgui/ewxScrolledWindow.H"

#include "ContextPanelInterface.H"

class CalcMgr;
class MDTaskPanel;
class Resource;
class SessionWorkflow;
class WxResourceTreeItemData;

class SessionContextPanel : public ewxPanel,
                            public ContextPanelInterface
{
  public:

    friend class ContextPanelFactory;
    friend class SessionProjectContextPanel;
    friend class SessionWorkflow;

    ~SessionContextPanel();

    void refresh();

    MDTaskPanel * findTaskPanel(const EcceURL & url);

    CalcMgr * getCalcMgr();

    virtual vector<EcceURL> getSelections();

    virtual void loseFocus();

    virtual void selectResource(const EcceURL & url);

    void ensureVisible(const EcceURL & url);
    void ensureVisiblePrimaryLeaf();

  protected:

    SessionContextPanel( );

    SessionContextPanel( CalcMgr *calcmgr,
                         WxResourceTreeItemData *session,
                         wxWindow *parent,
                         wxWindowID id = -1,
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxDefaultSize,
                         long style = wxHSCROLL | wxVSCROLL |
                         wxSUNKEN_BORDER| wxTAB_TRAVERSAL);

    void createControls();

    void showTargets(wxBoxSizer *parentSizer, vector<Resource*> targets,
                     Session::LinkType linkType = Session::INPUT_PROVIDER );

    void drawConnections(Session::LinkType linkType);

    wxColour getPathColor(Session::LinkType linkType);

    // Event Handling
    void paint( wxPaintEvent& event );
    void size( wxSizeEvent& event );
    void keyDown( wxKeyEvent& event );
    void orientationClick( wxCommandEvent& event );
    void pathColorClick( wxCommandEvent& event );

    /** A description of how tasks are related to on another. */
    Session *p_session;

    /** The last selected MDTaskPanel.*/
    EcceURL p_lastHighlighted;

    SessionWorkflow * p_workflow;

    wxToolBar * p_toolBar;

    /** Is the main orientation of the workflow vertically aligned? */
    bool p_isVertical;

};



class SessionWorkflow : public ewxScrolledWindow
{
public:
  SessionWorkflow(SessionContextPanel * parent, wxWindowID id=wxID_ANY);

protected:

  void OnPaint( wxPaintEvent& event );
  void OnSize( wxSizeEvent& event );
  void OnKeyDown( wxKeyEvent& event );

  SessionContextPanel * p_parent;
};



#endif
     
