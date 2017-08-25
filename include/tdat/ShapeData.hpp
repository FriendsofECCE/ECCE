///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: ShapeData.H
//
//
//
// CLASS SYNOPSIS:
//  Encapsulates shape data needed for building with nubs.
//
//
// DESCRIPTION:
//   This is based on Chris Parkinson's original ChemicalData class but
//   all gui specific data has been removed.  It is now only a container
//   for shape data.
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef SHAPEDATA_HH
#define SHAPEDATA_HH

#include <string>
using std::string;

class Preferences;


class ShapeData
{

public:

  // This is Tom's original definition of shapes.  The numbers at the
  // right represent #bondsites and the #electronpairs somehow.  A subset
  // of these shapes are used by the builder.  The enumeration is
  // provided with translations to between the enumeration and strings.
  // and that is about the sum of its value at this point.

  enum Shape {                        //        :(Bonds+Lonepairs).Lonepairs
    lone,                         // 0      :0.0
    terminal,                     // 1      :1.0
    linear,                       // 2      :2.0
    trigonalPlanar,               // 3      :3.0
    tetrahedral,                  // 4      :4.0
    pyramidal,                    // 5      :4.1 aka trigonal pyr.
    bent,                         // 6      :4.2
    trigonalBipyramidal,          // 7      :5.0 Chris calls this pentagonal
    irregularFourCoord,           // 8      :5.1 trigBiPyr w/o 1 equat.
    Tshaped,                      // 9      :5.2 not yet in builder
    octahedral,                   // 10     :6.0 Chris calls this hexagonal
    squarePyramidal,              // 11     :6.1 Chris calls this pent. pyr
    squarePlanar,                 // 12     :6.2
    pentagonalBipyramidal,        // 13     :7.0 not used, 7-coord, very rare
    squareDistortedPlanar,        // 14     :7.3 aka sq pyramid; not chemical
    cubic,                        // 15     :8.0 Chris calls this octahedral
    cappedOctahedronC3v,          // 16     :8.1 not used, 7-coord, very rare
    tricappedTrigonalPrismD3h,    // 17     :9.0 not used, rare 9-coord
    hexagonalBipyramidal,         // 18     :9.1 not used, rare 8-coord
    cappedTrigonalPrism,          // 19     :9.2 not used, 7-coord, very rare
    decahedral,                   // 20     :10.0 added for Andy 
    bicappedTrigonalPrismD3h,     // 21     :10.2 not used, rare 8-coord
    bicappedTrigonalAntiPrismD3d, // 22     :11.3 not used, rare 8-coord
    unknown                       // 23
  };

  ShapeData(void);
  ~ShapeData(void);

  // Member Objects - Geometry Stuff
  string getGeometryName(int atomicNumber);
  string getGeometryName(const char *elementName);

  int getGeometryTableIndex(const char *geometryName);
  int getDihedralOffset(const char *geometryName);
  int getNumberOfAtoms(const char *geometryName);
  const double* getGeometryCoordinates(const char *geometryName);

  static ShapeData::Shape stringToShape(const string& str);
  static string shapeToString(ShapeData::Shape shape);
  static string shapeToImage(ShapeData::Shape shape);
  static string shapeToSmallImage(ShapeData::Shape shape);

private:
    int *p_elementToGeometry;
    static Preferences *s_shapes;

};

#endif
