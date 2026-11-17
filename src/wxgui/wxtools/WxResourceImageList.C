/**
 * @file
 *
 * Resource ImageList 
 *
 *
 */
#include "wx/icon.h"
#include "wx/settings.h"
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/VDoc.H"

#include "util/EcceURL.H"

#include "wxgui/ewxBitmap.H"

#include "wxgui/WxResourceImageList.H"


WxResourceImageList* WxResourceImageList::p_singleton = 0;


WxResourceImageList::WxResourceImageList()
  : ewxImageList(),
    p_resourceIndex(NULL)
{
  p_inkedIndex[0] = p_inkedIndex[1] = NULL;
  Create();
}


bool WxResourceImageList::Create()
{
  // to work on MSW we must know the size of the images in the list and all
  // images must be this size
  ewxBitmap sizeBitmap(_T("server.xpm"));

  if (!ewxImageList::Create(sizeBitmap.GetWidth(), sizeBitmap.GetHeight(),
                            true, 0)) {
    wxFAIL_MSG( wxT("WxResourceImageList creation failed") );
    return false;
  }

  addAllImages();

  return true;
}


void WxResourceImageList::addAllImages()
{
  ewxBitmap bitmap;

  bitmap = ewxBitmap(_T("server.xpm"));
  Add(bitmap);
  Add(bitmap);
  Add(bitmap);
  Add(bitmap);

  bitmap = ewxBitmap(_T("folder_home.xpm"));
  Add(bitmap);
  Add(bitmap);
  Add(bitmap);
  Add(bitmap);

  bitmap = ewxBitmap(_T("user.xpm"));
  Add(bitmap);
  Add(bitmap);
  bitmap = ewxBitmap(_T("user_open.xpm"));
  Add(bitmap);
  Add(bitmap);

  bitmap = ewxBitmap(_T("users.xpm"));
  Add(bitmap);
  Add(bitmap);
  Add(bitmap);
  Add(bitmap);

  // Server roots: ECCE server, SSH folder, then both greyed (4 slots each).
  wxSize isz = bitmap.GetSize();
  for (int v = 0; v < 4; v++) {
    bool ssh = (v % 2) == 1;
    wxColour ink = v < 2 ? getServerInk(ssh)
                         : wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
    wxBitmap icon = ewxBitmap::tintedIcon(ssh ? "server-ssh" : "server-ecce",
                                          isz, ink);
    if (!icon.IsOk()) icon = bitmap;
    for (int k = 0; k < 4; k++) Add(icon);
  }

  vector<ResourceType *> resourceTypes =
          ResourceDescriptor::getResourceDescriptor().getResourceTypes();

  if (p_resourceIndex == 0) {
    p_resourceIndex = new int[resourceTypes.size()];
  }

  for (int resourceIndex = 0;
           resourceIndex < resourceTypes.size();
           resourceIndex++) {
    // important: the order of these add functions matters!!!
    int count = GetImageCount();
    p_resourceIndex[resourceIndex] = count;
    Add(ewxBitmap(resourceTypes[resourceIndex]->
                   getIcon(ResourceDescriptor::IT_NORMAL)));
    Add(ewxBitmap(resourceTypes[resourceIndex]->
                   getIcon(ResourceDescriptor::IT_SELECTED)));
    Add(ewxBitmap(resourceTypes[resourceIndex]->
                   getIcon(ResourceDescriptor::IT_EXPANDED)));
    Add(ewxBitmap(resourceTypes[resourceIndex]->
                   getIcon(ResourceDescriptor::IT_SELECTED_EXPANDED)));
  }

  // Folder-like types again in the SSH and ECCE-server inks.
  for (int v = 0; v < 2; v++) {
    if (p_inkedIndex[v] == 0) p_inkedIndex[v] = new int[resourceTypes.size()];
    for (size_t t = 0; t < resourceTypes.size(); t++) {
      p_inkedIndex[v][t] = -1;
      if (!resourceTypes[t]->getIsExpandable()) continue;
      p_inkedIndex[v][t] = GetImageCount();
      for (int k = 0; k < 4; k++)
        Add(inked(ewxBitmap(resourceTypes[t]->getIcon(
                  (ResourceDescriptor::IconType)k)), getServerInk(v == 0)));
    }
  }
}


void WxResourceImageList::refresh()
{
  getImageList()->RemoveAll();
  getImageList()->addAllImages();
}


WxResourceImageList::WxResourceImageList(const WxResourceImageList& that)
{

}


WxResourceImageList&
WxResourceImageList::operator=(const WxResourceImageList &that)
{
  return *this;
}


WxResourceImageList::~WxResourceImageList()
{

}


WxResourceImageList* WxResourceImageList::getImageList()
{
  if (p_singleton == 0)
    p_singleton = new WxResourceImageList();

  return p_singleton;
}


