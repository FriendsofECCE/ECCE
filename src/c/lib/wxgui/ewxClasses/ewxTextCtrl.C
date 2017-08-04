/**
 * @file
 *
 *
 */

#include <iostream>
  using std::cout;
  using std::endl;

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/textctrl.h"

#include "util/BadDataException.H"
#include "util/StringConverter.H"
#include "util/UnitFactory.H"
#include "util/UnitFamily.H"
#include "util/UnitConverter.H"

#include "wxgui/ewxDisabler.H"
#include "wxgui/ewxTextCtrl.H"
#include "wxgui/ewxHelpHandler.H"
#include "wxgui/ewxUnitHelper.H"
#include "wxgui/ewxGenericValidator.H"
#include "wxgui/ewxNumericValidator.H"


/**
 * Default ctor.
 */
ewxTextCtrl::ewxTextCtrl()
  : wxTextCtrl(),
    ewxStyledWindow()
{

}


/**
 * Constructor.
 */
ewxTextCtrl::ewxTextCtrl(wxWindow* parent, wxWindowID id,
                         const wxString& value, const wxPoint& pos,
                         const wxSize& size, long style,
                         const wxValidator& validator, const wxString& name)
  : wxTextCtrl(),
    ewxStyledWindow()
{
   Create(parent, id, value, pos, size, style, validator, name);
}


/**
 * Two-part ctor.
 * Auto-validation via the mouse leave notify is not supported for multi-line
 * text fields.  I couldn't think of why we needed it and it caused the
 * following error report
 * Gtk-CRITICAL **: file gtkentry.c: line 535 (gtk_entry_get_text): assertion `GTK_IS_ENTRY (entry)' failed.
 */
bool ewxTextCtrl::Create(wxWindow* parent, wxWindowID id,
                         const wxString& value, const wxPoint& pos,
                         const wxSize& size, long style,
                         const wxValidator& validator, const wxString& name)
{
   if (!wxTextCtrl::Create(parent, id, value, pos, size, style,
                           validator, name)) {
      wxFAIL_MSG( wxT("ewxTextCtrl creation failed") );
      return false;
   }

   p_unitHelper = 0;
   p_disabler = 0;

   // MultiLine doesn't support auto event handling
   // Note: isSingleLine() etc didn't work
   if (((style & wxTE_MULTILINE) == 0)  && ((style & wxTE_READONLY) == 0)) {
      p_disabler = new ewxDisabler();
      PushEventHandler(p_disabler);
      PushEventHandler(new ewxHelpHandler(this));
   }

   setStyles(this, false);

   return true;
}


/**
 * Destructor.
 * If a unit helper has been assigned, it is deleted.
 */
ewxTextCtrl::~ewxTextCtrl()
{
  if (p_unitHelper != 0)
    delete p_unitHelper;
}


/**
 * Set the unit helper instance associated with this control.
 * Memory is managed by ewxTextCtrl - apps should not delete the helper.
 * UnitHelpers are not supported on Multi-Line text fields.
 */
void ewxTextCtrl::setUnitHelper(ewxUnitHelper *helper)
{
   // IsSingleLine didn't work
   // m_windowStyle inherited from wxTextCtrl
   if ((m_windowStyle & wxTE_MULTILINE) == 0) {
      if (p_unitHelper != 0) {
         delete p_unitHelper;
      }
      p_unitHelper = helper;
      helper->setTextControl(this);
   }
}


/**
 * Get the unit helper instance associated with this control.
 * Memory is managed by ewxTextCtrl - apps should not delete the helper.
 * @return helper or 0 pointer if none is defined.
 */
ewxUnitHelper *ewxTextCtrl::getUnitHelper() const
{
   return p_unitHelper;
}


/**
 *
 */
void ewxTextCtrl::setLeaveAsEnter(bool value)
{
  ewxGenericValidator *validator = 0;
  validator = dynamic_cast<ewxGenericValidator *>(GetValidator());
  if (validator) {
    validator->setLeaveAsEnter(value);
  }
}


/**
 * Overrides wxWindow::SetValidator.
 * Additional behavior is that the text fields default value is set from the
 * validators current value.
 */
void ewxTextCtrl::SetValidator(const wxValidator& validator)
{
   wxTextCtrl::SetValidator(validator);
   const ewxGenericValidator *gv =
      dynamic_cast<const ewxGenericValidator*>(&validator);

   if (gv) {
      SetValue(gv->getValue());
   }
}


/**
 *
 */
void ewxTextCtrl::setValueAsInt(int value)
{
   SetValue(StringConverter::toString(value));
}


/**
 * Return the string represented in the text control as an integer.
 *
 * @throw BadDataException when the string is not an integer
 */
int ewxTextCtrl::getValueAsInt()
{
   string valstr = GetValue().c_str();

   // nasty bug sometimes reports values as floats with .00000 appended
   // just conveniently lop those off before int conversion
   // this is related to a bug in the validation code that sets the
   // values as floats and this code can be removed if that issue is fixed
   size_t dot;
   if ((dot=valstr.find('.')) != string::npos)
     valstr = valstr.substr(0, dot);

   int value;
   if (!StringConverter::toInt(valstr, value)) {
     throw BadDataException("Unable to convert to integer", WHERE);
   }
   return value;
}


/**
 * Return the string represented by the base value as an integer.
 *
 * @throw BadDataException when the string is not an integer
 */
