/**
 * @file
 *
 * Resource ImageList 
 *
 *
 */
#include "wx/dcmemory.h" // for wxMemoryDC

#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"

#include "util/EcceURL.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/WxStateImageList.H"

#include "wxgui/WxResourceStateImageList.H"


WxResourceStateImageList* WxResourceStateImageList::p_singleton = 0;


WxResourceStateImageList::WxResourceStateImageList()
  : ewxImageList(),
    p_resourceIndex(NULL)
{
  Create();
}


bool WxResourceStateImageList::Create()
{
  // to work on MSW we must know the size of the images in the list and all
  // images must be this size
  int index=0, w1, h1, w2, h2;
  WxResourceImageList::getImageList()->GetSize(index, w1, h1);
  WxStateImageList::getImageList()->GetSize(index, w2, h2);
  int width = w1 + w2;
  int height = h1 > h2 ? h1 : h2;

  if (!ewxImageList::Create(width, height, true, 0)) {
    wxFAIL_MSG( wxT("WxResourceStateImageList creation failed") );
    return false;
  }

  addAllImages();

  return true;
}


void WxResourceStateImageList::addAllImages()
{
  wxBitmap bitmap;

  // add the stateless icons
  bitmap = combineWithState(ewxBitmap(_T("server.xpm")));
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);

  bitmap = combineWithState(ewxBitmap(_T("folder_home.xpm")));
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);

  bitmap = combineWithState(ewxBitmap(_T("user.xpm")));
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);
  bitmap = combineWithState(ewxBitmap(_T("user_open.xpm")));
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);

  bitmap = combineWithState(ewxBitmap(_T("users.xpm")));
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);
  Add(bitmap, *wxWHITE);

  vector<ResourceType *> resourceTypes =
          ResourceDescriptor::getResourceDescriptor().getResourceTypes();

  if (p_resourceIndex == 0) {
    p_resourceIndex = new int[resourceTypes.size()];
  }

  for (int resourceIndex = 0;
           resourceIndex < resourceTypes.size();
           resourceIndex++) {
    // important: the order of these add functions matters!!!
    p_resourceIndex[resourceIndex] = GetImageCount();
    combineAndAddWithStates(resourceTypes[resourceIndex]);
  }
}


void WxResourceStateImageList::refresh()
{
  getImageList()->RemoveAll();
  getImageList()->addAllImages();
}


WxResourceStateImageList::WxResourceStateImageList(const WxResourceStateImageList& that)
{

}


WxResourceStateImageList&
WxResourceStateImageList::operator=(const WxResourceStateImageList &that)
{
  return *this;
}


WxResourceStateImageList::~WxResourceStateImageList()
{

}


WxResourceStateImageList* WxResourceStateImageList::getImageList()
{
  if (p_singleton == 0)
    p_singleton = new WxResourceStateImageList();

  return p_singleton;
}


wxBitmap
WxResourceStateImageList::combineWithState(wxBitmap bitmap,
                                           ResourceDescriptor::RUNSTATE state)
{
  int index=0, stateWidth, stateHeight;
  WxStateImageList::getImageList()->GetSize(index, stateHeight, stateWidth);
  int combinedWidth = bitmap.GetWidth() + stateWidth;
  int greaterHeight = bitmap.GetHeight() > stateHeight ? bitmap.GetHeight() :
                                                          stateHeight;
  wxBitmap combinedBitmap(combinedWidth, greaterHeight);
  wxMemoryDC combinedDC(combinedBitmap);
  // clear the entire combined bitmap
  // NOTE: This is done because there is (I assume) some undocumented static
  // memory or caching that occurs.  If the following is commented out, at
  // least using GTK2, images are drawn on top of older images even though
  // this 'combinedBitmap' is using memory from the stack.
  // This technique, used in combination with the "useMask=true" in the
  // wxBitmap.Blit function, is the only way I could combine two bitmaps
  // side-by-side into a new bitmap without severe distortion.
  combinedDC.SetBrush(*wxWHITE_BRUSH);
  combinedDC.SetPen(*wxWHITE);
  combinedDC.DrawRectangle(0, 0, combinedWidth, greaterHeight);

  // draw resource bitmap (given as param in this function call)
  wxMemoryDC resourceDC(bitmap);
  combinedDC.Blit(0, 0, bitmap.GetWidth(), bitmap.GetHeight(),
                  &resourceDC, 0, 0, wxCOPY, true);
  if (state != ResourceDescriptor::NUMBER_OF_STATES) {
    wxBitmap stateBitmap(WxStateImageList::getImageList()->GetBitmap(state));
    wxMemoryDC stateDC(stateBitmap);
    combinedDC.Blit(bitmap.GetWidth(), 0,
                    stateBitmap.GetWidth(), stateBitmap.GetHeight(),
                    &stateDC, 0, 0, wxCOPY, true);
  }

  return combinedBitmap;
}