int WxResourceImageList::getServerImageIndex(bool ssh, bool connected)
{
  return 16 + 4 * ((connected ? 0 : 2) + (ssh ? 1 : 0));
}


wxColour WxResourceImageList::getServerInk(bool ssh)
{
  return ssh ? wxColour(0x2E, 0x7D, 0x32) : wxColour(0x80, 0x00, 0x20);
}


// Keeps the shading and mask of the bitmap, replacing its hue by the ink:
// dark pixels go to black, mid-tones are the ink, highlights lighten it.
static wxBitmap inked(const wxBitmap& bmp, const wxColour& ink)
{
  wxImage img = bmp.ConvertToImage();
  if (!img.IsOk()) return bmp;
  unsigned char * d = img.GetData();
  const size_t n = (size_t)img.GetWidth() * img.GetHeight();
  for (size_t i = 0; i < n; i++, d += 3) {
    double l = (0.299 * d[0] + 0.587 * d[1] + 0.114 * d[2]) / 255.0;
    double f = l < 0.55 ? 0.35 + 0.65 * l / 0.55
                        : 1.0 + 0.5 * (l - 0.55) / 0.45;
    unsigned char c[3] = {ink.Red(), ink.Green(), ink.Blue()};
    for (int k = 0; k < 3; k++) {
      double v = f <= 1.0 ? c[k] * f : c[k] + (255 - c[k]) * (f - 1.0);
      d[k] = (unsigned char)(v < 0 ? 0 : v > 255 ? 255 : v + 0.5);
    }
  }
  return wxBitmap(img);
}


int WxResourceImageList::getInkedImageIndex(Resource * resource, int kind)
{
  WxResourceImageList * list = getImageList();
  ResourceType * t = resource->getDescriptor();
  if (!t || kind < 1 || kind > 2 || !list->p_inkedIndex[kind - 1]) return -1;
  EcceURL u = resource->getURL();
  if (u.isEcceRoot() || u.isEcceUsers() || u.isEcceUserHome()) return -1;
  int low, high;
  ResourceDescriptor::getResourceDescriptor().getResourceIdRange(low, high);
  return list->p_inkedIndex[kind - 1][t->getId() - low];
}


int WxResourceImageList::getImageIndex(Resource * resource,
                                       ResourceDescriptor::IconType iconType)
{
  int ret = 0;
  EcceURL resUrl = resource->getURL();

  if (resUrl.isEcceRoot())
    ret = getImageList()->getRootImageIndex(iconType);
  else if (resUrl.isEcceUsers())
    ret = getImageList()->getUsersImageIndex(iconType);
  else if (resUrl.isEcceUserHome())
    ret = getImageList()->getHomeImageIndex(iconType);
  else if (resUrl.getParent().isEcceUsers())
    ret = getImageList()->getUserImageIndex(iconType);
  else
    ret = getImageList()->getImageIndex(resource->getDescriptor(), iconType);
  
  return ret;
}


int WxResourceImageList::getImageIndex(ResourceType * resType,
                                       ResourceDescriptor::IconType iconType)
{
  if (resType == NULL) {
    return iconType;
  }
  else {
    int low, high;
    ResourceDescriptor::getResourceDescriptor().getResourceIdRange(low, high);
    return p_resourceIndex[resType->getId()-low] + iconType;
  }
}


int
WxResourceImageList::getRootImageIndex(ResourceDescriptor::IconType iconType)
{
  return iconType;
}


int
WxResourceImageList::getHomeImageIndex(ResourceDescriptor::IconType iconType)
{
  return 4 + iconType;
}


int
WxResourceImageList::getUserImageIndex(ResourceDescriptor::IconType iconType)
{
  return 8 + iconType;
}


int
WxResourceImageList::getUsersImageIndex(ResourceDescriptor::IconType iconType)
{
  return 12 + iconType;
}


wxIcon
WxResourceImageList::getIcon(Resource * resource,
                             ResourceDescriptor::IconType iconType)
{
  return getImageList()->GetIcon(
         getImageList()->getImageIndex(resource, iconType));
}


wxBitmap
WxResourceImageList::getBitmap(Resource * resource,
                               ResourceDescriptor::IconType iconType)
{
  return getImageList()->GetBitmap(
         getImageList()->getImageIndex(resource, iconType));
}



//  The icon of the code a calculation or task runs, for the detail pane's Code
//  row.  GROMACS tasks are registered under NWChem MD types, whose icons name
//  the wrong code.
wxBitmap
WxResourceImageList::getCodeBitmap(Resource * resource)
{
  if (resource->getProp(VDoc::getEcceNamespace() + ":application") ==
      "GROMACS") {
    ewxBitmap bitmap(wxT("svg/study-gromacs.svg"));
    if (bitmap.IsOk()) return bitmap;
  }
  return getBitmap(resource);
}
