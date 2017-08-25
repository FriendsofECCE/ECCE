/////////////////////////////////////////////////////////////////////////////
// Name:        DisplayDescriptor.H
// Purpose:
// Author:      Bruce Palmer
// Modified by:
// Created:     Thu 03 April 2006 11:12:47 PM PST
// RCS-ID:
// Licence:
/////////////////////////////////////////////////////////////////////////////

#ifndef DISPLAYDESCRIPTOR_H
#define DISPLAYDESCRIPTOR_H

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <iostream>
using std::ostream;
using std::istream;


class Preferences;

/**
 *  DisplayDescriptor class declaration. This is a simple class that contains
 *  information that describes the display style for a set of atoms and/or
 *  residues.
 */
class DisplayDescriptor
{
   public:
      /**
       * Default values
       */
      static const string DISPLAY;
      static const string COLORSCHEME;
      static const int LINEWDTH;
      static const bool ANTIALIAS;
      static const int CYLRQ;
      static const int CYLRAD;
      static const float RESCYLRAD;
      static const int CAPSTYLE;
      static const int SPHERERQ;
      static const int SPHERESZ;
      static const int SPHERESTYLE;
      static const bool BLENDED;
      static const bool ROUNDED;
      static const float COILWDTH;
      static const float HLXWDTH;
      static const float SHTWDTH;
      static const float RBTHICK;
      static const float HLXRAD;
      static const float CRTNCYL;
      static const float CRTNTB;
      static const int CRTNCYLRQ;
      static const int CRTNAWTS;
      static const int CRTNTBTS;
      static const int CRTNTBSS;
      //static const string ATOMCOLOR;
      //static const string RESCOLOR;
      static const char *THREECLR[];
      static const int ATMSPHR;
      static const int BNDCYL;
      static const string BLOCKCOLOR;
      static const string SPECTRUMTYPE;

      DisplayDescriptor(const string& name, const string& style, 
            const string& scheme);
      DisplayDescriptor(const string& data);
      DisplayDescriptor(const DisplayDescriptor& rhs);
      DisplayDescriptor();
      virtual ~DisplayDescriptor();

      DisplayDescriptor& operator=(const DisplayDescriptor& rhs);
      DisplayDescriptor& operator=(const string& data);
      bool operator==(const DisplayDescriptor& rhs) const;
      bool operator==(const string& rhs) const;
      string toString() const;
      bool isValid(string display, string colorScheme, bool hasResidues);
      string getDefaultColorScheme(string display);
      static vector<string> getCSResidueOptions(string style,
	                                        bool hasBio = true);
      static vector<string> getCSNoResidueOptions();

      void load(const Preferences& prefs, const string& style);
      void save(Preferences& prefs);

      /**
       * Modifiers
       */
      void setName(const string& name);
      void setStyle(const string& style);
      void setColorScheme(const string& scheme);
      void setBlockColor(const string& color);

      void setLineWidth(const int& lnwdth);
      void setAntiAlias(bool antialias); 
      void setCylinderRadius(const int& radius);
      void setResidueCylinderRadius(const float& radius);
      void setCylinderRQ(const int& rq);
      void setCapStyle(const int& style);
      void setSphereSize(const int& size);
      void setSphereStyle(const int& flag);
      void setSphereRQ(const int& rq);
      void setRibbonBlended(const bool& flag);
      void setRibbonRounded(const bool& flag);
      void setCoilWidth(const float& wdth);
      void setHelixWidth(const float& wdth);
      void setSheetWidth(const float& wdth);
      void setRibbonThickness(const float& thck);
      void setHelixRadius(const float& rad);
      void setCartoonCylinderRadius(const float& radius);
      void setCartoonCylinderRQ(const int& rq);
      void setCartoonTubeRadius(const float& radius);
      void setAtomColorScheme(const string& opt);
      void setResidueColorScheme(const string& opt);
      void set3ColorStyle(const vector<string>& colors);
      void setAtomSphereOpt(const int& opt);
      void setBondCylinderOpt(const int& opt);
      void setSpectrumType(const string& type);


      /**
       * Accessors
       */
      string getName() const;
      string getStyle() const;
      string getColorScheme() const;
      string getBlockColor() const;

      int getLineWidth() const;
      bool getAntiAlias() const;
      int getCylinderRadius() const;
      float getResidueCylinderRadius() const;
      int getCylinderRQ() const;
      int getCapStyle() const;
      int getSphereSize() const;
      int getSphereStyle() const;
      int getSphereRQ() const;
      bool getRibbonBlended() const;
      bool getRibbonRounded() const;
      float getCoilWidth() const;
      float getHelixWidth() const;
      float getSheetWidth() const;
      float getRibbonThickness() const;
      float getHelixRadius() const;
      float getCartoonCylinderRadius() const;
      int getCartoonCylinderRQ() const;
      float getCartoonTubeRadius() const;
      string getAtomColorScheme() const;
      string getResidueColorScheme() const;
      vector<string> get3ColorStyle() const;
      int getAtomSphereOpt() const;
      int getBondCylinderOpt() const;
      string getSpectrumType() const;


      /**
       *  Overwrite << and >> operators so that DisplayDescriptor objects can be
       *  used with JMS communication layer
       */
      friend ostream& operator<<(ostream& os, const DisplayDescriptor& display);
      friend istream& operator>>(istream& is, DisplayDescriptor& display);

   protected:

      void constructor();

   private:

      string p_name;
      string p_style;
      string p_colorScheme;

      string p_blockColor;
      string p_spectrumType;

      int p_lineWidth;
      /** Cylinder/wire antialias didn't work too well but leave for now */
      int p_antiAlias;

      int p_cylRadius;
      float p_cylResRadius;
      int p_cylinderRQ;
      int p_capStyle;

      int p_sphereSize;
      int p_sphereStyle;
      int p_sphereRQ;

      bool p_ribbonBlend;
      bool p_ribbonRound;
      float p_coilWidth;
      float p_helixWidth;
      float p_sheetWidth;
      float p_ribbonThickness;
      float p_ribbonHelixRad;

      float p_cartoonCylRadius;
      int p_cartoonCylRQ;
      float p_cartoonTubeRadius;

      string p_atomColor;
      string p_residueColor;
      vector<string> p_3ColorStyle;

      int p_atomSphere;
      int p_bondCylinder;

};
#endif
