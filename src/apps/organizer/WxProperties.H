/**
 * @file
 *
 * @author Jeff Daily
 *
 *
 */

#ifndef _WXPROPERTIES_H_
#define _WXPROPERTIES_H_

#include "WxPropertiesGUI.H"
#include "PropertyContextPanel.H"

/**
 * Implementation of WxPropertiesGUI.
 * This class displays the properties for a given Resource in a dialog.
 */
class WxProperties : public WxPropertiesGUI
{
  public:

    WxProperties( );
    WxProperties( wxWindow* parent,
                  wxWindowID id = SYMBOL_WXPROPERTIESGUI_IDNAME,
                  const string& caption = SYMBOL_WXPROPERTIESGUI_TITLE,
                  const wxPoint& pos = SYMBOL_WXPROPERTIESGUI_POSITION,
                  const wxSize& size = SYMBOL_WXPROPERTIESGUI_SIZE,
                  long style = SYMBOL_WXPROPERTIESGUI_STYLE );
    ~WxProperties();

    void setContext(WxResourceTreeItemData *itemData);

  protected:

    /* Call backs */
    void OnCloseWindow( wxCloseEvent& event );
    void OnButtonClosePropertiesClick( wxCommandEvent &event );

  private:

    WxResourceTreeItemData *p_itemData;

    PropertyContextPanel *p_propPanel;
};

#endif // _WXPROPERTIES_H_

