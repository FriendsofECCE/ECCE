/**
 * @file
 * @author Lisong Sun
 * 
 * 
 * Note:
 * This class was first designed as a modeless dialog. But it will cause trouble
 * for the caller to invoke updates based on the change in the dialog. For
 * example, when to let the calcmgr refresh the bookmark menu.
 *
 */

//#include <iostream>
//using namespace std;

#include "util/Bookmark.H"

#include "dsm/BookmarkList.H"

#include "wxgui/ewxButton.H"
#include "wxgui/ewxListBox.H"
#include "wxgui/ewxTextCtrl.H"

#include "BookmarkEditor.H"


IMPLEMENT_CLASS( BookmarkEditor, BookmarkEditorGUI )


/*!
 * BookmarkEditor constructors
 */
BookmarkEditor::BookmarkEditor( string prefFile,
                                bool hideAdd,
                                wxWindow* parent,
                                wxWindowID id,
                                const wxString& caption,
                                const wxPoint& pos,
                                const wxSize& size,
                                long style ):
  BookmarkEditorGUI( parent, id, caption, pos, size, style ),
  BookmarkList(prefFile)
{
  if (hideAdd) {
    FindWindow(ID_BOOKMARK_ADD)->Show(false);
    p_url->SetEditable(false);
    p_url->SetBackgroundColour(ewxStyledWindow::getReadonlyColor());
  }

  updateListBox();
  p_userInput = true;
  p_userSelect = true;
}


BookmarkEditor::~BookmarkEditor( )
{}


/*!
 * wxEVT_CLOSE_WINDOW event handler for ID_FRAME
 */
void BookmarkEditor::OnCloseWindow( wxCloseEvent& event )
{
  saveToFile();
  EndModal(0);
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_NAME
 */
void BookmarkEditor::OnBookmarkNameUpdated( wxCommandEvent& event )
{
  if (p_userInput) {
    int pos = p_listBox->GetSelection();
    if (pos == wxNOT_FOUND) {
      updateNameUrl();
    }
    else {
      wxString value = p_name->GetValue();
      p_userSelect = false;
      p_listBox->SetString(pos, value);
      p_userSelect = true;
      setBookmarkName(pos, value.c_str());
    }
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_URL
 */
void BookmarkEditor::OnBookmarkUrlUpdated( wxCommandEvent& event )
{
  if (p_userInput) {
    int pos = p_listBox->GetSelection();
    if (pos == wxNOT_FOUND) {
      updateNameUrl();
    }
    else {
      wxString value = p_url->GetValue();
      setBookmarkUrl(pos, value.c_str());
    }
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX
 */
void BookmarkEditor::OnListboxSelected( wxCommandEvent& event )
{
  if (p_userSelect)
    updateNameUrl();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX
 */
void BookmarkEditor::OnListboxDoubleClicked( wxCommandEvent& event )
{
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_ADD
 */
void BookmarkEditor::OnBookmarkAddClick( wxCommandEvent& event )
{
  addBookmark("New Name", "http://");
  updateListBox(getSize()-1);
  p_name->SetFocus();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DEL
 */
void BookmarkEditor::OnBookmarkDelClick( wxCommandEvent& event )
{
  int pos = p_listBox->GetSelection();
  if (pos != wxNOT_FOUND) {
    removeBookmark(pos);
    // See if deleting the last one.
    if (pos == getSize())
      pos --;
    updateListBox(pos);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_UP
 */
void BookmarkEditor::OnBookmarkUpClick( wxCommandEvent& event )
{
  int pos = p_listBox->GetSelection();
  if (pos != wxNOT_FOUND) {
    swap(pos, pos - 1);
    updateListBox(pos - 1);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DOWN
 */
void BookmarkEditor::OnBookmarkDownClick( wxCommandEvent& event )
{
  int pos = p_listBox->GetSelection();
  if (pos != wxNOT_FOUND) {
    swap(pos, pos + 1);
    updateListBox(pos + 1);
  }
  event.Skip();
}


/*!
 * wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
 */
void BookmarkEditor::OnCloseClick( wxCommandEvent& event )
{
  wxCloseEvent evt;
  OnCloseWindow(evt);
}


void BookmarkEditor::updateNameUrl()
{
  p_userInput = false;

  int pos = p_listBox->GetSelection();
  
  p_delBut->Enable(getSize() != 0 && pos >= 0);
  p_upBut->Enable(getSize() != 0 && pos > 0);
  p_downBut->Enable(getSize() != 0 && pos < getSize()-1 && pos >= 0);

  if (getSize() == 0) {
    p_name->setCustomDisabledStyle(false);
    p_url->setCustomDisabledStyle(false);
    p_name->SetValue("Click \"Add\" button or Alt+A to add new entry");
    p_url->SetValue("");
  }
  else {
    p_name->setCustomDisabledStyle(true);
    p_url->setCustomDisabledStyle(true);
    p_name->SetValue(getBookmarkName(pos));
    p_url->SetValue(getBookmarkUrl(pos));
  }

  p_userInput = true;
}


void BookmarkEditor::updateListBox(int pos)
{
  p_listBox->Clear();

  for (int i=0; i<getSize(); i++) {
    p_listBox->Append(getBookmarkName(i).c_str());
  }

  p_listBox->SetSelection(pos);
  updateNameUrl();
}
