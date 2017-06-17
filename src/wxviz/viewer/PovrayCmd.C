/////////////////////////////////////////////////////////////////////////////
// PovrayCmd
//
// Parameters: filename (string)
//
// Default:
//
// Description:
//   Dump out the scene graph in POV ray format.  We get the scene in pieces
//   and have left the following limitations in place waiting for somebody to
//   say that it matters:
//     . we don't dump the axis
//     . we don't support the slice planes
//     . we only support the solid surface
//     . we don't get the atom transparency
//     . we don't dump any text
//     . the lightsource color is hardwired (ok since user can't control)
//     . vector color is hardwired (user can't control)
//     . finishes are hardwired
//
//     . basically anything in ChemDisplayParam is hardwired
//       I have since added access to SGContainer so that we can get the
//       easy things fixed up pretty quick.  Things like double/triple
//       bonds will probably not get done
//       . bond cylinder radius is hardwired
//       . showing of multiple bonds
//       . wire line thickness is hardwired
//       . cylinder cap style is hardwired
//       ...
/////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <iostream>
  using std::cout;
  using std::endl;
#include <fstream>
  using std::ofstream;
  using std::ifstream;

#include "inv/SbColor.H"
#include "inv/actions/SoSearchAction.H"
#include "inv/actions/SoWriteAction.H"
#include "inv/nodes/SoCamera.H"
#include "inv/nodes/SoPerspectiveCamera.H"
#include "inv/nodes/SoOrthographicCamera.H"
#include "inv/nodes/SoCylinder.H"
#include "inv/nodes/SoCone.H"
#include "inv/nodes/SoScale.H"
#include "inv/nodes/SoDirectionalLight.H"
#include "inv/nodes/SoTransform.H"
#include "inv/nodes/SoVertexProperty.H"
#include "inv/nodes/SoIndexedTriangleStripSet.H"
#include "inv/misc/SoChildList.H"

#include "util/Ecce.H"

#include "tdat/TPerTab.H"
#include "tdat/TBond.H"

#include "inv/ChemKit/ChemRadii.H"
#include "inv/ChemKit/ChemColor.H"
#include "inv/ChemKit/ChemIso.H"

#include "viz/SGFragment.H"
#include "viz/SGContainer.H"
#include "viz/VRVector.H"
#include "viz/AtomLine.H"

#include "wxviz/SGViewer.H"
#include "wxviz/PovrayCmd.H"

// These are logical names used in #declare in povray file
string PovrayCmd::p_IsoFinish;
string PovrayCmd::p_surfacePigment;

PovrayCmd::PovrayCmd(const string& name, Receiver *receiver)
       : Command(name, receiver)
{
  init();
}

PovrayCmd::~PovrayCmd()
{
}

void PovrayCmd::init() 
{
   addParameter(new CommandParameter("filename","false"));
   addParameter(new CommandParameter("finishStyle","Plastic"));
   addParameter(new CommandParameter("bondStyle","Bicolor-Proportional"));
   addParameter(new CommandParameter("isosurfaceStyle","Glassy"));
}

bool PovrayCmd::execute() throw(EcceException)
{
   setErrorMessage("");
   SGViewer *viewer = dynamic_cast<SGViewer*>(getReceiver());

   string filename = getParameter("filename")->getString();

   ofstream os(filename.c_str());

   if (os) {

     // Add helpful comments
     addHeaderComments(os);

     // Set up atom symbolic constants
     preScanFragment(os, viewer->getTopNode());

     // Set up vector symbolic constants
     preScanVectors(os, viewer->getTopNode());

     // Set up surface symbolic constants
     preScanSurfaces(os, viewer->getTopNode());
     
     os << endl;
     
     // Get camera data
     addCameraAndLights(os,viewer->getCamera(),viewer->getHeadlight());

     // get background color 
     SbColor background = viewer->getBackgroundColor();
     float red,green,blue;
     background.getValue(red,green,blue);
     os << "background { color rgb <"<<red<<","<<green<<","<<blue<<"> }"<<endl;

     // Add Fragment
     addFragment(os, viewer->getTopNode());

     // Insert vectors
     addVectors(os,viewer->getTopNode());

     // insert surfaces - process one at a time to interject texture
     SoSearchAction getsurfs;
     getsurfs.setInterest(SoSearchAction::ALL);
     getsurfs.setType(ChemIso::getClassTypeId(),FALSE);
     getsurfs.apply(viewer->getTopNode());
     SoPathList &paths = getsurfs.getPaths();

     char buf[32];
     p_IsoFinish = getParameter("isosurfaceStyle")->getString();
     for (int i=0; i<paths.getLength(); i++) {
       // This nasty bit of code deciphered from CmdIsosurface comments/code
       SoPath* path = paths[i];

       sprintf(buf, "SurfPigment%d", i+1);
       p_surfacePigment = buf;
       addSurface(os,path->getTail());
     }

     addHBonds(os, viewer->getTopNode());

   } else {
     setErrorMessage("Unable to open file for write");
   }
   os.close();


   return false;
}

