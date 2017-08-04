#include "tdat/DisplayStyle.H"

// TODO toss this constructor and require name
DisplayStyle::DisplayStyle(DisplayStyle::Style style )
{
   p_name = "default";
   p_displayFlag = true;
   if (style == DisplayStyle::DNONE) {
      p_displayFlag = false;
      p_style = DisplayStyle::BALLWIRE;
   }
   p_style = style;
}

DisplayStyle::~DisplayStyle()
{
}

vector<string> DisplayStyle::getAllNames(bool includeNone)
{
   vector<string> ret;
   ret.push_back("Ball And Wireframe");
   ret.push_back("Ball And Stick");
   ret.push_back("Stick");
   ret.push_back("Wireframe");
   ret.push_back("CPK");
   ret.push_back("Residue Wire");
   ret.push_back("Residue Cylinders");
   ret.push_back("Line Ribbon");
   ret.push_back("Flat Ribbon");
   ret.push_back("Solid Ribbon");
   ret.push_back("Cartoon");
   if (includeNone) {
      ret.push_back("Invisible");
   }
   return ret;
}

vector<string> DisplayStyle::getNoResidueNames(bool includeNone)
{
   vector<string> ret;
   ret.push_back("Ball And Wireframe");
   ret.push_back("Ball And Stick");
   ret.push_back("Stick");
   ret.push_back("Wireframe");
   ret.push_back("CPK");
   if (includeNone) {
      ret.push_back("Invisible");
   }
   return ret;
}


DisplayStyle::Style DisplayStyle::toStyle(const string& name)
{
   DisplayStyle::Style ret = DisplayStyle::INVALIDSTYLE;
   if (name == "Ball And Wireframe") {
      ret = BALLWIRE;
   } else if ( name == "Ball And Stick") {
      ret = BALLSTICK;
   } else if (name == "Stick") {
      ret = STICK;
   } else if ( name == "Wireframe") {
      ret = WIRE;
   } else if (name == "CPK") {
      ret = CPK;
   } else if (name == "Residue Wire") {
      ret = CAWIRE;
   } else if (name == "Residue Cylinders") {
      ret = CYLINDER;
   } else if (name == "Line Ribbon") {
      ret = LINE;
   } else if (name == "Flat Ribbon") {
      ret = FLAT;
   } else if (name == "Solid Ribbon") {
      ret = SOLID;
   } else if (name == "Cartoon") {
      ret = CARTOON;
   } else if (name == "Invisible") {
      ret = DNONE;
   }
   return ret;
}



string DisplayStyle::fromStyle(DisplayStyle::Style style)
{
   string ret = "Invalid";

   if (style == BALLWIRE) {
      ret = "Ball And Wireframe";
   } else if ( style == BALLSTICK) {
      ret = "Ball And Stick";
   } else if (style == STICK) {
      ret = "Stick";
   } else if (style == WIRE) {
      ret = "Wireframe";
   } else if (style == CPK) {
      ret = "CPK";
   } else if (style == CAWIRE) {
      ret = "Residue Wire";
   } else if (style == CYLINDER) {
      ret = "Residue Cylinders";
   } else if (style == LINE) {
      ret = "Line Ribbon";
   } else if (style == FLAT) {
      ret = "Flat Ribbon";
   } else if (style == SOLID) {
      ret = "Solid Ribbon";
   } else if (style == CARTOON) {
      ret = "Cartoon";
   } else if (style == DNONE) {
      ret = "Invisible";
   }
   return ret;
}

/**
 * Set the name of the style.
 * Name is used to correlate to defined rendering styles.
 */
void DisplayStyle::setName(const string& name)
{
   p_name = name;
}

/**
 * Get the name of the style.
 */
string DisplayStyle::getName() const
{
   return p_name;
}

void DisplayStyle::setStyle(DisplayStyle::Style style)
{
   if (style == DisplayStyle::DNONE) {
      p_displayFlag = false;
   } else {
     p_style = style;
   }
}

DisplayStyle::Style DisplayStyle::getStyle() const
{
   return p_style;
}


void DisplayStyle::setDisplayed(bool displayed)
{
   p_displayFlag = displayed;
}

bool DisplayStyle::isDisplayed() const
{
   return p_displayFlag;
}

DisplayStyle& DisplayStyle::operator=(const DisplayStyle& rhs)
{
   if (this != &rhs) {
      p_displayFlag = rhs.p_displayFlag;
      p_style = rhs.p_style;
      p_name = rhs.p_name;
   }
   return *this;
}
