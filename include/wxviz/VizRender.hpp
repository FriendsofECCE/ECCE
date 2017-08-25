/**
 * @file
 *
 */
#ifndef VIZRENDER_H_
#define VIZRENDER_H_

#include <string>
using std::string;

class ChemistryTask;
class SbViewportRegion;
class SoNode;
class SoOffscreenRenderer;
class SGContainer;
class SGViewer;
class SFile;

class VizRender
{
  public:
    static bool thumbnail(const string& urlstr, int width = 64, int height = 64,
                          double r = 0.0, double g = 0.0, double b = 0.0);
    static bool thumbnail(SoNode *root, ChemistryTask *task,
                          int width = 64, int height = 64,
                          double r = 0.0, double g = 0.0, double b = 0.0);
    static bool file(SoNode *root, SFile *file, string type = "RGB",
                     int width = 64, int height = 64,
                     double r = 0.0, double g = 0.0, double b = 0.0);
    static string msg();

  protected:
    static void loadAtomColors(SGContainer *sg);
    static void loadAtomRadii(SGContainer *sg);
    static void loadDisplayStyle(SGViewer *viewer, SGContainer *sg);

  private:
   // For making a  movie, you run out of X
   // connections if the render area isn't static.  In my testing, I
   // ran out on about the 70th invocation even though I was
   // deleting the object!  But if we just have one static, the user
   // can't change width/height.
 
    static SbViewportRegion    *p_viewport;
    static SoOffscreenRenderer *p_renderer;
    static string               p_msg;
    static int                  p_oldWidth;
    static int                  p_oldHeight;
};

#endif // VIZRENDER_H_

