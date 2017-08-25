//////////////////////////////////////////////////////////////////////////
// Desgin
//////////////////////////////////////////////////////////////////////////
#ifndef EDSITRAVERSABLE_HH
#define EDSITRAVERSABLE_HH

#include <vector>
#include <string>
using std::string;
using std::vector;

#include "dsm/ITraversable.H"
#include "dsm/ResourceDescriptor.H"

class EDSI;
class TNode;

/**
 * EDSI-based implementation for sources of Hierarchical objects
 */
class EDSITraversable : public ITraversable {
public:
  EDSITraversable(const EcceURL& url);
  virtual ~EDSITraversable();
  
  bool operator==(const EDSITraversable& rhs) const;
  
  virtual string getRoot();
  virtual string getType();
  virtual vector<TNode*> *getChildren(TNode *node);
  virtual vector<string> getPropertyList();
  virtual bool updateProperties(TNode *node);
  virtual bool updateList(TNode *node,vector<TNode*>& newChildren);
  
  //virtual istream& get(TNode *node) = 0;
  virtual EcceURL *setName(TNode *node, const string& name);
  virtual EcceURL *move(TNode *node, TNode *parent);
  virtual bool remove(TNode *node);
  virtual EcceURL *makeCollection(TNode *node,
                                  const char *base="Untitled",
                                  const char *contenttype="");
  virtual TNode * createTNode(string name="",
                              string contentType="",
                              ResourceDescriptor::RESOURCETYPE resourceType=ResourceDescriptor::RT_DOCUMENT,
                              const EcceURL & url="");
};

#endif
