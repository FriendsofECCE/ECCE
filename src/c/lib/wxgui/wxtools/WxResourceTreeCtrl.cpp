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

#if 000
#if defined(__GNUG__) && !defined(__APPLE__)
#pragma implementation "WxResourceTreeCtrl.C"
#endif
#endif

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include <iostream>
using std::cerr;
using std::endl;

#include "wx/treectrl.h"

#include "util/Ecce.H"
#include "util/ErrMsg.H"
#include "util/ETimer.H"
#include "util/TDateTime.H"
#include "util/Bookmark.H"
#include "util/ResourceUtils.H"
#include "util/StringConverter.H"

#include "dsm/BookmarkList.H"
#include "dsm/EDSIServerCentral.H"
#include "dsm/EDSIFactory.H"
#include "dsm/HistoryList.H"
#include "dsm/ResourceDescriptor.H"
#include "dsm/ResourceType.H"
#include "dsm/Resource.H"
#include "dsm/Session.H"
#include "dsm/TaskJob.H"
#include "dsm/VDoc.H"

#include "wxgui/ewxBitmap.H"
#include "wxgui/ewxTreeCtrl.H"
#include "wxgui/WxResourceImageList.H"
#include "wxgui/WxResourceStateImageList.H"
#include "wxgui/WxResourceTreeItemData.H"
#include "wxgui/WxResourceTreeCtrl.H"
#include "wxgui/WxStateImageList.H"


/**
 * Constructor.
 */
WxResourceTreeCtrl::WxResourceTreeCtrl()
  : ewxTreeCtrl()
{

}


/**
 * Constructor.
 */
WxResourceTreeCtrl::WxResourceTreeCtrl(wxWindow* parent,
                                       wxWindowID id,
                                       const wxPoint& pos,
                                       const wxSize& size,
                                       long style,
                                       const wxValidator& validator,
                                       const wxString& name)
  : ewxTreeCtrl()
{
  Create(parent, id, pos, size, style, validator, name);
}


bool WxResourceTreeCtrl::Create(wxWindow* parent, wxWindowID id,
                                const wxPoint& pos, const wxSize& size,
                                long style, const wxValidator& validator,
                                const wxString& name)
{
  if (!ewxTreeCtrl::Create(parent, id, pos, size, style, validator, name)) {
    wxFAIL_MSG( wxT("WxResourceTreeCtrl creation failed") );
    return false;
  }

  // Somehow putting this space and indent setting in ewxTreeCtrl() won't work
  SetSpacing(8);
  SetIndent(10);

  // Image list for folder expand and collapse button
  wxImageList * butList = new wxImageList(12, 12, true);
  butList->Add(ewxBitmap("collapsed.xpm"));  // Button closed
  butList->Add(ewxBitmap("collapsed.xpm"));  // Button closed selected
  butList->Add(ewxBitmap("expanded.xpm"));  // Button expanded
  butList->Add(ewxBitmap("expanded.xpm"));  // Button expanded selected
  AssignButtonsImageList(butList);

  return true;
}


/**
 * Destructor.
 */
WxResourceTreeCtrl::~WxResourceTreeCtrl()
{
  clear();
}


/**
 * Clean up the memory before delete or re-init.
 */
void WxResourceTreeCtrl::clear()
{
  DeleteAllItems();
}


/**
 * Initialize setting and add roots.
 */
void WxResourceTreeCtrl::initialize(HistoryList * historyList,
                                    bool homeAsRoot,
                                    bool showRunStates,
                                    int treeSort)
{
  clear();

  p_historyList = historyList;
  p_homeAsRoot = homeAsRoot;
  p_showRunStates = showRunStates;
  p_treeSort = treeSort;

  // Image list for various resources nodes
  setImageList();

  // Since there can't be multiple roots in the wxTreeCtrl, one hidden root is
  // used as the parent of all the real server roots.
  AddRoot(wxT("GlobalRoot"), -1, -1, new WxResourceTreeItemData());
}


