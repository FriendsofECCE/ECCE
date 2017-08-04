///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: ShapeData.C
//
//
// CLASS SYNOPSIS:
//   "Legacy" class than contains information about the atom
//   shapes for the small molecule builder.
//
//
// DESCRIPTION:
//   Class contains various sets of chemical data and associated
//   accessor routines
//   i.e. periodic table data, element geometry mapping data
//
///////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "util/ErrMsg.H"
#include "util/Ecce.H"
#include "util/Preferences.H"    // for shapes
#include "tdat/TPerTab.H"
#include "tdat/ShapeData.H"

///////////////////////////////////////////////////////////////////////////////
// class statics
// file statics follow - at least the implmentation is now not public
///////////////////////////////////////////////////////////////////////////////
Preferences *ShapeData::s_shapes = (Preferences*)0;


///////////////////////////////////////////////////////////////////////////////
// Geometry coordinate lists ... vector lists showing directions of nubs
///////////////////////////////////////////////////////////////////////////////
typedef struct _coords{double x,y,z;}coordinates;

static coordinates p_LoneCoords[] = {{0,0,0}};
static coordinates p_TerminalCoords[] = {{1,0,0}};
static coordinates p_LinearCoords[]= {{1,0,0}, {-1,0,0}};
static coordinates p_BentCoords[]=
         {{0.816496581, 0.577350270,0.0}, {-0.816496581, 0.577350270,0.0}};
static coordinates p_TrigonalPlanarCoords[]=
         {{0,1,0}, {0.866025404, -0.5,0}, { -0.866025404, -0.5,0}};
static coordinates p_TrigonalPyramidalCoords[]=
         {{0.942809042,0.0,-0.333333333},
            { -0.471404521,0.816496581, -0.333333333},
            {-0.471404521,-0.816496581, -0.333333333}};
static coordinates p_TetrahedralCoords[]=
    {{-1,-1,1}, {1,1,1}, { 1,-1,-1}, {-1,1, -1} };
static coordinates p_SquarePyramidalCoords[]=
    {{1,-1,-1},{1,-1,1},{-1,-1,-1}, {-1,-1,1}};
static coordinates p_SquarePlanarCoords[]=
    {{1,0,1},{-1,0,1},{-1,0,-1},{1,0,-1}};
static coordinates p_PentagonalCoords[]=
    {{0.0,1,0.0}, {1.0,0.0,0.0}, {-0.5, 0.0, 0.866025404},
       {-0.5, 0.0, -0.866025404}, {0.0, -1.0, 0.0} };
static coordinates p_PentagonalPyramidalCoords[]=
    {{0,1,0},{-1,-0.5,-1},{1, -0.5,-1 },{1, -0.5, 1} ,{-1, -0.5, 1}};
static coordinates p_HexagonalCoords[]=
    {{0.0,1.0,0.0}, {0.0,-1.0,0.0}, {1.0,0.0,1.0}, {1.0,0.0,-1.0},
       {-1.0,0.0,1.0}, {-1.0, 0.0,-1.0}};

static coordinates p_OctahedralCoords[]=
  {{1,1,1},{1,1,-1},{1,-1,-1},{1,-1,1},{-1,1,1},{-1,1,-1},{-1,-1,-1},{-1,-1,1}};


// Not used at the moment 
/*
static coordinates p_decacapCoords[]= {
  {1,1,-1}, {1,-1,-1}, {-1,1,-1}, {-1,-1,-1},
  {1,1,1}, {1,-1,1}, {-1,1,1}, {-1,-1,1},
  {0,0,1}, {0,0,-1}};
*/

static coordinates p_decahedralCoords[]= {
   {0.866025404,-.5,0}, {0.866025404,.5,0}, {0,1,0},
{-0.866025404,0.5,0},
   {-.866025404,-.5,0}, {0,-1,0}, {.5,0,0.866025404}, {-.5,0,.866025404},
   {0,0.5,-.866025404}, {0,-.5,-.866025404} };


///////////////////////////////////////////////////////////////////////////////
// atom geometry list
///////////////////////////////////////////////////////////////////////////////
typedef struct _geometryDataTable {
    const char *geometry;
    int numberAtoms;
    int dihedralOffset;
    double *coords;
} geometryConfig;

