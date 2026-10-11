/**
 * @file
 *
 * Resource ImageList 
 *
 *
 */
#ifndef _WXRESOURCESTATEIMAGELIST_H_
#define _WXRESOURCESTATEIMAGELIST_H_

#include "wxgui/ewxImageList.H"


class Resource;


class WxResourceStateImageList : public ewxImageList
{
  public:
    static WxResourceStateImageList* getImageList();
    static int getImageIndex(Resource * resource,
                             ResourceDescriptor::RUNSTATE state,
                             ResourceDescriptor::IconType iconType =
                             ResourceDescriptor::IT_NORMAL);
    static wxIcon getIcon(Resource * resource,
                          ResourceDescriptor::RUNSTATE state,
                          ResourceDescriptor::IconType iconType =
                          ResourceDescriptor::IT_NORMAL);
    static wxBitmap getBitmap(Resource * resource,
                              ResourceDescriptor::RUNSTATE state,
                              ResourceDescriptor::IconType iconType =
                              ResourceDescriptor::IT_NORMAL);
    ~WxResourceStateImageList();
  
    static void refresh();

  protected:
    WxResourceStateImageList();
    bool Create();
    WxResourceStateImageList(const WxResourceStateImageList &that);
    WxResourceStateImageList& operator=(const WxResourceStateImageList &that);
  
    void addAllImages();

    // state: a WxStateImageList index, -1 for none.
    wxBitmap combineWithState(wxBitmap bitmap, int state = -1);
    void combineAndAddWithState(ResourceType * resourceType, int state = -1);
    void combineAndAddWithStates(ResourceType * resourceType);

    int getImageIndex(ResourceType * resourceType,
                      ResourceDescriptor::RUNSTATE state,
                      ResourceDescriptor::IconType iconType =
                      ResourceDescriptor::IT_NORMAL);
    int getRootImageIndex(ResourceDescriptor::IconType iconType = 
                          ResourceDescriptor::IT_NORMAL);
    int getHomeImageIndex(ResourceDescriptor::IconType iconType =
                          ResourceDescriptor::IT_NORMAL);
    int getUserImageIndex(ResourceDescriptor::IconType iconType =
                          ResourceDescriptor::IT_NORMAL);
    int getUsersImageIndex(ResourceDescriptor::IconType iconType =
                           ResourceDescriptor::IT_NORMAL);

  private:
    static WxResourceStateImageList *p_singleton;
    int *p_resourceIndex;
};

#endif // _WXRESOURCESTATEIMAGELIST_H_