void WxResourceTreeCtrl::mountEDSIProviders()
{
  EDSIServerCentral servers;
  vector<Bookmark> * mount = servers.getEDSIProviders();

  vector<Bookmark>::iterator mountItor = mount->begin();
  for (;mountItor!= mount->end(); mountItor++) {
    addRoot(mountItor->getName(), mountItor->getUrl());
  }
  delete mount;
  autoOpen();
}


void WxResourceTreeCtrl::mountServerList(BookmarkList * serverList)
{
  if (serverList) {
    vector<Bookmark *>::iterator localMountItor =
      serverList->getBookmarks()->begin();
    for (;localMountItor!= serverList->getBookmarks()->end();) {
      if (!addRoot((*localMountItor)->getName(), (*localMountItor)->getUrl())) {
        serverList->getBookmarks()->erase(localMountItor);
      } else {
        localMountItor++;
      }
    }
  }
}


/**
 * Add one more root to the TreeCtrl.
 *
 * There is no duplicate check in this function, don't know if it is necessary
 * to check if the server is already in the list.
 */
WxResourceTreeItemData * WxResourceTreeCtrl::addRoot(const string& name,
                                                     const EcceURL& url)
{
  string rootName = name;
  EcceURL rootUrl = url;
  if (p_homeAsRoot) {
    rootName = rootName + " - " + Ecce::serverUser();
    EDSIServerCentral servers;
    rootUrl = servers.getUserHome(url);
  }

  Resource * res = EDSIFactory::getResource(rootUrl);
  WxResourceTreeItemData * node = 0;
  if (res) {
    node = new WxResourceTreeItemData(res);
    
    int serverIcon = getImageIndex(node->getResource());
    wxTreeItemId itemId = AppendItem(GetRootItem(), rootName,
                                     serverIcon, serverIcon+1, node);
    SetItemImage(itemId, serverIcon+2, wxTreeItemIcon_Expanded);
    SetItemImage(itemId, serverIcon+3, wxTreeItemIcon_SelectedExpanded);
    
    SetItemHasChildren(itemId);
  }

  // @todo Lisong Need to know whether succeeded.
  return node;
}


/**
 * Load the children of node.
 *
 * @param node The target parent node.
 * @param refresh Whether force to reload children from server.
 * @return Whether node can be opened, maynot because of authentication.
 */
bool WxResourceTreeCtrl::loadChildren(WxResourceTreeItemData * node, bool refresh)
{
  // If refresh, clear the children first
  if (refresh) {
    DeleteChildren(node->GetId());
  }
  // If not refresh and has children already, return without doing anything
  else if (GetChildrenCount(node->GetId(), false) != 0) {
    return true;
  }

#ifdef cputimetest
  ETimer timer;
  timer.start();
#endif

  vector<Resource *> * children = node->getResource()->getChildren(refresh);

#ifdef cputimetest
  timer.stop();
  cerr << "elapsed get children " << timer.elapsedTime() << endl
       << "cput get children " << timer.CPUTime() << endl;
  timer.reset();
  timer.start();
#endif
  
  if (children != 0) {
    vector<Resource *>::iterator resItor = children->begin();
    for (; resItor!= children->end(); resItor++) {
      WxResourceTreeItemData * childNode = new WxResourceTreeItemData(*resItor);
      int resIcon = getImageIndex(*resItor);
      wxTreeItemId itemId = AppendItem(node->GetId(), (*resItor)->getName(),
                                       resIcon, resIcon+1, childNode);
      SetItemImage(itemId, resIcon+2, wxTreeItemIcon_Expanded);
      SetItemImage(itemId, resIcon+3, wxTreeItemIcon_SelectedExpanded);
      
      setExpandable(childNode);
    }
    SortChildren(node->GetId());
  }

#ifdef cputimetest
  timer.stop();
  cerr << "create tree  nodes..." << timer.elapsedTime() << endl;
#endif

  return children != 0;
}


