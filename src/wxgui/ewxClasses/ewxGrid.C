#include <algorithm>

#include <wx/clipbrd.h>
#include <wx/menu.h>
#include <wx/settings.h>

#include "wxgui/ewxGrid.H"
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ExportTableDialog.H"


IMPLEMENT_CLASS(ewxGrid, wxGrid)


const wxWindowID ewxGrid::ID_EXPORT = wxNewId();
const wxWindowID ewxGrid::ID_COPY = wxNewId();
const wxWindowID ewxGrid::ID_COPY_ALL = wxNewId();


BEGIN_EVENT_TABLE(ewxGrid, wxGrid)
  EVT_GRID_CELL_RIGHT_CLICK(ewxGrid::OnRightClick)
  EVT_GRID_LABEL_RIGHT_CLICK(ewxGrid::OnRightClick)
  EVT_MENU(ID_EXPORT, ewxGrid::OnExportClick)
  EVT_MENU(ID_COPY, ewxGrid::OnCopyClick)
  EVT_MENU(ID_COPY_ALL, ewxGrid::OnCopyClick)
  EVT_KEY_DOWN(ewxGrid::OnKeyDown)
END_EVENT_TABLE()


ewxGrid::ewxGrid(wxWindow* parent, wxWindowID id,
        const wxPoint& pos, const wxSize& size,
        long style, const wxString& name)
  : wxGrid(parent, id, pos, size, style, name)
  , ewxStyledWindow()
  , p_contextMenuEnabled(true)
  , p_clickRow(-1)
  , p_clickCol(-1)
{
  PushEventHandler(new ewxHelpHandler(this));

  setStyles(this);
  // Cells as a GTK list view draws them, labels as the window (#210).
  SetDefaultCellTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOXTEXT));
  SetDefaultCellBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
  SetDefaultCellFont(getNormalFont());
  SetLabelTextColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT));
  SetLabelBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
  SetLabelFont(getNormalFont());
  SetGridLineColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW));
  SetSelectionBackground(getFocusedSelectionColor());
  SetSelectionForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));

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
  menu.Append(ID_COPY, "Copy", "Copy the selected cells");
  menu.Append(ID_COPY_ALL, "Copy all",
              "Copy the whole table, tab-separated, for a spreadsheet");
  menu.AppendSeparator();
  menu.Append(ID_EXPORT, "Export...", "Save table data to a file");
}


//  One line per row: labels lose their line breaks ("Frequency\ncm-1"),
//  values the padding that aligns them in the cell.
static wxString clipCell(wxString text)
{
  text.Replace("\n", " ");
  text.Replace("\t", " ");
  return text.Strip(wxString::both);
}


wxString ewxGrid::copyText(bool all) const
{
  const int rows = GetNumberRows(), cols = GetNumberCols();
  wxString text;
  if (all) {
    for (int c = 0; c < cols; c++)
      text += (c ? "\t" : "") + clipCell(GetColLabelValue(c));
    text += "\n";
    for (int r = 0; r < rows; r++) {
      for (int c = 0; c < cols; c++)
        text += (c ? "\t" : "") + clipCell(GetCellValue(r, c));
      text += "\n";
    }
    return text;
  }
  //  The rectangle around the selection; cells outside it stay empty.
  int top = rows, left = cols, bottom = -1, right = -1;
  for (const wxGridBlockCoords& b : GetSelectedBlocks()) {
    top = std::min(top, b.GetTopRow());
    left = std::min(left, b.GetLeftCol());
    bottom = std::max(bottom, b.GetBottomRow());
    right = std::max(right, b.GetRightCol());
  }
  if (bottom < 0) {
    if (GetGridCursorRow() < 0 || GetGridCursorCol() < 0) return text;
    return clipCell(GetCellValue(GetGridCursorRow(), GetGridCursorCol()));
  }
  if (top == bottom && left == right) return clipCell(GetCellValue(top, left));
  for (int r = top; r <= bottom; r++) {
    for (int c = left; c <= right; c++)
      text += (c > left ? "\t" : "") +
              (IsInSelection(r, c) ? clipCell(GetCellValue(r, c)) : wxString());
    text += "\n";
  }
  return text;
}


void ewxGrid::toClipboard(const wxString& text)
{
  if (text.empty() || !wxTheClipboard->Open()) return;
  wxTheClipboard->SetData(new wxTextDataObject(text));
  wxTheClipboard->Close();
}


void ewxGrid::OnCopyClick(wxCommandEvent& event)
{
  if (event.GetId() == ID_COPY_ALL) {
    toClipboard(copyText(true));
  } else if (p_clickRow >= 0 && p_clickCol >= 0 &&
             !IsInSelection(p_clickRow, p_clickCol)) {
    //  A right-click outside the selection means that one cell.
    toClipboard(clipCell(GetCellValue(p_clickRow, p_clickCol)));
  } else {
    toClipboard(copyText(false));
  }
}


void ewxGrid::OnKeyDown(wxKeyEvent& event)
{
  if (event.GetKeyCode() == 'C' && event.GetModifiers() == wxMOD_CONTROL)
    toClipboard(copyText(false));
  else
    event.Skip();
}


void ewxGrid::doExport()
{
  ExportTableDialog dialog(this, this);
  dialog.ShowModal();
}


void ewxGrid::OnRightClick(wxGridEvent& event)
{
  if (p_contextMenuEnabled) {
    p_clickRow = event.GetRow();
    p_clickCol = event.GetCol();
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

