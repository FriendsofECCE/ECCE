#ifdef DEBUG
#include <iostream>
  using std::cerr;
  using std::endl;
  using std::flush;
#endif // DEBUG

#include "inv/nodes/SoSeparator.H"

#include <string>
  using std::string;
#include <string.h>

#include <wx/colour.h>

#include "tdat/TPerTab.H"

#include "util/Preferences.H"

#include "viz/CSLoadColorsCmd.H"
#include "viz/CSRadiiCmd.H"
#include "viz/SGContainer.H"

#include "wxviz/SGContainerManager.H"


SO_NODE_SOURCE(SGContainerManager);


/**
 * Constructor.
 * The SGContainerManager always has at least one context.  This constructor
 * creates a default "NewExperiment1" context.
 */
SGContainerManager::SGContainerManager(void)
  : p_context("")
{
  SO_NODE_CONSTRUCTOR(SGContainerManager);
}



SGContainerManager::~SGContainerManager()
{
}



void SGContainerManager::initClass(void)
{
  SO_NODE_INIT_CLASS(SGContainerManager, SoSeparator, "SoSeparator");
}



SGContainer* SGContainerManager::getSceneGraph()
{
  if (p_context == "") {
    // this indicates no children, which is invalid
    return NULL;
  }
  return (SGContainer*) SoNode::getByName(p_context);
}



/**
 * Sets the scene graph by name.
 * If it already existed, then this is a context switch.
 * Otherwise, a new SGContainer is created first and becomes the new context.
 *
 * @param name of the scene graph
 */
void SGContainerManager::setSceneGraph(const string& _name)
{
  SbName name = goodName(_name);

  if (getSceneGraph() && getSceneGraph()->getName() == name) {
    // attempting to set current context, bail
    return;
  }

  // see if the context already exists
  if (SoNode::getByName(name)) {
    p_context = name;
  } else {
    createAndSwitchToNewSceneGraph(_name);
  }

  // hide all then show the scenegraph in case it wasn't before
  showAll(false);
  ((SGContainer*)SoNode::getByName(name))->whichChild.setValue(0);
}



void SGContainerManager::removeSceneGraph()
{
  removeChild(getSceneGraph());
  p_context = "";
}



void SGContainerManager::removeSceneGraph(const string& _name)
{
  SbName name = goodName(_name);

  if (getSceneGraph()->getName() == name) {
    // attempting to remove current child
    removeSceneGraph();
    return;
  }

  SoNode * node = SoNode::getByName(name);
  if (node) {
    removeChild(node);
  } else {
    // no child with the name was found, this is likely an error
    throw "No scene graph found to remove with the given name " + _name;
  }
}



void SGContainerManager::renameSceneGraph(const string& oldName,
                                          const string& newName)
{
  SoNode * node = SoNode::getByName(goodName(oldName));
  if (node) {
    node->setName(goodName(newName));
  } else {
    // no child with the name was found, this is likely an error
    throw "No scene graph found to remove with the given name " + oldName;
  }
}



void SGContainerManager::showSceneGraph(const string& name, const bool& show)
{
  SGContainer * sg = (SGContainer*) SoNode::getByName(goodName(name));
  if (sg) {
    sg->whichChild.setValue(show ? 0 : -1);
  } else {
    throw "No scene graph found to show with the given name " + name;
  }
}



void SGContainerManager::showAll(const bool& show)
{
  int numChildren = getNumChildren();
  for (int i = 0; i < numChildren; i++) {
    ((SGContainer*)getChild(i))->whichChild.setValue(show ? 0 : -1);
  }
}



void SGContainerManager::loadAtomColors()
{
  int numChildren = getNumChildren();
  for (int i = 0; i < numChildren; i++) {
    loadAtomColors((SGContainer*) getChild(i));
  }
}



void SGContainerManager::loadAtomRadii()
{
  int numChildren = getNumChildren();
  for (int i = 0; i < numChildren; i++) {
    loadAtomRadii((SGContainer*) getChild(i));
  }
}



void SGContainerManager::createAndSwitchToNewSceneGraph(const string& _name)
{
  SbName name = goodName(_name);

  SGContainer* sg = new SGContainer;
  sg->setName(name);

  loadAtomColors(sg);
  loadAtomRadii(sg);

  addChild(sg);

  // turn on our new node
  p_context = name;
}



/**
 * Load atomic colors from preferences.
 * I would like the command to do all the work but don't have a way
 * to convert color names to rgb without using a gui toolkit.
 *
 * Note
 * @todo augment Color.H to support converting a string to a name
 *       and move the bulk of this code into a command.  Think about
 *       this though because, as constructed, a user can easily 
 *       change atom colors.
 */
void SGContainerManager::loadAtomColors(SGContainer* sg)
{
   Command *cmd = new CSLoadColorsCmd("Colors", sg);

   Preferences prefs("PerTable");  // pertab should be ecce global

   string xcolor, colorKey;
   TPerTab tpt;
   double r, g, b;
   int numAtoms = tpt.numAtoms();
   for (int ele=0; ele<numAtoms; ele++) {
      xcolor = tpt.colorName(ele);
      colorKey = tpt.atomicSymbol(ele);
      colorKey += ".Color";
      prefs.getString(colorKey,xcolor);

      wxColour wxcolor(xcolor);
      r = (int) wxcolor.Red() / 255.;
      g = (int) wxcolor.Green() / 255.;
      b = (int) wxcolor.Blue() / 255.;

      cmd->getParameter("atomIdx")->setInteger(ele);
      cmd->getParameter("red")->setDouble(r);
      cmd->getParameter("green")->setDouble(g);
      cmd->getParameter("blue")->setDouble(b);
      cmd->execute();
   }
}



void SGContainerManager::loadAtomRadii(SGContainer* sg)
{
   double radii, cpkradii;
   TPerTab tpt;

   // Get the command just to save time...
   Command *cmd = new CSRadiiCmd("radii", sg);

   int numAtoms = tpt.numAtoms();
   for (int ele=0; ele<numAtoms; ele++) {
      cpkradii = tpt.vwr(ele);
      radii = tpt.covalentRadius(ele);

      cmd->getParameter("atomIdx")->setInteger(ele);
      cmd->getParameter("radius")->setDouble(radii);
      cmd->getParameter("cpkradius")->setDouble(cpkradii);
      cmd->execute();
   }
}



/**
 * Replace bad characters with underscores.
 */
SbName SGContainerManager::goodName(const string& name) const
{
  if (name.empty()) return SbName();

#ifdef DEBUG
  bool isBad = false;
#endif
  SbString goodString;

  // Prepend underscore if name begins with number
  if (!SbName::isBaseNameStartChar(name[0])) {
    goodString += "_";
#ifdef DEBUG
    isBad = true;
#endif
  }
  for (size_t i = 0; i < name.size(); i++) {
    // Ugly little hack so we can use SbString's += operator,
    // which doesn't do char's (only char *'s):
    char temp[2];
    temp[0] = name[i]; temp[1] = '\0';
    if (!SbName::isBaseNameChar(name[i])) {
      goodString += "_";
#ifdef DEBUG
      isBad = true;
#endif
    } else {
      goodString += temp;
    }
  }

#ifdef DEBUG
  if (isBad) {
    cerr << "SGContainerManager::goodName: Bad characters in name '" << flush
         << name << "'. Replacing with name '" << flush
         << goodString.getString() << "'" << endl;
  }
#endif

  return SbName(goodString);
}