/**
 * Load the node into the tree in the fastest way.
 *
 * This function will add a node into the gui tree. This function should only 
 * be used when a new Resource is created and the tree needs to be updated.
 *
 * If the node is already in the tree or if the parent is not loaded yet,
 *   just return and don't do anything.
 * Otherwise load it in the treectrl.
 *
 * @param url The child node's url.
 * @return pointer to the newly added resource tree item, 0 if no need to add
 */
WxResourceTreeItemData * WxResourceTreeCtrl::addToTree(const EcceURL & url)
{
  EDSIFactory::createPoolResource(url);

  // See if the node is ready in the tree, if so no need to do anything.
  WxResourceTreeItemData * ret = findNode(url, false, false);
  if (ret != 0)
    return ret;

  // Try to find its parent without going back to server.
  WxResourceTreeItemData * parent = findNode(url.getParent(), false, false);
  // If didn't find, no need to add that node.
  if (parent == 0)
    return 0;
  
  vector<Resource *> * children = parent->getResource()->getCachedChildren();

  if (children != 0) {

    Resource * resource = EDSIFactory::getResource(url);
    ret = new WxResourceTreeItemData(resource);
    int resIcon = getImageIndex(resource);
    wxTreeItemId itemId = AppendItem(parent->GetId(),
                                     resource->getName(),
                                     resIcon, resIcon+1, ret);
    SetItemImage(itemId, resIcon+2, wxTreeItemIcon_Expanded);
    SetItemImage(itemId, resIcon+3, wxTreeItemIcon_SelectedExpanded);
    
    setExpandable(ret);
    SortChildren(parent->GetId());
  }

  return ret;
}


/**
 * Remove the node from the tree in the fastest way.
 *
 * This function will remove the node from the gui tree. Delete the file
 * from server if the deleteFromServer is set. Otherwise it will assume
 * the file is already deleted and just remove the Resource object in the
 * resource pool.
 *
 * @param url The child node's url.
 */
bool WxResourceTreeCtrl::removeFromTree(const EcceURL & url,
                                        bool deleteFromServer)
{
  // See if the node is in the tree, if not no need to do anything.
  // GDB 2/11/13 pass loadFromServer as true or sometimes the node isn't found
  WxResourceTreeItemData * node = findNode(url, false, true);
  if (node == 0)
    return true;

  bool ret = true;
  
  EcceURL selection = getSelection()->getUrl();

  // Find its parent without going back to server.
  WxResourceTreeItemData * parent = getParent(node);

  if (selection == url)
    SelectItem(parent->GetId());

  if (deleteFromServer) {
    Resource * res = node->getResource();
    ret = res->remove();
  }
  else
    EDSIFactory::removePoolResource(url);

  Delete(node->GetId());

  return ret;
}


/**
 * Automatically open to the resource tree either to the last place the user
 * was working on, if failed then open to the default home area.
 */
bool WxResourceTreeCtrl::autoOpen()
{
  WxResourceTreeItemData * autoOpenNode = 0;
  if (p_historyList && p_historyList->size() > 0) {
    EcceURL url = p_historyList->pop();
    autoOpenNode = findNode(url, true);
  }

  // We'll be nice and try to open to a default location on the first
  // mount point if autoOpenNode is still 0.
  // If the user cancels out of authenticating from the above call to
  // findNode, they will probably have to authenticate again here which is
  // confusing and obnoxious but hopefully won't happen often.
  if (autoOpenNode == 0) {
    EDSIServerCentral servers;
    EcceURL url = servers.getDefaultUserHome();
    autoOpenNode = findNode(url, true);
  }
  return autoOpenNode != 0;
}


/**
 * Find the target url recursively.
 *
 * Will load the nodes on the path from server if not loaded.
 * Start searching from the hidden root.
 * @param targetUrl The target URL.
 * @param openWhenFound When found whether open the target (default true).
 * @param loadFromServer Whether load the children from server (default true).
 * @return The target node, 0 if not found.
 */
