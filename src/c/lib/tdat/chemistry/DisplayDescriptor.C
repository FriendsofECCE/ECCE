#include <iostream>
   using std::ios;
   using std::cout;
   using std::cerr;
   using std::endl;
   using std::ends;
#include <strstream>
   using std::istrstream;
   using std::ostrstream;

#include <stdlib.h>

#include "util/StringTokenizer.H"
#include "util/Preferences.H"

#include "tdat/DisplayStyle.H"
#include "tdat/DisplayDescriptor.H"

//Default values
const string DisplayDescriptor::DISPLAY = "Ball And Wireframe";
const string DisplayDescriptor::COLORSCHEME = "Element";
const int DisplayDescriptor::LINEWDTH = 1;
const bool DisplayDescriptor::ANTIALIAS = false;
const int DisplayDescriptor::CYLRQ = 56;
const int DisplayDescriptor::CYLRAD = 14;
const float DisplayDescriptor::RESCYLRAD = 1.0;
const int DisplayDescriptor::CAPSTYLE = 2;
const int DisplayDescriptor::SPHERESZ = 50;
const int DisplayDescriptor::SPHERERQ = 50;
const int DisplayDescriptor::SPHERESTYLE = 1;
const bool DisplayDescriptor::BLENDED = true;
const bool DisplayDescriptor::ROUNDED = true;
const float DisplayDescriptor::COILWDTH = 1.0;
const float DisplayDescriptor::HLXWDTH = 3.0;
const float DisplayDescriptor::SHTWDTH = 3.0;
const float DisplayDescriptor::RBTHICK = 0.25;
const float DisplayDescriptor::HLXRAD = 3.0;
const float DisplayDescriptor::CRTNCYL = 2.3;
const float DisplayDescriptor::CRTNTB = 0.7;
const int DisplayDescriptor::CRTNCYLRQ = 70;
const int DisplayDescriptor::CRTNAWTS = 5;
const int DisplayDescriptor::CRTNTBTS = 5;
const int DisplayDescriptor::CRTNTBSS = 8;
//const string DisplayDescriptor::ATOMCOLOR = "Element";
//const string DisplayDescriptor::RESCOLOR = "Secondary Structure";
const char *DisplayDescriptor::THREECLR[] = {"#ff0000", "#ffffff", "#0000ff" };
const int DisplayDescriptor::ATMSPHR = 0;
const int DisplayDescriptor::BNDCYL = 11; // SOCYLINDER + ROUND
const string DisplayDescriptor::SPECTRUMTYPE = "Rainbow";
const string DisplayDescriptor::BLOCKCOLOR = "blue";


/**
 * Constructor
 */
DisplayDescriptor::DisplayDescriptor(const string& name,
      const string& style, const string& scheme)
{
   constructor();
   p_name = name;
   p_style = style;
   p_colorScheme = scheme;
}

/**
 * Constructor
 */
DisplayDescriptor::DisplayDescriptor( )
{
   constructor();
}


/**
 * Constructor.
 * Initializes object from serialized data stream.
 */
DisplayDescriptor::DisplayDescriptor(const string& data )
{
   constructor();
   *this = data;
}

DisplayDescriptor::DisplayDescriptor(const DisplayDescriptor& rhs )
{
   constructor();
   *this = rhs;
}

/**
 * Destructor
 */
DisplayDescriptor::~DisplayDescriptor()
{
}

void DisplayDescriptor::constructor()
{
   p_name = "default";
   p_style = DISPLAY;
   p_colorScheme = COLORSCHEME;
   p_spectrumType = SPECTRUMTYPE;
   p_blockColor = BLOCKCOLOR;

   p_lineWidth = LINEWDTH;
   //p_antiAlias = ANTIALIAS;

   p_cylinderRQ = CYLRQ;
   p_cylRadius = CYLRAD;
   p_cylResRadius = RESCYLRAD;
   p_capStyle = CAPSTYLE;
   p_bondCylinder = BNDCYL;

   p_sphereSize = SPHERESZ;
   p_sphereRQ = SPHERERQ;
   p_sphereStyle = SPHERESTYLE;
   p_atomSphere = ATMSPHR;

   p_ribbonBlend = BLENDED;
   p_ribbonRound = ROUNDED;
   p_coilWidth = COILWDTH;
   p_helixWidth = HLXWDTH;
   p_sheetWidth = SHTWDTH;
   p_ribbonThickness = RBTHICK;
   p_ribbonHelixRad = HLXRAD;

   p_cartoonCylRadius = CRTNCYL;
   p_cartoonCylRQ = CRTNCYLRQ;
   p_cartoonTubeRadius = CRTNTB;

   //p_atomColor = ATOMCOLOR;
   //p_residueColor = RESCOLOR;
   for (int idx=0; idx<3; idx++) {
      p_3ColorStyle.push_back(THREECLR[idx]);
   }
   p_spectrumType = SPECTRUMTYPE;

}

