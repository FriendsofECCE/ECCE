//  How the Builder's side panels are organised: the classic layout, and
//  the three one-column layouts (View > Panel layout).  Every pane stays an
//  ordinary AUI pane in all of them -- a one-column layout is the same
//  panes docked in one place, with the inactive tab's panes hidden -- so
//  the menus, the layout save/restore and the viz-panel focus logic in
//  Builder.C need no second implementation.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <utility>

#include <wx/menu.h>
#include <wx/menuitem.h>
#include <wx/timer.h>
#include <wx/radiobox.h>
#include <deque>
#include <functional>
#include <memory>

#include "dsm/ICalculation.H"
#include "dsm/IPropCalculation.H"
#include "dsm/JCode.H"
#include "dsm/TaskJob.H"
#include "tdat/TProperty.H"
#include "tdat/TRunType.H"
#include "tdat/TTheory.H"

#include "wxgui/ewxConfig.H"
#include "wxgui/ewxWindowUtils.H"

#include "AbstractPropCalculation.H"
#include "Builder.H"
#include "DefaultCalculation.H"
#include "PropertyIndexPanel.H"
#include "PropertyPanel.H"
#include "PropertyPanelFactory.H"
#include "VizPropertyPanel.H"


namespace {

//  Panels are built, re-docked and hidden here as a matter of layout, not
//  because the user picked one -- see Builder::OnChildFocus (#111).
struct PanelGuard
{
  explicit PanelGuard(int& depth) : p_depth(depth) { ++p_depth; }
  ~PanelGuard() { --p_depth; }
  int& p_depth;
};

const char *MODE_NAMES[] = { "classic", "stacked", "accordion", "detail" };

//  What opens first when a layout shows one panel and the user has not
//  chosen: the data people came for, not the summary the header repeats.
//  Not the MOs: that panel draws orbitals and is opened when wanted.
const char *DEFAULT_PANEL_ORDER[] = {
  "Energies", "Vibrational Frequencies", "Calculation Summary"
};


//  True if the saved classic layout `layout` is what an old build wrote
//  as its default and nothing has been changed since: for every tool pane
//  the saved shown/hidden flag is the default's, and the shown ones are
//  docked on the right.  Positions, sizes and toolbars are not compared --
//  they move without anyone having arranged anything.
bool oldLayoutIsDefault(ewxConfig *config, wxAuiManager& mgr,
                        const char *layout, const set<string>& shown)
{
  const string *tools[] = {
    &Builder::NAME_TOOL_CONTEXT, &Builder::NAME_TOOL_BUILD,
    &Builder::NAME_TOOL_COORDINATES, &Builder::NAME_TOOL_SELECTION,
    &Builder::NAME_TOOL_ATOM_TABLE, &Builder::NAME_TOOL_RESIDUE_TABLE,
    &Builder::NAME_TOOL_SYMMETRY, &Builder::NAME_TOOL_DNA_BUILDER,
    &Builder::NAME_TOOL_PEPTIDE_BUILDER, &Builder::NAME_TOOL_PBC,
    &Builder::NAME_TOOL_SLICER, &Builder::NAME_TOOL_LOG
  };
  for (size_t i = 0; i < sizeof(tools) / sizeof(tools[0]); ++i) {
    wxString info;
    wxString key = wxString("/PaneLayout/") + layout + "/" + *tools[i];
    if (!config->Read(key, &info)) {
      continue;
    }
    wxAuiPaneInfo saved;
    mgr.LoadPaneInfo(info, saved);
    const bool want = shown.count(*tools[i]) > 0;
    if (saved.IsShown() != want) {
      return false;
    }
    if (want && (saved.IsFloating() ||
                 saved.dock_direction != wxAUI_DOCK_RIGHT)) {
      return false;
    }
  }
  return true;
}

}   // namespace


string Builder::layoutPrefix() const
{
  return isColumnMode() ? string("/PaneLayoutColumn/") : NAME_LAYOUT_PREFIX;
}


/**
 * Reads the panel layout setting.  A user with no setting gets list +
 * detail, except one whose saved classic layout was arranged by hand,
 * who keeps Classic: a saved layout that is exactly the old default is
 * dropped, anything else is kept as their Classic layout.
 *
 * ECCE_PANEL_MODE=classic|stacked|accordion|detail overrides it for one
 * run without saving it (headless captures and tests); ECCE_PANEL_COLLAPSED=1
 * starts with the side panels collapsed.
 */
void Builder::initPanelMode()
{
  ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
  wxString saved = config->Read("/PanelMode", "");
  int mode = -1;
  for (int i = 0; i < 4; ++i) {
    if (saved == MODE_NAMES[i]) mode = i;
  }
  if (mode < 0) {
    mode = PANELS_DETAIL;
    const bool hasDefault = config->HasGroup("/PaneLayout/Default");
    const bool hasReadOnly = config->HasGroup("/PaneLayout/ReadOnly");
    if (hasDefault || hasReadOnly) {
      const set<string> editing = { NAME_TOOL_CONTEXT, NAME_TOOL_BUILD,
                                    NAME_TOOL_COORDINATES, NAME_TOOL_SYMMETRY,
                                    NAME_TOOL_LOG };
      const set<string> viewing = { NAME_TOOL_CONTEXT, NAME_TOOL_ATOM_TABLE,
                                    NAME_TOOL_SELECTION, NAME_TOOL_LOG };
      const bool unchanged =
        (!hasDefault ||
         oldLayoutIsDefault(config, p_mgr, "Default", editing)) &&
        (!hasReadOnly ||
         oldLayoutIsDefault(config, p_mgr, "ReadOnly", viewing));
      if (unchanged) {
        config->DeleteGroup("/PaneLayout");
      } else {
        mode = PANELS_CLASSIC;
      }
    }
    config->Write("/PanelMode", MODE_NAMES[mode]);
  }
  if (const char *forced = getenv("ECCE_PANEL_MODE")) {
    for (int i = 0; i < 4; ++i) {
      if (string(forced) == MODE_NAMES[i]) mode = i;
    }
  }
  p_panelMode = (PanelMode)mode;

  p_columnHidden = config->ReadBool("/ColumnHidden", false) ||
                   getenv("ECCE_PANEL_COLLAPSED") != 0;
  //  The first tab follows the calculation (refreshColumn), not the tab
  //  last chosen: a finished calculation opens on its Properties.
  config->DeleteEntry("/ColumnTab");
}


void Builder::createViewMenu()
{
  p_viewMenu = new wxMenu;
  p_viewMenu->AppendCheckItem(ID_VIEW_TOGGLE_COLUMN, _("Hide Side Panels\tF9"),
      _("Collapse the side panels to a thin strip for a larger view"))
      ->Check(p_columnHidden);
  p_viewMenu->AppendSeparator();
  p_panelModeMenu = new wxMenu;
  p_panelModeMenu->AppendRadioItem(ID_VIEW_PANELS_CLASSIC,
      _("Classic (panels on both sides)"));
  p_panelModeMenu->AppendRadioItem(ID_VIEW_PANELS_STACKED,
      _("One column, stacked"));
  p_panelModeMenu->AppendRadioItem(ID_VIEW_PANELS_ACCORDION,
      _("One column, accordion"));
  p_panelModeMenu->AppendRadioItem(ID_VIEW_PANELS_DETAIL,
      _("One column, list and detail"));
  p_viewMenu->AppendSubMenu(p_panelModeMenu, _("Panel layout"),
      _("How the side panels are arranged"));
  p_viewMenu->Append(ID_VIEW_RESET_LAYOUT, _("Reset layout"),
      _("Back to the default panel layout and sizes"));

  wxMenuBar *bar = GetMenuBar();
  int before = bar->FindMenu(_("Options"));
  if (before == wxNOT_FOUND) before = bar->GetMenuCount();
  bar->Insert(before, p_viewMenu, _("View"));

  Bind(wxEVT_MENU, &Builder::OnViewToggleColumn, this, ID_VIEW_TOGGLE_COLUMN);
  Bind(wxEVT_MENU, &Builder::OnViewResetLayout, this, ID_VIEW_RESET_LAYOUT);
  Bind(wxEVT_MENU, &Builder::OnViewPanelMode, this, ID_VIEW_PANELS_CLASSIC,
       ID_VIEW_PANELS_DETAIL);
  syncMenuChecks();
}