void
WxResourceStateImageList::combineAndAddWithState(ResourceType * resourceType,
                                           ResourceDescriptor::RUNSTATE state)
{
  int iconType = ResourceDescriptor::IT_NORMAL;
  string normalName = resourceType->getIcon(
                      (ResourceDescriptor::IconType)iconType);
  ewxBitmap normalBitmap(normalName);
  wxBitmap combinedNormal = combineWithState(normalBitmap, state);
  Add(combinedNormal, *wxWHITE);
  for (iconType = iconType + 1;
       iconType < ResourceDescriptor::IT_UNDEFINED;
       iconType++) {
    string iconName = resourceType->getIcon(
                      (ResourceDescriptor::IconType)iconType);
    if (iconName == normalName) {
      Add(combinedNormal, *wxWHITE);
    } else {
      Add(combineWithState(ewxBitmap(iconName), state), *wxWHITE);
    }
  }
}


void
WxResourceStateImageList::combineAndAddWithStates(ResourceType * resourceType)
{
  if (resourceType->getStateBased()) {
    for (int state = ResourceDescriptor::STATE_ILLEGAL;
             state < ResourceDescriptor::NUMBER_OF_STATES;
             state++) {
      combineAndAddWithState(resourceType,
                             (ResourceDescriptor::RUNSTATE) state);
    }
  } else {
    combineAndAddWithState(resourceType);
  }
}


int
WxResourceStateImageList::getImageIndex(Resource * resource,
                                        ResourceDescriptor::RUNSTATE state,
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
    ret = getImageList()->getImageIndex(resource->getDescriptor(),
                                        state, iconType);
  
  return ret;
}


int
WxResourceStateImageList::getImageIndex(ResourceType * resType,
                                        ResourceDescriptor::RUNSTATE state,
                                        ResourceDescriptor::IconType iconType)
{
  int ret = 0;

  if (resType == NULL) {
    ret = iconType;
  }
  else {
    int low, high;
    ResourceDescriptor::getResourceDescriptor().getResourceIdRange(low, high);
    int indexBase = p_resourceIndex[resType->getId()-low];
    if (resType->getStateBased()) {
      ret = indexBase + (state * ResourceDescriptor::IT_UNDEFINED) + iconType;
    } else {
      ret = indexBase + iconType;
    }
  }

  return ret;
}


int
WxResourceStateImageList::getRootImageIndex(ResourceDescriptor::IconType iconType)
{
  return iconType;
}


int
WxResourceStateImageList::getHomeImageIndex(ResourceDescriptor::IconType iconType)
{
  return 4 + iconType;
}


int
WxResourceStateImageList::getUserImageIndex(ResourceDescriptor::IconType iconType)
{
  return 8 + iconType;
}


int
WxResourceStateImageList::getUsersImageIndex(ResourceDescriptor::IconType iconType)
{
  return 12 + iconType;
}


wxIcon
WxResourceStateImageList::getIcon(Resource * resource,
                                  ResourceDescriptor::RUNSTATE state,
                                  ResourceDescriptor::IconType iconType)
{
  return getImageList()->GetIcon(
         getImageList()->getImageIndex(resource, state, iconType));
}


wxBitmap
WxResourceStateImageList::getBitmap(Resource * resource,
                                    ResourceDescriptor::RUNSTATE state,
                                    ResourceDescriptor::IconType iconType)
{
  return getImageList()->GetBitmap(
         getImageList()->getImageIndex(resource, state, iconType));
}

