/**
 * @file SelectionPanel.H
 */

#ifndef _SELECTIONPANEL_H_
#define _SELECTIONPANEL_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "SelectionPanel.C"
#endif

#include "wxviz/WxVizTool.H"
#include "wxviz/SelectionPanelGUI.H"
#include "util/Listener.H"
using namespace ecce;


/*!
 * SelectionPanel class declaration
 */
class SelectionPanel: public SelectionPanelGUI,
                      public WxVizTool,
                      public Listener
{    
DECLARE_DYNAMIC_CLASS( SelectionPanel )

public:
  /// Constructors
  SelectionPanel( );
  SelectionPanel( wxWindow* parent,
                  wxWindowID id = SYMBOL_SELECTIONPANELGUI_IDNAME,
                  const wxPoint& pos = SYMBOL_SELECTIONPANELGUI_POSITION,
                  const wxSize& size = SYMBOL_SELECTIONPANELGUI_SIZE,
                  long style = SYMBOL_SELECTIONPANELGUI_STYLE );
  void initializeGUI();

  // Event handlers

  /// wxEVT_COMMAND_COMBOBOX_SELECTED event handler for ID_COMBOBOX_ELMNT
  virtual    void OnComboboxElmntSelected( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_COMBOBOX_ELMNT
  virtual    void OnComboboxElmntUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_COMBOBOX_ELMNT
  virtual    void OnComboboxElmntEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_FRST_AT
  virtual    void OnSpinctrlFrstAtUpdated( wxSpinEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_FRST_AT
  virtual    void OnSpinctrlFrstAtTextUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_LAST_AT
  virtual    void OnSpinctrlLastAtUpdated( wxSpinEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_LAST_AT
  virtual    void OnSpinctrlLastAtTextUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_SELECT
  virtual    void OnAtomSelectClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_XSELECT
  virtual    void OnAtomXselectClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_INVERSE
  virtual    void OnAtomInverseClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_CLEAR
  virtual    void OnAtomClearClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_ATOM_ADD_GRP
  virtual    void OnAtomAddGrpClick( wxCommandEvent& event);

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ATYP
  virtual    void OnTextctrlResAtypUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RES_ANAM
  virtual    void OnTextctrlResAnamUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_RESNAM
  virtual    void OnTextctrlResnamUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_CHAIN
  virtual    void OnTextctrlChainUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_SELECT
  virtual    void OnButtonResSelectClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_XSELECT
  virtual    void OnButtonResXselectClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_INVERSE
  virtual    void OnButtonResInverseClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_CLEAR
  virtual    void OnButtonResClearClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_RES_ADD_GRP
  virtual    void OnButtonResAddGrpClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MINDIS
  virtual    void OnTextctrlMindisUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MINDIS
  virtual    void OnTextctrlMindisEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_TEXTCTRL_MAXDIS
  virtual    void OnTextctrlMaxdisUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_TEXT_ENTER event handler for ID_TEXTCTRL_MAXDIS
  virtual    void OnTextctrlMaxdisEnter( wxCommandEvent& event );

  /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MINBND
  virtual    void OnSpinctrlMinbndUpdated( wxSpinEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MINBND
  virtual    void OnSpinctrlMinbndTextUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_SPINCTRL_UPDATED event handler for ID_SPINCTRL_MAXBND
  virtual    void OnSpinctrlMaxbndUpdated( wxSpinEvent& event );

  /// wxEVT_COMMAND_TEXT_UPDATED event handler for ID_SPINCTRL_MAXBND
  virtual    void OnSpinctrlMaxbndTextUpdated( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_SELECT
  virtual    void OnButtonDisSelectClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_XSELECT
  virtual    void OnButtonDisXselectClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_INVERSE
  virtual    void OnButtonDisInverseClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_DIS_CLEAR
  virtual    void OnButtonDisClearClick( wxCommandEvent& event );


  // Utility functions

  void atomSelection(bool clear, bool reverse);

  void residueSelection(bool clear, bool reverse);

  virtual void eventMCB(const Event& event);

private:

  string p_oldElement;
};

#endif