void Builder::OnViewToggleColumn(wxCommandEvent&)
{
  setColumnCollapsed(!p_columnHidden);
}


//  Collapses the side panels (the column; in Classic the right-hand
//  tools) to the strip with the arrow, or brings them back.  Saved, so
//  the next session starts the same way.
void Builder::setColumnCollapsed(bool collapsed)
{
  p_columnHidden = collapsed;
  ewxConfig::getConfig("wxbuilder.ini")->Write("/ColumnHidden", collapsed);
  updatePanes();
}


void Builder::OnViewPanelMode(wxCommandEvent& event)
{
  setPanelMode((PanelMode)(event.GetId() - ID_VIEW_PANELS_CLASSIC));
}


void Builder::OnViewResetLayout(wxCommandEvent&)
{
  setPanelMode(PANELS_DETAIL, true);
}


//  Which tab a pane belongs to; none for the viewer, toolbars, the Log,
//  the tab buttons' own pane, and anything floating.
Builder::PaneGroup Builder::paneGroup(const wxAuiPaneInfo& pane) const
{
  if (!pane.window || pane.IsToolbar() || pane.IsFloating()) {
    return GROUP_NONE;
  }
  const string name = pane.name.ToStdString();
  if (name == NAME_COLUMN_TABS || name == NAME_TOOL_LOG) {
    return GROUP_NONE;
  }
  if (name == NAME_PROPERTY_INDEX ||
      dynamic_cast<PropertyPanel*>(pane.window)) {
    return GROUP_PROPERTIES;
  }
  return p_structureNames.count(name) ? GROUP_STRUCTURE : GROUP_NONE;
}


//  Open as far as the user is concerned, whether or not a tab, the F9 key
//  or the list + detail layout is keeping it off the screen at the moment.
bool Builder::paneWanted(const wxAuiPaneInfo& pane) const
{
  if (p_panelMode == PANELS_DETAIL && paneGroup(pane) == GROUP_PROPERTIES) {
    return pane.window == p_detail;
  }
  return pane.IsShown() || p_tabHidden.count(pane.window) > 0;
}


/**
 * Where each pane docks in the current layout.  Classic puts the tools on
 * the right and the property panels on the left, as before.  The one-column
 * layouts put the tab buttons, then the tools or the properties, in one
 * dock on the right (layer 1, so it runs the full height) and the Log
 * under the viewer.  Visibility is not touched here.
 */
void Builder::applyGeometry(bool resetSizes)
{
  const bool col = isColumnMode();
  const bool detail = p_panelMode == PANELS_DETAIL;
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  for (size_t i = 0; i < panes.GetCount(); ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (!pane.window || pane.IsToolbar() || pane.IsFloating() ||
        pane.dock_direction == wxAUI_DOCK_CENTER) {
      continue;
    }
    const string name = pane.name.ToStdString();
    if (name == NAME_COLUMN_TOGGLE) {
      pane.Right().Layer(0).Row(0).Position(0);
      continue;
    }
    if (name == NAME_COLUMN_TABS || name == NAME_PROPERTY_INDEX) {
      if (col) {
        pane.Right().Layer(1).Row(0).Position(name == NAME_COLUMN_TABS ? 0 : 1);
        //  AUI shares the dock's height by proportion even for a fixed
        //  pane; the tab buttons must not take a share of it.
        if (name == NAME_COLUMN_TABS) pane.dock_proportion = 1;
      }
      continue;
    }
    if (dynamic_cast<PropertyPanel*>(pane.window)) {
      if (col) {
        const int order =
          PropertyPanelFactory::getPropertyPanelFactory().indexOf(name);
        pane.Right().Layer(1).Row(0).Position(2 + (order < 0 ? 500 : order));
      } else {
        pane.Left().Layer(2).Row(0);
      }
      pane.PinButton(!detail);
      if (resetSizes && !p_folded.count(pane.window)) {
        map<wxWindow*, int>::const_iterator base =
          p_baseProportion.find(pane.window);
        pane.dock_proportion = detail ? detailProportion()
          : (base == p_baseProportion.end() ? 150 : base->second);
      }
      continue;
    }
    if (name == NAME_TOOL_LOG) {
      if (col) {
        pane.Bottom().Layer(0).Row(0).Position(0).PinButton(true);
        if (resetSizes && !p_folded.count(pane.window)) {
          pane.BestSize(wxSize(300, 120)).MinSize(wxSize(100, 60));
        }
      } else {
        const map<string, int>::const_iterator idx = p_toolIndex.find(name);
        pane.Right().Layer(1).Row(0).Position(
            idx == p_toolIndex.end() ? 0 : idx->second).PinButton(false);
        if (resetSizes && !p_folded.count(pane.window)) {
          pane.BestSize(wxDefaultSize).MinSize(wxSize(200, 150));
        }
      }
      continue;
    }
    if (p_structureNames.count(name)) {
      const map<string, int>::const_iterator idx = p_toolIndex.find(name);
      const int at = idx == p_toolIndex.end() ? (int)p_toolIndex.size()
                                              : idx->second;
      if (col) {
        pane.Right().Layer(1).Row(0).Position(at + 1);
      } else if (name == NAME_TOOL_ATOM_TABLE ||
                 name == NAME_TOOL_RESIDUE_TABLE) {
        pane.Left().Layer(0).Row(0).Position(at);
      } else {
        pane.Right().Layer(1).Row(0).Position(at);
      }
    }
  }
}


/**
 * Brings the one-column state in line with what exists: gives each new
 * property pane the layout's starting visibility, settles the list + detail
 * selection or the accordion's open panel, picks the first tab, refreshes
 * the list, and applies it all.  Safe to call repeatedly -- it runs every
 * time the calculation gains a property -- and leaves what the user has
 * arranged alone.  Does not lay out; the caller does.
 */
void Builder::refreshColumn()
{
  if (!isColumnMode()) {
    syncColumn(false);
    return;
  }
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  bool anyProperties = false;
  for (size_t i = 0; i < panes.GetCount(); ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (paneGroup(pane) != GROUP_PROPERTIES ||
        pane.name == NAME_PROPERTY_INDEX) {
      continue;
    }
    anyProperties = true;
    if (p_columnSeen.insert(pane.window).second) {
      if (p_panelMode == PANELS_ACCORDION) {
        pane.Show(true);
        foldPane(pane.window, true);
      } else if (p_panelMode == PANELS_DETAIL) {
        pane.dock_proportion = detailProportion();
      }
    }
  }
  if (p_panelMode == PANELS_ACCORDION) {
    accordionNormalize();
  } else if (p_panelMode == PANELS_DETAIL) {
    ensureDetail();
  }
  //  A calculation without results has an empty Properties tab, whatever
  //  tab was last chosen: it opens on the building tools.
  if (!anyProperties) {
    p_columnTab = 0;
  } else if (!p_columnTabChosen) {
    p_columnTab = 1;
  }
  updatePropertyIndex();
  syncColumn(false);
}