void PovrayCmd::undo()
{
}

void PovrayCmd::addCameraAndLights(ostream& os, SoCamera *camera, 
                                  SoDirectionalLight *light)
{
     float height;
     bool isOrtho;
     // Find out if projection is perspective or orthographic
     float distance;
     if (camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
       os << "camera {" << endl;
       os << "orthographic" << endl;
       height = ((SoOrthographicCamera*)camera)->height.getValue();
       distance = camera->farDistance.getValue();
       isOrtho = true;
     } else {
       os << "camera {" << endl;
       height = ((SoPerspectiveCamera*)camera)->heightAngle.getValue();
       distance = camera->focalDistance.getValue();
       height = distance*tan(height);
       isOrtho = false;
     }
     float ratio = camera->aspectRatio.getValue();
     float width = ratio * height;

     const SbVec3f &loc = camera->position.getValue();
     SbVec3f orient;
     SbVec3f z_axis(0,0,-1);
     camera->orientation.getValue().multVec(z_axis,orient);
     SbVec3f x_axis, y_axis;
     float angle = 180.0*atan(2.0*width/distance)/3.141592654;
     if (isOrtho) {
       y_axis.setValue(0,3.0*width/4.0,0);
       x_axis.setValue(width,0,0);
     } else {
       y_axis.setValue(0,1.0,0);
       x_axis.setValue(1.0,0,0);
     }
     SbVec3f sky, right;
     camera->orientation.getValue().multVec(y_axis,sky);
     camera->orientation.getValue().multVec(x_axis,right);
     float x,y,z;


     // Get light data
     const SbVec3f &lloc = light->direction.getValue();

     float scale = 1.0;
     os << "location <" << loc[0] << "," << loc[1] << "," << -loc[2] << ">" << endl;;
     sky.getValue(x,y,z);
     os << "sky <"<<x<<","<<y<<","<<-z<<">"<<endl;
     os << "angle "<<angle <<endl;
     os << "look_at <"<<loc[0]+distance*orient[0]<<","<<loc[1]+distance*orient[1]<<","
                      <<-loc[2]-distance*orient[2]<<">" << endl; 
     os << "}" << endl;
     os << "light_source { <";
     os << loc[0]-scale*lloc[0] << "," << loc[1]-scale*lloc[1] << ","
        << -loc[2]+scale*lloc[2] << ">";
     os << " color rgb<1,1,1> ";
     os << "}" << endl;
}

/**
 * Finds the FIRST node of the given type starting the search at top.
 * Subclasses will not be included - exact type match required.
 * This is just a convenience funtion to reduce some code bloat.
 */
SoNode *PovrayCmd::findFirst(SoNode *top, SoType type,bool anysubclass)
{
  SoNode *ret = 0;

  SoSearchAction action;
  action.setInterest(SoSearchAction::FIRST);
  action.setType(type,anysubclass);
  action.apply(top);
  SoPath *path = action.getPath();
  if (path) {
    ret = path->getTail();
  }
  return ret;
}

/**
 * Finds the LAST node of the given type starting the search at top.
 * Subclasses will not be included - exact type match required.
 * This is just a convenience funtion to reduce some code bloat.
 */
SoNode *PovrayCmd::findLast(SoNode *top, SoType type, bool anysubclass)
{
  SoNode *ret = 0;

  SoSearchAction action;
  action.setInterest(SoSearchAction::LAST);
  action.setType(type,anysubclass);
  action.apply(top);
  SoPath *path = action.getPath();
  if (path) {
    ret = path->getTail();
  }
  return ret;
}

string makeWood(double red, double green, double blue)
{
  string ret;
  char color[80];
  sprintf(color,"%4.2f %4.2f %4.2f",red,green,blue);
  ret =  "    pigment {\n";
  ret += "              wood\n";
  ret += "              colour_map {\n";
  ret += "                [0.00 0.25   color rgb <";
  ret += color;
  ret += ">*0.52\n";
  ret += "                             color rgb <";
  ret += color;
  ret += ">*0.59]\n";
  ret += "                [0.25 0.40   color rgb <";
  ret += color;
  ret += ">*0.59\n";
  ret += "                             color rgb <";
  ret += color;
  ret += ">*0.61]\n";
  ret += "                [0.40 0.50   color rgb <";
  ret += color;
  ret += ">*0.61\n";
  ret += "                             color rgb <";
  ret += color;
  ret += ">*0.52]\n";
  ret += "                [0.50 0.70   color rgb <";
  ret += color;
  ret += ">*0.52\n";
  ret += "                             color rgb <";
  ret += color;
  ret += ">*0.50]\n";
  ret += "                [0.70 0.98   color rgb <";
  ret += color;
  ret += ">*0.50\n";
  ret += "                             color rgb <";
  ret += color;
  ret += ">*0.52]\n";
  ret += "                [0.98 1.00   color rgb <";
  ret += color;
  ret += ">*0.52\n";
  ret += "                             color rgb <";
  ret += color;
  ret += ">*0.47]\n";
  ret += "              }\n";
  ret += "    turbulence 0.08\n";
  ret += "    omega 0.4\n";
  ret += "    scale <0.1 0.1 1>\n";
  ret += "    rotate -x*4\n";
  ret += "  }";
  return ret;
}
/**
 *  Scan fragment so that we can construct a color table.
 */
