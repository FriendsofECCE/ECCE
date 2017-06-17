/**
 * @file
 *
 * Resource ImageList 
 *
 *
 */
#include "dsm/Resource.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"

#include "util/EcceURL.H"

#include "wxgui/ewxBitmap.H"

#include "wxgui/WxResourceImageList.H"


WxResourceImageList* WxResourceImageList::p_singleton = 0;


WxResourceImageList::WxResourceImageList()
  : ewxImageList(),
    p_resourceIndex(NULL)
{
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

