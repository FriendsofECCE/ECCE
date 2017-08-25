/**
 * @file
 * @author Lisong Sun
 *
 * A subclass derived from ewxTreeCtrl that manage Ecce Resource.
 * It populates the resource tree on the ewxTreeCtrl widget and handles
 * all the tree related events.
 *
 * Relationship between this class and the old gui design:
 * This class will stop using ITraversable interface under dsm and kind of
 * replace the ITravTreeWrapper2 class too.
 *
 * When dealing with tree nodes, there are two options, one is to use
 * the WxResourceTreeItemData which is derived from wxTreeItemData and contains
 * a dsm/Resource obj, the other is to use the wxTreeItemId that corresponse
 * to each node. I decided to use WxResourceTreeItemData. That is why there
 * is no usage of wxTreeItemId in this interface.
 *
 */

#ifndef WXRESOURCETREECTRL_H
#define WXRESOURCETREECTRL_H

#include <string>
using std::string;

#include "wx/treectrl.h"

#include "dsm/ResourceDescriptor.H"

#include "wxgui/ewxTreeCtrl.H"


class BookmarkList;
class EcceURL;
class HistoryList;
class Resource;
class WxResourceTreeItemData;


class WxResourceTreeCtrl : public ewxTreeCtrl
{
public:

  WxResourceTreeCtrl(wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxTR_HAS_BUTTONS|wxTR_HIDE_ROOT|wxTR_NO_LINES,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = "treeCtrl");
  bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
              const wxPoint& pos = wxDefaultPosition,
              const wxSize& size = wxDefaultSize,
              long style = wxTR_HAS_BUTTONS|wxTR_HIDE_ROOT|wxTR_NO_LINES,
              const wxValidator& validator = wxDefaultValidator,
              const wxString& name = "treeCtrl");
  virtual ~WxResourceTreeCtrl();
  
  virtual void initialize(HistoryList * historyList = 0,
                          bool homeAsRoot = false,
                          bool showRunStates = true,
                          int treeSort = 0);
  virtual void mountEDSIProviders();
  virtual void mountServerList(BookmarkList * serverList);

  virtual WxResourceTreeItemData * addRoot(const string& name,
                                           const EcceURL& url);

  virtual bool loadChildren(WxResourceTreeItemData * node,
                            bool refresh = false);

  virtual WxResourceTreeItemData * addToTree(const EcceURL & url);
  virtual bool removeFromTree(const EcceURL & url, bool deleteFromServer);

  virtual WxResourceTreeItemData * findNode(const EcceURL & targetUrl,
                                            bool openWhenFound = true,
                                            bool loadFromServer = true);

  virtual WxResourceTreeItemData * getParent(WxResourceTreeItemData * child);

  virtual WxResourceTreeItemData * getServerRoot(WxResourceTreeItemData * child = 0);
  virtual string getServerRootName(WxResourceTreeItemData * child = 0);

  virtual void selectParent();
  virtual bool canSelectParent();

  virtual WxResourceTreeItemData * openHome(bool useDefaultServer = false);

  /*
   * @warning this function will invalidate all the WxResourceTreeItemData *
   *          reference to its children since it will delete all children and
   *          reinsert all children. Use with care.
   */
  virtual void refresh(WxResourceTreeItemData * node = 0);

  virtual bool setExpandable(WxResourceTreeItemData * node);

  virtual void recolorStatePixmaps();

  virtual void refreshState(WxResourceTreeItemData * node);

  virtual void setTreeSort(int treeSort);

  virtual WxResourceTreeItemData * getSelection();


protected:

  WxResourceTreeCtrl();

  virtual void clear();

  virtual bool autoOpen();

  virtual WxResourceTreeItemData * findNode(const EcceURL & targetUrl,
                                            WxResourceTreeItemData * current,
                                            bool openWhenFound = true,
                                            bool loadFromServer = true);

  virtual void sortTree(wxTreeItemId nodeId);

  virtual int OnCompareItems(const wxTreeItemId& item1,
                             const wxTreeItemId& item2);

  void setImageList();
  int getImageIndex(Resource * resource,
                    ResourceDescriptor::IconType iconType =
                    ResourceDescriptor::IT_NORMAL);

private:
  
  HistoryList * p_historyList;
  bool p_deepTraverse;
  bool p_homeAsRoot;
  bool p_showRunStates;
  int p_treeSort;
};

#endif