void PovrayCmd::preScanFragment(ostream& ostr, SoNode *top)
{
  TPerTab ptb;
  SoNode *node = 0;
  SGFragment *frag = 0;
  ChemRadii *cradii = 0;
  ChemColor *ccolor = 0;
  SGContainer *sg = 0;

  //Initialize p_useLabel
  p_useLabel = true;
  
  // Get finish style and declare it in header
  string finishParam = getParameter("finishStyle")->getString();
  string finish;
  if (finishParam == "Plastic") {
    finish = "#declare SGFinish = finish { phong 1 ambient 0.4 diffuse 0.8 };";
  } else if (finishParam == "Soft") {
    finish = "#declare SGFinish = finish { phong 0.2 phong_size 1 ambient 0.4 diffuse 0.8 };";
  } else if (finishParam == "Wood") {
    finish =  "#declare SGFinish = finish  { phong 0.2 phong_size 1 ambient 0.8 diffuse 0.8 };";
  } else if (finishParam == "Metallic") {
    finish =  "#declare SGFinish = finish { specular 1 ambient 0.1 diffuse 0.1\n";
    finish += "                             metallic reflection {0.75 metallic}};";
  }
  ostr << finish << endl;
  if (finishParam == "Wood") {
    p_useLabel = false;
    return;
  }

  string bondParam = getParameter("bondStyle")->getString();

  // Find the SGContainer or SGContainer object for easy access to
  // ChemDisplayParam objects, radii,...
  node = findFirst(top,SGContainer::getClassTypeId(),true);
  if (node) 
    sg = (SGContainer *)node;
  else {
    node = findFirst(top,SGContainer::getClassTypeId(),true);
    sg = (SGContainer *)(SGContainer*)node;
  }

  // Find the Fragment
  node = findFirst(top,SGFragment::getClassTypeId(),true);
  if (node) frag = (SGFragment *)node;

  cradii = sg->getRadiiNode();

  ccolor = sg->getColorNode();

  vector<bool> atomMask;
  int numElem = ptb.numAtoms();
  int adx;
  for (adx=0; adx<=numElem; adx++) {
    atomMask.push_back(false);
  }
  if (frag && ccolor && cradii) {
    int adx; // atom index
    TAtm *atm;
    int numAtoms = frag->numAtoms();
    const SbColor *colors = ccolor->atomColor.getValues(0);
    SbColor color;
    DisplayStyle style;
    if (ccolor->atomColorBinding.getValue() == ChemColor::ATOM_PER_ATOM) {
      p_useLabel = false;
      return;
    }

    string atName;
    for (adx=0; adx<numAtoms; adx++) {
      atm = frag->atomRef(adx);
      style = atm->displayStyle();
      if (style.isDisplayed()) {
        atomMask[atm->atomicNumber()] = true;
      }
    }
    if (atomMask.size() > 0) {
      for (adx = 0; adx<=numElem; adx++) {
        if (atomMask[adx]) {
          atName = ptb.name(adx);
          color = colors[adx];
          const float *rgb = color.getValue();
          ostr << "#declare   "<<atName<<" = color rgb<"<<rgb[0] << "," << rgb[1]
                          << "," << rgb[2] << ">;" << endl;
        }
      }
      if (bondParam == "Brass") {
        ostr << "#declare   Brass = color rgb < 0.71, 0.65, 0.26>;" <<endl;
      }

    }
    ostr << "#declare   HBond = color rgb < 1.0, 1.0, 1.0>;" <<endl;
  }
}

/**
 * Prescan for vectors and create associated symbolic names.
 */