WxResourceTreeItemData * WxResourceTreeCtrl::findNode(const EcceURL & targetUrl,
                                                      bool openWhenFound,
                                                      bool loadFromServer)
{
  return findNode(targetUrl, dynamic_cast<WxResourceTreeItemData *>
                  (GetItemData(GetRootItem())), openWhenFound, loadFromServer);
}


/**
 * Find the target url recursively.
 *
 * @param targetUrl The target URL.
 * @param current The current node's ID.
 * @param openWhenFound When found whether open the target (default true).
 * @param loadFromServer Whether load the children from server (default true).
 * @return The target node, 0 if not found
 */
WxResourceTreeItemData *
WxResourceTreeCtrl::findNode(const EcceURL & targetUrl,
                             WxResourceTreeItemData * current,
                             bool openWhenFound,
                             bool loadFromServer)

{
  EcceURL currentUrl = current->getUrl();

  WxResourceTreeItemData * result = 0;

  if (targetUrl == currentUrl) {
    if (openWhenFound) {
      SelectItem(current->GetId());

      // open projects/sessions in the tree when they are created on the
      // assumption the user most likely will want to work in it
      Resource * res = current->getResource();
      if (res->getContentType()==ResourceDescriptor::CT_PROJECT ||
          res->getContentType()==ResourceDescriptor::CT_SESSION)
        Expand(current->GetId());
    }
    result = current;
  }
  // When currentUrl equals EcceURL(), it means the node is the hidden root
  // of the tree
  else if (targetUrl.isChildOf(currentUrl) || currentUrl == EcceURL()) {
    if (loadFromServer)
      loadChildren(current, false);
    if (openWhenFound)
      Expand(current->GetId());
    wxTreeItemIdValue cookie;
    wxTreeItemId childId = GetFirstChild(current->GetId(), cookie);
    for (; childId.IsOk(); childId = GetNextChild(current->GetId(), cookie)) {
      WxResourceTreeItemData * childNode = 
        dynamic_cast<WxResourceTreeItemData *>(GetItemData(childId));
      result = findNode(targetUrl, childNode, openWhenFound, loadFromServer);
      if (result)
        break;
    }
  }
  return result;
}


/**
 * Get the parent of current node.
 *
 * In the old design, since TNodes don't have back pointers, trying to find
 * parent is done by searching the tree. Now since wxTreeCtrl provided the
 * function, there is no need to do the search.
 */
WxResourceTreeItemData * WxResourceTreeCtrl::getParent(WxResourceTreeItemData * child)
{
  WxResourceTreeItemData * ret = child;
  wxTreeItemId parentId = GetItemParent(child->GetId());
  // Never return wxTreeCtrl root, stop at server root level which are children
  // of the real root
  if (parentId != GetRootItem()) {
    ret = dynamic_cast<WxResourceTreeItemData *>(GetItemData(parentId));
  }
  return ret;
}


/**
 * Get the server root of current node.
 *
 * Instead of comparing the URL string, this function use the get parent
 * approach to traverse along the path back to the root.
 */
WxResourceTreeItemData * WxResourceTreeCtrl::getServerRoot(WxResourceTreeItemData * child)
{
  wxTreeItemId currentId;
  if (child == 0) {
    currentId = GetSelection();
  }
  else {
    currentId = child->GetId();
  }
  wxTreeItemId parentId = GetItemParent(currentId);
  while (parentId != GetRootItem()) {
    currentId = parentId;
    parentId = GetItemParent(currentId);
  }
  return dynamic_cast<WxResourceTreeItemData *>(GetItemData(currentId));
}


/**
 * Get the server root name of current node.
 */
string WxResourceTreeCtrl::getServerRootName(WxResourceTreeItemData * child)
{
  return string(GetItemText(getServerRoot(child)->GetId()).c_str());
}


/**
 * Select the parent of current selection, skip if it is already at root
 */
