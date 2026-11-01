/**
 * @file
 * @author Lisong Sun
 *
 * A subclass derived from wxTreeItemData that contains a Resource pointer.
 * This class will be used by WxResourceTreeCtrl to construct a tree.
 *
 */

#ifndef WXRESOURCETREEITEMDATA_H
#define WXRESOURCETREEITEMDATA_H

#include <string>
using std::string;

#include "wx/treectrl.h"


class Resource;
class EcceURL;

class WxResourceTreeItemData : public wxTreeItemData
{
public:
  WxResourceTreeItemData(Resource * resource = NULL);
  
  virtual ~WxResourceTreeItemData();
  
  virtual Resource * getResource();

  EcceURL getUrl() const;

  string getName() const;

  // Root of an ECCE server the user added (#767): its EcceURL::getRef(),
  // and the name from the server list.  Without a resource the server is
  // not connected.
  void setServer(const string& name, const string& ref) { p_serverName = name; p_serverRef = ref; }
  const string& serverRef() const { return p_serverRef; }
  const string& serverName() const { return p_serverName; }
  
protected:
  Resource * p_resource;
  string p_serverRef;
  string p_serverName;
};

#endif