void PovrayCmd::preScanVectors(ostream& ostr, SoNode *top) 
{
   // Check to see if there are any arrows in the scene
   SoSearchAction getvecs;
   getvecs.setInterest(SoSearchAction::ALL);
   getvecs.setType(VRVector::getClassTypeId(),FALSE);
   getvecs.apply(top);
   SoPathList &paths = getvecs.getPaths();
   vector<float> rgb;
   float RGB[3];

   int idx, jdx;
   for (idx=0; idx <paths.getLength(); idx++) {
     SoPath *path = paths[idx];
     VRVector *vec = (VRVector*)path->getTail();
     vec->getColor(RGB);
     bool storedColor = false;
     int ncolor = p_colors.size();
     // Check to see if colors have already been encountered. This is
     // pretty much a complete hack. If performance is needed, rewrite
     // this using maps.
     for (jdx=0; jdx < ncolor; jdx++) {
       if (RGB[0] == p_colors[jdx][0] && RGB[1] == p_colors[jdx][1] &&
           RGB[2] == p_colors[jdx][2]) {
         storedColor = true;
       }
     }
     if (!storedColor) {
       rgb.clear();
       rgb.push_back(RGB[0]);
       rgb.push_back(RGB[1]);
       rgb.push_back(RGB[2]);
       p_colors.push_back(rgb);
     }
   }
   if (paths.getLength()>0) {
     // ostr << "#declare   Arrow = color rgb < 0.8, 0.8, 0.0>;" <<endl;
     for (idx=0; idx < p_colors.size(); idx++) {
       ostr << "#declare Arrow_"<<idx+1<< " = color rgb < "<<p_colors[idx][0]
       <<", "<<p_colors[idx][1]<<", "<<p_colors[idx][2]<<">;"<<endl;
     }
   }
   ostr << endl;
}

/**
 * Prescan for vectors and create associated symbolic names.
 */
void PovrayCmd::preScanSurfaces(ostream& ostr, SoNode *top) 
{
     SoSearchAction getsurfs;
     getsurfs.setInterest(SoSearchAction::ALL);
     getsurfs.setType(ChemIso::getClassTypeId(),FALSE);
     getsurfs.apply(top);
     SoPathList &paths = getsurfs.getPaths();
  
     // Declare some finishes
     ostr << "#declare Glassy  = finish {ambient 0.8 diffuse 0.7 "
                    "reflection 0.20 specular 0.5};" << endl;
     ostr << "#declare Soft = finish { ambient 0.8 diffuse 1.0 "
                    "reflection 0.20 specular 0.0};" << endl;

     char buf[32];
     for (int i=0; i<paths.getLength(); i++) {
       // This nasty bit of code deciphered from CmdIsosurface comments/code
       SoPath* path = paths[i];
       ChemIso *iso = (ChemIso*)path->getTail();
       SoChildList *childlist = iso->getChildren();
       SoIndexedTriangleStripSet *triStrip
         = (SoIndexedTriangleStripSet*)(*childlist)[0];
       const SoVertexProperty *vp =
         (SoVertexProperty *)triStrip->vertexProperty.getValue();
       uint32_t packedcolor = vp->orderedRGBA[0];
       SbColor anycolor(0,0,0);
       sprintf(buf, "SurfPigment%d", i+1);
       p_surfacePigment = buf;
       SbColor vcolor = anycolor.setPackedValue(packedcolor,p_transparency);
       ostr << "#declare " << p_surfacePigment << " = color rgbt<"
            << vcolor[0] << "," << vcolor[1] << "," << vcolor[2]
            << "," << p_transparency << ">;\n";
     }
}


/**
 * Dump out atoms and bonds using the radii and colors in the scene graph
 * We currently only support RADII_PER_ATOM_INDEXED as far as I know.
 * For colors we use ATOM_PER_ATOM and ATOM_PER_ATOM_INDEXED
 */
