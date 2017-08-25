///////////////////////////////////////////////////////////////////////////////
// HEADER FILENAME: EcceDispParam.H
//
//
// CLASS SYNOPSIS:
//    Encapsulation of display attributes used within Viewers such
//    as colors and fonts.
//
// EXPORTED TYPES:
//    Class EcceDispParam
//
// LOCAL TYPES:
//
// SEE ALSO:
//      
// DESCRIPTION:
//   This class is designed as a "single instance" pattern.
//   It is cheap to contruct and each instance will have the
//   same display parameters as the others.  The purpose is to
//   provide encapsulation of colors and font information used
//   within viewer based applications.  Typically, these parameters
//   are set in one place in the interface then must be used
//   everywhere.  The class is an alternative to a bunch of globaal
//   variables.
//
//   The font size and font name information only apply to 
//   molecular inventor text.  It does not apply to other ECCE
//   gl-based bitmap text.  The man pages for ChemDispParam says 
//     "The available fonts are listed in /usr/lib/DPS/DPSFonts.upr".
//
///////////////////////////////////////////////////////////////////////////////
#ifndef ECCEDISPPARAM_HH
#define ECCEDISPPARAM_HH

// system includes

// library includes
#include <string>
using std::string;
#include "inv/SbColor.H"

class EcceDispParam;

// application includes

class EcceDispParam
{

public:
     EcceDispParam(void);
     virtual ~EcceDispParam(void);

     // Accessors
     void   getBackground(float& r, float& g, float& b) const;
     void   getForeground(float& r, float& g, float& b) const;
     void   getInfoText(float& r, float& g, float& b) const;
     void   getHighlight(float& r, float& g, float& b) const;
     void   getLasso(float& r, float& g, float& b) const;
     string getMIFontName(void) const;
     int       getMIFontSize(void) const;

     // Modifiers
     void   setBackground(const float& r,const float& g, const float& b);
     void   setForeground(const float& r,const float& g, const float& b);
     void   setInfoText(const float& r,const float& g, const float& b);
     void   setHighlight(const float& r,const float& g, const float& b);
     void   setLasso(const float& r,const float& g, const float& b);
     void   setMIFontName(const string& name);
     void   setMIFontSize(int size);

     void   load(const char *fileName);


protected:


private:

     EcceDispParam(const EcceDispParam&);

     // State
     static int       p_MIFontSize;
     static SbColor   p_bg;
     static SbColor   p_fg;
     static SbColor   p_highlight;
     static SbColor   p_text;
     static SbColor   p_lasso;

}; // EcceDispParam

#endif /* ECCEDISPPARAM_HH */
