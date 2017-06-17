/**
 * @file
 *
 *
 */

#include <iostream>
using std::ostream;
using std::istream;
using std::ios;
using std::cout;
using std::endl;

#include <string.h>
#include <stdio.h>  // sprintf


#include "util/ErrMsg.H"
#include "util/Preferences.H"
#include "util/Color.H"


// Colors - non integral types can't be initialized in declaration
const char* Color::WINDOW  = "#e0e1e1";
const char* Color::MENUBAR = "#b7b8ba";
const char* Color::NOTAPPFG = "#bbc4d1";
const char* Color::READONLY = "#b7b8ba";
const char* Color::TEXT = "#000000";
const char* Color::TOGGLE = "#eaba7f";
const char* Color::INPUT = "#f9dd99";
const char* Color::BUTTON = "#bfbfbf";
const char* Color::SHADOW = "#bfbfbf";
const char* Color::CAPTION = "#d57500";
const char* Color::CAPTION_GRADIENT = "#e29e4c";
const char* Color::HIGHLIGHTBG = "#000000";
const char* Color::HIGHLIGHTFG = "#ffffff";
const char* Color::FOREGROUND = "#000000";
const char* Color::WARNING = "#ffffc6";
const char* Color::ERROR = "#ff0000";

const char* Color::TOPSHADOW = "#f2f2f2";
const char* Color::BOTTOMSHADOW = "#000000";
const char* Color::CURRENTMODE = "#8cb9be";

const char* Color::PUSHEDINBTN = "#a3a3a3";
const char* Color::INVALIDFG = "#ce6363";

const char* Color::NEWITEM = "#5abd8c";
const char* Color::MULTIPLYDEFINEDITEM = "#efe7ad";
const char* Color::INCOMPLETE = "#ff7585";

const char* Color::MOGRAPH1 = "#949494";
const char* Color::MOGRAPH2 = "#ff0000";
const char* Color::MOGRAPH3 = "#00bfff";

const char* Color::NEUTRAL = "#cccc00";
const char* Color::POSITIVE = "#3333ff";
const char* Color::NEGATIVE = "#ff3333";


void Color::initialize()
{
  Preferences userColors("MyAppColors", false);
  Preferences siteColors("../../../siteconfig/AppColors", true);

  string theme;
  if (userColors.getString("Theme", theme) && theme!="Site Default") {
    setTheme(theme);
  } else if (siteColors.getString("Theme", theme)) {
    setTheme(theme);
  }

  string color;
  if (userColors.getString("Window", color)) {
    Color::WINDOW = strdup(color.c_str());
  } else if (siteColors.getString("Window", color)) {
    Color::WINDOW = strdup(color.c_str());
  }

  if (userColors.getString("Menubar", color)) {
    Color::MENUBAR = strdup(color.c_str());
  } else if (siteColors.getString("Menubar", color)) {
    Color::MENUBAR = strdup(color.c_str());
  }

  if (userColors.getString("Text", color)) {
    Color::TEXT = strdup(color.c_str());
  } else if (siteColors.getString("Text", color)) {
    Color::TEXT = strdup(color.c_str());
  }

  if (userColors.getString("Input", color)) {
    Color::INPUT = strdup(color.c_str());
  } else if (siteColors.getString("Input", color)) {
    Color::INPUT = strdup(color.c_str());
  }

  if (userColors.getString("Readonly", color)) {
    Color::READONLY = strdup(color.c_str());
  } else if (siteColors.getString("Readonly", color)) {
    Color::READONLY = strdup(color.c_str());
  }

  if (userColors.getString("Caption", color)) {
    Color::CAPTION = strdup(color.c_str());
  } else if (siteColors.getString("Caption", color)) {
    Color::CAPTION = strdup(color.c_str());
  }

  if (userColors.getString("CaptionGradient", color)) {
    Color::CAPTION_GRADIENT = strdup(color.c_str());
  } else if (siteColors.getString("CaptionGradient", color)) {
    Color::CAPTION_GRADIENT = strdup(color.c_str());
  }

}

