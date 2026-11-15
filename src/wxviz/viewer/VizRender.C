/**
 * @file
 *
 */
#include "inv/SoOffscreenRenderer.H"
#include "inv/nodes/SoGroup.H"
#include "inv/nodes/SoDrawStyle.H"
#include "inv/actions/SoSearchAction.H"
#include "inv/actions/SoGLRenderAction.H"
#include <wx/image.h>
#include <vector>

//#include <iostream>
//using std::cout;
//using std::endl;

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "dsm/EDSIFactory.H"
#include "dsm/ChemistryTask.H"
#include "dsm/Resource.H"

#include "wxviz/ImageConverter.H"
#include "util/TempStorage.H"

#include "viz/AtomLabelsCmd.H"
#include "viz/CSLoadColorsCmd.H"
#include "viz/CSRadiiCmd.H"
#include "viz/CSStyleCmd.H"
#include "viz/ForegroundCmd.H"
#include "viz/NewFragmentCmd.H"
#include "viz/SGContainer.H"
#include "viz/SGFragment.H"

#include "wxgui/ewxColor.H"
#include "wxgui/ewxConfig.H"

#include "wxviz/BackgroundCmd.H"
#include "wxviz/SGViewer.H"
#include "wxviz/ViewerEvtHandler.H"
#include "wxviz/VizRender.H"
#include "wxviz/DefaultStyle.H"

SbViewportRegion    * VizRender::p_viewport  = NULL;
SoOffscreenRenderer * VizRender::p_renderer  = NULL;
string                VizRender::p_msg       = "";
int                   VizRender::p_oldWidth  = 0;
int                   VizRender::p_oldHeight = 0; 

bool VizRender::thumbnail(const string& urlstr, int width, int height)
{
  p_msg = "";
  bool ret = true;

  // Check whether site has disabled creation of thumbnails
  if (getenv("ECCE_NO_VIZIMAGES")) {
    return ret;
  }

  // declared here in case of exception thrown
  SGContainer *sg;
  SGViewer *viewer;

  try {
    Resource * resource = EDSIFactory::getResource(urlstr);
    if (!resource) {
      //cout << "Could not retrieve Resource at " + urlstr << endl;
      throw EcceException("Could not retrieve Resource at " + urlstr,
              __FILE__, __LINE__);
    }
    ChemistryTask * task = dynamic_cast<ChemistryTask*>(resource);
    if (!task) {
      //cout << "Could not cast to ChemistryTask: " + urlstr << endl;
      throw EcceException("Could not cast to ChemistryTask: " + urlstr,
              __FILE__, __LINE__);
    }

    sg = new SGContainer();
    sg->whichChild.setValue(SO_SWITCH_ALL);

    wxDialog dialog(NULL, wxID_ANY, "Viz Thumbnail");
    wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
    dialog.SetSizer(mainSizer);
    viewer = new SGViewer(&dialog, wxID_ANY);
    viewer->setSceneGraph(sg);
    viewer->setViewing(false);
    viewer->showTextLabels(false);   // no corner texts in the picture
    mainSizer->Add(viewer, 1, wxEXPAND|wxALL, 0);

    loadAtomColors(sg);
    loadAtomRadii(sg);
    loadDisplayStyle(viewer, sg);

    SGFragment *sgfrag = sg->getFragment();

    // Force clear of system
    sgfrag->clear();
    sgfrag->touchNumbers();
    sg->touchChemDisplay();

    if (!task->getFragment(*sgfrag)) {
      //cout << "Could not retrieve Fragment from ChemistryTask" << endl;
      throw EcceException("Could not retrieve Fragment from ChemistryTask",
              __FILE__, __LINE__);
    }

    // TODO do all the initialization crap
    sgfrag->touchNumbers();
    sg->touchChemDisplay();

    // center the fragment in the viewer
    viewer->viewAll();

    // loadDisplayStyle gave the viewer the Background preference.
    const SbColor bg = viewer->getBackgroundColor();
    if (!VizRender::thumbnail(viewer->getTopNode(), task, bg[0], bg[1], bg[2],
                              width, height))
    {
      //cout <<  p_msg << endl;
      throw EcceException(p_msg, __FILE__, __LINE__);
    }

  } catch (EcceException& ex) {
    p_msg += "VizRender::thumbnail(url) error:\n";
    p_msg += ex.what();
    ret = false;
  }
  return ret;
}


