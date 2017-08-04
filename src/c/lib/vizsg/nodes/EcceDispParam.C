///////////////////////////////////////////////////////////////////////////////
// SOURCE FILENAME: EcceDispParam.C
//
//
// DESIGN:
//   The public interface allows setting and getting of various
//   viz display parameters.  The idea is that when needed, the
//   class can be enhanced to read these values for a system and/or
//   user preference file.
//
//   The get/set naming convention was used due to function overloading
//   problems that the compiler couldn't handle (but should have been
//   able to handle).
//
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
  using std::cerr;
  using std::endl;

#include "viz/EcceDispParam.H"

///////////////////////////////////////////////////////////////////////////////
// class statics
///////////////////////////////////////////////////////////////////////////////
SbColor   EcceDispParam::p_bg( 0.0, 0.0, 0.0);         // black
SbColor   EcceDispParam::p_fg( 1.0, 1.0, 1.0);         // white
SbColor   EcceDispParam::p_highlight( 0.4, 0.4, 0.4);  // magenta
SbColor   EcceDispParam::p_text( 0.0, 1.0, 0.0);       // green
SbColor   EcceDispParam::p_lasso( 1.0, 1.0, 0.0);      // yellow
//string    EcceDispParam::p_MIFontName = "";
int       EcceDispParam::p_MIFontSize = 14;

// -----------------------
// Public Member Functions
// -----------------------

// ---------- Constructors ------------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//    Constructor.
//
///////////////////////////////////////////////////////////////////////////////
EcceDispParam::EcceDispParam(void)
{
}

EcceDispParam::EcceDispParam(const EcceDispParam& rhs)
{
  //noop
}

 
// ---------- Virtual Destructor ------------
EcceDispParam::~EcceDispParam(void)
{
}



// ------------ Modifiers ------------
void EcceDispParam::setBackground(const float& r,const float& g,const float& b)
{
  p_bg.setValue(r,g,b);
}
void EcceDispParam::setForeground(const float& r,const float& g,const float& b)
{
  p_fg.setValue(r,g,b);
}

void EcceDispParam::setLasso(const float& r,const float& g,const float& b)
{
  p_lasso.setValue(r,g,b);
}

void EcceDispParam::setHighlight(const float& r,const float& g,const float& b)
{
  p_highlight.setValue(r,g,b);
}

void EcceDispParam::setInfoText(const float& r,const float& g,const float& b)
{
  p_text.setValue(r,g,b);
}

// ------------ Accessors ------------
void EcceDispParam::getBackground(float& r, float& g, float& b) const
{ 
  p_bg.getValue(r,g,b);
}

void EcceDispParam::getForeground(float& r, float& g, float& b) const
{
  p_fg.getValue(r,g,b);
}

void EcceDispParam::getLasso(float& r, float& g, float& b) const
{
  p_lasso.getValue(r,g,b);
}

void EcceDispParam::getHighlight(float& r, float& g, float& b) const
{
  p_highlight.getValue(r,g,b);
}

void EcceDispParam::getInfoText(float& r, float& g, float& b) const
{
  p_text.getValue(r,g,b);
}

int EcceDispParam::getMIFontSize(void) const
{ 
  return p_MIFontSize; 
}

string EcceDispParam::getMIFontName(void) const
{ 
  //return p_MIFontName;
  return "";
}


// ---------- General Methods  ----------
///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//   **Currently a noop.
//    Load the settings from a file to override the compiled in
//    defaults.
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
void EcceDispParam(const char *fileName)
{
}



// --------------------------
// Protected Member Functions
// --------------------------

// ---------- Derived Class Accessors ----------



// ------------------------
// Private Member Functions
// ------------------------


// ---------- Encapsulated Behavior ----------

///////////////////////////////////////////////////////////////////////////////
//  man
//
//  Description
//
//  Implementation
//
///////////////////////////////////////////////////////////////////////////////
