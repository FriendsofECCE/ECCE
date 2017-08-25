/**
 * @file
 *
 * Superclass for all ecce styled wxWindow subclasses.
 * Through multiple inheritance, set object styles at creation time.
 * including wx dialogs (non-ecce dialogs) and their child widgets.
 * This is the main reason for using this design with a single setStyles
 * method.
 *
 */

#ifndef _EWXSTYLED_WINDOW_H  
#define _EWXSTYLED_WINDOW_H  

#include "wx/dcclient.h"

class Preferences;

class ewxStyledWindow
{
   public:
      virtual ~ewxStyledWindow();

      static wxColour getWindowColor();
      static wxColour getReadonlyColor();
      static wxColour getButtonColor();
      static wxColour getInputColor();
      static wxColour getTextColor();
      static wxColour getBtn3DDkShadowColor();
      static wxColour getBtn3DDkLightColor();
      static wxColour getFocusedSelectionColor();
      static wxColour getUnfocusedSelectionColor();
      static wxColour getHightLightRowColor();
      static wxFont   getBoldFont();
      static wxFont   getNormalFont();
      static wxFont   getUnitFont();
      static wxFont   getMonoSpaceFont();
      static wxFont   getAtomicNumFont();
      static wxFont   getBigAtomicSymbolFont();
      static wxFont   getSmallLabelFont();

      virtual void setCustomDisabledStyle(bool disabled);

      static void drawButtonBorder(wxPaintDC & dc, bool isSunken, int width=2);

   protected:
      ewxStyledWindow();
      ewxStyledWindow(wxWindow *win, bool recursive=true);
      void setStyles(wxWindow *win, bool recursive=true);
      void setChildStyles(wxWindow *win);
      static int getFontSize(int basesize);

      static Preferences *p_prefs;

};

#endif