/**
 * Makes the visible panes match the layout.  The invariant: a pane of
 * the inactive tab is never shown; if it was open it is remembered in
 * p_tabHidden and comes back with its tab.  With allowSwitch, a pane
 * somebody just showed (a menu item, ECCE_OPEN_PANEL) switches to its
 * own tab instead, and un-hides the column.
 */
void Builder::syncColumn(bool allowSwitch)
{
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  wxAuiPaneInfo &toggle = p_mgr.GetPane(NAME_COLUMN_TOGGLE);
  if (toggle.IsOk()) toggle.Show(true);
  if (!isColumnMode()) {
    //  Classic: the right-hand tools are the "column".  They sit in layer
    //  1 or higher so the arrow's layer 0 is next to the viewer.
    auto rightTool = [&](wxAuiPaneInfo &pane) {
      return pane.window && !pane.IsToolbar() && !pane.IsFloating() &&
             pane.dock_direction == wxAUI_DOCK_RIGHT &&
             pane.name != NAME_COLUMN_TOGGLE;
    };
    for (size_t i = 0; i < panes.GetCount(); ++i) {
      if (rightTool(panes.Item(i)) && panes.Item(i).dock_layer < 1) {
        panes.Item(i).Layer(1);
      }
    }
    if (allowSwitch && p_columnHidden && !p_stayCollapsed) {
      for (size_t i = 0; i < panes.GetCount(); ++i) {
        if (rightTool(panes.Item(i)) && panes.Item(i).IsShown()) {
          p_columnHidden = false;
          ewxConfig::getConfig("wxbuilder.ini")->Write("/ColumnHidden", false);
        }
      }
    }
    for (set<wxWindow*>::iterator it = p_tabHidden.begin();
         it != p_tabHidden.end(); ) {
      wxAuiPaneInfo &pane = p_mgr.GetPane(*it);
      if (pane.IsOk() && p_columnHidden && rightTool(pane)) {
        ++it;
        continue;
      }
      if (pane.IsOk()) pane.Show(true);
      it = p_tabHidden.erase(it);
    }
    for (size_t i = 0; p_columnHidden && i < panes.GetCount(); ++i) {
      wxAuiPaneInfo &pane = panes.Item(i);
      if (rightTool(pane) && pane.IsShown()) {
        pane.Show(false);
        p_tabHidden.insert(pane.window);
      }
    }
    wxAuiPaneInfo &tabs = p_mgr.GetPane(NAME_COLUMN_TABS);
    if (tabs.IsOk()) tabs.Show(false);
    wxAuiPaneInfo &index = p_mgr.GetPane(NAME_PROPERTY_INDEX);
    if (index.IsOk()) index.Show(false);
    return;
  }

  const bool detail = p_panelMode == PANELS_DETAIL;
  if (allowSwitch) {
    int newTab = -1;
    wxWindow *newDetail = 0;
    for (size_t i = 0; i < panes.GetCount(); ++i) {
      wxAuiPaneInfo &pane = panes.Item(i);
      const PaneGroup g = paneGroup(pane);
      if (g == GROUP_NONE || pane.name == NAME_COLUMN_TABS ||
          pane.name == NAME_PROPERTY_INDEX || !pane.IsShown()) {
        continue;
      }
      const int tab = g == GROUP_PROPERTIES ? 1 : 0;
      if (detail && g == GROUP_PROPERTIES && pane.window != p_detail) {
        newDetail = pane.window;
        newTab = 1;
      } else if (tab != p_columnTab || p_columnHidden) {
        newTab = tab;
      }
    }
    if (newDetail) {
      if (getenv("ECCE_DEBUG_PANELS")) fprintf(stderr, "[PANELS] detail taken by shown pane %s\n", p_mgr.GetPane(newDetail).name.ToStdString().c_str());
      p_detail = newDetail;
    }
    if (newTab >= 0) {
      p_columnTab = newTab;
      if (!p_stayCollapsed && p_columnHidden) {
        p_columnHidden = false;
        ewxConfig::getConfig("wxbuilder.ini")->Write("/ColumnHidden", false);
      }
    }
  }

  for (size_t i = 0; i < panes.GetCount(); ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (pane.name == NAME_COLUMN_TABS) {
      pane.Show(!p_columnHidden);
      continue;
    }
    const PaneGroup g = paneGroup(pane);
    if (g == GROUP_NONE) {
      continue;
    }
    if (pane.name == NAME_PROPERTY_INDEX) {
      pane.Show(detail && p_columnTab == 1 && !p_columnHidden);
      continue;
    }
    wxWindow *win = pane.window;
    const bool byList = detail && g == GROUP_PROPERTIES;
    const bool wanted = byList ? win == p_detail
                               : (pane.IsShown() || p_tabHidden.count(win));
    const int tab = g == GROUP_PROPERTIES ? 1 : 0;
    if (wanted && tab == p_columnTab && !p_columnHidden) {
      pane.Show(true);
      p_tabHidden.erase(win);
    } else {
      if (pane.IsShown()) {
        pane.Show(false);
        //  A panel that is off the screen must not keep its overlay in
        //  the viewer, same as when it is closed.
        VizPropertyPanel *viz = dynamic_cast<VizPropertyPanel*>(win);
        if (viz) viz->setFocus(false);
      }
      if (wanted && !byList) p_tabHidden.insert(win);
      else p_tabHidden.erase(win);
    }
  }

  for (int t = 0; t < 2; ++t) {
    if (p_tabButtons[t]) p_tabButtons[t]->SetValue(t == p_columnTab);
  }
}


//  The tool and Properties menus tick what the user has open.
void Builder::syncMenuChecks()
{
  if (p_viewMenu) {
    p_viewMenu->Check(ID_VIEW_TOGGLE_COLUMN, p_columnHidden);
    p_panelModeMenu->Check(ID_VIEW_PANELS_CLASSIC + p_panelMode, true);
  }
  if (p_toggleButton) {
    p_toggleButton->SetBitmap(wxArtProvider::GetBitmap(
        p_columnHidden ? wxART_GO_BACK : wxART_GO_FORWARD, wxART_BUTTON));
    p_toggleButton->SetToolTip(p_columnHidden ? _("Show the side panels")
                                              : _("Hide the side panels"));
  }
  if (!p_toolMenu || !p_propertyMenu) {
    return;
  }
  for (int i = 0; i < p_toolCount; ++i) {
    wxMenuItem *item = p_toolMenu->FindItem(ID_TOOLMENU_ITEM + i);
    if (!item) continue;
    wxAuiPaneInfo &pane = p_mgr.GetPane(item->GetItemLabelText());
    if (pane.IsOk() && item->IsCheckable()) {
      item->Check(paneWanted(pane));
    }
  }
  for (size_t i = 0; i < p_propertyMenu->GetMenuItemCount(); ++i) {
    wxMenuItem *item = p_propertyMenu->FindItemByPosition(i);
    if (!item || !item->IsCheckable()) continue;
    wxAuiPaneInfo &pane = p_mgr.GetPane(item->GetItemLabelText());
    if (pane.IsOk()) {
      item->Check(paneWanted(pane));
    }
  }
}