DisplayDescriptor& DisplayDescriptor::operator=(const DisplayDescriptor& rhs)
{
   if (this != &rhs) {
      setName(rhs.getName());
      setStyle(rhs.getStyle());
      setColorScheme(rhs.getColorScheme());
      setBlockColor(rhs.getBlockColor());
      setSpectrumType(rhs.getSpectrumType());

      setLineWidth(rhs.getLineWidth());
      setAntiAlias(rhs.getAntiAlias());

      setCylinderRadius(rhs.getCylinderRadius());
      setCylinderRQ(rhs.getCylinderRQ());
      setCapStyle(rhs.getCapStyle());

      setSphereSize(rhs.getSphereSize());
      setSphereRQ(rhs.getSphereRQ());
      setSphereStyle(rhs.getSphereStyle());

      setRibbonBlended(rhs.getRibbonBlended());
      setRibbonRounded(rhs.getRibbonRounded());
      setCoilWidth(rhs.getCoilWidth());
      setHelixWidth(rhs.getHelixWidth());
      setSheetWidth(rhs.getSheetWidth());
      setRibbonThickness(rhs.getRibbonThickness());
      setHelixRadius(rhs.getHelixRadius());

      setCartoonCylinderRadius(rhs.getCartoonCylinderRadius());
      setCartoonCylinderRQ(rhs.getCartoonCylinderRQ());
      setCartoonTubeRadius(rhs.getCartoonTubeRadius());

      setAtomColorScheme(rhs.getAtomColorScheme());
      setResidueColorScheme(rhs.getResidueColorScheme());
      set3ColorStyle(rhs.get3ColorStyle());

      setAtomSphereOpt(rhs.getAtomSphereOpt());
      setBondCylinderOpt(rhs.getBondCylinderOpt());

   }
   return *this;
}


/**
 * Initializes DisplayDescriptor from streamed representation.
 */
DisplayDescriptor& DisplayDescriptor::operator=(const string& data)
{
   istrstream is(data.c_str());
   is >> *this;
   return *this;
}


/**
 * Objects are equal if the names are the same.
 */
bool DisplayDescriptor::operator==(const DisplayDescriptor& rhs) const
{
   return p_name == rhs.getName();
}

/**
 * Objects are equal if the names are the same.
 */
bool DisplayDescriptor::operator==(const string& rhs) const
{
   return p_name == rhs;
}

string DisplayDescriptor::toString() const
{
   ostrstream os;
   os << *this << ends;
   return os.str();
}

/**
 * verify that color scheme is a valid option for a given display
 * style
 */
bool DisplayDescriptor::isValid(string display, string colorScheme,
                                bool hasResidues)
{
  vector<string> csoptions;
  if (hasResidues) {
    csoptions = getCSResidueOptions(display);
  } else {
    csoptions = getCSNoResidueOptions();
  }
  bool ret = false;
  for (size_t i=0; i<csoptions.size(); i++) {
    if (csoptions[i] == colorScheme) {
      ret = true;
    }
  }
  return ret;
}

string DisplayDescriptor::getDefaultColorScheme(string display)
{
  string ret = "Element";
  if (display == "Residue Wire" || display == "Residue Cylinders" ||
      display == "Line Ribbon" || display == "Flat Ribbon" ||
      display == "Solid Ribbon" || display == "Cartoon") {
    ret = "Secondary Structure";
  }
  return ret;
}

