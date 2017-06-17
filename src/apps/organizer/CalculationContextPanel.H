/**
 * @file
 *
 * @author Jeff Daily
 *
 *
 */

#ifndef CALCULATIONCONTEXTPANEL_H
#define CALCULATIONCONTEXTPANEL_H

#include "wxgui/ewxScrolledWindow.H"

#include "ContextPanelInterface.H"


class CalcMgr;
class WxState;
class EcceMap;
class EcceTool;
class WxResourceTreeItemData;
class ContextPanelFactory;
class wxBoxSizer;


/**
 * Calculation view.
 * Displays all of the available information about the calculation in context.
 * Any unavailable text fields are left blank.  Buttons conveniently allow the
 * user to alter calculation properties without having to use the menu bar.
 * Properties displayed are updated once the user saves them outside of the
 * Organizer.
 */
class CalculationContextPanel : public ewxScrolledWindow,
                                public ContextPanelInterface
{
  public:

    friend class ContextPanelFactory;

    ~CalculationContextPanel();

    void refresh();

  protected:

    CalculationContextPanel( CalcMgr * calcMgr,
                             WxResourceTreeItemData *itemData,
                             wxWindow *parent,
                             wxWindowID id = wxID_ANY,
                             const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxHSCROLL | wxVSCROLL |
                             wxSUNKEN_BORDER| wxTAB_TRAVERSAL);

    // Internal helper functions
    void            createControls();
    EcceTool*       createToolButton(ResourceTool* resTool);
    wxBoxSizer*     createPropLabel(ResourceProperty* resProp);
    wxBoxSizer*     createPropHolder(ResourceProperty* resProp);
    void            setPropValue(ResourceProperty* resProp);

    // Callbacks
    void OnButtonClick(wxCommandEvent &event);

  private:

    /** Pointer to a tree item containing the property data. */
    WxResourceTreeItemData *p_itemData;

    /** Pointer to the state icon. */
    WxState *p_state;

    /** Pointer to resource's own properties. */
    EcceMap *p_properties;

    /** If an MDTask, searches input providers for missing properties.*/
    EcceMap *p_inpProvProps;
};

#endif // CALCULATIONCONTEXTPANEL_H