void PovrayCmd::addFragment(ostream& ostr, SoNode *top)
{
  TPerTab tpt;
  SoNode *node = 0;
  SGFragment *frag = 0;
  ChemRadii *cradii = 0;
  ChemRadii *cpkradii = 0;
  ChemColor *ccolor = 0;
  SGContainer *sg = 0;

  string finishParam = getParameter("finishStyle")->getString();

  string bondParam = getParameter("bondStyle")->getString();

  // Find the SGContainer or SGContainer object for easy access to
  // ChemDisplayParam objects, radii,...
  node = findFirst(top,SGContainer::getClassTypeId(),true);
  if (node) 
    sg = (SGContainer *)node;
  else {
    node = findFirst(top,SGContainer::getClassTypeId(),true);
    sg = (SGContainer *)(SGContainer*)node;
  }

  // Find the Fragment
  node = findFirst(top,SGFragment::getClassTypeId(),true);
  if (node) frag = (SGFragment *)node;

  cradii = sg->getRadiiNode();
  cpkradii = sg->getCPKRadiiNode();

  ccolor = sg->getColorNode();

  if (frag && ccolor && cradii) {
    int adx; // atom index
    TAtm *atm;
    int atomicNum;
    float radius;
    int numAtoms = frag->numAtoms();
    const SbColor *colors = ccolor->atomColor.getValues(0);
    SbColor color;
    bool colorByAtom = false;
    DisplayStyle style;
    if (ccolor->atomColorBinding.getValue() == ChemColor::ATOM_PER_ATOM)
      colorByAtom = true;
    for (adx=0; adx<numAtoms; adx++) {
      atm = frag->atomRef(adx);
      atomicNum = atm->atomicNumber();
      style = atm->displayStyle();
      if (style.isDisplayed()) {
        if (style.getStyle() == DisplayStyle::CPK) {
          // Fix me - replace 1.0 from ChemDisplayParam
          radius = 1.0*cpkradii->atomRadii[atomicNum];
        } else if (style.getStyle() == DisplayStyle::WIRE) {
          radius = 0.03;
          if (atm->connectedAtoms()->size() == 0) {
            radius = 0.15;
          }
        } else if (style.getStyle() == DisplayStyle::STICK) {
          radius = 0.15;
        } else {
          // Fix me - replace 0.5 from ChemDisplayParam
          // ostr << 0.5 *cradii->atomRadii[atm->atomicNumber()] << endl;
          radius = 0.5*cradii->atomRadii[atomicNum];
        }
        const double *coords = atm->coordinates();
        if (colorByAtom)
          color = colors[adx];
        else
          color = colors[atm->atomicNumber()];
        const float *rgb = color.getValue();
        ostr << "sphere {" << endl;
        ostr << "< " << coords[0] << "," << coords[1] << "," << -coords[2] << ">, ";
        ostr << radius << endl;
        ostr << "  texture {" << endl;
        ostr << "    SGFinish" <<endl;
        if (finishParam == "Wood") {
          ostr << makeWood(rgb[0],rgb[1],rgb[2])<< endl;
        } else {
          if (p_useLabel) {
            ostr << "    pigment { color "<<tpt.name(atm->atomicNumber())
                      <<" }" << endl;
          } else {
            ostr << "    pigment { color rgb <" << rgb[0] << "," << rgb[1]
                      << "," << rgb[2] << "> }" << endl;
          }
        }
        ostr << "  }" << endl;
        ostr << "}" << endl;
      }
    }
    // get bonds
    vector<TBond*>* bonds = frag->bonds();
    int nbonds = bonds->size();
    TBond *bond;
    TAtm *at1, *at2;
    const double *coords1, *coords2;
    double rad1, rad2, scale;
    double bondrad;
    double mid[3];
    double red,green,blue;
    for (adx=0; adx<nbonds; adx++) {
      bond = (*bonds)[adx];
      style = bond->displayStyle();
      if (style.isDisplayed() && style.getStyle() != DisplayStyle::CPK) {
        at1 = bond->atom1();
        at2 = bond->atom2();
        coords1 = at1->coordinates();
        coords2 = at2->coordinates();
        if (bondParam == "Brass") {
          scale = 1.0;
        } else if (bondParam == "Bicolor-Symmetric") {
          scale = 0.5;
        } else {
          rad1 = cradii->atomRadii[at1->atomicNumber()];
          rad2 = cradii->atomRadii[at2->atomicNumber()];
          //rad1 = tpt.covalentRadius(at1->atomicNumber());
          //rad2 = tpt.covalentRadius(at2->atomicNumber());
          scale = rad1/(rad1+rad2);
        }
        mid[0] = coords1[0]+ scale*(coords2[0]-coords1[0]);
        mid[1] = coords1[1]+ scale*(coords2[1]-coords1[1]);
        mid[2] = coords1[2]+ scale*(coords2[2]-coords1[2]);
        if (style.getStyle() == DisplayStyle::BALLWIRE || 
            style.getStyle() == DisplayStyle::WIRE) {
          bondrad = 0.03;
        } else {
          bondrad = 0.15;
        }
        // print out first cylinder
        const float *rgb;
        // get atom colors
        if (colorByAtom)
          color = colors[at1->index()];
        else
          color = colors[at1->atomicNumber()];
        rgb = color.getValue();
        ostr << "cylinder {"<<endl;
        ostr << "  <"<<coords1[0]<<","<<coords1[1]<<","<<-coords1[2]<<">,"<<endl;
        ostr << "  <"<<mid[0]<<","<<mid[1]<<","<<-mid[2]<<">,"<<endl;
        ostr << "  "<<bondrad<<endl;
        ostr << "  texture {" << endl;
        ostr << "    SGFinish" <<endl;
        if (bondParam == "Brass") {
          red = 0.71;
          green = 0.65;
          blue = 0.26;
          if (finishParam == "Wood") {
            ostr << makeWood(red,green,blue)<< endl;
          } else {
            ostr << "    pigment { color Brass }" <<endl;
          }
        } else {
          if (finishParam == "Wood") {
            ostr << makeWood(rgb[0],rgb[1],rgb[2])<< endl;
          } else {
            if (p_useLabel) {
              ostr << "    pigment { color "<<tpt.name(at1->atomicNumber())
                        <<" }" << endl;
            } else {
              ostr << "    pigment { color rgb <" << rgb[0] << "," << rgb[1]
                        << "," << rgb[2] << "> }" << endl;
            }
          }
        }
        ostr << "  }" << endl;
        ostr << "}" << endl;
        if (bondParam != "Brass") {
          // print out second cylinder
          if (colorByAtom)
            color = colors[at2->index()];
          else
            color = colors[at2->atomicNumber()];
          rgb = color.getValue();
          ostr << "cylinder {"<<endl;
          ostr << "  <"<<mid[0]<<","<<mid[1]<<","<<-mid[2]<<">,"<<endl;
          ostr << "  <"<<coords2[0]<<","<<coords2[1]<<","<<-coords2[2]<<">,"<<endl;
          ostr << "  "<<bondrad<<endl;
          ostr << "  texture {" << endl;
          ostr << "    SGFinish" <<endl;
          if (finishParam == "Wood") {
            ostr << makeWood(rgb[0],rgb[1],rgb[2])<< endl;
          } else {
            if (p_useLabel) {
              ostr << "    pigment { color "<<tpt.name(at2->atomicNumber())
                        <<" }" << endl;
            } else {
              ostr << "    pigment { color rgb <" << rgb[0] << "," << rgb[1]
                        << "," << rgb[2] << "> }" << endl;
            }
          }
          ostr << "  }" << endl;
          ostr << "}" << endl;
        }
      }
    }
    delete bonds;
  }
}