//  wxAuiManager::Update() with the layout's own rules applied first and
//  the folded panes' windows hidden again after (Update() re-shows them).
void Builder::updatePanes(bool allowSwitch)
{
  //  wxAuiManager::Update() lays the frame out, and a size event handled on
  //  the way can ask for another Update(), which frees the sizer items the
  //  outer one is still walking (a crash opening a calculation with MOs on
  //  macOS).  A nested request is run once the outer one is done.
  static bool updating = false, again = false, againSwitch = false;
  if (updating) {
    again = true;
    againSwitch = againSwitch || allowSwitch;
    if (getenv("ECCE_DEBUG_PANELS"))
      fprintf(stderr, "[PANELS] updatePanes nested, deferred\n");
    return;
  }
  updating = true;
  PanelGuard guard(p_panelBuildDepth);
  syncColumn(allowSwitch);
  p_mgr.Update();
  updating = false;
  if (again) {
    again = false;
    const bool sw = againSwitch;
    againSwitch = false;
    updatePanes(sw);
    return;
  }
  for (map<wxWindow*, FoldState>::iterator it = p_folded.begin();
       it != p_folded.end(); ++it) {
    wxAuiPaneInfo &pane = p_mgr.GetPane(it->first);
    if (pane.IsOk() && pane.IsShown()) {
      it->first->Hide();
    }
  }
  syncMenuChecks();
  initShownPanels();
}


//  Builds the property panels that are really on screen; PropertyPanel::
//  Show() skips those wxAUI shows while their frame is still hidden.
void Builder::initShownPanels()
{
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  for (size_t i = 0; i < panes.GetCount(); ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (!pane.IsShown() || pane.IsToolbar() || !pane.window ||
        !pane.window->IsShownOnScreen()) {
      continue;
    }
    PropertyPanel *pp = dynamic_cast<PropertyPanel*>(pane.window);
    if (pp) pp->ensureInitialized();
  }
}


void Builder::setColumnTab(int tab, bool save)
{
  p_columnTab = tab ? 1 : 0;
  p_columnHidden = false;
  if (save) {
    p_columnTabChosen = true;   // for this calculation, see setContext()
  }
  updatePanes();
}


//  List + detail: make `win` the one open property panel.
void Builder::setDetail(wxWindow *win)
{
  if (p_detail && p_detail != win) {
    wxAuiPaneInfo &old = p_mgr.GetPane(p_detail);
    if (old.IsOk()) old.Show(false);
    unfocusPanel(p_detail);
  }
  p_detail = win;
}


void Builder::selectDetail(const string& name)
{
  wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
  if (!pane.IsOk()) {
    return;
  }
  if (p_panelMode == PANELS_DETAIL && paneGroup(pane) == GROUP_PROPERTIES) {
    setDetail(pane.window);
  }
  pane.Show(true);
  if (pane.IsFloating() && pane.frame) {
    pane.frame->Raise();
  }
  updatePanes(true);
  focusShownPanel(pane.window);
  if (p_index) p_index->setSelected(name);
}


//  List + detail: exactly one docked property panel is open.  Keeps the
//  current one, else one that happens to be shown, else the first of
//  DEFAULT_PANEL_ORDER, else the first in the descriptor.
void Builder::ensureDetail()
{
  if (p_panelMode != PANELS_DETAIL) {
    return;
  }
  PropertyPanelFactory &factory = PropertyPanelFactory::getPropertyPanelFactory();
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  vector<wxAuiPaneInfo*> docked;
  wxWindow *shown = 0;
  for (size_t i = 0; i < panes.GetCount(); ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (paneGroup(pane) == GROUP_PROPERTIES &&
        pane.name != NAME_PROPERTY_INDEX) {
      docked.push_back(&pane);
      if (!shown && pane.IsShown()) shown = pane.window;
    }
  }
  wxAuiPaneInfo &current = p_mgr.GetPane(p_detail);
  const bool valid = p_detail && current.IsOk() &&
                     paneGroup(current) == GROUP_PROPERTIES;
  if (!valid) {
    if (getenv("ECCE_DEBUG_PANELS")) fprintf(stderr, "[PANELS] detail chosen by default\n");
    p_detail = shown;
    for (size_t k = 0; !p_detail &&
         k < sizeof(DEFAULT_PANEL_ORDER) / sizeof(DEFAULT_PANEL_ORDER[0]);
         ++k) {
      for (size_t i = 0; i < docked.size(); ++i) {
        if (docked[i]->name == DEFAULT_PANEL_ORDER[k]) {
          p_detail = docked[i]->window;
        }
      }
    }
    int best = 1 << 30;
    for (size_t i = 0; !p_detail && i < docked.size(); ++i) {
      const int order = factory.indexOf(docked[i]->name.ToStdString());
      if (order >= 0 && order < best) {
        best = order;
        p_detail = docked[i]->window;
      }
    }
  }
  for (size_t i = 0; i < docked.size(); ++i) {
    if (docked[i]->window != p_detail) {
      if (docked[i]->IsShown()) {
        VizPropertyPanel *viz = dynamic_cast<VizPropertyPanel*>(docked[i]->window);
        if (viz) viz->setFocus(false);
      }
      docked[i]->Show(false);
      p_tabHidden.erase(docked[i]->window);
      unfoldPane(*docked[i]);
    }
  }
  if (p_detail) {
    unfoldPane(p_mgr.GetPane(p_detail));
  }
}


//  Accordion: every docked property panel is a caption bar and exactly one
//  is open -- `keepOpen` if given, else the one open already, else the
//  default.  Opening one folds the others.
void Builder::accordionNormalize(wxWindow *keepOpen)
{
  if (p_panelMode != PANELS_ACCORDION) {
    return;
  }
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  vector<wxAuiPaneInfo*> docked;
  for (size_t i = 0; i < panes.GetCount(); ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (paneGroup(pane) == GROUP_PROPERTIES &&
        pane.name != NAME_PROPERTY_INDEX && paneWanted(pane)) {
      docked.push_back(&pane);
    }
  }
  wxWindow *open = keepOpen;
  for (size_t i = 0; !open && i < docked.size(); ++i) {
    if (docked[i]->window == p_accordionOpen &&
        !p_folded.count(docked[i]->window)) {
      open = p_accordionOpen;
    }
  }
  for (size_t i = 0; !open && i < docked.size(); ++i) {
    if (!p_folded.count(docked[i]->window)) open = docked[i]->window;
  }
  for (size_t k = 0; !open &&
       k < sizeof(DEFAULT_PANEL_ORDER) / sizeof(DEFAULT_PANEL_ORDER[0]); ++k) {
    for (size_t i = 0; i < docked.size(); ++i) {
      if (docked[i]->name == DEFAULT_PANEL_ORDER[k]) open = docked[i]->window;
    }
  }
  if (!open && !docked.empty()) open = docked[0]->window;
  for (size_t i = 0; i < docked.size(); ++i) {
    foldPane(docked[i]->window, docked[i]->window != open);
  }
  p_accordionOpen = open;
}


