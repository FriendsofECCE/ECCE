#ifndef POVRAYCMD_H
#define POVRAYCMD_H

#include <string>
#include <map>
using std::string;
using std::map;
using std::less;

#include "inv/actions/SoCallbackAction.H"
#include "inv/SoPrimitiveVertex.H"
#include "inv/nodes/SoIndexedTriangleStripSet.H"
#include "inv/SbColor.H"

#include "util/Command.H"

class SoNode;
class SoType;
class SoCamera;
class SoDirectionalLight;

class PovrayCmd : public Command
{
  public:
    PovrayCmd(const string& name, Receiver * receiver);
    ~PovrayCmd();

    virtual bool execute() throw(EcceException);
    virtual void undo();

  protected:
    void init();

    // convenience method to find the first object of a given type.
    SoNode *findFirst(SoNode *start, SoType type, bool anysubclass=false);
    SoNode *findLast(SoNode *start, SoType type, bool anysubclass=false);

    void addHeaderComments(ostream& os);
    void addCameraAndLights(ostream& ostr, SoCamera *camera,
                            SoDirectionalLight *light);
    void preScanFragment(ostream& ostr, SoNode *top);
    void addFragment(ostream& ostr, SoNode *top);
    void addHBonds(ostream& ostr, SoNode *top);

    void preScanVectors(ostream& ostr, SoNode *top);
    void addVectors(ostream& ostr, SoNode *top);

    void preScanSurfaces(ostream& ostr, SoNode *top);
    void addSurface(ostream& ostr, SoNode *top);
    static void printVertex(ostream& ostr, const SoPrimitiveVertex *v);
    static SoCallbackAction::Response solidSurfacePreCB(void *,
           SoCallbackAction *, const SoNode *node);
    static SoCallbackAction::Response solidSurfacePostCB(void *,
           SoCallbackAction *, const SoNode *node);
    static void printTriangleCB(void *, SoCallbackAction *,
                                const SoPrimitiveVertex *v1,
                                const SoPrimitiveVertex *v2,
                                const SoPrimitiveVertex *v3);
    // These have to be static now because we have all these c-style callbacks.
    // But, we could fix this by sending in the class as the data instead of
    // the ostream as is done now.
    static string p_IsoFinish;
    static string p_surfacePigment;
    vector<vector<float> > p_colors;
    float p_transparency;
    bool p_useLabel;
};
#endif
