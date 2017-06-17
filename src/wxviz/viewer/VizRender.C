/**
 * @file
 *
 */
#include "inv/SoOffscreenRenderer.H"
#include "inv/nodes/SoGroup.H"

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

#include "util/ImageConverter.H"
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

SbViewportRegion    * VizRender::p_viewport  = NULL;
SoOffscreenRenderer * VizRender::p_renderer  = NULL;
string                VizRender::p_msg       = "";
int                   VizRender::p_oldWidth  = 0;
int                   VizRender::p_oldHeight = 0; 

bool VizRender::thumbnail(const string& urlstr, int width, int height,
                          double r, double g, double b)
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

    if (!VizRender::thumbnail(viewer->getTopNode(), task, width, height, r,g,b))
    {
      //cout <<  p_msg << endl;
      throw EcceException(p_msg, __FILE__, __LINE__);
    }

  } catch (string& error) {
    //cout << error << endl;
    p_msg += "VizRender::thumbnail(url) error:\n" + error;
    ret = false;
  }
  return ret;
}


bool VizRender::thumbnail(SoNode *root, ChemistryTask *task,
                          int width, int height,
                          double r, double g, double b)
{
  p_msg = "";
  bool ret = true;

  // Check whether site has disabled creation of thumbnails
  if (getenv("ECCE_NO_VIZIMAGES")) {
    return ret;
  }

  // declared here in case of exception thrown
  SFile *rgbFile;
  SFile *jpegFile;

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
  } catch (string& error) {
    p_msg += "VizRender::thumbnail(SoNode) error:\n" + error;
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
  FILE * myFile;

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
      myFile = fopen(file->path().c_str(), "w");
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
  int styleId;
  config->Read("DefaultStyle", &styleId,
               ViewerEvtHandler::ID_STYLE_BALL_WIREFRAME);
  string style = wxStripMenuCodes(
          _T(styleNames[styleId-ViewerEvtHandler::ID_STYLE_BALL_WIREFRAME])
          ).c_str();
  string scheme = "Element";
  ewxConfig *styleconfig = ewxConfig::getConfig("vizstyles.ini");
  string styledd = styleconfig->Read(style).c_str();
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

