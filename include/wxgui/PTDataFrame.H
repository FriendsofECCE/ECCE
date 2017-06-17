/////////////////////////////////////////////////////////////////////////////
// Name:        PTDataFrame.H
// Purpose:     
// Author:      Lisong Sun
// Modified by: 
// Created:     Thu 04 Nov 2004 01:45:47 PM PST
// RCS-ID:      
// Licence:     
/////////////////////////////////////////////////////////////////////////////

#ifndef _PTDATAFRAME_H_
#define _PTDATAFRAME_H_

#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface "PTDataFrame.C"
#endif

#include "wx/frame.h"
#include "wx/notebook.h"
#include "wx/statline.h"
#include "wx/grid.h"

#include "tdat/TChartOfNuclides.H"
#include "tdat/TRefIsotope.H"
#include "wxgui/PTDataFrameGUI.H"
#include "wxgui/PerTabPanel.H"

#define ID_PTDATA_ISOTOPEGRID 30001

/*!
 * PTDataFrame class declaration
 */

class PTDataFrame: public PTDataFrameGUI
{
  DECLARE_DYNAMIC_CLASS( PTDataFrame )

public:
  PTDataFrame();
  ~PTDataFrame();
  PTDataFrame( int atomicNum, PerTabPanel * parent,
               wxWindowID id = SYMBOL_PTDATAFRAMEGUI_IDNAME,
               const wxString& caption = SYMBOL_PTDATAFRAMEGUI_TITLE,
               const wxPoint& pos = SYMBOL_PTDATAFRAMEGUI_POSITION,
               const wxSize& size = SYMBOL_PTDATAFRAMEGUI_SIZE,
               long style = SYMBOL_PTDATAFRAMEGUI_STYLE );

  void setContent(int atomicNum);
  void chooseColor();
  void setElementColor(ewxColor color);
  

  void OnCloseWindow( wxCloseEvent& event );
  void OnCloseClicked( wxCommandEvent& event );
  void OnHelpClicked( wxCommandEvent& event );
  void OnEditColorClicked( wxCommandEvent& event );
  void OnResetColorClicked( wxCommandEvent& event );
  void OnResetRadiiClicked( wxCommandEvent& event );
  void OnResetAllColorClicked( wxCommandEvent& event );
  void OnResetAllRadiiClicked( wxCommandEvent& event );
  void OnRadiiUpdated( wxCommandEvent& event );


protected:
  bool equal( double a, double b);

  PerTabPanel * p_parent;
  int p_atomicNum;
  ewxGrid * p_isotopeGrid;
  TChartOfNuclides * p_tChartNuclides;

};

#endif
    // _PTDATAFRAME_H_
