#include "wx/dcmemory.h"

#include "wxgui/WxState.H"

#include "wxgui/WxStateImageList.H"
#include "wxgui/ewxBitmap.H"


map<int, WxStateImageList*> WxStateImageList::p_singletons;


WxStateImageList::WxStateImageList(int size)
  : ewxImageList(),
    p_size(size)
{
  Create();
}


bool WxStateImageList::Create()
{
  if (!ewxImageList::Create(ewxBitmap::scaled(p_size),
                            ewxBitmap::scaled(p_size), true, 0)) {
    wxFAIL_MSG( wxT("WxStateImageList creation failed") );
    return false;
  }

  addAllImages();

  return true;
}


void WxStateImageList::addAllImages()
{
  // One past the last state: WxState::OUT_OF_CONTACT.
  for (int state = ResourceDescriptor::STATE_ILLEGAL;
           state <= WxState::OUT_OF_CONTACT;
           state++) {
    Add(WxState::alphaBitmap((ResourceDescriptor::RUNSTATE)state,
                             ewxBitmap::scaled(p_size)));
  }
}


void WxStateImageList::refresh()
{
  map<int, WxStateImageList*>::iterator it;
  for (it = p_singletons.begin(); it != p_singletons.end(); it++) {
    it->second->RemoveAll();
    it->second->addAllImages();
  }
}


WxStateImageList::WxStateImageList(const WxStateImageList &that)
{

}


WxStateImageList& WxStateImageList::operator=(const WxStateImageList &that)
{
  return *this;
}


WxStateImageList::~WxStateImageList()
{

}


WxStateImageList* WxStateImageList::getImageList(int size)
{
  if (p_singletons.find(size) == p_singletons.end()) {
    p_singletons[size] = new WxStateImageList(size);
  }

  return p_singletons[size];
}