//  The header and the list at the top of the list + detail layout.
void Builder::updatePropertyIndex()
{
  if (!p_index || !p_calculation || p_panelMode != PANELS_DETAIL) {
    return;
  }
  wxString title;
  if (dynamic_cast<DefaultCalculation*>(p_calculation)) {
    title = "New structure";
  } else {
    string url = p_calculation->getURL().toString();
    while (!url.empty() && url[url.size() - 1] == '/') {
      url.erase(url.size() - 1);
    }
    const size_t cut = url.find_last_of('/');
    title = wxString::FromUTF8(cut == string::npos ? url.c_str()
                                                   : url.c_str() + cut + 1);
  }

  string app, theory;
  TaskJob *job = dynamic_cast<TaskJob*>(p_calculation);
  if (job) {
    const JCode *code = job->application();
    if (code) app = code->name();
  }
  ICalculation *calc = dynamic_cast<ICalculation*>(p_calculation);
  if (calc) {
    TTheory *t = calc->theory();
    if (t) {
      theory = t->toConciseString();
      delete t;
    }
    const string run = calc->runtype().name();
    if (!run.empty()) {
      theory += (theory.empty() ? "" : " / ") + run;
    }
  }
  wxString codeTheory = wxString::FromUTF8(app.c_str());
  if (!app.empty() && !theory.empty()) codeTheory += " - ";
  codeTheory += wxString::FromUTF8(theory.c_str());

  wxString energy;
  //  A structure file (PDB, XYZ, CAR, cube, ...) and a new structure are
  //  AbstractPropCalculations, whose getProperty() throws: they have no
  //  properties, so they get no energy line.
  TProperty *te = dynamic_cast<AbstractPropCalculation*>(p_calculation)
                  ? 0 : p_calculation->getProperty("TE");
  if (te) {
    const string units = te->units() == "NA" ? "" : te->units();
    energy = wxString::Format("Total energy %.6f %s", te->scalarize(),
                              units.c_str());
  }
  p_index->setHeader(title, codeTheory, energy);

  PropertyPanelFactory &factory = PropertyPanelFactory::getPropertyPanelFactory();
  vector<std::pair<int, PropertyIndexPanel::Entry> > ordered;
  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(p_calculation->getURL().toString());
  for (set<PropertyPanel*>::iterator it = panels.begin(); it != panels.end();
       ++it) {
    PropertyIndexPanel::Entry entry;
    entry.name = (*it)->getName();
    entry.group = factory.groupOf(entry.name);
    ordered.push_back(std::make_pair(factory.indexOf(entry.name), entry));
  }
  std::sort(ordered.begin(), ordered.end(),
            [](const std::pair<int, PropertyIndexPanel::Entry>& a,
               const std::pair<int, PropertyIndexPanel::Entry>& b) {
              return a.first < b.first;
            });
  vector<PropertyIndexPanel::Entry> entries;
  for (size_t i = 0; i < ordered.size(); ++i) {
    entries.push_back(ordered[i].second);
  }
  p_index->setEntries(entries);
  if (p_detail) {
    wxAuiPaneInfo &pane = p_mgr.GetPane(p_detail);
    if (pane.IsOk()) p_index->setSelected(pane.name.ToStdString());
  }
}


//  The Log lives as one caption line in the one-column layouts.
void Builder::collapseLog()
{
  wxAuiPaneInfo &log = p_mgr.GetPane(NAME_TOOL_LOG);
  if (isColumnMode() && log.IsOk() && log.IsShown() && log.window) {
    foldPane(log.window, true);
  }
}


/**
 * Switches the panel layout.  What was open stays open or reachable:
 * the property panels open before are open after (as the one selected
 * panel in list + detail, as the open bar in the accordion).  The two
 * families -- classic, and the one-column layouts -- each keep their own
 * saved tool layout.  `reset` also forgets every saved layout, size and
 * tab.
 */
void Builder::setPanelMode(PanelMode mode, bool reset)
{
  if (mode == p_panelMode && !reset) {
    return;
  }
  PanelGuard guard(p_panelBuildDepth);
  wxBusyCursor busy;
  ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
  PropertyPanelFactory &factory = PropertyPanelFactory::getPropertyPanelFactory();

  vector<wxWindow*> wasOpen;
  wxAuiPaneInfoArray &panes = p_mgr.GetAllPanes();
  for (size_t i = 0; i < panes.GetCount(); ++i) {
    wxAuiPaneInfo &pane = panes.Item(i);
    if (dynamic_cast<PropertyPanel*>(pane.window) && paneWanted(pane) &&
        !(p_panelMode == PANELS_ACCORDION && p_folded.count(pane.window))) {
      wasOpen.push_back(pane.window);
    }
  }
  std::sort(wasOpen.begin(), wasOpen.end(), [&](wxWindow *a, wxWindow *b) {
    return factory.indexOf(p_mgr.GetPane(a).name.ToStdString()) <
           factory.indexOf(p_mgr.GetPane(b).name.ToStdString());
  });
  wxWindow *lastChosen = p_detail ? p_detail : p_accordionOpen;

  if (!reset) {
    savePaneLayout();
  }
  //  Leave the old layout: bring back what a tab was hiding, open every fold.
  const PanelMode previous = p_panelMode;
  const bool wasCollapsed = p_columnHidden;
  p_columnHidden = false;
  p_panelMode = PANELS_CLASSIC;
  syncColumn(false);
  p_columnHidden = wasCollapsed;
  for (map<wxWindow*, FoldState>::iterator it = p_folded.begin();
       it != p_folded.end(); ++it) {
    wxAuiPaneInfo &pane = p_mgr.GetPane(it->first);
    if (pane.IsOk()) {
      pane.BestSize(it->second.best).MinSize(it->second.min)
          .Resizable(it->second.resizable);
      pane.dock_proportion = it->second.proportion;
    }
  }
  p_folded.clear();
  p_tabHidden.clear();
  p_columnSeen.clear();
  p_accordionOpen = 0;
  p_propertyPanelInfo.clear();
  if (reset) {
    config->DeleteGroup("/PaneLayout");
    config->DeleteGroup("/PaneLayoutColumn");
    config->DeleteEntry("/ColumnTab");
    config->DeleteEntry("/ColumnHidden");
    p_columnHidden = false;
    p_columnTabChosen = false;
    p_columnTab = 0;
    p_detail = 0;
    wasOpen.clear();
  }

  p_panelMode = mode;
  config->Write("/PanelMode", MODE_NAMES[mode]);
  (void)previous;

  if (mode == PANELS_DETAIL && !reset) {
    p_detail = 0;
    for (size_t i = 0; i < wasOpen.size(); ++i) {
      if (wasOpen[i] == lastChosen) p_detail = lastChosen;
    }
    if (!p_detail && !wasOpen.empty()) p_detail = wasOpen[0];
  }
  applyGeometry(true);
  loadPaneLayout("", false);

  //  loadPaneLayout() re-added the property panels with the new layout's
  //  starting visibility; put back what was open.
  if (!reset && mode != PANELS_DETAIL) {
    for (size_t i = 0; i < panes.GetCount(); ++i) {
      wxAuiPaneInfo &pane = panes.Item(i);
      if (!dynamic_cast<PropertyPanel*>(pane.window) || pane.IsFloating()) {
        continue;
      }
      const bool open = std::find(wasOpen.begin(), wasOpen.end(),
                                  pane.window) != wasOpen.end();
      pane.Show(mode == PANELS_ACCORDION ? true : open);
    }
    if (mode == PANELS_ACCORDION) {
      wxWindow *keep = 0;
      for (size_t i = 0; i < wasOpen.size(); ++i) {
        if (wasOpen[i] == lastChosen) keep = lastChosen;
      }
      accordionNormalize(keep ? keep : (wasOpen.empty() ? 0 : wasOpen[0]));
    }
  }
  refreshColumn();
  collapseLog();
  updatePanes();
}


/**
 * A tool pane's contents changed size (the Periodic Builder grows once a
 * lattice exists): make sure it still has room.
 */
void Builder::toolPaneResized(const string& name)
{
  wxAuiPaneInfo &pane = p_mgr.GetPane(wxString(name));
  if (pane.IsOk() && pane.IsShown()) {
    makeRoomFor(wxString(name));
  }
}


