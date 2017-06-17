/**
 * @file
 * @author Lisong Sun
 * 
 * Read and write bookmark list from a preference file in xml format
 *
 */

#include <fstream>
using std::ofstream;

#include <string>
using std::string;

#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMNamedNodeMap.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/util/XMLString.hpp>
using namespace xercesc;

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/EcceException.H"
#include "util/SFile.H"
#include "util/Bookmark.H"

#include "dsm/BasicDOMParser.H"
#include "dsm/BookmarkList.H"

const string BookmarkList::p_rootKey = "Bookmarks";
const string BookmarkList::p_bookmarkKey = "Bookmark";
const string BookmarkList::p_urlKey = "Url";
const string BookmarkList::p_nameKey = "Desc";
const string BookmarkList::p_xmlHeader = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<!-- This file is generated automatically by ECCE, please do not modify! -->\n<" + p_rootKey + ">\n";
const string BookmarkList::p_xmlTail = "</" + p_rootKey + ">\n";

/**
 * Constructor.
 * It should always take the name of the preference file as input.
 */
BookmarkList::BookmarkList(string prefFile)
{
  p_prefFile = Ecce::realUserPrefPath() + prefFile;
  parseFile();
}


/**
 * Destructor, all the cleaning work is done by clear function.
 */
BookmarkList::~BookmarkList()
{
  clear();
}


/**
 * Clear all the data member and release the memory.
 * Used by both destructor and parseFile to reload setting from file.
 */
void BookmarkList::clear()
{
  vector<Bookmark *>::iterator listItor = p_list.begin();
  for (; listItor != p_list.end(); listItor++)
    delete (*listItor);
  p_list.clear();
}


/**
 * Load/reload bookmark list from preference file.
 */
void BookmarkList::parseFile()
{
  clear();

  BasicDOMParser parser;
  SFile prefFile(p_prefFile);

  if (!prefFile.exists()) {
    saveToFile();
  }

  DOMDocument * doc;
  try {
    doc = parser.parse(prefFile);
  } catch (const EcceException& toCatch) {
    EE_RT_ASSERT(false, EE_FATAL, toCatch.what());
  }
  
  XMLCh * tmpStr = XMLString::transcode(p_nameKey.c_str());
  DOMNodeList * nameList = doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;

  tmpStr = XMLString::transcode(p_urlKey.c_str());
  DOMNodeList * urlList = doc->getElementsByTagName(tmpStr);
  delete [] tmpStr;
    
  for (int i=0; i<urlList->getLength(); i++) {
    string name = getNodeValue(nameList->item(i));
    string url = getNodeValue(urlList->item(i));
    if (!name.empty() || !url.empty()) {
      p_list.push_back(new Bookmark(name, url));
    }
  }
  doc->release();
}


/**
 * A static utility function copied from EDSIServerCentral.
 * @todo Shoud this go to some xml util class?
 */
string BookmarkList::getNodeValue(DOMNode *domNode)
{
  string ret;

  DOMNode *child = 0;
  char *value;

  DOMElement *ele = (DOMElement *) domNode;
  if (ele->hasChildNodes()) {
    child = ele->getFirstChild();
    value = XMLString::transcode(child->getNodeValue());
    ret = value;
    delete [] value;
  }

  if (!ret.empty()) {
    int start = ret.find_first_not_of(" \t");
    int end = ret.find_last_not_of(" \t");
    if (start != 0 || end != 0)
      ret = ret.substr(start,end-start+1);
  }
  return ret;
}


/**
 * Add a bookmark to the bookmark list
 */
void BookmarkList::addBookmark(string name, string url)
{
  p_list.push_back(new Bookmark(name, url));
  saveToFile();
}


/**
 * Remove the bookmark at pos from the bookmark list
 */
void BookmarkList::removeBookmark(int pos)
{
  if (pos < 0 || pos >= p_list.size())
    return;
  delete p_list[pos];
  p_list.erase(p_list.begin()+pos);
  saveToFile();
}


/**
 * Save the bookmark list to the preference file.
 */
void BookmarkList::saveToFile()
{
  ofstream os(p_prefFile.c_str());
  os << p_xmlHeader;

  vector<Bookmark *>::iterator listItor = p_list.begin();
  for (; listItor != p_list.end(); listItor++) {
    os << "  <" << p_bookmarkKey << ">\n"
       << "    <" << p_urlKey << ">" << (*listItor)->getUrl()
       << "</" << p_urlKey << ">\n"
       << "    <" << p_nameKey << ">" << (*listItor)->getName()
       << "</" << p_nameKey << ">\n"
       << "  </" << p_bookmarkKey << ">\n";
  }

  os << p_xmlTail;
  os.close();
}


/**
 * Get the bookmark names as a vector.
 * Caller needs to delete the pointer after usage.
 */
vector<string> * BookmarkList::getBookmarkNames()
{
  vector<string> * ret = new vector<string>;
  vector<Bookmark *>::iterator listItor = p_list.begin();
  for (; listItor != p_list.end(); listItor++) {
    ret->push_back((*listItor)->getName());
  }
  return ret;
}


/**
 * Get the bookmark name at pos
 */
string BookmarkList::getBookmarkName(int pos)
{
  if (pos < 0 || pos >= p_list.size())
    return "";
  return p_list[pos]->getName();
}


/**
 * Set the bookmark name at pos
 */
void BookmarkList::setBookmarkName(int pos, string name)
{
  if (pos >= 0 && pos < p_list.size())
    p_list[pos]->setName(name);
}


/**
 * Get the bookmark URL at pos
 */
string BookmarkList::getBookmarkUrl(int pos)
{
  if (pos < 0 || pos >= p_list.size())
    return "";
  return p_list[pos]->getUrl();
}


/**
 * Set the bookmark URL at pos
 */
void BookmarkList::setBookmarkUrl(int pos, string url)
{
  if (pos >= 0 && pos < p_list.size())
    p_list[pos]->setUrl(url);
}


/**
 * Get the size of the bookmark list
 */
int BookmarkList::getSize()
{
  return p_list.size();
}


vector<Bookmark *> * BookmarkList::getBookmarks()
{
  return &p_list;
}



void BookmarkList::swap(int pos1, int pos2)
{
  Bookmark * tmp = p_list[pos1];
  p_list[pos1] = p_list[pos2];
  p_list[pos2] = tmp;
}