void Color::setTheme(const string& theme)
{
  if (theme=="Classic" || theme=="classic" || theme=="CLASSIC") {
    Color::WINDOW = strdup("#e5e5e5");
    Color::MENUBAR = strdup("#bbc4d1");
    Color::TEXT = strdup("#000000");
    Color::INPUT = strdup("#8cb9be");
    Color::READONLY = strdup("#bbc4d1");
    Color::CAPTION = strdup("#4c5489");
    Color::CAPTION_GRADIENT = strdup("#646c99");
    Color::WARNING = strdup("#ffffc6");
  } else if (theme=="Pnnl" || theme=="pnnl" || theme=="PNNL") {
    Color::WINDOW = strdup("#e0e1e1");
    Color::MENUBAR = strdup("#b7b8ba");
    Color::TEXT = strdup("#000000");
    Color::INPUT = strdup("#f9dd99");
    Color::READONLY = strdup("#b7b8ba");
    Color::CAPTION = strdup("#d57500");
    Color::CAPTION_GRADIENT = strdup("#e29e4c");
    Color::WARNING = strdup("#d57500");
  } else if (theme=="Emsl" || theme=="emsl" || theme=="EMSL") {
    Color::WINDOW = strdup("#e0e1e1");
    Color::MENUBAR = strdup("#b7b8ba");
    Color::TEXT = strdup("#000000");
    Color::INPUT = strdup("#c6d599");
    Color::READONLY = strdup("#b7b8ba");
    Color::CAPTION = strdup("#719500");
    Color::CAPTION_GRADIENT = strdup("#b8ca7f");
    Color::WARNING = strdup("#ffffc6");
  } else if (theme=="PnnlEmsl" || theme=="pnnlemsl" || theme=="PNNLEMSL" ||
             theme=="Pnnl_Emsl" || theme=="pnnl_emsl" || theme=="PNNL_EMSL") {
    Color::WINDOW = strdup("#e0e1e1");
    Color::MENUBAR = strdup("#b7b8ba");
    Color::TEXT = strdup("#000000");
    Color::INPUT = strdup("#b8ca7f");
    Color::READONLY = strdup("#b7b8ba");
    Color::CAPTION = strdup("#d57500");
    Color::CAPTION_GRADIENT = strdup("#e29e4c");
    Color::WARNING = strdup("#d57500");
  } 
}


// ---------- Constructor/Destructor ----------
///////////////////////////////////////////////////////////////////////////////
// man
// Description
//   Create a default black Color object
///////////////////////////////////////////////////////////////////////////////
Color::Color(void) : p_red(0.0), p_green(0.0), p_blue(0.0)
{}

///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Create a Color object from specified red/green/blue values [0..1]
// Implementation
//    We simply assign the colors and then rely on tha validation
//    routine to check and modify them if they are out of range.
///////////////////////////////////////////////////////////////////////////////
Color::Color(float red, float green, float blue) : p_red(red),
	       				           p_green(green),
	                                           p_blue(blue)
{ validate_color(); }


Color::Color(unsigned char red, unsigned char green, unsigned char blue) 
{ 
   p_red = ((int)red)/255.0;
   p_blue = ((int)blue)/255.0;
   p_green = ((int)green)/255.0;
   validate_color(); 
}

///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Create a Color object from specified red/green/blue values [0..1] input
//    as an string formatted as "#ffffff"
// Implementation
//    Pick out the three bytes in hexadecimal from the input string. Convert
//    each byte to a float in the range 0 <= x <= 1.
///////////////////////////////////////////////////////////////////////////////
Color::Color(const string& color) : p_red(0.0), p_green(0.0), p_blue(0.0)
{
  if ((color.length() == 7) && (color[0] == '#')) {
    string red = color.substr(1,2);
    string green = color.substr(3,2);
    string blue = color.substr(5,2);
    p_red = strtol(red.c_str(),(char **)NULL,16)/255.0;
    p_green = strtol(green.c_str(),(char **)NULL,16)/255.0;
    p_blue = strtol(blue.c_str(),(char **)NULL,16)/255.0;
    validate_color();
  }
}

///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Create a Color object from another one
// Implementation
//    We assume the color object being copied has valid values
//    (it must have been validated somewhere).  All we do is copy
//    the rgb values.  We do the copy here rather than using
//    the operator= for efficiency.
///////////////////////////////////////////////////////////////////////////////
Color::Color(const Color& color) : p_red(color.red()),
                                   p_green(color.green()),
                                   p_blue(color.blue())
{ }

///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Destroy the Color object
///////////////////////////////////////////////////////////////////////////////
Color::~Color(void)
{}

