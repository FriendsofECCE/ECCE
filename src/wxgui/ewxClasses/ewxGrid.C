#include <wx/menu.h>
#include <wx/settings.h>

#include "wxgui/ewxGrid.H"
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ExportTableDialog.H"


IMPLEMENT_CLASS(ewxGrid, wxGrid)


const wxWindowID ewxGrid::ID_EXPORT = wxNewId();


BEGIN_EVENT_TABLE(ewxGrid, wxGrid)
  EVT_GRID_CELL_RIGHT_CLICK(ewxGrid::OnRightClick)
  EVT_GRID_LABEL_RIGHT_CLICK(ewxGrid::OnRightClick)
  EVT_MENU(ID_EXPORT, ewxGrid::OnExportClick)
END_EVENT_TABLE()


ewxGrid::ewxGrid(wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size,
        long style, const wxString& name)
  : wxGrid(parent, id, pos, size, style, name)
  , ewxStyledWindow()
  , p_contextMenuEnabled(true)
{
  PushEventHandler(new ewxHelpHandler(this));

  setStyles(this);
  SetCellTextColour(getTextColor());
  SetDefaultCellBackgroundColour(getWindowColor());
  SetCellTextFont(getNormalFont());
  SetLabelTextColour(getTextColor());
  SetLabelBackgroundColour(getWindowColor());
  SetLabelFont(getNormalFont());
  SetGridLineColour(wxSystemSettings::GetColour(wxSYS_COLOUR_3DDKSHADOW));
  SetSelectionBackground(getFocusedSelectionColor());
  SetSelectionForeground("WHITE");

  SetColLabelAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);
  SetRowLabelAlignment(wxALIGN_LEFT, wxALIGN_CENTRE);

  SetDefaultCellAlignment(wxALIGN_CENTRE, wxALIGN_CENTRE);

  EnableGridLines(true);

  SetDefaultRowSize(20, true);
  SetColLabelSize(25);
  
  AutoSize();
}


ewxGrid::~ewxGrid()
{
  PopEventHandler(true);
}


void ewxGrid::SetReadOnly(bool readOnly)
{
  EnableDragColSize(!readOnly);
  EnableDragRowSize(!readOnly);
  EnableEditing(!readOnly);
}


void ewxGrid::enableContextMenu(bool enable)
{
  p_contextMenuEnabled = enable;
}


void ewxGrid::getContextMenu(wxMenu& menu)
{
  menu.Append(ID_EXPORT, "Export...", "Save table data to a file");
}


void ewxGrid::doExport()
{
  ExportTableDialog dialog(this, this);
  dialog.ShowModal();
}


void ewxGrid::OnRightClick(wxGridEvent& event)
{
  if (p_contextMenuEnabled) {
    wxMenu menu;
    getContextMenu(menu);
    PopupMenu(&menu);
  }
  event.Skip();
}


void ewxGrid::OnExportClick(wxCommandEvent& event)
{
  doExport();
  event.Skip();
}

