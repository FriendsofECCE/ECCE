/////////////////////////////////////////////////////////////////////////////
// Name:        PopupFrame.H
// Purpose:     
// Author:      
// Modified by: 
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _POPUPFRAME_H_
#define _POPUPFRAME_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PopupFrame.C"
#endif

/*!
 * Includes
 */

////@begin includes
#include "wxgui/ewxFrame.H"
////@end includes

/*!
 * Forward declarations
 */

////@begin forward declarations
////@end forward declarations

/*!
 * Control identifiers
 */

////@begin control identifiers
#define ID_FRAME_POPUP 22600
#define SYMBOL_POPUPFRAME_STYLE wxCAPTION|wxRESIZE_BORDER|wxCLOSE_BOX|wxFRAME_TOOL_WINDOW|wxFRAME_FLOAT_ON_PARENT
#define SYMBOL_POPUPFRAME_TITLE _("ECCE stuff")
#define SYMBOL_POPUPFRAME_IDNAME ID_FRAME_POPUP
#define SYMBOL_POPUPFRAME_SIZE wxSize(600, 400)
#define SYMBOL_POPUPFRAME_POSITION wxDefaultPosition
////@end control identifiers

/*!
 * Compatibility
 */

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif

/*!
 * PopupFrame class declaration
 */

class PopupFrame: public ewxFrame, public Listener
{    
   DECLARE_CLASS( PopupFrame )
      DECLARE_EVENT_TABLE()

   public:
      /// Constructors
      PopupFrame( );
      PopupFrame( wxWindow* parent, wxWindowID id = SYMBOL_POPUPFRAME_IDNAME, const wxString& caption = SYMBOL_POPUPFRAME_TITLE, const wxPoint& pos = SYMBOL_POPUPFRAME_POSITION, const wxSize& size = SYMBOL_POPUPFRAME_SIZE, long style = SYMBOL_POPUPFRAME_STYLE );

      ~PopupFrame( );

      bool Create( wxWindow* parent, wxWindowID id = SYMBOL_POPUPFRAME_IDNAME, const wxString& caption = SYMBOL_POPUPFRAME_TITLE, const wxPoint& pos = SYMBOL_POPUPFRAME_POSITION, const wxSize& size = SYMBOL_POPUPFRAME_SIZE, long style = SYMBOL_POPUPFRAME_STYLE );

      void setNotifyOnClose(bool flag);

      void eventMCB(const Event& event);

   protected:
      /// Creates the controls and sizers
      void CreateControls();

      /// wxEVT_CLOSE_WINDOW event handler for ID_FRAME_POPUP
      virtual    void mainWindowCloseCB( wxCloseEvent& event );


      /// Retrieves bitmap resources
      wxBitmap GetBitmapResource( const wxString& name );

      /// Retrieves icon resources
      wxIcon GetIconResource( const wxString& name );

      /// Should we show tooltips?
      static bool ShowToolTips();

      bool p_notifyClose;

};

#endif