/**
 * Supply list of color schemes that are appropriate for a given
 * display style (with and without residues)
 */
vector<string> DisplayDescriptor::getCSResidueOptions(string style, bool hasBio)
{
  vector<string> ret;
  if (style == "Residue Wire" || style == "Residue Cylinders" ||
      style == "Line Ribbon" || style == "Flat Ribbon" ||
      style == "Solid Ribbon") {
    ret.push_back("Secondary Structure");
    ret.push_back("Position from CM");
    ret.push_back("Residue Index");
    ret.push_back("Residue Charge");
    ret.push_back("Hydrophobicity");
    ret.push_back("Chains");
    ret.push_back("Residue Name");
    ret.push_back("Block Color");
  } else if (style == "Cartoon") {
    ret.push_back("Secondary Structure");
  } else {
    ret.push_back("Element");
    ret.push_back("Position from CM");
    ret.push_back("Atom Index");
    if (hasBio) {
    ret.push_back("Secondary Structure");
    }
      ret.push_back("Residue Index");
      ret.push_back("Atom Charge");
    if (hasBio) {
      ret.push_back("Residue Charge");
      ret.push_back("Hydrophobicity");
    }
    ret.push_back("Chains");
    ret.push_back("Residue Name");
    ret.push_back("Block Color");
  }
  return ret;
}

vector<string> DisplayDescriptor::getCSNoResidueOptions()
{
  vector<string> ret;
  ret.push_back("Element");
  ret.push_back("Position from CM");
  ret.push_back("Atom Index");
  ret.push_back("Atom Charge");
  ret.push_back("Block Color");
  return ret;
}

/**
 * Modifiers
 */
void DisplayDescriptor::setName(const string& name)
{
  p_name = name;
}

void DisplayDescriptor::setStyle(const string& style)
{
  p_style = style;
}

void DisplayDescriptor::setColorScheme(const string& color)
{
  p_colorScheme = color;
}

void DisplayDescriptor::setBlockColor(const string& color)
{
  p_blockColor = color;
}

void DisplayDescriptor::setLineWidth(const int& lnwdth)
{
  p_lineWidth = lnwdth;
}

void DisplayDescriptor::setAntiAlias(bool antialias)
{
  p_antiAlias = antialias;
}

void DisplayDescriptor::setCylinderRadius(const int& radius)
{
  p_cylRadius = radius;
}

void DisplayDescriptor::setResidueCylinderRadius(const float& radius)
{
  p_cylResRadius = radius;
}

void DisplayDescriptor::setCylinderRQ(const int& rq)
{
  p_cylinderRQ = rq;
}

void DisplayDescriptor::setCapStyle(const int& style)
{
  p_capStyle = style;
}

void DisplayDescriptor::setSphereSize(const int& size)
{
  p_sphereSize = size;
}

void DisplayDescriptor::setSphereStyle(const int& flag)
{
  p_sphereStyle = flag;
}

void DisplayDescriptor::setSphereRQ(const int& rq)
{
  p_sphereRQ = rq;
}

void DisplayDescriptor::setRibbonBlended(const bool& flag)
{
  p_ribbonBlend = flag;
}

void DisplayDescriptor::setRibbonRounded(const bool& flag)
{
  p_ribbonRound = flag;
}


void DisplayDescriptor::setCoilWidth(const float& wdth)
{
  p_coilWidth = wdth;
}

void DisplayDescriptor::setHelixWidth(const float& wdth)
{
  p_helixWidth = wdth;
}

void DisplayDescriptor::setSheetWidth(const float& wdth)
{
  p_sheetWidth = wdth;
}

void DisplayDescriptor::setRibbonThickness(const float& thck)
{
  p_ribbonThickness = thck;
}

void DisplayDescriptor::setHelixRadius(const float& rad)
{
  p_ribbonHelixRad = rad;
}

void DisplayDescriptor::setCartoonCylinderRadius(const float& radius)
{
  p_cartoonCylRadius = radius;
}

void DisplayDescriptor::setCartoonCylinderRQ(const int& rq)
{
  p_cartoonCylRQ = rq;
}

void DisplayDescriptor::setCartoonTubeRadius(const float& radius)
{
  p_cartoonTubeRadius = radius;
}