int ewxTextCtrl::getBaseValueAsInt()
{
   int value;
   if (!StringConverter::toInt(GetBaseValue().c_str(), value)) {
     throw BadDataException("Unable to convert to integer", WHERE);
   }
   return value;
}


/**
 *
 */
void ewxTextCtrl::setValueAsDouble(double value)
{
   char buf[120];
   if (fabs(value) < 1.0e-04 && fabs(value) > 0.0) {
     sprintf(buf,"%e", value);
   } else {
     sprintf(buf,"%lf", value);
   }
   SetValue(buf);
}


/**
 * Convert value from units family used for input to user specified
 * units. This function is usually used to convert values that have
 * been recieved from some other source (e.g. the current fragment)
 * that may not reflect the user selection.
 */
void ewxTextCtrl::setValueAsDouble(double value, string curFamilyName)
{
   char buf[120];
   if (p_unitHelper) {
     string userFamilyName = p_unitHelper->getBaseFamily();
     UnitFactory& uf = UnitFactory::getInstance();
     UnitFamily& userFamily = uf.getUnitFamily(userFamilyName);
     UnitFamily& curFamily = uf.getUnitFamily(curFamilyName);

     if (userFamilyName != curFamilyName) {
       string unitClass = p_unitHelper->getUnitClass();
       string from = userFamily.get(unitClass);
       string to = curFamily.get(unitClass);

       // Get converter & convert
       UnitConverter& uconv = uf.getUnitConverter(unitClass);
       uconv.setBaseUnits(from);
       value = uconv.convertTo(value, to);
     }
   }
   if (fabs(value) < 1.0e-04 && fabs(value) > 0.0) {
     sprintf(buf,"%e", value);
   } else {
     sprintf(buf,"%lf", value);
   }
   SetValue(buf);
}


/**
 * Return the string represented in the text control as a double.
 *
 * @throw BadDataException when the string is not a double
 */
double ewxTextCtrl::getValueAsDouble()
{
   double value;
   if (!StringConverter::toDouble(GetValue().c_str(), value)) {
     throw BadDataException("Unable to convert to double", WHERE);
   }
   return value;
}


/**
 * Return the string represented by the base value as a double.
 *
 * @throw BadDataException when the string is not a double
 */
double ewxTextCtrl::getBaseValueAsDouble()
{
   double value;
   if (!StringConverter::toDouble(GetBaseValue().c_str(), value)) {
     throw BadDataException("Unable to convert to double", WHERE);
   }
   return value;
}


/**
 * Utility function that guarantees that doubles are returned
 * as a string containing a decimal point
 */
string ewxTextCtrl::getBaseValueAsDoubleString()
{
   char buf[120];
   if (fabs(atof(GetBaseValue().c_str())) < 1.0e-04 &&
       fabs(atof(GetBaseValue().c_str())) > 0.0) {
     sprintf(buf,"%e",atof(GetBaseValue().c_str()));
   } else {
     sprintf(buf,"%lf",atof(GetBaseValue().c_str()));
   }
   string ret(buf);
   return ret;
}


/**
 * Converts the given base unit value to the current unit value and sets the
 * value of the widget.  If there is no unit helper, this method acts like
 * SetValue.
 */
void ewxTextCtrl::SetBaseValue(wxString value)
{
  ewxUnitHelper *helper = getUnitHelper();
  if (helper) {
    UnitFactory& uf = UnitFactory::getInstance();
    string baseFamilyName = helper->getBaseFamily();
    string curFamilyName = helper->getFamily();
    if (baseFamilyName != curFamilyName) {
      UnitFamily& baseFamily = uf.getUnitFamily(baseFamilyName);
      UnitFamily& curFamily = uf.getUnitFamily(curFamilyName);
      string unitClass = helper->getUnitClass();
      string baseUnit = baseFamily.get(unitClass);
      string curUnit = curFamily.get(unitClass);
      // Get converter and convert
      UnitConverter& uconv = uf.getUnitConverter(unitClass);
      uconv.setBaseUnits(baseUnit);
      value = uconv.convertTo(value.c_str(), curUnit);
    }
  }
  SetValue(value);
}


/**
 * Get the value of the widget converted to the base unit as
 * defined when the ewxUnitHelper was created.  If there is no unit
 * helper, this method acts like GetValue.
 */
wxString ewxTextCtrl::GetBaseValue()
{
   wxString value = GetValue();

   ewxUnitHelper *helper = getUnitHelper();
   if (helper) {

      UnitFactory& uf = UnitFactory::getInstance();
      string baseFamilyName = helper->getBaseFamily();
      string curFamilyName = helper->getFamily();
      UnitFamily& curFamily = uf.getUnitFamily(baseFamilyName);
      UnitFamily& baseFamily = uf.getUnitFamily(curFamilyName);

      if (baseFamilyName != curFamilyName) {
         string unitClass = helper->getUnitClass();
         string to = baseFamily.get(unitClass);
         string from = curFamily.get(unitClass);

         // Get converter & convert
         UnitConverter& uconv = uf.getUnitConverter(unitClass);
         uconv.setBaseUnits(to);
         value = uconv.convertTo(value.c_str(), from).c_str();
      }
   }
   return value;
}


/**
 * Set widget to our custom disabled style.
 * @param enabled true to enable, false to disable
 */
void ewxTextCtrl:: setCustomDisabledStyle(bool enabled)
{
   if (p_disabler) p_disabler->setEnabled(enabled);
   SetEditable(enabled);
   setStyles(this,false);
}