/**
 * Find and dump all Hydrogen bonds in the scene graph.
 * Hydrogen bonds currently implemented as AtomLine objects.
 * This same object is used for drawing arbitrary lines and
 * this method will get them all.
 */
void PovrayCmd::addHBonds(ostream& ostr, SoNode *top)
{
  SoSearchAction getvecs;
  getvecs.setInterest(SoSearchAction::ALL);
  getvecs.setType(AtomLine::getClassTypeId(),FALSE);
  getvecs.apply(top);
  SoPathList &paths = getvecs.getPaths();
  double r, radius, dx, dy, dz;
  int nr, ir;
  double xmin, ymin, zmin, xmax, ymax, zmax;
  float white[3];
  string finishParam = getParameter("finishStyle")->getString();

  for (int idx=0; idx <paths.getLength(); idx++) {
    SoPath *path = paths[idx];
    AtomLine *hbond = (AtomLine*)path->getTail();
    TAtm *atm1 = hbond->atom1();
    TAtm *atm2 = hbond->atom2();
    const double *coords1 = atm1->coordinates();
    const double *coords2 = atm2->coordinates();
    dx = coords2[0] - coords1[0];
    dy = coords2[1] - coords1[1];
    dz = coords2[2] - coords1[2];
    r = sqrt(dx*dx + dy*dy + dz*dz);
    dx = dx/r;
    dy = dy/r;
    dz = dz/r;
    nr = (int)(r/0.15);
    radius = 0.03;
    white[0] = 1.0;
    white[1] = 1.0;
    white[2] = 1.0;
    for (ir = 0; ir<nr; ir++) {
      xmin = coords1[0] + 0.2*((double)ir)*dx;
      ymin = coords1[1] + 0.2*((double)ir)*dy;
      zmin = coords1[2] + 0.2*((double)ir)*dz;
      if (((double)(ir+1))*0.2-0.1 < r) {
        xmax = coords1[0] + (0.2*((double)ir)+0.1)*dx;
        ymax = coords1[1] + (0.2*((double)ir)+0.1)*dy;
        zmax = coords1[2] + (0.2*((double)ir)+0.1)*dz;
      } else {
        xmax = coords2[0];
        ymax = coords2[1];
        zmax = coords2[2];
      }
      // write out spherical caps at end of each segment
      ostr << "sphere {"<<endl;
      ostr << "< " << xmin << "," << ymin << "," << -zmin << ">, ";
      ostr << radius << endl;
      ostr << "  texture {" << endl;
      ostr << "    SGFinish" << endl;
      if (finishParam == "Wood") {
        ostr << makeWood(white[0], white[1], white[2]) << endl;
      } else {
        ostr << "    pigment { color HBond } " << endl;
      }
      ostr << "  }" <<endl;
      ostr << "}" <<endl;
      ostr << "sphere {"<<endl;
      ostr << "< " << xmax << "," << ymax << "," << -zmax << ">, ";
      ostr << radius << endl;
      ostr << "  texture {" << endl;
      ostr << "    SGFinish" << endl;
      if (finishParam == "Wood") {
        ostr << makeWood(white[0], white[1], white[2]) << endl;
      } else {
        ostr << "    pigment { color HBond } " << endl;
      }
      ostr << "  }" <<endl;
      ostr << "}" <<endl;
      //write out cylinder composing each segment
      ostr << "cylinder {"<<endl;
      ostr << "  <"<< xmin <<","<< ymin <<","<< -zmin <<">,"<<endl;
      ostr << "  <"<< xmax <<","<< ymax <<","<< -zmax <<">,"<<endl;
      ostr << "  "<<radius<<endl;
      ostr << "  texture {" << endl;
      ostr << "    SGFinish" << endl;
      if (finishParam == "Wood") {
        ostr << makeWood(white[0], white[1], white[2]) << endl;
      } else {
        ostr << "    pigment { color HBond } " << endl;
      }
      ostr << "  }" <<endl;
      ostr << "}" <<endl;
    }
  }
}

