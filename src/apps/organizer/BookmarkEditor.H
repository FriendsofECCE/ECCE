/////////////////////////////////////////////////////////////////////////////
// Name:        BookmarkEditor.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _BOOKMARKEDITOR_H_
#define _BOOKMARKEDITOR_H_

#include "dsm/BookmarkList.H"

#include "BookmarkEditorGUI.H"


/*!
 * BookmarkEditor class declaration
 */
class BookmarkEditor: public BookmarkEditorGUI, public BookmarkList
{    
DECLARE_CLASS( BookmarkEditor )

public:
  /// Constructors
  BookmarkEditor( string prefFile,
                  bool hideAdd,
                  wxWindow* parent,
                  wxWindowID id = SYMBOL_BOOKMARKEDITORGUI_IDNAME,
                  const wxString& caption = SYMBOL_BOOKMARKEDITORGUI_TITLE,
                  const wxPoint& pos = SYMBOL_BOOKMARKEDITORGUI_POSITION,
                  const wxSize& size = SYMBOL_BOOKMARKEDITORGUI_SIZE,
                  long style = SYMBOL_BOOKMARKEDITORGUI_STYLE );

  ~BookmarkEditor( );


  /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME
  void OnCloseWindow( wxCloseEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_NAME
  void OnBookmarkNameUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_BOOKMARK_URL
  void OnBookmarkUrlUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_LISTBOX_SELECTED event handler for ID_LISTBOX
  void OnListboxSelected( wxCommandEvent& event );

  /// wxEVT_COMMAND_LISTBOX_DOUBLECLICKED event handler for ID_LISTBOX
  void OnListboxDoubleClicked( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_ADD
  void OnBookmarkAddClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DEL
  void OnBookmarkDelClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_UP
  void OnBookmarkUpClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BOOKMARK_DOWN
  void OnBookmarkDownClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CLOSE
  void OnCloseClick( wxCommandEvent& event );

  void updateListBox( int pos = 0);

protected:
  BookmarkEditor( );

  void updateNameUrl();

  /**
   * This variable is need by the p_name to decide whether the update is
   * caused by user keyboard input or SetValue.
   */
  bool p_userInput;

  bool p_userSelect;
};

#endif

