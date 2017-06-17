/////////////////////////////////////////////////////////////////////////////
// Name:        ewxColorDialog.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// Created:     Tue 16 Nov 2004 04:14:28 PM PST
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _EWXCOLORDIALOG_H_
#define _EWXCOLORDIALOG_H_

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>

#include "wxgui/ewxButton.H"
#include "wxgui/ewxColor.H"
#include "wxgui/ewxDialog.H"
#include "wxgui/ewxSpinCtrl.H"
#include "wxgui/ewxStyledWindow.H"

#define ID_COLORBUTTON 10001
#define SYMBOL_COLORBUTTON_STYLE wxSUNKEN_BORDER
#define SYMBOL_COLORBUTTON_IDNAME ID_COLORBUTTON
#define SYMBOL_COLORBUTTON_SIZE wxSize(20, 15)
#define SYMBOL_COLORBUTTON_POSITION wxDefaultPosition

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif
#ifndef wxFIXED_MINSIZE
#define wxFIXED_MINSIZE 0
#endif

class ColorButton: public wxPanel {

  DECLARE_EVENT_TABLE()

public:
  /// Constructors
  ColorButton();
  ~ColorButton();
  ColorButton( wxColour color, wxWindow* parent,
               wxWindowID id = SYMBOL_COLORBUTTON_IDNAME,
               const wxPoint& pos = SYMBOL_COLORBUTTON_POSITION,
               const wxSize& size = SYMBOL_COLORBUTTON_SIZE,
               long style = SYMBOL_COLORBUTTON_STYLE );

  /// wxEVT_LEFT_DOWN event handler for ID_DIALOG1
  void OnPanelLeftDown( wxMouseEvent& event );
};


#define ID_EWXCOLORDIALOG 10000
#define SYMBOL_EWXCOLORDIALOG_STYLE wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX|wxTAB_TRAVERSAL
#define SYMBOL_EWXCOLORDIALOG_TITLE "ECCE Color Chooser"
#define SYMBOL_EWXCOLORDIALOG_IDNAME ID_EWXCOLORDIALOG
#define SYMBOL_EWXCOLORDIALOG_SIZE wxSize(400, 300)
#define SYMBOL_EWXCOLORDIALOG_POSITION wxDefaultPosition
#define ID_CURRENTCOLOR 10001

#define ID_SLIDER_RED 10002
#define ID_SLIDER_GREEN 10003
#define ID_SLIDER_BLUE 10004
#define ID_SPINCTRL_RED 10005
#define ID_SPINCTRL_GREEN 10006
#define ID_SPINCTRL_BLUE 10007

#ifndef wxCLOSE_BOX
#define wxCLOSE_BOX 0x1000
#endif
#ifndef wxFIXED_MINSIZE
#define wxFIXED_MINSIZE 0
#endif




class ewxColorDialog: public ewxDialog {

  DECLARE_EVENT_TABLE()

public:
  /// Constructors
  ewxColorDialog();
  ~ewxColorDialog();
  ewxColorDialog( wxColour color, wxWindow* parent,
		  wxWindowID id = SYMBOL_EWXCOLORDIALOG_IDNAME,
		  const wxString& caption = SYMBOL_EWXCOLORDIALOG_TITLE,
		  const wxPoint& pos = SYMBOL_EWXCOLORDIALOG_POSITION,
		  const wxSize& size = SYMBOL_EWXCOLORDIALOG_SIZE,
		  long style = SYMBOL_EWXCOLORDIALOG_STYLE );


  /// Creation
  bool Create( wxColour color, wxWindow* parent,
	       wxWindowID id = SYMBOL_EWXCOLORDIALOG_IDNAME,
	       const wxString& caption = SYMBOL_EWXCOLORDIALOG_TITLE,
	       const wxPoint& pos = SYMBOL_EWXCOLORDIALOG_POSITION,
	       const wxSize& size = SYMBOL_EWXCOLORDIALOG_SIZE,
	       long style = SYMBOL_EWXCOLORDIALOG_STYLE );

  void SetColor( wxColour color );
  wxColour GetColor();

protected:
  /// Creates the controls and sizers
  void CreateControls( wxColour color );

  /// wxEVT_SCROLL_* event handler for ID_SLIDER3
  void OnSliderScroll( wxScrollEvent& event );

  /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL3
  void OnSpinctrlUpdated( wxSpinEvent& event );

  void UpdateAll();

  wxGridSizer* p_basicColorGrid;
  wxGridSizer* p_customColorGrid;
  
  wxSlider * p_sliderRed;
  wxSlider * p_sliderGreen;
  wxSlider * p_sliderBlue;
  ewxSpinCtrl * p_spinRed;
  ewxSpinCtrl * p_spinGreen;
  ewxSpinCtrl * p_spinBlue;

  wxPanel * p_currentColorPanel;

  int p_red, p_green, p_blue;
  
  static const ewxColor PredefinedColors[];

};

#endif
// _EWXCOLORDIALOG_H_