bool VizRender::thumbnail(SoNode *root, ChemistryTask *task,
                          double r, double g, double b,
                          int width, int height)
{
  p_msg = "";
  bool ret = true;

  // Check whether site has disabled creation of thumbnails
  if (getenv("ECCE_NO_VIZIMAGES")) {
    return ret;
  }

  // declared here in case of exception thrown
  SFile *rgbFile = 0;
  SFile *jpegFile = 0;

  try {
    rgbFile = TempStorage::getTempFile();
    rgbFile->move(rgbFile->path() + ".rgb");

    if (!VizRender::file(root, rgbFile, "RGB", width, height, r, g, b)) {
      //cout << p_msg << endl;
      throw EcceException(p_msg, __FILE__, __LINE__);
    }

    ImageConverter imconv;
    // deletes rgb tmp file on disk after conversion to jpeg
    imconv.convert(rgbFile->path(), rgbFile->path() + ".jpeg", 
                   width, height, 8 /*image depth*/, true /*remove inFile*/);

    jpegFile = new SFile(rgbFile->path() + ".jpeg");
    if (!jpegFile->exists()) {
      //cout << "Lost the image after conversion" <<endl;
      throw EcceException("Lost the image after conversion",
              __FILE__, __LINE__);
    }

    if (!task->setThumbnail(jpegFile)) {
      //cout<<"Could not set thumbnail to ChemistryTask "+task->messages()<<endl;
      throw EcceException("Could not set thumbnail to ChemistryTask "
              + task->messages(), __FILE__, __LINE__);
    }
  } catch (EcceException& ex) {
    p_msg += "VizRender::thumbnail(SoNode) error:\n";
    p_msg += ex.what();
    ret = false;
  }

  // cleanup tmp files
  if (rgbFile) {
    if (rgbFile->exists())
      rgbFile->remove();
    delete rgbFile;
  }
  if (jpegFile) {
    if (jpegFile->exists())
      jpegFile->remove();
    delete jpegFile;
  }
  
  return ret;
}


bool VizRender::file(SoNode *root, SFile *file, string type,
                  int width, int height, double r, double g, double b)
{
  p_msg = "";
  bool ret = true;

  // declared here in case of exception thrown
  FILE * myFile = 0;

  try {
    // Create a viewport to render the scene into.
    if (p_viewport == 0 || p_oldWidth != width || p_oldHeight != height) {
      p_viewport = new SbViewportRegion;
      p_viewport->setWindowSize(SbVec2s(width,height));
      p_renderer = new SoOffscreenRenderer(*p_viewport);
      p_oldWidth = width;
      p_oldHeight = height;
    }
    p_renderer->setBackgroundColor(SbColor(r,g,b));

    if (p_renderer->render(root)) {
      // Generate RGB and write it to the given file
      myFile = fopen(file->path().c_str(), "wb");
      if (myFile) {
        if (type.find("RGB") == 0) {
          p_renderer->writeToRGB(myFile);
        } else if (type.find("Postscript") == 0) {
          p_renderer->writeToPostScript(myFile);
        } else {
          //cout << "Undefined offscreen render type" << endl;
          throw EcceException("Undefined offscreen render type",
                  __FILE__, __LINE__);
        }
      } else {
         //cout << "Unable to open file for write" <<endl;
         throw EcceException("Unable to open file for write: " + file->path(),
                  __FILE__, __LINE__);
      }
    } else {
      //cout << "Unable to render scene graph" << endl;
      throw EcceException("Unable to render scene graph", __FILE__, __LINE__);
    }
  } catch (EcceException& ex) {
    p_msg += "VizRender error:\n";
    p_msg += ex.what();
    ret = false;
  }

  // cleanup
  if (myFile)
    fclose(myFile);

  return ret;
}


