#include "wx/dcmemory.h"

#include "wxgui/WxState.H"

#include "wxgui/WxStateImageList.H"


map<int, WxStateImageList*> WxStateImageList::p_singletons;


WxStateImageList::WxStateImageList(int size)
  : ewxImageList(),
    p_size(size)
{
  Create();
}


bool WxStateImageList::Create()
{
  if (!ewxImageList::Create(p_size, p_size, true, 0)) {
    wxFAIL_MSG( wxT("WxStateImageList creation failed") );
    return false;
  }

  addAllImages();

  return true;
}


void WxStateImageList::addAllImages()
{
  for (int state = ResourceDescriptor::STATE_ILLEGAL;
           state < ResourceDescriptor::NUMBER_OF_STATES;
           state++) {
    wxBitmap stateMap(p_size, p_size);
    wxMemoryDC stateDC(stateMap);
    stateDC.SetBackground(*wxWHITE_BRUSH);
    WxState::draw(stateDC, (ResourceDescriptor::RUNSTATE)state);
    Add(stateMap, *wxWHITE);
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