/**
 * A tool pane opened from the Tools menu gets the height its controls need.
 * The right-hand column holds a fixed number of panes at their minimum
 * height; one more used to be laid out with no height at all, its window
 * left floating over the viewer.  Panes of the same dock that were opened
 * for convenience (tables, Selection, Symmetry, ...) are closed, least
 * wanted first, until the new one fits; they come back from the Tools menu.
 */
void Builder::makeRoomFor(const wxString& name)
{
  wxAuiPaneInfo &pane = p_mgr.GetPane(name);
  if (!pane.IsOk() || !pane.IsShown() || pane.IsFloating() || !pane.window ||
      p_columnHidden) {
    return;
  }
  //  The Periodic Builder scrolls, but is useless if it shows one row.
  const int need = name == NAME_TOOL_PBC ? 200 : 150;
  const string victims[] = {
    NAME_TOOL_ATOM_TABLE, NAME_TOOL_RESIDUE_TABLE, NAME_TOOL_SELECTION,
    NAME_TOOL_SYMMETRY, NAME_TOOL_COORDINATES, NAME_TOOL_DNA_BUILDER,
    NAME_TOOL_PEPTIDE_BUILDER, NAME_TOOL_SLICER, NAME_TOOL_BUILD
  };
  //  AUI keeps a docked pane's position as a pixel offset from the top of
  //  its dock and leaves the gap before it empty: put this pane directly
  //  under the ones above it before closing anything.
  bool moved = false;
  for (size_t v = 0; ; ) {
    wxAuiPaneInfo &now = p_mgr.GetPane(name);
    if (now.rect.height >= need) {
      return;
    }
    if (!moved) {
      moved = true;
      vector<wxAuiPaneInfo*> column;
      wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
      for (size_t i = 0; i < all.GetCount(); ++i) {
        wxAuiPaneInfo &o = all.Item(i);
        if (o.IsShown() && !o.IsFloating() && !o.IsToolbar() && o.window &&
            o.dock_direction == now.dock_direction &&
            o.dock_layer == now.dock_layer) {
          column.push_back(&o);
        }
      }
      std::sort(column.begin(), column.end(),
                [](wxAuiPaneInfo *x, wxAuiPaneInfo *y) {
                  return x->rect.y < y->rect.y;
                });
      for (size_t i = 0; i < column.size(); ++i) {
        column[i]->Position((int)i);
      }
      //  Share the column's spare height in favour of the new pane.
      now.dock_proportion = 300000;
      updatePanes();
      continue;
    }
    if (v == sizeof(victims) / sizeof(victims[0])) {
      return;
    }
    wxAuiPaneInfo &other = p_mgr.GetPane(wxString(victims[v++]));
    if (!other.IsOk() || !other.IsShown() || other.IsFloating() ||
        other.name == name || other.dock_direction != now.dock_direction) {
      continue;
    }
    other.Show(false);
    p_tabHidden.erase(other.window);
    const int id = p_toolMenu->FindItem(other.name);
    if (id != wxNOT_FOUND) {
      p_toolMenu->Check(id, false);
    }
    updatePanes();
  }
}


//  ---- test hook ------------------------------------------------------

/**
 * ECCE_PANEL_TEST=<file>: for each panel layout, open every property
 * panel through the Properties menu and every tool through the Tools menu,
 * as a click would, and check each one ends up shown with a real size;
 * check the tab buttons, F9, the list, and that switching layouts keeps
 * the open panels.  Writes one line per check to <file>, ending in
 * "RESULT ok" or "RESULT FAILED n".  Inert unless set.
 */
