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


#ifndef HISTORYLIST_HH
#define HISTORYLIST_HH

#include <vector>
#include <string>
  using std::string;
  using std::vector;

#include "util/EcceURL.H"


class HistoryList : protected vector<EcceURL> {
public:
  HistoryList(int maxsize=-1/*no max*/);
  HistoryList(const HistoryList& rhs);
  ~HistoryList();

  // file or http url
  bool load(const EcceURL& path);
  bool save(const EcceURL& path) const;

  // maxsize is the point at which first-in objects fall out when adding
  void setMaxSize(int size);
  int  getMaxSize() const;

  // actual number of entries.
  int size() const;

  // Move back/foward in stack
  EcceURL back();
  EcceURL forward();
  EcceURL jumpTo(int index);

  bool canMoveForward() const;
  bool canMoveBack() const;
  bool backAtEnd() const;  // is current back location head of list

  // Get url at index and pop anything newer off the list.
  EcceURL pop(int index=-1);
  EcceURL get(int index) const;

  // non-destructive get of top 
  EcceURL top() const;

  void add(const EcceURL& url);
  void add(const string& strurl);

  void clear();


protected:
  int p_max;
  int p_backptr;

};

#endif
