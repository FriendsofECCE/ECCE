/**
 * @file
 *
 * @author Jeff Daily
 *
 *
 */

#ifndef _DOCUMENTCONTEXTPANEL_H
#define _DOCUMENTCONTEXTPANEL_H

#include <wx/panel.h>

#include "DocumentContextPanelGUI.H"
#include "ContextPanelInterface.H"

class WxResourceTreeItemData;
class ContextPanelFactory;
class CalcMgr;

/**
 * Document view.
 * Displays the contents of the file when recognized as a text format.
 */
class DocumentContextPanel : public DocumentContextPanelGUI,
                             public ContextPanelInterface
{

  public:

    friend class ContextPanelFactory;

    ~DocumentContextPanel();

    void refresh();

  protected:

    DocumentContextPanel( CalcMgr * calcMgr,
                          WxResourceTreeItemData *itemData, 
                          wxWindow *parent, wxWindowID id = -1,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize, 
                          long style = wxTAB_TRAVERSAL );

    /** Pointer to a tree item containing the property data. */
    WxResourceTreeItemData *p_itemData;
};

#endif