void Builder::runPanelLayoutTest()
{
  const char *path = getenv("ECCE_PANEL_TEST");
  if (!path || !p_calculation) {
    return;
  }
  FILE *out = fopen(path, "w");
  if (!out) {
    return;
  }
  int fails = 0;
  auto note = [&](const string& line) { fprintf(out, "%s\n", line.c_str()); };
  auto fail = [&](const string& line) { ++fails; note("FAIL " + line); };
  auto settle = [&]() {
    for (int i = 0; i < 8; ++i) {
      wxTheApp->Yield(true);
      wxMilliSleep(25);
    }
  };
  auto visible = [&](const wxString& name, string& why) {
    wxAuiPaneInfo &pane = p_mgr.GetPane(name);
    if (!pane.IsOk()) { why = "no pane"; return false; }
    if (!pane.IsShown()) { why = "pane not shown"; return false; }
    //  A folded pane is its caption bar: window hidden by design.
    const bool folded = p_folded.count(pane.window) > 0;
    if (!folded && !pane.window->IsShown()) { why = "window hidden"; return false; }
    if (!pane.IsFloating() && (pane.rect.width < 20 || pane.rect.height < 10)) {
      why = wxString::Format("size %dx%d", pane.rect.width,
                             pane.rect.height).ToStdString();
      return false;
    }
    return true;
  };

  vector<string> names;
  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(p_calculation->getURL().toString());
  for (set<PropertyPanel*>::iterator it = panels.begin(); it != panels.end();
       ++it) {
    names.push_back((*it)->getName());
  }
  note(wxString::Format("panels: %d", (int)names.size()).ToStdString());

  const PanelMode modes[] = { PANELS_CLASSIC, PANELS_STACKED,
                              PANELS_ACCORDION, PANELS_DETAIL };
  for (int m = 0; m < 4; ++m) {
    setPanelMode(modes[m]);
    settle();
    const string tag = string("[") + MODE_NAMES[modes[m]] + "] ";

    //  Every property panel, from the Properties menu.
    for (size_t i = 0; i < names.size(); ++i) {
      const int id = p_propertyMenu->FindItem(names[i]);
      if (id == wxNOT_FOUND) {
        fail(tag + names[i] + ": no Properties menu item");
        continue;
      }
      //  One at a time: all of them together cannot fit one screen, in
      //  any layout that stacks them, and that is not what is tested.
      wxAuiPaneInfoArray &now = p_mgr.GetAllPanes();
      for (size_t k = 0; k < now.GetCount(); ++k) {
        wxAuiPaneInfo &other = now.Item(k);
        if (dynamic_cast<PropertyPanel*>(other.window) &&
            other.name != names[i] && !other.IsFloating()) {
          other.Show(false);
          p_tabHidden.erase(other.window);
        }
      }
      wxCommandEvent off(wxEVT_MENU, id);
      off.SetInt(0);
      OnPropertyMenuClick(off);
      settle();
      wxCommandEvent on(wxEVT_MENU, id);
      on.SetInt(1);
      OnPropertyMenuClick(on);
      settle();
      string why;
      if (!visible(names[i], why)) {
        fail(tag + names[i] + ": " + why);
      } else if (p_folded.count(p_mgr.GetPane(names[i]).window)) {
        fail(tag + names[i] + ": opened but still folded");
      } else if (!p_propertyMenu->IsChecked(id)) {
        fail(tag + names[i] + ": shown but menu item not ticked");
      }
    }
    note(tag + "property menu checked for all panels");

    //  Every tool pane, from the Tools menu.
    for (int i = 0; i < p_toolCount; ++i) {
      const int id = ID_TOOLMENU_ITEM + i;
      wxMenuItem *item = p_toolMenu->FindItem(id);
      if (!item || !item->IsEnabled()) continue;
      const wxString label = item->GetItemLabelText();
      wxCommandEvent on(wxEVT_MENU, id);
      on.SetInt(1);
      for (int k = 0; k < p_toolCount; ++k) {
        wxMenuItem *oi = p_toolMenu->FindItem(ID_TOOLMENU_ITEM + k);
        if (!oi || oi->GetItemLabelText() == label ||
            oi->GetItemLabelText() == NAME_TOOL_CONTEXT ||
            oi->GetItemLabelText() == NAME_TOOL_LOG) continue;
        wxAuiPaneInfo &other = p_mgr.GetPane(oi->GetItemLabelText());
        if (other.IsOk() && !other.IsFloating()) {
          other.Show(false);
          p_tabHidden.erase(other.window);
        }
      }
      wxAuiPaneInfo &pane = p_mgr.GetPane(label);
      if (pane.IsOk() && label == NAME_TOOL_LOG.c_str()) {
        //  Collapsed to a caption line by design: expand it as a click on
        //  the caption would.
        if (p_folded.count(pane.window)) { foldPane(pane.window, false); updatePanes(); }
      }
      OnToolMenuClick(on);
      settle();
      string why;
      if (!visible(label, why)) {
        fail(tag + label.ToStdString() + ": " + why);
      }
    }
    note(tag + "tools menu checked for all tools");

    if (isColumnMode()) {
      //  The tab buttons.
      for (int tab = 0; tab < 2; ++tab) {
        setColumnTab(tab, false);
        settle();
        wxAuiPaneInfo &tabs = p_mgr.GetPane(NAME_COLUMN_TABS);
        if (!tabs.IsShown()) fail(tag + "tab buttons hidden");
        wxAuiPaneInfo &panesArr = p_mgr.GetPane(NAME_PROPERTY_INDEX);
        if (m == 3 && panesArr.IsShown() != (tab == 1)) {
          fail(tag + "index pane visibility wrong on tab " + std::to_string(tab));
        }
        wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
        for (size_t i = 0; i < all.GetCount(); ++i) {
          wxAuiPaneInfo &pane = all.Item(i);
          const PaneGroup g = paneGroup(pane);
          if (g == GROUP_NONE || pane.name == NAME_COLUMN_TABS ||
              pane.name == NAME_PROPERTY_INDEX) continue;
          const int pt = g == GROUP_PROPERTIES ? 1 : 0;
          if (pt != tab && pane.IsShown()) {
            fail(tag + pane.name.ToStdString() + " shown on the wrong tab");
          }
          if (pt == tab && paneWanted(pane)) {
            string why;
            if (!visible(pane.name, why)) {
              fail(tag + pane.name.ToStdString() + " open on tab " +
                   std::to_string(tab) + " but " + why);
            }
          }
        }
      }
      //  F9 and back.
      setColumnTab(1, false);
      settle();
      vector<wxString> before;
      wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
      for (size_t i = 0; i < all.GetCount(); ++i) {
        if (paneGroup(all.Item(i)) != GROUP_NONE && all.Item(i).IsShown())
          before.push_back(all.Item(i).name);
      }
      wxCommandEvent f9;
      OnViewToggleColumn(f9);
      settle();
      for (size_t i = 0; i < before.size(); ++i) {
        if (p_mgr.GetPane(before[i]).IsShown())
          fail(tag + before[i].ToStdString() + " still shown after F9");
      }
      OnViewToggleColumn(f9);
      settle();
      for (size_t i = 0; i < before.size(); ++i) {
        string why;
        if (!visible(before[i], why))
          fail(tag + before[i].ToStdString() + " not back after F9: " + why);
      }
      note(tag + "tabs and F9 checked");
    } else {
      //  Classic: collapsing hides the right-hand tools, keeps the left.
      vector<wxString> rightShown, leftShown;
      wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
      for (size_t i = 0; i < all.GetCount(); ++i) {
        wxAuiPaneInfo &pane = all.Item(i);
        if (!pane.window || pane.IsToolbar() || pane.IsFloating() ||
            !pane.IsShown() || pane.name == NAME_COLUMN_TOGGLE) continue;
        if (pane.dock_direction == wxAUI_DOCK_RIGHT) rightShown.push_back(pane.name);
        if (pane.dock_direction == wxAUI_DOCK_LEFT) leftShown.push_back(pane.name);
      }
      if (rightShown.empty()) fail(tag + "no right-hand tool open to collapse");
      wxCommandEvent f9;
      OnViewToggleColumn(f9);
      settle();
      for (size_t i = 0; i < rightShown.size(); ++i) {
        if (p_mgr.GetPane(rightShown[i]).IsShown())
          fail(tag + rightShown[i].ToStdString() + " still shown after collapse");
      }
      for (size_t i = 0; i < leftShown.size(); ++i) {
        string why;
        if (!visible(leftShown[i], why))
          fail(tag + leftShown[i].ToStdString() + " lost by collapse: " + why);
      }
      if (!p_mgr.GetPane(NAME_COLUMN_TOGGLE).IsShown())
        fail(tag + "arrow strip gone when collapsed");
      OnViewToggleColumn(f9);
      settle();
      for (size_t i = 0; i < rightShown.size(); ++i) {
        string why;
        if (!visible(rightShown[i], why))
          fail(tag + rightShown[i].ToStdString() + " not back after expand: " + why);
      }
      note(tag + "collapse checked");
    }

    if (m == 3) {
      for (size_t i = 0; i < names.size(); ++i) {
        bool listed = false;
        vector<string> list = p_index->listing();
        for (size_t k = 0; k < list.size(); ++k) {
          if (list[k].size() > names[i].size() &&
              list[k].compare(list[k].size() - names[i].size(),
                              names[i].size(), names[i]) == 0) listed = true;
        }
        if (!listed) fail(tag + names[i] + " missing from the list");
        selectDetail(names[i]);
        settle();
        string why;
        if (!visible(names[i], why)) fail(tag + names[i] + " from the list: " + why);
        if (!p_mgr.GetPane(names[i]).IsFloating()) {
          for (size_t k = 0; k < names.size(); ++k) {
            wxAuiPaneInfo &other = p_mgr.GetPane(names[k]);
            if (k != i && other.IsOk() && other.IsShown() &&
                !other.IsFloating())
              fail(tag + names[k] + " open beside the selected " + names[i]);
          }
        }
      }
      note(tag + "list checked");
    }

    //  The viewer's share of the window.
    wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
    for (size_t i = 0; i < all.GetCount(); ++i) {
      if (all.Item(i).dock_direction == wxAUI_DOCK_CENTER) {
        note(tag + wxString::Format("viewer %d of %d px wide",
                                    all.Item(i).rect.width,
                                    GetClientSize().x).ToStdString());
      }
    }
  }

  //  Switching keeps the open panels: open two in stacked, visit the
  //  others, and check both are reachable (shown, or in the list).
  if (names.size() >= 2) {
    setPanelMode(PANELS_STACKED);
    settle();
    for (size_t i = 0; i < names.size(); ++i) {
      wxAuiPaneInfo &pane = p_mgr.GetPane(names[i]);
      if (pane.IsOk() && !pane.IsFloating()) pane.Show(i < 2);
    }
    updatePanes();
    for (int m = 0; m < 4; ++m) {
      setPanelMode(modes[m]);
      settle();
      for (size_t i = 0; i < 2; ++i) {
        wxAuiPaneInfo &pane = p_mgr.GetPane(names[i]);
        if (pane.IsFloating()) continue;
        if (modes[m] == PANELS_DETAIL) {
          if (p_detail != p_mgr.GetPane(names[0]).window &&
              p_detail != p_mgr.GetPane(names[1]).window)
            fail(string("[switch] detail shows neither of the two open panels"));
        } else if (modes[m] == PANELS_ACCORDION) {
          if (!paneWanted(pane)) fail("[switch] " + names[i] + " lost in accordion");
        } else if (!paneWanted(pane)) {
          fail(string("[switch] ") + names[i] + " lost in " + MODE_NAMES[modes[m]]);
        }
      }
    }
    note("[switch] open panels kept across layouts");
  }

  setPanelMode(PANELS_DETAIL, true);
  settle();
  note(string("reset layout -> ") + MODE_NAMES[p_panelMode]);
  if (p_panelMode != PANELS_DETAIL) fail("reset layout did not give list + detail");

  note(fails ? "RESULT FAILED " + std::to_string(fails) : string("RESULT ok"));
  fflush(out);
  fclose(out);
  if (getenv("ECCE_EXIT_AFTER_DUMP")) {
    _exit(fails ? 1 : 0);
  }
}


