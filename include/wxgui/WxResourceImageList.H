/**
 * @file
 *
 * Resource ImageList 
 *
 *
 */

#ifndef _WXRESOURCEIMAGELIST_H_
#define _WXRESOURCEIMAGELIST_H_

#include "wxgui/ewxImageList.H"


class Resource;


class WxResourceImageList : public ewxImageList
{
  public:
    static WxResourceImageList* getImageList();
    // Roots of added ECCE servers (maroon) and SSH folders (green); a root
    // that is not connected is grey.  The tint is the colour mixed lightly
    // into the tree background, for the rows below the root.
    static int getServerImageIndex(bool ssh, bool connected);
    static wxColour getServerInk(bool ssh);
    static wxColour getServerTint(bool ssh);
    static int getImageIndex(Resource * resource,
                             ResourceDescriptor::IconType iconType =
                             ResourceDescriptor::IT_NORMAL);
    static wxIcon getIcon(Resource * resource,
                          ResourceDescriptor::IconType iconType =
                          ResourceDescriptor::IT_NORMAL);
    static wxBitmap getBitmap(Resource * resource,
                              ResourceDescriptor::IconType iconType =
                              ResourceDescriptor::IT_NORMAL);
    static wxBitmap getCodeBitmap(Resource * resource);
    ~WxResourceImageList();

    static void refresh();
  
  protected:
    WxResourceImageList();
    bool Create();
    WxResourceImageList(const WxResourceImageList &that);
    WxResourceImageList& operator=(const WxResourceImageList &that);
  
    void addAllImages();

    int getImageIndex(ResourceType * resourceType,
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
    static WxResourceImageList *p_singleton;
    int *p_resourceIndex;
};

#endif
