#ifndef  SGCONTAINERMANAGER_H_
#define  SGCONTAINERMANAGER_H_

#include "inv/nodes/SoSeparator.H"

class SGContainer;

/**
 * As the name implies, the SGContainerManager manages its SGContainer
 * children.  SGContainers are referenced by unique names which for the most
 * part are represented by Resource URLs.  Any text string will work, however,
 * such as the context-less "NewExperiment1", "NewExperiment2", etc.
 *
 * Some global style information about the scene graph is also managed here.
 *
 * Any combination of children can be turned on in unison.  There is the 
 * additional notion of a "current" child implied by get/setSceneGraph.
 */
class SGContainerManager : public SoSeparator
{
  SO_NODE_HEADER(SGContainerManager);

  public:  
    static void initClass(void);
    SGContainerManager(void);

    SGContainer* getSceneGraph();
    void setSceneGraph(const string& name);
    void removeSceneGraph();
    void removeSceneGraph(const string& name);

    void renameSceneGraph(const string& oldName, const string& newName);

    void showSceneGraph(const string& name, const bool& show=true);
    void showAll(const bool& show=true);

    void loadAtomColors();
    void loadAtomRadii();

  protected:
    virtual ~SGContainerManager();

    void createAndSwitchToNewSceneGraph(const string& name);

    void loadAtomColors(SGContainer* sg);
    void loadAtomRadii(SGContainer* sg);

    SbName goodName(const string& name) const;

  private:
    SbName p_context;
};

#endif // SGCONTAINERMANAGER_H_ 