// The one labeled Junk below is one Hugh told me to take out since it
// wasn't real.  Since this whole thing revolves around b1,b2... I
// decided it was easiest to unmanage the button.  See BuilderTables.C
// constructor.
static geometryConfig p_geomTable[] =
{
  {"Lone",              0,  0,   (double*)p_LoneCoords },
  {"Terminal",          1,  0,   (double*)p_TerminalCoords },
  {"Linear",            2,  0,   (double*)p_LinearCoords },
  {"Bent",              2,  180, (double*)p_BentCoords },
  {"TrigonalPlanar",    3,  0,   (double*)p_TrigonalPlanarCoords },
  {"Pyramidal",         3,  60,  (double*)p_TrigonalPyramidalCoords },
  {"Tetrahedral",       4,  60,  (double*)p_TetrahedralCoords },
  {"Junk",              4,  45,  (double*)p_SquarePyramidalCoords },
  {"SquarePlanar",      4,  0,   (double*)p_SquarePlanarCoords },
  {"TrigonalBipyramidal", 5,  30,(double*)p_PentagonalCoords},
  {"SquarePyramidal",   5,  30,  (double*)p_PentagonalPyramidalCoords},
  {"Octahedral",        6,  30,  (double*)p_HexagonalCoords},
  {"Cubic",             8,  30,  (double*)p_OctahedralCoords},
  {"Decahedral",        10,  30, (double*)p_decahedralCoords}
};

static const int p_numberGeometries=
                          sizeof(p_geomTable)/sizeof(geometryConfig);


static const char*  s_shapeStrs[] = {
    "Lone",                         // 0      :0.0
    "Terminal",                     // 1      :1.0
    "Linear",                       // 2      :2.0
    "TrigonalPlanar",               // 3      :3.0
    "Tetrahedral",                  // 4      :4.0
    "Pyramidal",                    // 5      :4.1 aka trigonal pyr.
    "Bent",                         // 6      :4.2
    "TrigonalBipyramidal",          // 7      :5.0 Chris calls this pentagonal
    "IrregularFourCoord",           // 8      :5.1 trigBiPyr w/o 1 equat.
    "Tshaped",                      // 9      :5.2 not yet in builder
    "Octahedral",                   // 10     :6.0 Chris calls this hexagonal
    "SquarePyramidal",              // 11     :6.1 Chris calls this pent. pyr
    "SquarePlanar",                 // 12     :6.2
    "PentagonalBipyramidal",        // 13     :7.0 not used, 7-coord, very rare
    "SquareDistortedPlanar",        // 14     :7.3 aka sq pyramid; not chemical
    "Cubic",                        // 15     :8.0 Chris calls this octahedral
    "CappedOctahedronC3v",          // 16     :8.1 not used, 7-coord, very rare
    "TricappedTrigonalPrismD3h",    // 17     :9.0 not used, rare 9-coord
    "HexagonalBipyramidal",         // 18     :9.1 not used, rare 8-coord
    "CappedTrigonalPrism",          // 19     :9.2 not used, 7-coord, very rare
    "Decahedral",                   // 20     :10.0 added for Andy
    "BicappedTrigonalPrismD3h",     // 21     :10.2 not used, rare 8-coord
    "BicappedTrigonalAntiPrismD3d", // 22     :11.3 not used, rare 8-coord
    "Unknown"                       // 23
};


static const char*  s_shapeImage[] = {
    "lone24.xpm",                   // 0      :0.0
    "terminal24.xpm",               // 1      :1.0
    "linear24.xpm",                 // 2      :2.0
    "trigplanar24.xpm",             // 3      :3.0
    "tetrahedral24.xpm",            // 4      :4.0
    "pyramidal24.xpm",              // 5      :4.1 aka trigonal pyr.
    "bent24.xpm",                   // 6      :4.2
    "trigbipyramidal24.xpm",        // 7      :5.0 Chris calls this pentagonal
    "",                             // 8      :5.1 trigBiPyr w/o 1 equat.
    "",                             // 9      :5.2 not yet in builder
    "octahedral24.xpm",             // 10     :6.0 Chris calls this hexagonal
    "squarepyramidal24.xpm",        // 11     :6.1 Chris calls this pent. pyr
    "squareplanar24.xpm",           // 12     :6.2
    "",                             // 13     :7.0 not used, 7-coord, very rare
    "",                             // 14     :7.3 aka sq pyramid; not chemical
    "cubic24.xpm",                  // 15     :8.0 Chris calls this octahedral
    "",                             // 16     :8.1 not used, 7-coord, very rare
    "",                             // 17     :9.0 not used, rare 9-coord
    "",                             // 18     :9.1 not used, rare 8-coord
    "",                             // 19     :9.2 not used, 7-coord, very rare
    "decahedral24.xpm",             // 20     :10.0 added for Andy
    "",                             // 21     :10.2 not used, rare 8-coord
    "",                             // 22     :11.3 not used, rare 8-coord
    ""                              // 23
};


static const int p_numTomsShapes = sizeof(s_shapeStrs)/sizeof(const char*);