void WxResourceTreeCtrl::selectParent()
{
  if (canSelectParent()) {
    SelectItem(GetItemParent(GetSelection()));
  }
}


/**
 * Check if the current selection is one of the server roots
 */
bool WxResourceTreeCtrl::canSelectParent()
{
  return GetItemParent(GetSelection()) != GetRootItem();
}


/**
 * Go to the user's home directory
 *
 * @param useDefaultServer When it is true, go to the home on the first server
 *                         in the server list;
 *                         When it is false, go to the home on the server that
 *                         contains the currently selected node;
 *                         Default is false.
 */
WxResourceTreeItemData * WxResourceTreeCtrl::openHome(bool useDefaultServer)
{
  WxResourceTreeItemData * homeNode;
  EDSIServerCentral servers;
  if (useDefaultServer) {
    EcceURL url = servers.getDefaultUserHome();
    homeNode = findNode(url, true);
  }
  else {
    if (p_homeAsRoot) {
      homeNode = findNode(getServerRoot()->getUrl(), true);
    }
    else {
      homeNode = findNode(servers.getUserHome(getServerRoot()->getUrl()),true);
    }

    // If can't find user home dir on this server, load home on default server
    if (!homeNode)
      homeNode = findNode(servers.getDefaultUserHome(), true);
  }

  // If still can't find user home dir on this server, load root
  if (!homeNode) {
    homeNode = getServerRoot();
  }

  loadChildren(homeNode, false);

  wxTreeItemId homeId = homeNode->GetId();

  Expand(homeId);

  SelectItem(homeId);
  //  EnsureVisible(homeId);

  return homeNode;
}


/**
 * Refresh the node by update the node's properties and
 * reload the children from server.
 */
void WxResourceTreeCtrl::refresh(WxResourceTreeItemData * node)
{
  if (node == 0) {
    node = dynamic_cast<WxResourceTreeItemData *>(GetItemData(GetSelection()));
  }

  if (setExpandable(node)) {
    loadChildren(node, true);
    wxTreeItemId nodeId = node->GetId();
    wxTreeItemIdValue cookie;
    wxTreeItemId childId = GetFirstChild(nodeId, cookie);
    for (; childId.IsOk(); childId = GetNextChild(nodeId, cookie)) {
      WxResourceTreeItemData * child =
        dynamic_cast<WxResourceTreeItemData *>(GetItemData(childId));
      child->getResource()->clearChildren();
      DeleteChildren(childId);
    }
  }

  if (node->getResource()->hasAccess())
    refreshState(node);

  wxTreeItemId nodeId = node->GetId();
  // Do not refresh root node's label.
  if (GetItemParent(nodeId) != GetRootItem())
    SetItemText(node->GetId(), node->getResource()->getName());
}



/**
 * Set the node's expand/collapse button.
 *
 * @return whether the node is expandable
 */
bool WxResourceTreeCtrl::setExpandable( WxResourceTreeItemData * node )
{
  ResourceType * resType = node->getResource()->getDescriptor();
  wxTreeItemId id = node->GetId();
  bool ret = false;
  ret = resType->getIsExpandable();
  SetItemHasChildren(id, ret);
  return ret;
}


void WxResourceTreeCtrl::recolorStatePixmaps()
{
  WxStateImageList::refresh();
  WxResourceStateImageList::refresh();
}


