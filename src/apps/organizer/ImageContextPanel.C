/**
 * @file
 *
 * @author Lisong Sun
 *
 * A image viewer panel that support basic zooming functions.
 *
 * Note it is using staticbitmap and according to staticbitmap's documentation
 * it should only be used for small icon images. If it no longer works, need to
 * switch to use paintDC instead.
 *
 */

#include <fstream>
using std::ofstream;

#include <iostream>
using std::cerr;

#include <strstream>
using std::ostrstream;

#include <algorithm>
using std::min;


#include <wx/mstream.h>
#include <wx/sstream.h>
#include <wx/statbmp.h>

#include "util/TempStorage.H"
#include "util/StringConverter.H"
#include "util/SFile.H"

#include "dsm/EcceDAVClient.H"
#include "dsm/EDSIFactory.H"
#include "dsm/Resource.H"

#include "wxgui/ewxPanel.H"
#include "wxgui/ewxScrolledWindow.H"
#include "wxgui/WxResourceTreeItemData.H"

#include "ImageContextPanel.H"

class CalcMgr;


/**
 * scale step controls the speed of zoom in and out.
 */
const double ImageContextPanel::p_scaleStep = 1.2;


/**
 * Constructor.
 */
ImageContextPanel::ImageContextPanel(CalcMgr * calcMgr,
                                     WxResourceTreeItemData *itemData, 
                                     wxWindow *parent, 
                                     wxWindowID id,
                                     const wxPoint& pos,
                                     const wxSize& size, 
                                     long style):
  ImageContextPanelGUI(parent, id, pos, size, style),
  ContextPanelInterface(calcMgr)
{
  // Need it to load wxImage from various format image files
  wxInitAllImageHandlers();

  p_itemData = itemData;
  p_staticBitmap->SetEventHandler(this);
  p_startX = -1;
  p_startY = -1;

  refresh();

  /*
  cerr << "Image size: " << p_bitmap->GetWidth()
       << "\t" << p_bitmap->GetHeight() << "\n";
  cerr << "Splitter size: " << GetParent()->GetSize().GetWidth()
       << "\t" << GetParent()->GetSize().GetHeight() << "\n";
  cerr << "Panel size: " << GetSize().GetWidth()
       << "\t" << GetSize().GetHeight() << "\n";
  cerr << "ScrollWin size: " << p_imageWindow->GetSize().GetWidth()
       << "\t" << p_imageWindow->GetSize().GetHeight() << "\n";
  */

  /*
   Tried to use stream but not working, used temp file instead
  wxMemoryInputStream is(os.str(), strlen(os.str()));
  wxImage image(is);
  if (p_bitmap != NULL) delete p_bitmap;
  p_bitmap = new wxBitmap(image);
  */
}


/**
 * Destructor.
 */
ImageContextPanel::~ImageContextPanel()
{
  p_itemData = 0;
}


/**
 * Refresh function to reload image without recreate the panel
 */
void ImageContextPanel::refresh()
{
  SFile * sfile = TempStorage::getTempFile();
  p_itemData->getResource()->getDocument(sfile);

  string fileExt = StringConverter::getFileNameExt(p_itemData->getName());
  sfile->move(sfile->path() + "." + fileExt);

  p_image = wxImage(_T(sfile->path()));
  sfile->remove();
  delete sfile;

  p_width = p_image.GetWidth();
  p_height = p_image.GetHeight();
  p_scale = 1.0;

  redraw();
}


/*!
 * wxEVT_PAINT event handler for ID_IMAGE_PANEL
 */
void ImageContextPanel::OnPaint( wxPaintEvent& event )
{
  /*
    This also works and should use this if wxStaticBitmap is not working
    There is a strange flickering when scrolling.
  wxPaintDC dc(p_imageWindow);
  p_imageWindow->PrepareDC(dc);
  PrepareDC(dc);

  dc.BeginDrawing();

  int x, y;
  dc.GetSize(&x, &y);
  if (p_width != 0) {
    dc.DrawBitmap(wxBitmap(p_image), 0, 0, true);
  }
  dc.EndDrawing();
  */
  event.Skip();
}


/*!
 * wxEVT_LEFT_DOWN event handler for ID_SCROLLEDWINDOW
 */
void ImageContextPanel::OnLeftDown( wxMouseEvent& event )
{
  p_startX = event.GetX();
  p_startY = event.GetY();
  event.Skip();
}


/*!
 * wxEVT_LEFT_UP event handler for ID_SCROLLEDWINDOW
 */
void ImageContextPanel::OnLeftUp( wxMouseEvent& event )
{
  autoScroll(event);
  p_startX = -1;
  p_startY = -1;
  event.Skip();
}


/*!
 * wxEVT_MOTION event handler for ID_SCROLLEDWINDOW
 */
void ImageContextPanel::OnMotion( wxMouseEvent& event )
{
  if (p_startX != -1)
    autoScroll(event);
  event.Skip();
}


/*!
 * wxEVT_MOTION event handler for ID_SCROLLEDWINDOW
 */
void ImageContextPanel::OnMouseWheel( wxMouseEvent& event )
{
  int rotation = event.GetWheelRotation();
  if (rotation > 0)
    p_scale /= p_scaleStep;
  else if (rotation < 0)
    p_scale *= p_scaleStep;
  redraw();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_100
 */
void ImageContextPanel::OnZoom100Click( wxCommandEvent& event )
{
  p_scale = 1.0;
  redraw();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_FIT
 */
void ImageContextPanel::OnZoomFitClick( wxCommandEvent& event )
{
  double panelWidth = p_imageWindow->GetClientSize().GetWidth();
  double panelHeight = p_imageWindow->GetClientSize().GetHeight();
  p_scale = min(1.0, min(panelWidth/p_width, panelHeight/p_height));
  redraw();

  // Draw twice to remove the margin caused by the scrollbar space
  if (p_scale < 1.0) {
    panelWidth = p_imageWindow->GetClientSize().GetWidth();
    panelHeight = p_imageWindow->GetClientSize().GetHeight();
    p_scale = min(1.0, min(panelWidth/p_width, panelHeight/p_height));
    redraw();
  }

  event.Skip();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_IN
 */
void ImageContextPanel::OnZoomInClick( wxCommandEvent& event )
{
  p_scale *= p_scaleStep;
  redraw();
  event.Skip();
}


/*!
 * wxEVT_COMMAND_MENU_SELECTED event handler for wxID_ZOOM_OUT
 */
void ImageContextPanel::OnZoomOutClick( wxCommandEvent& event )
{
  p_scale /= p_scaleStep;
  redraw();
  event.Skip();
}


/**
 * Redraw the image using the new scale.
 */
void ImageContextPanel::redraw()
{
  int width = (int) (p_width * p_scale);
  int height = (int) (p_height * p_scale);
  p_staticBitmap->SetBitmap(wxBitmap(p_image.Scale(width, height)));
  p_imageWindow->SetVirtualSize(width, height);
  p_toolBar->EnableTool(wxID_ZOOM_100, p_scale != 1.0);
  //  cerr << "image size: " << width << '\t' << height << endl;
  Refresh();
}


void ImageContextPanel::autoScroll( wxMouseEvent& event )
{
  long currentX = event.GetX();
  long currentY = event.GetY();
  
  int x, y;
  p_imageWindow->GetViewStart(&x, &y);
  p_imageWindow->Scroll(x - currentX + p_startX, y - currentY + p_startY);

  p_startX = currentX;
  p_startY = currentY;
}
