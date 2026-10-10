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

#include "wx/bmpbndl.h"
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
  EcceTool(wxWindow * parent, ResourceTool * resTool, const wxString& label);
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
  void OnFocus( wxFocusEvent& event );
  void OnKeyDown( wxKeyEvent& event );
  //  void OnMenuClick( wxCommandEvent& event );

  void toolActivate(bool forceNew);
  void setBitMap(wxBitmap bitmap);
  void clearBitMap();   // back to the tool's own icon
  const wxString& getLabel() const { return p_label; }

protected:

  InvokeParam * p_invokeParam;
  wxPaintDC * p_dc;
  wxBitmap p_bitmap;
  wxBitmapBundle p_bundle;   // the SVG icon, sharp at any scale
  wxString p_label;          // drawn under the icon as text
  wxBitmap p_thumbnail;      // setBitMap's picture, in the icon's place
  bool p_isHover;
  //  wxMenu * p_newMenu;
  bool p_isSunken;
};

#endif
