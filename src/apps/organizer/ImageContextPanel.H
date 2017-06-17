/**
 * @file
 *
 * @author Lisong Sun
 *
 *
 */

#ifndef _IMAGECONTEXTPANEL_H
#define _IMAGECONTEXTPANEL_H

#include "wx/panel.h"
#include "wx/image.h"

#include "ImageContextPanelGUI.H"
#include "ContextPanelInterface.H"

class WxResourceTreeItemData;
class ContextPanelFactory;
class CalcMgr;

/**
 * Document view.
 * Displays the contents of the file when recognized as a text format.
 */
class ImageContextPanel : public ImageContextPanelGUI,
                          public ContextPanelInterface
{

public:

  friend class ContextPanelFactory;

  ~ImageContextPanel();

  void refresh();

protected:

  ImageContextPanel( CalcMgr * calcMgr,
                     WxResourceTreeItemData *itemData, 
                     wxWindow *parent, wxWindowID id = -1,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize, 
                     long style = wxTAB_TRAVERSAL );

  /// wxEVT_LEFT_DOWN event handler for ID_SCROLLEDWINDOW
  virtual void OnLeftDown( wxMouseEvent& event );
  
  /// wxEVT_LEFT_UP event handler for ID_SCROLLEDWINDOW
  virtual void OnLeftUp( wxMouseEvent& event );
  
  /// wxEVT_MOTION event handler for ID_SCROLLEDWINDOW
  virtual void OnMotion( wxMouseEvent& event );
  
  /// wxEVT_MOUSEWHEEL event handler for ID_SCROLLEDWINDOW
  virtual void OnMouseWheel( wxMouseEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_100
  virtual void OnZoom100Click( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_FIT
  virtual void OnZoomFitClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_IN
  virtual void OnZoomInClick( wxCommandEvent& event );

  /// wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_OUT
  virtual void OnZoomOutClick( wxCommandEvent& event );

  virtual void OnPaint( wxPaintEvent& event );

  virtual void redraw();

  virtual void autoScroll( wxMouseEvent& event );
  
  /** Pointer to a tree item containing the property data. */
  WxResourceTreeItemData *p_itemData;

  wxImage p_image;

  double p_scale;

  static const double p_scaleStep;

  int p_width, p_height;
  long p_startX;
  long p_startY;
};

#endif
