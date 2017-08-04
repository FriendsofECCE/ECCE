/**
 * @file
 *
 *
 */

#include "wx/wxprec.h"


#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxStaticText.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxStyledWindow.H"
#include "util/UnitFamily.H"


/**
 * Create a helper class for managing units for a text control.
 * @param family the unit family - SI...
 * @param type type of unit - Energy, Angle...
 * @param unitLabelWid - optional window used to show unit label
 * @param autoConvert - should text widget GetValue conver to base units
 */
ewxUnitHelper::ewxUnitHelper(const string& family, const string& type,
                             ewxStaticText *unitLabelWin, bool autoConvert)
{

   p_autoConvert = autoConvert;
   p_baseFamily = p_family = family;
   p_type = type;
   p_labelWin = unitLabelWin;
   // Convert label to units font
   if (p_labelWin) {
     p_labelWin->SetFont(ewxStyledWindow::getUnitFont());
   }
   p_textWin = 0;
}

ewxUnitHelper::~ewxUnitHelper()
{
   p_textWin = 0;
   p_labelWin = 0;
}



void ewxUnitHelper::setFamily(const string& family)
{
   p_family = family;
}


string ewxUnitHelper::getFamily() const
{
   return p_family;
}


string ewxUnitHelper::getBaseFamily() const
{
   return p_baseFamily;
}


bool ewxUnitHelper::shouldAutoConvert() const
{
   return p_autoConvert;
}



string ewxUnitHelper::getUnitClass() const
{
   return p_type;
}


void ewxUnitHelper::setTextControl(ewxTextCtrl *textCtrl)
{
   p_textWin = textCtrl;
}

ewxTextCtrl *ewxUnitHelper::getTextControl()
{
   return p_textWin;
}


ewxStaticText *ewxUnitHelper::getLabel()
{
   return p_labelWin;
}