bool VizRender::image(SoNode *root, const string& pngPath, int width,
                      int height, double r, double g, double b, int factor)
{
  p_msg = "";
  if (getenv("ECCE_NO_VIZIMAGES")) return true;
  if (factor < 1) factor = 1;
  if (width < 1 || height < 1) {
    p_msg = "VizRender::image: the view has no size";
    return false;
  }
  // Render at 2x the wanted pixels and average 2x2 blocks: the offscreen
  // GL context has no multisample or accumulation buffer to rely on.
  int ss = (width * factor * 2 > 8192 || height * factor * 2 > 8192) ? 1 : 2;
  const int rw = width * factor * ss, rh = height * factor * ss;
  const int ow = width * factor, oh = height * factor;

  // Line widths are in pixels, so they must grow with the image.
  std::vector<std::pair<SoDrawStyle *, float> > widths;
  SoSearchAction search;
  search.setType(SoDrawStyle::getClassTypeId());
  search.setInterest(SoSearchAction::ALL);
  search.apply(root);
  const SoPathList &paths = search.getPaths();
  for (int i = 0; i < paths.getLength(); i++) {
    SoDrawStyle *ds = (SoDrawStyle *)paths[i]->getTail();
    float w0 = ds->lineWidth.getValue();
    widths.push_back(std::make_pair(ds, w0));
    ds->lineWidth.setValue((w0 > 0 ? w0 : 1.0f) * factor * ss);
  }

  bool ok = false;
  SbViewportRegion vp;
  vp.setWindowSize(SbVec2s(rw, rh));
  // Kept alive for the process: the offscreen renderer crashes in Mesa
  // when destroyed.
  static SoOffscreenRenderer *rend = 0;
  if (!rend) rend = new SoOffscreenRenderer(vp);
  rend->setViewportRegion(vp);
  rend->setComponents(SoOffscreenRenderer::RGB);
  rend->setBackgroundColor(SbColor(r, g, b));
  rend->getGLRenderAction()->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
  if (rend->render(root) && rend->getBuffer()) {
    const unsigned char *src = rend->getBuffer();
    unsigned char *px = (unsigned char *)malloc((size_t)ow * oh * 3);
    for (int y = 0; y < oh; y++) {
      for (int x = 0; x < ow; x++) {
        int sum[3] = {0, 0, 0};
        for (int dy = 0; dy < ss; dy++)
          for (int dx = 0; dx < ss; dx++) {
            // GL rows run bottom to top
            const unsigned char *s = src +
              3 * (size_t)((rh - 1 - (y * ss + dy)) * rw + x * ss + dx);
            sum[0] += s[0]; sum[1] += s[1]; sum[2] += s[2];
          }
        for (int k = 0; k < 3; k++) px[3 * ((size_t)y * ow + x) + k] = sum[k] / (ss * ss);
      }
    }
    wxImage img(ow, oh, px);   // takes ownership
    ok = img.SaveFile(wxString::FromUTF8(pngPath.c_str()), wxBITMAP_TYPE_PNG);
    if (!ok) p_msg = "Unable to write " + pngPath;
  } else {
    p_msg = "Unable to render scene graph";
  }
  for (size_t i = 0; i < widths.size(); i++)
    widths[i].first->lineWidth.setValue(widths[i].second);
  return ok;
}


string VizRender::msg()
{
  return p_msg;
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
void VizRender::loadAtomColors(SGContainer *sg)
{
   CSLoadColorsCmd cmd("Colors", sg);

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

      cmd.getParameter("atomIdx")->setInteger(ele);
      cmd.getParameter("red")->setDouble(r);
      cmd.getParameter("green")->setDouble(g);
      cmd.getParameter("blue")->setDouble(b);
      cmd.execute();
   }
}


void VizRender::loadAtomRadii(SGContainer *sg)
{
   double radii, cpkradii;
   TPerTab tpt;

   // Get the command just to save time...
   CSRadiiCmd cmd("radii", sg);

   int numAtoms = tpt.numAtoms();
   for (int ele=0; ele<numAtoms; ele++) {
      cpkradii = tpt.vwr(ele);
      radii = tpt.covalentRadius(ele);

      cmd.getParameter("atomIdx")->setInteger(ele);
      cmd.getParameter("radius")->setDouble(radii);
      cmd.getParameter("cpkradius")->setDouble(cpkradii);
      cmd.execute();
   }
}