// ---------- Operators ----------
///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Assign a colors RGB values to this Color object
///////////////////////////////////////////////////////////////////////////////
Color& Color::operator=(const Color& color)
{
  setRGB(color.red(), color.green(), color.blue());
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Value-based equality and inequality
///////////////////////////////////////////////////////////////////////////////
bool Color::operator==(const Color& color) const
{
  return ((p_red == color.red()) &&
          (p_green == color.green()) &&
          (p_blue == color.blue()));
}
bool Color::operator!=(const Color& color) const
{
  return ((p_red != color.red()) ||
          (p_green != color.green()) ||
          (p_blue != color.blue()));
}

// ---------- Modifiers ----------
///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Change this Color by setting the RGB values
//
///////////////////////////////////////////////////////////////////////////////
void Color::setRGB(float red, float green, float blue)
{
  p_red = red;
  p_green = green;
  p_blue = blue;
  validate_color();
}


// ---------- Accessors ----------
string Color::xcolor() const
{
  char buf[10];
  int r = (int) (p_red * 255);
  int g = (int) (p_green * 255);
  int b = (int) (p_blue * 255);
  sprintf(buf,"#%02x%02x%02x",r,g,b);
  return buf;
}

///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Simple accessors
//
///////////////////////////////////////////////////////////////////////////////
float Color::red(void) const
{ return p_red; }
float Color::green(void) const
{ return p_green; }
float Color::blue(void) const
{ return p_blue; }

///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Set the three float reference to the RGB Values of this Color
///////////////////////////////////////////////////////////////////////////////
void Color::getRGB(float& red, float& green, float& blue) const
{
   red = p_red;
   green = p_green;
   blue = p_blue;
}



// ------------------------
// Private Member Functions
// ------------------------

  // ---------- Encapsualted Behavior ----------
///////////////////////////////////////////////////////////////////////////////
// Description
//    Check and reset private Color values if they are out of range
// Implementation
//    For each of the private color values we check it's range and
//    signal a warning if we have to change the value to make it valid.
///////////////////////////////////////////////////////////////////////////////
void Color::validate_color(void)
{
  ErrMsg err;
  if (p_red < LOWCOLOR) {
    p_red = LOWCOLOR;
    err.message("EE_COLOR_RANGE", EE_WARNING, WHERE, "Red",p_red,LOWCOLOR);
  } else if (p_red > HIGHCOLOR) {
    p_red = HIGHCOLOR;
    err.message("EE_COLOR_RANGE", EE_WARNING, WHERE, "Red",p_red,HIGHCOLOR);
  }
  if (p_green < LOWCOLOR) {
    p_green = LOWCOLOR;
    err.message("EE_COLOR_RANGE", EE_WARNING, WHERE,"Green",p_green,LOWCOLOR);
  } else if (p_green > HIGHCOLOR) {
    p_green = HIGHCOLOR;
    err.message("EE_COLOR_RANGE", EE_WARNING, WHERE,"Green",p_green,HIGHCOLOR);
  }
  if (p_blue < LOWCOLOR) {
    p_blue = LOWCOLOR;
    err.message("EE_COLOR_RANGE", EE_WARNING, WHERE, "Blue",p_blue,LOWCOLOR);
  } else if (p_blue > HIGHCOLOR) {
    p_blue = HIGHCOLOR;
    err.message("EE_COLOR_RANGE", EE_WARNING, WHERE, "Blue",p_blue,HIGHCOLOR);
  }
}

// -------------- friends ---------------
///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Dump an RGB Color triplet to a stream
// Implementation
//    This doesn't need to be a friend but we keep it as such for symmetry
//    with operator<<().
///////////////////////////////////////////////////////////////////////////////
ostream& operator<<(ostream& os, const Color& color)
{
  os << "[Color<" << color.red() << "," << color.green() << ",";
  os << color.blue() << ">]" << endl;
  return os;
}

///////////////////////////////////////////////////////////////////////////////
// man
// Description
//    Read a RGB Color triplet from a stream to modify a Color
// Implementation
//    Read three floats into local variables and set the color's values
//    using one of the modifiers.  Validate the colors afterward (this
//    requires friendship of the Color class.
//    We always reset errors on is if any read errors were encountered.
///////////////////////////////////////////////////////////////////////////////
istream& operator>>(istream& is, Color& color)
{
  float red, green, blue;
  is >> red >> green >> blue;
  color.setRGB(red, green, blue);

  is.clear(ios::badbit|is.rdstate());             // clear read error, if any
  return is;
}

// Static Constants for Validity Checking
float Color::HIGHCOLOR = 1.0;
float Color::LOWCOLOR = 0.0;
