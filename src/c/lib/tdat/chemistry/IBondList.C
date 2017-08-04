#include <string.h>

#include "tdat/IBondList.H"

IBondList::IBondList(size_t newNumBonds) 
      : mnumBonds(0), matomIndices((int*)0), mbondOrders((double*)0),
        mstyle((DisplayStyle*)0)
{
  if (newNumBonds != 0) {
    this->mnumBonds = newNumBonds;
    matomIndices = new int[(2 * newNumBonds)];
    memset(matomIndices, 0, ((2 * newNumBonds) * sizeof(int)));
    mbondOrders = new double[newNumBonds];
    memset(mbondOrders, 0, (newNumBonds * sizeof(double)));
    mstyle = new DisplayStyle[newNumBonds];
    //memset(mstyle, 0, (newNumBonds * sizeof(int)));
    for (int idx=0; idx<newNumBonds; idx++) {
       mstyle[idx] = DisplayStyle();
    }
  }
}

IBondList::~IBondList(void)
{
  if (matomIndices != (int*)0) {
    delete[] matomIndices; matomIndices = (int*)0;
    delete[] mbondOrders; mbondOrders = (double*)0;
    delete[] mstyle; mstyle = (DisplayStyle*)0;
  }
}