void VizRender::loadDisplayStyle(SGViewer *viewer, SGContainer *sg)
{
  ewxConfig *config = ewxConfig::getConfig("wxbuilder.ini");
  wxString buffer;

  // Set a background color and compute a default fg color
  config->Read("Background", &buffer, "#000000");
  ewxColor bcol(buffer);
  float r = bcol.Red()/255.0;
  float g = bcol.Green()/255.0;
  float b = bcol.Blue()/255.0;
  BackgroundCmd backgroundCmd("Background", viewer);
  backgroundCmd.getParameter("red")->setDouble(r);
  backgroundCmd.getParameter("green")->setDouble(g);
  backgroundCmd.getParameter("blue")->setDouble(b);
  backgroundCmd.execute();
  // Automatically compute something that contrasts
  SbColor fg = viewer->calculateContrastingColor();
  ForegroundCmd foregroundCmd("Foreground", sg);
  foregroundCmd.getParameter("red")->setDouble(fg[0]);
  foregroundCmd.getParameter("green")->setDouble(fg[1]);
  foregroundCmd.getParameter("blue")->setDouble(fg[2]);
  foregroundCmd.execute();

  // Restore from styles preferences
  string styleNames[] = {
    "Ball And Wireframe\tALT+1",
    "Ball And Stick\tALT+2",
    "Stick\tALT+3",
    "Wireframe\tALT+4",
    "CPK\tALT+5",
    "Residue Wire\tALT+6",
    "Residue Cylinders\tALT+7",
    "Line Ribbon",
    "Flat Ribbon",
    "Solid Ribbon\tALT+8",
    "Cartoon\tALT+9",
    "Custom Style...\tALT+0"
  };
  // The viewer saves the style's name; an unknown one means the default.
  wxString saved = savedDefaultStyle(config);
  int styleIdx = 1;   // Ball And Stick
  for (int i = 0; i < 12; i++) {
    if (wxStripMenuCodes(styleNames[i]) == saved) {
      styleIdx = i;
      break;
    }
  }
  // _T()/wxT() is only valid on string literals (it token-pastes an "L"
  // prefix in Unicode builds) - it was being misapplied here to a runtime
  // std::string expression, which happened to be silently a no-op in old
  // ANSI-mode wx builds but is a hard preprocessor error under wx3.x, which
  // is Unicode-only. wxStripMenuCodes() takes the std::string directly (via
  // its implicit wxString conversion) instead.
  string style = wxStripMenuCodes(
          styleNames[styleIdx]
          ).ToStdString();
  string scheme = "Element";
  ewxConfig *styleconfig = ewxConfig::getConfig("vizstyles.ini");
  string styledd = styleconfig->Read(style).ToStdString();
  DisplayDescriptor *dd = 0;
  if (styledd != "")
     dd = new DisplayDescriptor(styledd);
  else
     dd = new DisplayDescriptor("default", style, scheme);
  if (!dd->isValid(dd->getStyle(), dd->getColorScheme(),
           sg->getFragment()->numResidues() > 0)) {
     dd->setColorScheme(dd->getDefaultColorScheme(dd->getStyle()));
  }
  CSStyleCmd csStyleCmd("Style", (Receiver*)sg);
  csStyleCmd.getParameter("descriptor")->setString(dd->toString());
  csStyleCmd.getParameter("all")->setBoolean(true);
  csStyleCmd.execute();
  sg->fixAllNubs();
  delete dd;

  // Now override foreground only if set
  if (config->Read("Foreground", &buffer, "#FFFFFF")) {
     ewxColor fcol(buffer);
     ForegroundCmd foregroundCmd2("Foreground", sg);
     foregroundCmd2.getParameter("red")->setDouble(fcol.Red()/255.0);
     foregroundCmd2.getParameter("green")->setDouble(fcol.Green()/255.0);
     foregroundCmd2.getParameter("blue")->setDouble(fcol.Blue()/255.0);
     foregroundCmd2.execute();
  }
}

