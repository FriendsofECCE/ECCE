#ifndef WXSTATEIMAGELIST_H_
#define WXSTATEIMAGELIST_H_

#include <map>
  using std::map;

#include "ewxImageList.H"

class WxStateImageList : public ewxImageList
{
  public:
    static WxStateImageList* getImageList(int size=16);
    ~WxStateImageList();

    static void refresh();

  protected:
    WxStateImageList(int size);
    bool Create();
    WxStateImageList(const WxStateImageList &that);
    WxStateImageList& operator=(const WxStateImageList &that);

    void addAllImages();

  private:
    int p_size;

    static map<int, WxStateImageList*> p_singletons;
};

#endif // WXSTATEIMAGELIST_H_