/**
 * ECCE_CLIP_AUDIT=<file> with ECCE_CLIP_BUILDER=1: in each panel layout open
 * every property panel and tool alone, and every choice of the radio boxes
 * in it, and report the clipped controls of the Builder window each time
 * (ewxWindowUtils::clipAuditReport).  Ends with a "DONE" line.
 *
 * One step per timer tick, so the window is laid out and painted between
 * the steps (a nested event loop inside one handler never paints).
 */
void Builder::runClipAudit()
{
  const char *path = getenv("ECCE_CLIP_AUDIT");
  if (!path || !p_calculation) return;
  typedef std::function<void()> Step;
  std::shared_ptr<std::deque<Step> > steps(new std::deque<Step>());
  vector<string> names;
  set<PropertyPanel*> panels =
      PropertyPanel::getPanels(p_calculation->getURL().toString());
  for (set<PropertyPanel*>::iterator it = panels.begin(); it != panels.end();
       ++it) {
    names.push_back((*it)->getName());
  }
  const PanelMode modes[] = { PANELS_CLASSIC, PANELS_STACKED,
                              PANELS_ACCORDION, PANELS_DETAIL };
  //  What is shown, so a report that does not match the screenshot is
  //  visible in the file.
  auto note = [this, path](const string& tag) {
    FILE *f = fopen(path, "a");
    if (!f) return;
    fprintf(f, "NOTE\t%s\tmode %s;", tag.c_str(), MODE_NAMES[p_panelMode]);
    wxAuiPaneInfoArray &all = p_mgr.GetAllPanes();
    for (size_t i = 0; i < all.GetCount(); ++i)
      if (all.Item(i).IsShown())
        fprintf(f, " [%s %d,%d]", (const char*) all.Item(i).name.utf8_str(),
                all.Item(i).rect.width, all.Item(i).rect.height);
    fprintf(f, "\n");
    fclose(f);
  };
  for (int m = 0; m < 4; ++m) {
    const PanelMode mode = modes[m];
    const string tag = string(MODE_NAMES[mode]) + "-";
    steps->push_back([this, mode]() { setPanelMode(mode); });
    for (size_t i = 0; i < names.size(); ++i) {
      const string name = names[i];
      steps->push_back([this, name, mode]() {
        const int id = p_propertyMenu->FindItem(name);
        if (id == wxNOT_FOUND) return;
        wxAuiPaneInfoArray &now = p_mgr.GetAllPanes();
        for (size_t k = 0; k < now.GetCount(); ++k) {
          wxAuiPaneInfo &other = now.Item(k);
          if (dynamic_cast<PropertyPanel*>(other.window) &&
              other.name != name) {
            other.Show(false);
            p_tabHidden.erase(other.window);
          }
        }
        wxCommandEvent off(wxEVT_MENU, id);
        off.SetInt(0);
        OnPropertyMenuClick(off);
        wxCommandEvent on(wxEVT_MENU, id);
        on.SetInt(1);
        OnPropertyMenuClick(on);
        if (mode == PANELS_DETAIL) selectDetail(name);
      });
      steps->push_back([this, name, tag, steps, note]() {
        note(tag + name);
        ewxWindowUtils::clipAuditReport(this, tag + name);
        wxAuiPaneInfo &pane = p_mgr.GetPane(name);
        if (!pane.IsOk() || !pane.window) return;
        //  Every other choice of each radio box in the panel.
        vector<wxRadioBox*> boxes;
        vector<wxWindow*> todo(1, pane.window);
        while (!todo.empty()) {
          wxWindow *w = todo.back();
          todo.pop_back();
          if (wxRadioBox *rb = wxDynamicCast(w, wxRadioBox)) {
            boxes.push_back(rb);
            continue;
          }
          for (wxWindowList::compatibility_iterator n =
                   w->GetChildren().GetFirst(); n; n = n->GetNext())
            todo.push_back(n->GetData());
        }
        std::deque<Step> add;
        for (size_t b = 0; b < boxes.size(); ++b) {
          wxRadioBox *rb = boxes[b];
          const int keep = rb->GetSelection();
          auto choose = [rb](int sel) {
            rb->SetSelection(sel);
            wxCommandEvent ev(wxEVT_RADIOBOX, rb->GetId());
            ev.SetInt(sel);
            ev.SetEventObject(rb);
            rb->GetEventHandler()->ProcessEvent(ev);
          };
          for (int sel = 0; sel < (int)rb->GetCount(); ++sel) {
            if (sel == keep) continue;
            const string label = rb->GetString(sel).ToStdString();
            add.push_back([choose, sel]() { choose(sel); });
            add.push_back([this, tag, name, label]() {
              string clean;
              for (size_t c = 0; c < label.size(); ++c)
                if (label[c] != '&') clean += label[c];
              ewxWindowUtils::clipAuditReport(this, tag + name + "-" + clean);
            });
          }
          add.push_back([choose, keep]() { choose(keep); });
        }
        steps->insert(steps->begin(), add.begin(), add.end());
      });
    }
    for (int i = 0; i < p_toolCount; ++i) {
      steps->push_back([this, i, tag]() {
        wxMenuItem *item = p_toolMenu->FindItem(ID_TOOLMENU_ITEM + i);
        if (!item || !item->IsEnabled()) return;
        const wxString label = item->GetItemLabelText();
        if (label == NAME_TOOL_CONTEXT || label == NAME_TOOL_LOG) return;
        for (int k = 0; k < p_toolCount; ++k) {
          wxMenuItem *oi = p_toolMenu->FindItem(ID_TOOLMENU_ITEM + k);
          if (!oi || oi->GetItemLabelText() == label ||
              oi->GetItemLabelText() == NAME_TOOL_CONTEXT ||
              oi->GetItemLabelText() == NAME_TOOL_LOG) continue;
          wxAuiPaneInfo &other = p_mgr.GetPane(oi->GetItemLabelText());
          if (other.IsOk()) {
            other.Show(false);
            p_tabHidden.erase(other.window);
          }
        }
        wxCommandEvent on(wxEVT_MENU, ID_TOOLMENU_ITEM + i);
        on.SetInt(1);
        OnToolMenuClick(on);
        //  Reported by the step after.
        p_clipTag = tag + "tool-" + label.ToStdString();
      });
      steps->push_back([this]() {
        if (p_clipTag.empty()) return;
        ewxWindowUtils::clipAuditReport(this, p_clipTag);
        p_clipTag.clear();
      });
    }
  }
  steps->push_back([path]() {
    FILE *f = fopen(path, "a");
    if (f) { fprintf(f, "DONE\n"); fclose(f); }
  });
  wxTimer *timer = new wxTimer();   // lives until the process exits
  timer->Bind(wxEVT_TIMER, [steps, timer](wxTimerEvent&) {
    if (steps->empty()) { timer->Stop(); return; }
    Step step = steps->front();
    steps->pop_front();
    step();
  });
  timer->Start(1200);
}