// If these aren't fast enough, we can make a hash map out of them.
ShapeData::Shape ShapeData::stringToShape(const string& str)
{
  ShapeData::Shape ret = ShapeData::unknown;
  for (int idx=0; idx<p_numTomsShapes; idx++) {
    if (str == s_shapeStrs[idx]) {
      ret = (ShapeData::Shape)idx;
    }
  }
  return ret;
}

string ShapeData::shapeToString(ShapeData::Shape shape)
{
  EE_ASSERT(shape>=0 && shape<p_numTomsShapes,EE_WARNING,"Bad shape index");
  return s_shapeStrs[shape];
}


string ShapeData::shapeToImage(ShapeData::Shape shape)
{
  EE_ASSERT(shape>=0 && shape<p_numTomsShapes,EE_WARNING,"Bad shape index");
  return s_shapeImage[shape];  
}


string ShapeData::shapeToSmallImage(ShapeData::Shape shape)
{
  EE_ASSERT(shape>=0 && shape<p_numTomsShapes,EE_WARNING,"Bad shape index");
  string ret = s_shapeImage[shape];
  EE_ASSERT(ret.length()>6, EE_FATAL, "Bad shape image");
  return ret.replace(ret.length()-6, 2, "18");
}


///////////////////////////////////////////////////////////////////////////////
// Description
//   Constructor.
//
// Implementation
//   Since the shapes are read in from a Preference file, construction
//   could be slow if that was done each time.  Therefore the 
//   Preference file is loaded only one time.  This was done because I
//   believe that several of the commands "new" this class.
//   data.
//
///////////////////////////////////////////////////////////////////////////////
ShapeData::ShapeData(void)
{
  TPerTab tpt;
  if (s_shapes == (Preferences*)0)
    s_shapes = new Preferences("Shapes", true);

  string defaultShape;
  int nAtoms = tpt.numAtoms();
  p_elementToGeometry = new int[nAtoms+1];
  for (int ele=0; ele<nAtoms; ele++) {
    if (s_shapes->getString(tpt.atomicSymbol(ele),defaultShape)) {
      p_elementToGeometry[ele]
        = getGeometryTableIndex((char*)defaultShape.c_str());
    } else {
      p_elementToGeometry[ele] = getGeometryTableIndex("Terminal");
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// Description
//    Destructor.
///////////////////////////////////////////////////////////////////////////////

ShapeData::~ShapeData()
{
}


///////////////////////////////////////////////////////////////////////////////
// Description
//   Return the name of the shape for the specified atom.
///////////////////////////////////////////////////////////////////////////////

string ShapeData::getGeometryName(int atomNum)
{
  string ret = "";
  TPerTab tpt;
  EE_ASSERT((atomNum>0 || atomNum<=tpt.numAtoms()),
      EE_WARNING, "range error on atom");

  int index=p_elementToGeometry[atomNum];
  return p_geomTable[index].geometry;
};



///////////////////////////////////////////////////////////////////////////////
// Description
//   Return the name of the shape for the specified atom.
///////////////////////////////////////////////////////////////////////////////
string ShapeData::getGeometryName(const char *elementName)
{
  TPerTab tpt;
  return getGeometryName(tpt.atomicNumber(elementName));
}



///////////////////////////////////////////////////////////////////////////////
// Description
//  Get an index into the Geometry Data table, given a particular
//  geometry name.
//
///////////////////////////////////////////////////////////////////////////////
int ShapeData::getGeometryTableIndex(const char *geometryName)
{
  int ret= -1;
  for (int idx=0; idx<p_numberGeometries; idx++) {
    if (strcmp(p_geomTable[idx].geometry, geometryName)==0) {
      ret=idx;
      break;
    }
  }
  EE_ASSERT(ret != -1, EE_FATAL, "geometryName not recognized");
  return ret;
}


///////////////////////////////////////////////////////////////////////////////
// Description
//  Get the dihedral offset associated with a given geometry name.
///////////////////////////////////////////////////////////////////////////////
int ShapeData::getDihedralOffset(const char *geometryName)
{
  int ret = 0;
  int index = getGeometryTableIndex(geometryName);
  if (index != -1) {
    ret=p_geomTable[index].dihedralOffset;
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
// Description
//  Get the number of atoms associated with a given geometry name.
///////////////////////////////////////////////////////////////////////////////
int ShapeData::getNumberOfAtoms(const char *geometryName)
{
  int ret = 0;
  int index = getGeometryTableIndex(geometryName);
  if (index != -1) {
    ret=p_geomTable[index].numberAtoms;
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
//
// ShapeData methods - Get the coordinates info about a geometry
//
///////////////////////////////////////////////////////////////////////////////
const double * ShapeData::getGeometryCoordinates(const char *geometryName)
{
  double *ret  = 0;
  int index = getGeometryTableIndex(geometryName);
  if (index != -1) {
    ret = p_geomTable[index].coords;
  }
  return ret;
}
