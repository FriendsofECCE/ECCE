/**
 * @file
 *
 *
 */
#ifndef _EWXUNITHELPER_H  
#define _EWXUNITHELPER_H  

#include <string>
using std::string;

class wxWindow;
class ewxTextCtrl;
class ewxStaticText;


/**
 * A ewxUnitHelper is a class that can be attached to a ewxTextControl
 * to help automate conversion of inputs to between families of units
 * at the users discretion.  For example, many people prefer the "SI" family
 * of units while NWChem uses what we've called the "CGS" family.
 * A given tool should be able to automatically change all of its inputs to
 * a given family.  To support this, a ewxUnitHelper object can be attached
 * to numeric input field.  Each input will have a type (energy, distance...)
 * and belong to a family.  A fields type cannot be changed but its family can.
 * Such a change should update the values as well as validation ranges and
 * units label (if any).
 */
class ewxUnitHelper 
{
   public:

      ewxUnitHelper(const string& family, const string& type,
            ewxStaticText *unitLabelWin=0, bool autoConvert=true);
      virtual ~ewxUnitHelper();

      void setTextControl(ewxTextCtrl *textCtrl);
      ewxTextCtrl *getTextControl();

      bool shouldAutoConvert() const;

      ewxStaticText *getLabel();

      void setFamily(const string& family);
      string getFamily() const;
      string getBaseFamily() const;

      string getUnitClass() const;

   protected:

      /** if true, the text widget should convert to base units. */
      bool           p_autoConvert;

      /** the base unit family - set at construction time. */
      string         p_baseFamily;

      /** the current unit family. */
      string         p_family;

      /** the type of units, Length, Temperature... see units design */
      string         p_type;

      /** associated label widget - may be 0. */
      ewxStaticText *p_labelWin;

      /** associated text widget - probably don't really need this. */
      ewxTextCtrl   *p_textWin;


};

#endif
