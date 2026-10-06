#ifndef SCENESCRIPT_H_
#define SCENESCRIPT_H_

#include <functional>
#include <string>
#include <vector>
using std::string;
using std::vector;

class IPropCalculation;
class SGContainer;
class SGViewer;
class wxWindow;

/**
 * Drives the viewer from a text script and writes offscreen snapshots, so
 * two builds of the viewer stack can be compared with no input events.
 * Used by the Builder (ECCE_VIEWER_SCENE=<script>, with the open
 * calculation) and by tools/coin/viewer-scenes.C (plain XYZ systems).
 *
 * One command per line, '#' starts a comment:
 *   style <DisplayStyle name>     labels none|element|name|charge2|charge3|charge4|type
 *   resetview                     camera back to home orientation, view all
 *   bondlabels on|off   residuelabels on|off
 *   select <n>... | deselect      (1-based atom numbers)
 *   mo <n> [isovalue [res]]       compute MO n and show it  (needs calc)
 *   nmvect <mode>                 normal-mode vectors       (needs calc)
 *   nmstep <mode> <step>          one animation frame       (needs calc)
 *   gtstep <n>                    one geometry-trace step   (needs calc)
 *   transparency SCREEN_DOOR|DELAYED_ADD|SORTED_OBJECT_BLEND
 *   pick <name> <n>...            click atoms; selection -> <name>.txt
 *   drag <name> <n> <dx> <dy>     select atom n, drag it by pixels; coords
 *   redraws <name> <step>...      one gt step per change, frames -> <name>.txt
 *   viewall | clear               reset the camera / hide MO and modes
 *   snap <name>   the canvas, read back with glReadPixels -> <name>.ppm
 *   thumb <name>  64x64 black background, as the calculation thumbnail
 *   nmcheck <name>  per normal-mode arrow: tip distance from its atom centre
 *                   (arrow triangles) and the atom's sphere surface along
 *                   the arrow (ray pick) -> <name>.txt
 * Commands this class does not know go to the caller's extension, if set.
 * The Builder adds "mopanel <name>" (press the MO panel's Compute),
 * "motable <name>" (the MO table's selected row and scroll position, and
 * the alpha HOMO from ORBOCC -> <name>.txt) and "hold <seconds>" (keep the
 * event loop running, so the windows can be captured from outside).
 */
class SceneScript
{
  public:
    SceneScript(SGViewer *viewer, SGContainer *sg, IPropCalculation *calc,
                const string& outdir);

    // Runs every line; stops at the first failing one.
    bool run(const string& scriptPath);
    const string& message() const { return p_msg; }

    // Unknown commands go here; return false (and set fail()) on error.
    typedef std::function<bool(SceneScript&, const vector<string>&)> Extension;
    void setExtension(Extension e) { p_ext = e; }

    // Runs act() and records every frame the render area under canvasOwner
    // paints meanwhile: <name>-NN.ppm plus one line per frame in
    // <name>-frames.txt with the thresholds of the isosurfaces drawn.
    bool recordFrames(wxWindow *canvasOwner, const string& name,
                      std::function<void()> act);
    bool fail(const string& m) { p_msg = m; return false; }

  private:
    bool exec(const vector<string>& w, const string& rest);
    bool pickAtoms(const string& name, const vector<string>& atoms);
    bool dragAtom(const string& name, int atom, int dx, int dy);
    bool countRedraws(const string& name, const vector<string>& steps);
    bool snapshot(const string& name, int size, float r, float g, float b);
    bool exportIso(const string& name);
    bool nmCheck(const string& name);

    SGViewer *p_viewer;
    SGContainer *p_sg;
    IPropCalculation *p_calc;
    string p_outdir;
    string p_msg;
    Extension p_ext;
};

#endif