int WxResourceTreeCtrl::OnCompareItems(const wxTreeItemId& item1,
                                       const wxTreeItemId& item2)
{
  WxResourceTreeItemData *node1 =
    dynamic_cast<WxResourceTreeItemData *>(GetItemData(item1));
  Resource * res1 = node1->getResource();

  WxResourceTreeItemData *node2 =
    dynamic_cast<WxResourceTreeItemData *>(GetItemData(item2));
  Resource * res2 = node2->getResource();

  int ret = 0;

  // Sort session tasks first
  // Assume the parent is the session resource
  Resource * parentRes = getParent(node1)->getResource();
  if (parentRes->getDescriptor()->getIsSessionBased ()) {
    string index1 = res1->getProp(VDoc::getDcQualifierNamespace() + ":taskIndex");
    string index2 = res2->getProp(VDoc::getDcQualifierNamespace() + ":taskIndex");
    if (index1 != "" && index2 != "") {
      int left = 0;
      int right = 0;
      StringConverter::toInt(index1, left);
      StringConverter::toInt(index2, right);
      ret = left - right;
    }
    else if (index1 == "") {
      ret = -1;
    }
    else if (index2 == "") {
      ret = 1;
    }
  }

  // Sort other Resources based on the p_treeSort
  if (ret == 0) {
    switch(p_treeSort) {
    case 0:
    case 1:
      ret = node1->getResource()->getDescriptor()->getSortPriority() -
        node2->getResource()->getDescriptor()->getSortPriority();
      if (ret == 0)
        ret = StringConverter::compare(node1->getName(), node2->getName(), false);
      break;
    case 2:
    case 3:
      ret = StringConverter::compare(node1->getName(), node2->getName(), false);
      break;
    case 4:
    case 5:
      TDateTime * time1 = node1->getResource()->getModifiedDate();
      TDateTime * time2 = node2->getResource()->getModifiedDate();
      if (time1 && time2)
        ret = time1->compare(*time2);
      delete time1;
      delete time2;
      break;
    }
    // Reverse the odd ones
    if (p_treeSort%2 == 1)
      ret = -ret;
  }

  return ret;
}


void WxResourceTreeCtrl::refreshState(WxResourceTreeItemData * node)
{
  wxTreeItemId itemId = node->GetId();
  int resIcon = getImageIndex(node->getResource());
  SetItemImage(itemId, resIcon, wxTreeItemIcon_Normal);
  SetItemImage(itemId, resIcon+1, wxTreeItemIcon_Selected);
  SetItemImage(itemId, resIcon+2, wxTreeItemIcon_Expanded);
  SetItemImage(itemId, resIcon+3, wxTreeItemIcon_SelectedExpanded);
}


/**
 * Set the sort method and sort the tree
 */
void WxResourceTreeCtrl::setTreeSort(int treeSort)
{
  p_treeSort = treeSort;
  if (GetCount() != 0) {
    sortTree(GetRootItem());
  }
  ScrollTo(GetSelection());
}


/**
 * Recursively sort the tree
 */
void WxResourceTreeCtrl::sortTree(wxTreeItemId nodeId)
{
  // Server roots order shouldn't change
  if (nodeId != GetRootItem())
    SortChildren(nodeId);
  wxTreeItemIdValue cookie;
  wxTreeItemId childId = GetFirstChild(nodeId, cookie);
  for (; childId.IsOk(); childId = GetNextChild(nodeId, cookie)) {
    sortTree(childId);
  }
}


WxResourceTreeItemData * WxResourceTreeCtrl::getSelection()
{
  if (!GetSelection().IsOk())
    openHome(true);
  
  return dynamic_cast<WxResourceTreeItemData *>
    (GetItemData(GetSelection()));
}


void WxResourceTreeCtrl::setImageList()
{
  if (p_showRunStates) {
    SetImageList(WxResourceStateImageList::getImageList());
  } else {
    SetImageList(WxResourceImageList::getImageList());
  }
}


int WxResourceTreeCtrl::getImageIndex(Resource * resource,
                                      ResourceDescriptor::IconType iconType)
{
  int ret;

  if (p_showRunStates) {
    TaskJob * taskJob = dynamic_cast<TaskJob*>(resource);
    ret = WxResourceStateImageList::getImageIndex(
            resource,
            taskJob ? taskJob->getState() :
                      ResourceDescriptor::NUMBER_OF_STATES,
            iconType);
  } else {
    ret = WxResourceImageList::getImageIndex(resource, iconType);
  }

  return ret;
}

