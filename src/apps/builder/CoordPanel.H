/**
 * @file CoordPanel.H
 */

#ifndef _COORDPANEL_H_
#define _COORDPANEL_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "CoordPanel.C"
#endif

#include "wxviz/WxVizTool.H"

#include "CoordPanelGUI.H"


/*!
 * CoordPanel class declaration
 */
class CoordPanel: public CoordPanelGUI, public WxVizTool
{    
  DECLARE_DYNAMIC_CLASS( CoordPanel )

public:
  /// Constructors
  CoordPanel( );
  CoordPanel( wxWindow* parent, wxWindowID id = SYMBOL_COORDPANELGUI_IDNAME,
              const wxPoint& pos = SYMBOL_COORDPANELGUI_POSITION,
              const wxSize& size = SYMBOL_COORDPANELGUI_SIZE,
              long style = SYMBOL_COORDPANELGUI_STYLE );
  bool Create( wxWindow* parent, wxWindowID id = SYMBOL_COORDPANELGUI_IDNAME,
               const wxPoint& pos = SYMBOL_COORDPANELGUI_POSITION,
               const wxSize& size = SYMBOL_COORDPANELGUI_SIZE,
               long style = SYMBOL_COORDPANELGUI_STYLE );
  void initializeGUI();

  /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_CENTER_ORIGIN
  virtual void OnCenterSelected( wxCommandEvent& event );

  /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_ALIGN_TO_AXIS
  virtual void OnAlignToAxisSelected( wxCommandEvent& event );

  /// wxEVT_COMMAND_CHOICE_SELECTED event handler for ID_ALIGN_TO_PLANE
  virtual void OnAlignToPlaneSelected( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_X_YZ
  virtual void OnButtonAlignX_YZClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Y_ZX
  virtual void OnButtonAlignY_ZXClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_ALIGN_Z_XY
  virtual void OnButtonAlignZ_XYClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_CENTER
  virtual void OnCenterClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_BY_X
  virtual void OnTransByXEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_BY_Y
  virtual void OnTransByYEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_BY_Z
  virtual void OnTransByZEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_TRANS_EXPAND
  virtual void OnTransExpandClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_FROM_X
  virtual void OnTransFromXEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_FROM_Y
  virtual void OnTransFromYEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_FROM_Z
  virtual void OnTransFromZEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_FROM_PICK
  virtual void OnTransFromPickClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_TO_X
  virtual void OnTransToXEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_TO_Y
  virtual void OnTransToYEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TRANS_TO_Z
  virtual void OnTransToZEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANS_TO_PICK
  virtual void OnTransToPickClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_TRANSLATE
  virtual void OnTranslateClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_X
  virtual    void OnDuplicateDim1XEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Y
  virtual    void OnDuplicateDim1YEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM1_Z
  virtual    void OnDuplicateDim1ZEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_X
  virtual    void OnDuplicateDim2XEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Y
  virtual    void OnDuplicateDim2YEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM2_Z
  virtual    void OnDuplicateDim2ZEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_X
  virtual    void OnDuplicateDim3XEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Y
  virtual    void OnDuplicateDim3YEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_DUPLICATE_DIM3_Z
  virtual    void OnDuplicateDim3ZEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_START_X
  virtual void OnRotateStartUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_START_PICK
  virtual void OnRotateStartPickClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_ROTATE_END_X
  virtual void OnRotateEndUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE_END_PICK
  virtual void OnRotateEndPickClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ROTATE
  virtual void OnRotateClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_RADIOBUTTON_SELECTED event handler for ID_ALIGN_TO_X
  virtual void OnAlignToSelected( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ALIGN
  virtual void OnAlignClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_DUPLICATE_DIM2_COUNT
  virtual void OnDuplicateDim2CountUpdated( wxSpinEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DUPLICATE
  virtual void OnDuplicateClick( wxCommandEvent& event );
};

#endif