void DisplayDescriptor::setAtomColorScheme(const string& opt)
{
  p_atomColor = opt;
}

void DisplayDescriptor::setResidueColorScheme(const string& opt)
{
  p_residueColor = opt;
}

void DisplayDescriptor::set3ColorStyle(const vector<string>& colors)
{
  p_3ColorStyle = colors;
}

void DisplayDescriptor::setAtomSphereOpt(const int& opt)
{
  p_atomSphere = opt;
}

void DisplayDescriptor::setBondCylinderOpt(const int& opt)
{
  p_bondCylinder = opt;
}

void DisplayDescriptor::setSpectrumType(const string& type)
{
  p_spectrumType = type;
}

/**
 * Accessors
 */
string DisplayDescriptor::getName() const
{
  return p_name;
}

string DisplayDescriptor::getStyle() const
{
  return p_style;
}

string DisplayDescriptor::getColorScheme() const
{
  return p_colorScheme;
}

string DisplayDescriptor::getBlockColor() const
{
  return p_blockColor;
}

int DisplayDescriptor::getLineWidth() const
{
  return p_lineWidth;
}

bool DisplayDescriptor::getAntiAlias() const
{
  return p_antiAlias;
}

int DisplayDescriptor::getCylinderRadius() const
{
  return p_cylRadius;
}

float DisplayDescriptor::getResidueCylinderRadius() const
{
  return p_cylResRadius;
}

int DisplayDescriptor::getCylinderRQ() const
{
  return p_cylinderRQ;
}

int DisplayDescriptor::getCapStyle() const
{
  return p_capStyle;
}

int DisplayDescriptor::getSphereSize() const
{
  return p_sphereSize;
}

int DisplayDescriptor::getSphereStyle() const
{
  return p_sphereStyle;
}

int DisplayDescriptor::getSphereRQ() const
{
  return p_sphereRQ;
}

bool DisplayDescriptor::getRibbonBlended() const
{
  return p_ribbonBlend;
}

bool DisplayDescriptor::getRibbonRounded() const
{
  return p_ribbonRound;
}


float DisplayDescriptor::getCoilWidth() const
{
  return p_coilWidth;
}

float DisplayDescriptor::getHelixWidth() const
{
  return p_helixWidth;
}

float DisplayDescriptor::getSheetWidth() const
{
  return p_sheetWidth;
}

float DisplayDescriptor::getRibbonThickness() const
{
  return p_ribbonThickness;
}

float DisplayDescriptor::getHelixRadius() const
{
  return p_ribbonHelixRad;
}

float DisplayDescriptor::getCartoonCylinderRadius() const
{
  return p_cartoonCylRadius;
}

int DisplayDescriptor::getCartoonCylinderRQ() const
{
  return p_cartoonCylRQ;
}

float DisplayDescriptor::getCartoonTubeRadius() const
{
  return p_cartoonTubeRadius;
}

string DisplayDescriptor::getAtomColorScheme() const
{
  return p_atomColor;
}

string DisplayDescriptor::getResidueColorScheme() const
{
  return p_residueColor;
}

vector<string> DisplayDescriptor::get3ColorStyle() const
{
  return p_3ColorStyle;
}

int DisplayDescriptor::getAtomSphereOpt() const
{
  return p_atomSphere;
}

int DisplayDescriptor::getBondCylinderOpt() const
{
  return p_bondCylinder;
}

string DisplayDescriptor::getSpectrumType() const
{
  return p_spectrumType;
}

/**
 *  Save and load preferences from Preference file
 */