void PovrayCmd::addVectors(ostream& ostr, SoNode *top)
{
  SoSearchAction getvecs;
  getvecs.setInterest(SoSearchAction::ALL);
  getvecs.setType(VRVector::getClassTypeId(),FALSE);
  getvecs.apply(top);
  SoPathList &paths = getvecs.getPaths();

  string finishParam = getParameter("finishStyle")->getString();

  double red, green, blue;

  for (int idx=0; idx <paths.getLength(); idx++) {
    SoNode *node = 0;
    SoPath *path = paths[idx];
    VRVector *vec = (VRVector*)path->getTail();
    SoTransform *transf = vec->getTransform();
    SoScale *scale = vec->getScale();
    SbMatrix  Mtrans, iMtrans;
    SbVec3f t, s;
    SoSFVec3f st;
    SbRotation r, so;
    float xv,yv,zv;
    float xt,yt,zt;
    float x1s,y1s,z1s;
    float rgb[3];
    /* Get vector color */
    vec->getColor(rgb);
    red = rgb[0];
    green = rgb[1];
    blue = rgb[2];
    /* Find appropriate color index */
    int icolor = 0;
    int ncolor = p_colors.size();
    for (int jdx = 0; jdx < ncolor; jdx++) {
      if (rgb[0] == p_colors[jdx][0] && rgb[1] == p_colors[jdx][1] &&
          rgb[2] == p_colors[jdx][2]) {
        icolor = jdx;
      }
    }

    /* Get orientation of vector */
    transf->getRotationSpaceMatrix(Mtrans,iMtrans);
    Mtrans.getTransform(t,r,s,so);
    s.setValue(0.,1.,0.);
    r.multVec(s,t);
    t.getValue(xv,yv,zv);

    /* Get translation vector */
    transf->getTranslationSpaceMatrix(Mtrans,iMtrans);
    Mtrans.getTransform(t,r,s,so);
    t.getValue(xt,yt,zt);

    /* Get scale vector */
    transf->getScaleSpaceMatrix(Mtrans,iMtrans);
    Mtrans.getTransform(t,r,s,so);
    s.getValue(x1s,y1s,z1s);
    float sc1 = (x1s+y1s+z1s)/3.0;

    /* Get scale transformation for vector */
    st = scale->scaleFactor;
    s = st.getValue();
    float x2s,y2s,z2s;
    s.getValue(x2s,y2s,z2s);
    float sc = sc1*(x2s+y2s+z2s)/3.0;
    // Find out if vector is zero length. If so, don't include it
    // in output.
    float test = transf->scaleFactor.getValue().length();
    if (test == 0.0) continue;

    SoCylinder *cylinder;
    node = findFirst(top,SoCylinder::getClassTypeId());
    if (node) cylinder = (SoCylinder *)node;
    SoSFFloat value;
    value = cylinder->height;
    float vlen = value.getValue();
    value = cylinder->radius;
    float vrad = value.getValue();

    SoCone *cone;
    node = findFirst(top,SoCone::getClassTypeId());
    if (node) cone = (SoCone *)node;
    value = cone->height;
    float clen = value.getValue();
    value = cone->bottomRadius;
    float crad = value.getValue();

    // Parameters from cone have been obtained. Now need to construct
    // Pov-ray input.
    
    float xh,yh,zh,xc,yc,zc;

    xh = xt+xv*sc*vlen;
    yh = yt+yv*sc*vlen;
    zh = zt+zv*sc*vlen;

    xc = xh+xv*sc*clen;
    yc = yh+yv*sc*clen;
    zc = zh+zv*sc*clen;

    // print out parameters for cylinder
    ostr << "cylinder {"<<endl;
    ostr << "  <"<<xt<<","<<yt<<","<<-zt<<">,"<<endl;
    ostr << "  <"<<xh<<","<<yh<<","<<-zh<<">,"<<endl;
    ostr << "  "<<sc*vrad<<endl;
    ostr << "  texture {";
    ostr << "    SGFinish" << endl;
    if (finishParam == "Wood") {
      ostr << makeWood(red,green,blue)<< endl;
    } else {
      if (p_useLabel) {
        ostr << "    pigment { color rgb Arrow_"<<icolor+1<<" }" << endl;
      } else {
        ostr << "    pigment { color rgb <" << red << "," << green
             << "," << blue << "> }" << endl;
      }
    }
    ostr << "  }"<<endl;
    ostr << "}"<<endl;
 
    // print out parameters for cone
    ostr << "cone {"<<endl;
    ostr << "  <"<<xh<<","<<yh<<","<<-zh<<">, "<<sc*crad<<endl;
    ostr << "  <"<<xc<<","<<yc<<","<<-zc<<">, 0.0"<<endl;
    ostr << "  texture {"<<endl;
    ostr << "    SGFinish" << endl;
    if (finishParam == "Wood") {
      ostr << makeWood(red,green,blue)<< endl;
    } else {
      if (p_useLabel) {
        ostr << "    pigment { color rgb Arrow_"<<icolor+1<<" }" << endl;
      } else {
        ostr << "    pigment { color rgb <" << red << "," << green
             << "," << blue << "> }" << endl;
      }
    }
    ostr << "  }"<<endl;
    ostr << "}"<<endl;
  }
}

