/**
 * @file 
 *
 *
 */
#ifndef COLOR_HH
#define COLOR_HH

#include <iostream>
  using std::ostream;
  using std::istream;

#include <string>
  using std::string;


/**
 * This class provides the user to access to color representations.
 * Currently, only RGB representation is supported.
 * Other representations and conversions will be added upon demand.
 *
 * The RGB implementation enforces a valid range of
 * 0.0 - 1.0 (inclusive) for each RGB value.  Out of range values
 * result in a reset to 0.0.
 *
 *  This class also contains definitions for ecce system colors as
 *  string definitions.
 *
 * EXAMPLES:
 *    Color c(0.5, 0.0, 1.0);
 *    cout << c;
 *    Color cc = c;
 *    cc.setRGB(1.0, .4, .7);
 *    cin >> c;
 */
class Color 
{
   public:

      static const char* WINDOW;
      static const char* MENUBAR;
      static const char* NOTAPPFG;
      static const char* READONLY;
      static const char* TEXT;
      static const char* TOGGLE;
      static const char* INPUT;
      static const char* BUTTON;
      static const char* SHADOW;
      static const char* HIGHLIGHTBG;
      static const char* HIGHLIGHTFG;
      static const char* FOREGROUND;
      static const char* WARNING;
      static const char* ERROR;
      static const char* CAPTION;
      static const char* CAPTION_GRADIENT;

      // Since we are having problems with our color table, define top and
      // bottom shadow colors that are NOT computed from widget colors
      static const char* TOPSHADOW;
      static const char* BOTTOMSHADOW;

      // Shows current mode via border color
      static const char* CURRENTMODE;

      static const char* PUSHEDINBTN;
      static const char* INVALIDFG;

      // The topology editor
      static const char* NEWITEM;
      static const char* MULTIPLYDEFINEDITEM;
      static const char* INCOMPLETE;

      // MO graph colors
      static const char* MOGRAPH1;
      static const char* MOGRAPH2;
      static const char* MOGRAPH3;

      // Pos/Neg colors
      static const char* NEUTRAL;
      static const char* POSITIVE;
      static const char* NEGATIVE;


      static void initialize();
      static void setTheme(const string& theme);

      Color();
      Color(const Color& color);
      Color(float red, float green, float blue);
      Color(unsigned char red, unsigned char green, unsigned char blue);
      Color(const string& color);  // Allow format "#ffffff"

      virtual ~Color(void);

      Color& operator=(const Color& color);
      bool operator==(const Color& color) const;
      bool operator!=(const Color& color) const;

      void setRGB(float red, float green, float blue);

      float red() const;
      float green() const;
      float blue() const;
      string xcolor() const;
      void getRGB(float& red, float& green, float& blue) const;



   protected:


   private:

      float p_red;
      float p_green;
      float p_blue;

      static float HIGHCOLOR;                // upper bound on color values
      static float LOWCOLOR;                 // lower bound on color values

      void validate_color(void);             // check and reset color ranges

      friend ostream& operator<<(ostream& os, const Color& color);
      friend istream& operator>>(istream& is, Color& color);

}; 

#endif // COLOR_HH