void DisplayDescriptor::save(Preferences& prefs)
{
  string style = getStyle();
  string key;
  key = "StyleChooser." + style + ".ColorScheme";
  prefs.setString(key,getColorScheme());

  key = "StyleChooser." + style + ".LineWidth";
  if (getLineWidth() != LINEWDTH) {
    prefs.setInt(key,getLineWidth());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".AntiAlias";
  if (getAntiAlias() != ANTIALIAS) {
    prefs.setInt(key,getAntiAlias());
  } else {
    prefs.remove_entry(key);
  }

  key = "StyleChooser." + style + ".CylinderQuality";
  if (getCylinderRQ() != CYLRQ) {
    prefs.setInt(key,getCylinderRQ());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".CylinderRadius";
  if (getCylinderRadius() != CYLRAD) {
    prefs.setInt(key,getCylinderRadius());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".ResidueCylinderRadius";
  if (getResidueCylinderRadius() != RESCYLRAD) {
    prefs.setFloat(key,getResidueCylinderRadius());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".CapStyle";
  if (getCapStyle() != CAPSTYLE) {
    prefs.setInt(key,getCapStyle());
  } else {
    prefs.remove_entry(key);
  }

  key = "StyleChooser." + style + ".SphereSize";
  if (getSphereSize() != SPHERESZ) {
    prefs.setInt(key,getSphereSize());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".SphereStyle";
  if (getSphereStyle() != SPHERESTYLE) {
    prefs.setInt(key,getSphereStyle());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".SphereQuality";
  if (getSphereRQ() != SPHERERQ) {
    prefs.setInt(key,getSphereRQ());
  } else {
    prefs.remove_entry(key);
  }

  key = "StyleChooser." + style + ".RibbonBlended";
  if (getRibbonBlended() != BLENDED) {
    prefs.setBool(key,getRibbonBlended());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".RibbonRounded";
  if (getRibbonRounded() != ROUNDED) {
    prefs.setBool(key,getRibbonRounded());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".CoilWidth";
  if (getCoilWidth() != COILWDTH) {
    prefs.setFloat(key,getCoilWidth());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".HelixWidth";
  if (getHelixWidth() != HLXWDTH) {
    prefs.setFloat(key,getHelixWidth());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".SheetWidth";
  if (getSheetWidth() != SHTWDTH) {
    prefs.setFloat(key,getSheetWidth());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".RibbonThickness";
  if (getRibbonThickness() != RBTHICK) {
    prefs.setFloat(key,getRibbonThickness());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".HelixRadius";
  if (getHelixRadius() != HLXRAD) {
    prefs.setFloat(key,getHelixRadius());
  } else {
    prefs.remove_entry(key);
  }

  key = "StyleChooser." + style + ".CartoonCylinderRadius";
  if (getCartoonCylinderRadius() != CRTNCYL) {
    prefs.setFloat(key,getCartoonCylinderRadius());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".CartoonCylinderRQ";
  if (getCartoonCylinderRQ() != CRTNCYLRQ) {
    prefs.setInt(key,getCartoonCylinderRQ());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".CartoonTubeRadius";
  if (getCartoonTubeRadius() != CRTNTB) {
    prefs.setFloat(key,getCartoonTubeRadius());
  } else {
    prefs.remove_entry(key);
  }

  key = "StyleChooser." + style + ".AtomSphere";
  if (getAtomSphereOpt() != ATMSPHR) {
    prefs.setInt(key,getAtomSphereOpt());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".BondCylinder";
  if (getBondCylinderOpt() != BNDCYL) {
    prefs.setInt(key,getBondCylinderOpt());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".SpectrumType";
  if (getSpectrumType() != SPECTRUMTYPE) {
    prefs.setString(key,getSpectrumType());
  } else {
    prefs.remove_entry(key);
  }
  key = "StyleChooser." + style + ".BlockColor";
  if (getBlockColor() != BLOCKCOLOR) {
    prefs.setString(key,getBlockColor());
  } else {
    prefs.remove_entry(key);
  }
}

void DisplayDescriptor::load(const Preferences& prefs, const string& style)
{
  int ival;
  float rval;
  bool bval;
  string sval;

  setStyle(style);
  if (prefs.getString("StyleChooser." + style + ".ColorScheme",sval)) {
    setColorScheme(sval);
  }
  if (prefs.getInt("StyleChooser." + style + ".LineWidth",ival)) {
    setLineWidth(ival);
  }
  if (prefs.getBool("StyleChooser." + style + ".AntiAlias",bval)) {
    setAntiAlias(bval);
  }
  if (prefs.getInt("StyleChooser." + style + ".CylinderRadius",ival)) {
    setCylinderRadius(ival);
  }
  if (prefs.getFloat("StyleChooser." + style + ".ResidueCylinderRadius",rval)) {
    setResidueCylinderRadius(rval);
  }
  if (prefs.getInt("StyleChooser." + style + ".CylinderQuality",ival)) {
    setCylinderRQ(ival);
  }
  if (prefs.getInt("StyleChooser." + style + ".CapStyle",ival)) {
    setCapStyle(ival);
  }
  if (prefs.getInt("StyleChooser." + style + ".SphereSize",ival)) {
    setSphereSize(ival);
  }
  if (prefs.getInt("StyleChooser." + style + ".SphereStyle",ival)) {
    setSphereStyle(ival);
  }
  if (prefs.getInt("StyleChooser." + style + ".SphereQuality",ival)) {
    setSphereRQ(ival);
  }
  if (prefs.getBool("StyleChooser." + style + ".RibbonBlended",bval)) {
    setRibbonBlended(bval);
  }
  if (prefs.getBool("StyleChooser." + style + ".RibbonRounded",bval)) {
    setRibbonRounded(bval);
  }
  if (prefs.getFloat("StyleChooser." + style + ".CoilWidth",rval)) {
    setCoilWidth(rval);
  }
  if (prefs.getFloat("StyleChooser." + style + ".HelixWidth",rval)) {
    setHelixWidth(rval);
  }
  if (prefs.getFloat("StyleChooser." + style + ".SheetWidth",rval)) {
    setSheetWidth(rval);
  }
  if (prefs.getFloat("StyleChooser." + style + ".RibbonThickness",rval)) {
    setRibbonThickness(rval);
  }
  if (prefs.getFloat("StyleChooser." + style + ".HelixRadius",rval)) {
    setHelixRadius(rval);
  }
  if (prefs.getFloat("StyleChooser." + style + ".CartoonCylinderRadius",rval)) {
    setCartoonCylinderRadius(rval);
  }
  if (prefs.getInt("StyleChooser." + style + ".CartoonCylinderRQ",ival)) {
    setCartoonCylinderRQ(ival);
  }
  if (prefs.getFloat("StyleChooser." + style + ".CartoonTubeRadius",rval)) {
    setCartoonTubeRadius(rval);
  }
  if (prefs.getInt("StyleChooser." + style + ".AtomSphere",ival)) {
    setAtomSphereOpt(ival);
  }
  if (prefs.getInt("StyleChooser." + style + ".BondCylinder",ival)) {
    setBondCylinderOpt(ival);
  }
  if (prefs.getString("StyleChooser." + style + ".SpectrumType",sval)) {
    setSpectrumType(sval);
  }
  if (prefs.getString("StyleChooser." + style + ".BlockColor",sval)) {
    setBlockColor(sval);
  }
}

/**
 *  Streaming operators
 *  @todo this code could be upgraded to only send things that are visible
 *        to reduce sending of unneeded data.
 */
ostream& operator<<(ostream& os, const DisplayDescriptor& display)
{
  string name = display.getName();
  string style = display.getStyle();
  string scheme = display.getColorScheme();
  /**
   * Replace all blanks with underscores
   */
  int nsize;
  nsize = name.size();
  for (int i=0; i<nsize; i++) {
    if (name[i] == ' ') name[i] = '_';
  }
  nsize = style.size();
  for (int i=0; i<nsize; i++) {
    if (style[i] == ' ') style[i] = '_';
  }
  nsize = scheme.size();
  for (int i=0; i<nsize; i++) {
    if (scheme[i] == ' ') scheme[i] = '_';
  }

  /**
   * Add underscores around all strings to protect against blank strings
   */
  os << "_" <<name << "_ ";
  os << "_" <<style << "_ ";
  os << "_" <<scheme << "_";
  if (display.getColorScheme() == "Block Color" &&
      display.getBlockColor() != display.BLOCKCOLOR) {
     // Only write this out if the scheme is block color
    os << " BC:" << display.getBlockColor();
  }
  if (display.getSpectrumType() != display.SPECTRUMTYPE) {
    os << " ST:" << display.getSpectrumType();
  }
  if (display.getLineWidth() != display.LINEWDTH) {
    os << " LW:"<<display.getLineWidth();
  }
  if (display.getAntiAlias() != display.ANTIALIAS) {
    os << " AA:"<<display.getAntiAlias();
  }
  if (display.getCylinderRadius() != display.CYLRAD) {
    os << " CR:"<<display.getCylinderRadius();
  }
  if (display.getResidueCylinderRadius() != display.RESCYLRAD) {
    os << " RCR:"<<display.getResidueCylinderRadius();
  }
  if (display.getCylinderRQ() != display.CYLRQ) {
    os << " CRQ:"<<display.getCylinderRQ();
  }
  if (display.getCapStyle() != display.CAPSTYLE) {
    os << " CS:"<<display.getCapStyle();
  }
  if (display.getSphereSize() != display.SPHERESZ) {
    os << " SS:"<<display.getSphereSize();
  }
  if (display.getSphereStyle() != display.SPHERESTYLE) {
    os << " SB:"<<display.getSphereStyle();
  }
  if (display.getSphereRQ() != display.SPHERERQ) {
    os << " SRQ:"<<display.getSphereRQ();
  }
  if (display.getRibbonBlended() != display.BLENDED) {
    os << " RB:"<<display.getRibbonBlended();
  }
  if (display.getRibbonRounded() != display.ROUNDED) {
    os << " RR:"<<display.getRibbonRounded();
  }
  if (display.getCoilWidth() != display.COILWDTH) {
    os << " CW:"<<display.getCoilWidth();
  }
  if (display.getHelixWidth() != display.HLXWDTH) {
    os << " HW:"<<display.getHelixWidth();
  }
  if (display.getSheetWidth() != display.SHTWDTH) {
    os << " SW:"<<display.getSheetWidth();
  }
  if (display.getRibbonThickness() != display.RBTHICK) {
    os << " RT:"<<display.getRibbonThickness();
  }
  if (display.getHelixRadius() != display.HLXRAD) {
    os << " HR:"<<display.getHelixRadius();
  }
  if (display.getCartoonCylinderRadius() != display.CRTNCYL) {
    os << " CCR:"<<display.getCartoonCylinderRadius();
  }
  if (display.getCartoonCylinderRQ() != display.CRTNCYLRQ) {
    os << " CCRQ:"<<display.getCartoonCylinderRQ();
  }
  if (display.getCartoonTubeRadius() != display.CRTNTB) {
    os << " CTR:"<<display.getCartoonTubeRadius();
  }
  /*
  if (display.getAtomColorScheme() != display.ATOMCOLOR) {
    os << " ACS:"<<display.getAtomColorScheme();
  }
  if (display.getResidueColorScheme() != display.RESCOLOR) {
    os << " RCS:"<<display.getResidueColorScheme();
  }
  */
  scheme = display.getColorScheme();
  if (scheme == "Hydrophobicity" || scheme == "Residue Charge" || 
      scheme == "Secondary Structure") {
    os << " 3C:";
    vector<string> colors = display.get3ColorStyle();
    for (size_t idx=0; idx<colors.size();idx++) {
       if (idx != 0) os << ",";
       os << colors[idx];
    }
  }
  if (display.getAtomSphereOpt() != display.ATMSPHR) {
    os << " ASO:"<<display.getAtomSphereOpt();
  }
  if (display.getBondCylinderOpt() != display.BNDCYL) {
    os << " BCO:"<<display.getBondCylinderOpt();
  }
  /*
  if (display.getCartoonArrowTSegments() != display.CRTNAWTS) {
    os << " GCATS:"<<display.getCartoonArrowTSegments();
  }
  */
  /*
  if (display.getCartoonTubeTSegments() != display.CRTNTBTS) {
    os << " GCTTS:"<<display.getCartoonTubeTSegments();
  }
  */
  /*
  if (display.getCartoonTubeSSegments() != display.CRTNTBSS) {
    os << " GCTSS:"<<display.getCartoonTubeSSegments();
  }
  */
  return os;
}


istream& operator>>(istream& is, DisplayDescriptor& display)
{
  string name, style, scheme;
  is >> name;
  is >> style;
  is >> scheme;
  string token;
  while (!is.eof()) {
    is >> token;
    StringTokenizer tokenizer(token,":");
    string key = tokenizer.next();
    string value = tokenizer.next();
    if (key == "BC") {
       if (value != "")  display.setBlockColor(value);
    } else if (key == "ST") {
      display.setSpectrumType(value);
    } else if (key == "LW") {
      display.setLineWidth(atoi(value.c_str()));
    } else if (key == "AA") {
      display.setAntiAlias((bool)atoi(value.c_str()));
    } else if (key == "CR") {
      display.setCylinderRadius(atoi(value.c_str()));
    } else if (key == "RCR") {
      display.setResidueCylinderRadius((float)atof(value.c_str()));
    } else if (key == "CRQ") {
      display.setCylinderRQ(atoi(value.c_str()));
    } else if (key == "CS") {
      display.setCapStyle(atoi(value.c_str()));
    } else if (key == "SS") {
      display.setSphereSize(atoi(value.c_str()));
    } else if (key == "SB") {
      display.setSphereStyle(atoi(value.c_str()));
    } else if (key == "SRQ") {
      display.setSphereRQ(atoi(value.c_str()));
    } else if (key == "RB") {
      display.setRibbonBlended((bool)atoi(value.c_str()));
    } else if (key == "RR") {
      display.setRibbonRounded((bool)atoi(value.c_str()));
    } else if (key == "CW") {
      display.setCoilWidth((float)(atof(value.c_str())));
    } else if (key == "HW") {
      display.setHelixWidth((float)(atof(value.c_str())));
    } else if (key == "SW") {
      display.setSheetWidth((float)(atof(value.c_str())));
    } else if (key == "RT") {
      display.setRibbonThickness((float)(atof(value.c_str())));
    } else if (key == "HR") {
      display.setHelixRadius((float)(atof(value.c_str())));
    } else if (key == "CCR") {
      display.setCartoonCylinderRadius((float)(atof(value.c_str())));
    } else if (key == "CCRQ") {
      display.setCartoonCylinderRQ((int)(atof(value.c_str())));
    } else if (key == "CTR") {
      display.setCartoonTubeRadius((float)(atof(value.c_str())));
    } else if (key == "ACS") {
      display.setAtomColorScheme(value.c_str());
    } else if (key == "RCS") {
      display.setResidueColorScheme(value.c_str());
    } else if (key == "3C") {
       StringTokenizer tokenizer(value,",");
       vector<string> colors = tokenizer.tokenize(",");
       display.set3ColorStyle(colors);
    } else if (key == "ASO") {
      display.setAtomSphereOpt(atoi(value.c_str()));
    } else if (key == "BCO") {
      display.setBondCylinderOpt(atoi(value.c_str()));
    } else if (key == "GCATS") {
      //display.setCartoonArrowTSegments(atoi(value.c_str()));
    } else if (key == "GCTTS") {
      //display.setCartoonTubeTSegments(atoi(value.c_str()));
    } else if (key == "GCTSS") {
      //display.setCartoonTubeSSegments(atoi(value.c_str()));
    } else  {
       cerr << "Unrecognized style key " << key << "=\"" << value << "\"" <<endl;
    }
  }

  /**
   * Remove underscores from either end of string
   */
  string newName;
  if (name.size()>2) {
    newName = name.substr(1,name.size()-2);
  } else {
    newName = "";
  }
  string newStyle;
  if (style.size()>2) {
    newStyle = style.substr(1,style.size()-2);
  } else {
    newStyle = "";
  }
  string newColor;
  if (scheme.size()>2) {
    newColor = scheme.substr(1,scheme.size()-2);
  } else {
    newColor = "";
  }
  /**
   * Replace remaining underscores with blanks
   */
  int i, nsize;
  nsize = newName.size();
  for (i=0; i<nsize; i++) {
    if (newName[i] == '_') newName[i] = ' ';
  }
  nsize = newStyle.size();
  for (i=0; i<nsize; i++) {
    if (newStyle[i] == '_') newStyle[i] = ' ';
  }
  nsize = newColor.size();
  for (i=0; i<nsize; i++) {
    if (newColor[i] == '_') newColor[i] = ' ';
  }
  display.setName(newName);
  display.setStyle(newStyle);
  display.setColorScheme(newColor);

  is.clear(ios::badbit|is.rdstate());  //Clear read error, if any
  return is;
}
