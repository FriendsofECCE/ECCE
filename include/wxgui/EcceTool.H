/**
 * @file
 * @author Lisong Sun
 *
 * ECCE tool button class
 *
 */

#ifndef _ECCETOOL_H_
#define _ECCETOOL_H_

#include "wx/wxprec.h"

// For compilers that support precompilation, includes "wx/wx.h".
#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxPanel.H"
#define ID_ECCETOOL_NEW 102300


class ResourceTool;


class InvokeParam : public wxClientData{
public:
  int forceNew;
  string name;
};


class EcceTool: public ewxPanel {

  DECLARE_EVENT_TABLE()

public:

  EcceTool();
  ~EcceTool();
  EcceTool(wxWindow * parent, ResourceTool * resTool);
  EcceTool(wxWindow * parent, const wxString& name);
  EcceTool(wxWindow * parent, wxWindowID id,
           const wxString& name, const wxBitmap& bitmap);

  void Create(wxWindow * parent, wxWindowID id, const wxString& name);

  void setStatus(bool isSunken);
  bool isSunken();

  void OnMouseLeftDown( wxMouseEvent& event );
  void OnMouseLeftUp( wxMouseEvent& event );
  void OnMouseRightDown( wxMouseEvent& event );
  void OnMouseEnterWindow( wxMouseEvent& event );
  void OnMouseLeaveWindow( wxMouseEvent& event );
  void OnPaint( wxPaintEvent& event );
  //  void OnMenuClick( wxCommandEvent& event );

  void toolActivate(bool forceNew);
  void setBitMap(wxBitmap bitmap);

protected:

  InvokeParam * p_invokeParam;
  wxPaintDC * p_dc;
  wxBitmap p_bitmap;
  //  wxMenu * p_newMenu;
  bool p_isSunken;
};

#endif
