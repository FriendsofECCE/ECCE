/**
 * @file
 *
 *
 */

#ifndef PROJECTCONTEXTPANEL_HH
#define PROJECTCONTEXTPANEL_HH

#include <vector>
  using std::vector;

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "util/InvalidException.H"
#include "util/EcceURL.H"

#include "wxgui/WxGridView.H"

#include "ProjectContextPanelGUI.H"
#include "ContextPanelInterface.H"
#include "ContextPanelFactory.H"


class CalcMgr;
class PropertyView;
class Resource;
class WxResourceTreeItemData;


/**
 * Default project view.  Displays a view of the
 * resources contained in an ecce project.  The
 * set of properties displayed for the resources
 * is defined in the Resource Descriptor file.
 */
class ProjectContextPanel : public ProjectContextPanelGUI
                          , public ContextPanelInterface
{
  DECLARE_CLASS(ProjectContextPanel)
  DECLARE_EVENT_TABLE()

  public:

    friend class ContextPanelFactory;
    friend class SessionProjectContextPanel;

    ~ProjectContextPanel();

    void refresh();

    vector<EcceURL> getSelections();

    void clearSelections();

    void selectResource(const EcceURL &url);

    void loseFocus();

  protected:

    ProjectContextPanel(CalcMgr *calcmgr,
            WxResourceTreeItemData *project,
            wxWindow* parent, 
            wxWindowID id=wxID_ANY,
            const wxPoint& pos=wxDefaultPosition,
            const wxSize& size=wxDefaultSize,
            long style=wxSUNKEN_BORDER|wxTAB_TRAVERSAL);

    void initialize();
    void saveSelections();

    void gridCellLeftDClick(wxGridEvent& event);
    void gridCellRightClick(wxGridEvent& event);
    void gridRangeSelect(wxGridRangeSelectEvent& event);
    //void gridCellChange(wxGridEvent& event);
    void gridLabelLeftClick(wxGridEvent& event);
    void gridOnKeyDown(wxKeyEvent& event);
    void onExport(wxCommandEvent& event);

    void getContextMenu(wxMenu & menu);

    void saveSortSettings();
    void restoreSortSettings();



    CalcMgr *p_calcMgr;  /**< Pointer to CalcMgr window */

    WxResourceTreeItemData 
      *p_project;        /**< A project tree item.  Property data for the 
                              resources contained by the project will be
                              presented. */

    WxGridView 
      *p_projectView;    /**< Pointer to a WxGridView object, that is responsible 
                              for presenting the property data for the resources
                              contained by the project. */

    bool p_initialized;  /**< If true, indicates the panel has been initialized
                              meaning the grid has been created and populated
                              with resource information. */

    //int p_renameCol;
                         /**< Identifies the editable column in the project
                              panel's grid.  This should be the resource
                              name column, allowing a user to rename the
                              resource within the project panel. */

    PropertyView
      *p_propertyView;   /**< Current selecion of properties being viewed on panel. */  

    vector<EcceURL>
      p_selections;     /**< Currently selected resources on the grid */


};


#endif