void PovrayCmd::addSurface(ostream& ostr, SoNode *top)
{
   SoCallbackAction myaction;
   myaction.addPreCallback(SoIndexedTriangleStripSet::getClassTypeId(),
       PovrayCmd::solidSurfacePreCB,&ostr);
   myaction.addPostCallback(SoIndexedTriangleStripSet::getClassTypeId(),
       PovrayCmd::solidSurfacePostCB,&ostr);
   myaction.addTriangleCallback(SoIndexedTriangleStripSet::getClassTypeId(),printTriangleCB,&ostr);
   myaction.apply(top);
}

SoCallbackAction::Response
PovrayCmd::solidSurfacePreCB(void *stream, SoCallbackAction *, const SoNode *node)
{
  SoCallbackAction::Response ret = SoCallbackAction::ABORT;
  SoIndexedTriangleStripSet *tri = (SoIndexedTriangleStripSet*)node;
  // The 1 here should really be 0 in my mind but I found that if there were
  // no triangles, I'd get a value of 1 and end up exporting the mesh
  // with texture but no triangles.  povray didn't like this.
  if (tri->coordIndex.getNum() > 1) {
    ostream& ostr = (ostream&)*(ostream*)stream;
    ostr << "mesh {" << endl;

    ret =  SoCallbackAction::CONTINUE;
  }
  return ret;

}

/**
 * Output the color and finish of the surface.
 * We found that using rgbf, the transparency seemed too solid so went
 * with rgbt.
 **/
SoCallbackAction::Response
PovrayCmd::solidSurfacePostCB(void *stream, SoCallbackAction *, const
    SoNode *node)
{
  SoCallbackAction::Response ret = SoCallbackAction::ABORT;
  SoIndexedTriangleStripSet *tri = (SoIndexedTriangleStripSet*)node;

  ostream& ostr = (ostream&)*(ostream*)stream;

  // The 1 here should really be 0 in my mind but I found that if there were
  // no triangles, I'd get a value of 1 and end up exporting the mesh
  // with texture but no triangles.  povray didn't like this.
  if (tri->coordIndex.getNum() > 1) {
    ostr << "  texture { \n";
    ostr << "    pigment {" << p_surfacePigment << "}" << endl;
    ostr <<  "    finish { " << p_IsoFinish << "}" <<  endl;
    ostr << "  }" << endl;
    ostr << "}" << endl;  // end  of mesh
    ret =  SoCallbackAction::CONTINUE;
  }
  return ret;
}

void PovrayCmd::printTriangleCB(void *stream, SoCallbackAction *,
          const SoPrimitiveVertex *v1,
          const SoPrimitiveVertex *v2,
          const SoPrimitiveVertex *v3)
{
  ostream& ostr = (ostream&)*(ostream*)stream;
  ostr << "  smooth_triangle {\n";
  PovrayCmd::printVertex(ostr,v1);
  ostr << ",";
  PovrayCmd::printVertex(ostr,v2);
  ostr << ",";
  PovrayCmd::printVertex(ostr,v3);
  ostr << "  }" << endl;
}

/**
 * Print a single point using syntax <x,y,z>
 */
void PovrayCmd::printVertex(ostream& ostr, const SoPrimitiveVertex *v)
{
   const SbVec3f &point = v->getPoint();
   const SbVec3f &normal = v->getNormal();
   ostr << "<" << point[0] << "," << point[1] << "," << -point[2] << ">,";
   ostr << "<" << normal[0] << "," << normal[1] << "," << -normal[2] << ">";
}


/**
 * Load some helpful information into the header of the output so that
 * the user can more quickly use povray and customize the output.
 **/
void PovrayCmd::addHeaderComments(ostream& os)
{
  string path = Ecce::ecceDataPrefPath();
  path.append("/povrayhelp");
  ifstream helpfile(path.c_str());
  if (helpfile) {
    char buf[256];
    while (helpfile) {
      helpfile.getline(buf,255);
      os << buf << endl;
    }
    helpfile.close();
  }
}
