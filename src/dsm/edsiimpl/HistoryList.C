/**
 * @file HistoryList.C
 *
 *  A simple class to track a history list.  Publicly inherits from
 *  vector so use vector methods for most operations.
 *
 *  HistoryList can enforce a max size where the oldest item gets
 *  removed when the max size is reached.
 *
 *  Supports saving and restoring from server or local disk.
 */

#include <strstream>
  using std::ostrstream;
#include <iostream>
  using std::endl;
  using std::ends;

#include <limits.h>
#include <string.h>

#include "util/ErrMsg.H"
#include "dsm/HistoryList.H"
#include "dsm/EDSIFactory.H"
#include "dsm/EDSI.H"

//////////////////////////////////////////////////////////////////////////////
// Constructor.
//   If size of -1, translate to INT_MAX.
//////////////////////////////////////////////////////////////////////////////
HistoryList::HistoryList(int maxsize)
{
  p_backptr = 0;
  p_max = maxsize;
  if (p_max == -1) p_max = INT_MAX;
}

//////////////////////////////////////////////////////////////////////////////
// Copy Constructor.
//////////////////////////////////////////////////////////////////////////////
HistoryList::HistoryList(const HistoryList& rhs)
{
  p_max = rhs.p_max;
  for (vector<EcceURL>::const_iterator it=rhs.begin(); it!=rhs.end(); it++) {
    push_back((*it));
  }
  p_backptr = rhs.p_backptr;
}

//////////////////////////////////////////////////////////////////////////////
// Destructor.
//////////////////////////////////////////////////////////////////////////////
HistoryList::~HistoryList()
{
  clear();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
int HistoryList::getMaxSize() const
{
  if (p_max == INT_MAX) {
    return -1;
  } else {
    return p_max;
  }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void HistoryList::setMaxSize(int newsize)
{
  int totoss = size() - newsize;

  if (totoss > 0) {
    // if the size is shrinking, remove excess
    vector<EcceURL>::iterator it=begin();
    erase(begin(),it+totoss);
    p_backptr = size();
  }

  p_max = newsize;
}

//////////////////////////////////////////////////////////////////////////////
// add
//   Add to end of queue removing from beginning to make room if
//   necessary.
//////////////////////////////////////////////////////////////////////////////
void HistoryList::add(const EcceURL& url)
{
  //  cout << p_backptr << "  " << size() << endl;
  if (size() > 0 && p_backptr > 0 && (url == get(p_backptr-1))) {
  } else {
    /*
    if (!backAtEnd()) {
      vector<EcceURL>::iterator it = begin();
      erase(it+p_backptr, end());
    }
    */

    // If the url is in the list already, remove the previous appearances
    vector<EcceURL>::iterator it = begin();
    for (; it != end(); ++it) {
      if ((*it) == url) {
        erase(it);
        --it;
      }
    }

    if (size()+1 > p_max) {
      vector<EcceURL>::iterator it = begin();
      erase(it);
    }
    push_back(url);

    p_backptr = size();
  }
}


void HistoryList::add(const string& strurl)
{
  EcceURL url = strurl;
  add(url);
}


int HistoryList::size() const
{
  return vector<EcceURL>::size();
}

EcceURL HistoryList::get(int index) const
{
  EcceURL url;
  EE_ASSERT(index >= 0 && index < size(),
            EE_WARNING, "HistoryList: range error");
  url = (*this)[index];
  return url;
}

EcceURL HistoryList::top() const
{
  EcceURL url;
  int index = size()-1;
  if (index >= 0) {
    url = (*this)[index];
  }
  return url;
}

EcceURL HistoryList::back()
{
  EcceURL url;
  
  if (p_backptr > 1) {
    p_backptr--;
    url = get(p_backptr-1);
  }
  return url;
}

EcceURL HistoryList::forward()
{
  EcceURL url;
  EE_ASSERT((p_backptr>0 && p_backptr < size()), 
            EE_WARNING, "HistoryList: can't go forward");
  if  (canMoveForward()) {
    p_backptr++;
    url = get(p_backptr-1);
  }
  return url;
}


EcceURL HistoryList::jumpTo(int index)
{
  EE_ASSERT(index >= 0 && index < size(),
            EE_WARNING, "HistoryList: range error");
  p_backptr = index+1;
  return get(index);
}


bool HistoryList::backAtEnd() const
{
  return p_backptr == size();
}

bool HistoryList::canMoveBack() const
{
  return p_backptr > 1;
}

bool HistoryList::canMoveForward() const
{
  return p_backptr > 0 && p_backptr < size();
}

EcceURL HistoryList::pop(int index)
{
  EcceURL url;
  int myindex = index;
  if (index == -1 && size() > 0) {
    myindex = size()-1;
  }
  url = get(myindex);
  vector<EcceURL>::iterator it=begin();
  erase(it+myindex,end());
  if (p_backptr > myindex) p_backptr = size();
  return url;
}

void HistoryList::clear()
{
  vector<EcceURL>::clear();
}

bool HistoryList::load(const EcceURL& url)
{
  bool ret = false;
  EDSI *edsi = EDSIFactory::getEDSI(url);
  if (edsi) {
    ostrstream os;
    ret = edsi->getDataSet(os);
    if (ret) {
      os << ends;
      char *cptr = os.str();
      char *tok = strtok(cptr,"\n");
      do {
        if (tok) add((string)tok);
      } while ((tok = strtok(NULL,"\n")) != NULL);
    }
  }
  return ret;
}

bool HistoryList::save(const EcceURL& url) const
{
  bool ret = true;
  ostrstream os;
  for (vector<EcceURL>::const_iterator it=begin(); it!=end(); it++) {
    os << (*it).toString() << endl;
  }
  os << ends;

  EDSI *edsi = EDSIFactory::getEDSI(url);
  ret = edsi->putDataSet(os.str());

  return ret;
}
