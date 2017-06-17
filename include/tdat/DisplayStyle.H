#ifndef DISPLAYSTYLE_HH
#define DISPLAYSTYLE_HH

#include <string>
  using std::string;
#include <vector>
  using std::vector;

/**
 * Class to encapsulate the value of a display style.
 * The style is independent of whether or not the item is displayed.
 */
class DisplayStyle 
{

   public:
      enum Style { STICK, BALLSTICK, BALLWIRE, WIRE, DNONE, CPK,
         CAWIRE, CYLINDER, LINE, FLAT, SOLID, CARTOON, INVALIDSTYLE };

      DisplayStyle(DisplayStyle::Style style = DisplayStyle::BALLWIRE);
      virtual ~DisplayStyle();

      void setName(const string& name);
      string getName() const;

      void setStyle(DisplayStyle::Style style);
      DisplayStyle::Style getStyle() const;

      void setDisplayed(bool displayed);
      bool isDisplayed() const;

      DisplayStyle& operator=(const DisplayStyle& style);

      static vector<string> getAllNames(bool includeNone=true);
      static vector<string> getNoResidueNames(bool includeNone=true);
      static DisplayStyle::Style toStyle(const string& name);
      static string fromStyle(DisplayStyle::Style style);


   protected:

      DisplayStyle::Style p_style;
      bool                p_displayFlag;

      /** A name that uniquely identifies a style. */
      string              p_name;


};

#endif

