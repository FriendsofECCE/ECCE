/**
 * @file
 *
 *
 */

#ifndef PROPERTYCONTEXTPANEL_HH
#define PROPERTYCONTEXTPANEL_HH


#include <wx/panel.h>

#include "PropertyContextPanelGUI.H"
#include "ContextPanelInterface.H"

class WxResourceTreeItemData;
class ContextPanelFactory;
class WxProperties;
class CalcMgr;
class WxGridView;

/**
 * Default property view.  Displays ALL properties for a 
 * given resource.  The view includes the following
 * information:<br>
 * <p>
 * <ul>
 *   <li>Property Name</li>
 *   <li>Property Namespace</li>
 *   <li>Property Value</li>
 * </ul>
 * </p>
 */
class PropertyContextPanel : public PropertyContextPanelGUI, 
                             public ContextPanelInterface
{
  public:

    friend class ContextPanelFactory;
    friend class WxProperties;

    ~PropertyContextPanel();

    void refresh();

    /**
     * Column position values for property grid data.
     */
    enum ColPosition 
    {
      NAMESPACE_COL = 0,
      NAME_COL,
      VALUE_COL,
      NUMBER_OF_COLS
    };

  protected:

    PropertyContextPanel(CalcMgr * calcMgr,
                         WxResourceTreeItemData *propertyData,
                         wxWindow* parent,
                         wxWindowID id = SYMBOL_PROPERTYCONTEXTPANELGUI_IDNAME,
                         const wxPoint& pos = SYMBOL_PROPERTYCONTEXTPANELGUI_POSITION,
                         const wxSize& size = SYMBOL_PROPERTYCONTEXTPANELGUI_SIZE,
                         long style = SYMBOL_PROPERTYCONTEXTPANELGUI_STYLE);

    void initialize();

    WxResourceTreeItemData *p_propData; 
                           /**< Pointer to a tree item containing
                                the property data */

    WxGridView *p_propView; /**< Pointer to WxGridView member */

};


#endif
