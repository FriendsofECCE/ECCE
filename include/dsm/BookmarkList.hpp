/**
 * @file
 * @author Lisong Sun
 * 
 */

#ifndef BOOKMARKLIST_H
#define BOOKMARKLIST_H

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <xercesc/dom/DOMDocument.hpp>

class Bookmark;


/**
 * Read and write bookmark list from a preference file in xml format
 */
class BookmarkList
{
public:

  BookmarkList(string p_prefFile);

  ~BookmarkList();

  void parseFile();

  void addBookmark(string name, string url);

  void removeBookmark(int pos);

  void saveToFile();

  vector<string> * getBookmarkNames();

  string getBookmarkName(int pos);

  string getBookmarkUrl(int pos);

  vector<Bookmark *> * getBookmarks();

  int getSize();

protected:

  void clear();

  void swap(int pos1, int pos2);

  void setBookmarkName(int pos, string name);

  void setBookmarkUrl(int pos, string url);

  string p_prefFile;
  vector<Bookmark *> p_list;

  static string getNodeValue(xercesc::DOMNode *domNode);

  static const string p_rootKey;
  static const string p_bookmarkKey;
  static const string p_urlKey;
  static const string p_nameKey;
  static const string p_xmlHeader;
  static const string p_xmlTail;
};

#endif
