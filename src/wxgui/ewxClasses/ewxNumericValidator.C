#include <wx/log.h>

#include "util/STLUtil.H"
#include "util/StringConverter.H"
#include "util/StringTokenizer.H"
#include "wxgui/ewxNumericValidator.H"
#include "wxgui/WxFeedback.H"

//#define DEBUG

IMPLEMENT_DYNAMIC_CLASS(ewxNumericValidator, ewxGenericValidator)


BEGIN_EVENT_TABLE(ewxNumericValidator, ewxGenericValidator)
    EVT_CHAR(ewxNumericValidator::onChar)
END_EVENT_TABLE()


/**
 * Assumes hard double precision range [0..1].
 * No value is assigned, so "empty is okay."
 */
ewxNumericValidator::ewxNumericValidator()
  : ewxGenericValidator()
  , p_isInt(false)
  , p_emptyOk(false)
  , p_useHardRange(false)
  , p_useHardMin(false)
  , p_useHardMax(false)
  , p_hardMin(0)
  , p_hardMax(0)
  , p_hardMinOpen(false)
  , p_hardMaxOpen(false)
  , p_useSoftRange(false)
  , p_useSoftMin(false)
  , p_useSoftMax(false)
  , p_softMin(0)
  , p_softMax(0)
  , p_softMinOpen(false)
  , p_softMaxOpen(false)
{
}


/**
 * Constructor for double-precision, hard-ranged number entry.
 * No value is assigned, so "empty is okay."
 */
ewxNumericValidator::ewxNumericValidator(double min, double max)
  : ewxGenericValidator()
{
  p_emptyOk = true;
  setHardRange(min,max);
  initSoftRange();
}


/**
 * Constructor for double-precision, hard-ranged number entry.
 */
ewxNumericValidator::ewxNumericValidator(double min, double max, double val)
  :ewxGenericValidator(val)
{
   p_emptyOk = false;
   setHardRange(min,max);
   initSoftRange();
}


/**
 * Constructor for double-precision, hard-ranged number entry.
 * Value is a pointer.
 */
ewxNumericValidator::ewxNumericValidator(double min, double max, double * val)
  :ewxGenericValidator(val)
{
   p_emptyOk = false;
   setHardRange(min,max);
   initSoftRange();
}


/**
 * Constructor for double-precision, hard-ranged number entry.
 */
ewxNumericValidator::ewxNumericValidator(double min, double max,
                                         const string& val)
  :ewxGenericValidator()
{
   setHardRange(min,max);
   initSoftRange();

   if (val.empty()) {
     p_emptyOk = true;
   } else {
     double dval;
     if (StringConverter::toDouble(val,dval)) {
       setValue(dval);
     }
   }
}


/**
 * Constructor for integer-precision, hard-ranged number entry.
 * No value is assigned, so "empty is okay."
 */
ewxNumericValidator::ewxNumericValidator(int min, int max)
  :ewxGenericValidator()
{
   p_emptyOk = true;
   setHardRange(min,max);
   initSoftRange();
}


/**
 * Constructor for integer-precision, hard-ranged number entry.
 */
ewxNumericValidator::ewxNumericValidator(int min, int max, int val)
  :ewxGenericValidator(val)
{
   p_emptyOk = false;
   setHardRange(min,max);
   initSoftRange();
}


/**
 * Constructor for integer-precision, hard-ranged number entry.
 * Value is a pointer
 */
ewxNumericValidator::ewxNumericValidator(int min, int max, int * val)
  :ewxGenericValidator(val)
{
   p_emptyOk = false;
   setHardRange(min,max);
   initSoftRange();
}


/**
 * Constructor for integer-precision, hard-ranged number entry.
 */
ewxNumericValidator::ewxNumericValidator(int min, int max, const string& val)
  :ewxGenericValidator()
{
   setHardRange(min,max);
   initSoftRange();

   if (val.empty()) {
     p_emptyOk = true;
   } else {
     p_emptyOk = false;
     int ival;
     if (StringConverter::toInt(val,ival)) {
       setValue(ival);
     }
   }
}


/**
 * Copy constructor.
 */
ewxNumericValidator::ewxNumericValidator(const ewxNumericValidator& val)
    : ewxGenericValidator(val)
{
    Copy(val);
}


/**
 * Destructor does nothing.
 */
ewxNumericValidator::~ewxNumericValidator()
{
}


ewxNumericValidator& ewxNumericValidator::HardMin(int val)
{
  p_useHardRange = true;
  p_useHardMin = true;
  p_hardMin = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::HardMax(int val)
{
  p_useHardRange = true;
  p_useHardMax = true;
  p_hardMax = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::SoftMin(int val)
{
  p_useSoftRange = true;
  p_useSoftMin = true;
  p_softMin = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::SoftMax(int val)
{
  p_useSoftRange = true;
  p_useSoftMax = true;
  p_softMax = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::Value(int val)
{
  ewxGenericValidator::setValue(val);
  return *this;
}


ewxNumericValidator& ewxNumericValidator::HardMin(double val)
{
  p_useHardRange = true;
  p_useHardMin = true;
  p_hardMin = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::HardMax(double val)
{
  p_useHardRange = true;
  p_useHardMax = true;
  p_hardMax = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::SoftMin(double val)
{
  p_useSoftRange = true;
  p_useSoftMin = true;
  p_softMin = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::SoftMax(double val)
{
  p_useSoftRange = true;
  p_useSoftMax = true;
  p_softMax = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::Value(double val)
{
  ewxGenericValidator::setValue(val);
  return *this;
}


ewxNumericValidator& ewxNumericValidator::HardMinOpen(bool val)
{
  p_hardMinOpen = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::HardMaxOpen(bool val)
{
  p_hardMaxOpen = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::SoftMinOpen(bool val)
{
  p_softMinOpen = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::SoftMaxOpen(bool val)
{
  p_softMaxOpen = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::AllowEmpty(bool val)
{
  p_emptyOk = val;
  return *this;
}


ewxNumericValidator& ewxNumericValidator::IsInt(bool val)
{
  p_isInt = val;
  return *this;
}


void ewxNumericValidator::setValue(int* val)
{
  p_isInt = true;
  ewxGenericValidator::setValue(val);
}


void ewxNumericValidator::setValue(double* val)
{
  p_isInt = false;
  ewxGenericValidator::setValue(val);
}


void ewxNumericValidator::setValue(int val)
{
  p_isInt = true;
  ewxGenericValidator::setValue(val);
}


void ewxNumericValidator::setValue(double val)
{
  p_isInt = false;
  ewxGenericValidator::setValue(val);
}


/**
 * Copy always returns true?
 */
bool ewxNumericValidator::Copy(const ewxNumericValidator& val)
{
    bool ret = true;

    ret &= ewxGenericValidator::Copy(val);

    p_isInt = val.p_isInt;
    p_emptyOk = val.p_emptyOk;

    p_useHardRange = val.p_useHardRange;
    p_useHardMin = val.p_useHardMin;
    p_useHardMax = val.p_useHardMax;
    p_hardMin = val.p_hardMin;
    p_hardMax = val.p_hardMax;
    p_hardMinOpen = val.p_hardMinOpen;
    p_hardMaxOpen = val.p_hardMaxOpen;

    p_useSoftRange = val.p_useSoftRange;
    p_useSoftMin = val.p_useSoftMin;
    p_useSoftMax = val.p_useSoftMax;
    p_softMin = val.p_softMin;
    p_softMax = val.p_softMax;
    p_softMinOpen = val.p_softMinOpen;
    p_softMaxOpen = val.p_softMaxOpen;

    return ret;
}


/**
 * Retrieve the double-precision range of the validator.
 */
void ewxNumericValidator::getHardRange(double& min, double& max)
{
   min = p_hardMin;
   max = p_hardMax;
}


/**
 * Retrieve the integer-precision range of the validator.
 */
void ewxNumericValidator::getHardRange(int& min, int& max)
{
   min = static_cast<int>(p_hardMin);
   max = static_cast<int>(p_hardMax);
}


void ewxNumericValidator::getSoftRange(double& min, double& max)
{
   min = p_softMin;
   max = p_softMax;
}


void ewxNumericValidator::getSoftRange(int& min, int& max)
{
   min = static_cast<int>(p_softMin);
   max = static_cast<int>(p_softMax);
}


/**
 * Set the double-precision hard range of the validator.
 */
void ewxNumericValidator::setHardRange(const double& min, const double& max)
{
   p_isInt = false;
   p_useHardRange = true;
   p_useHardMin = true;
   p_useHardMax = true;
   p_hardMin = min;
   p_hardMax = max;
   p_hardMinOpen = false;
   p_hardMaxOpen = false;
}


/**
 * Set the integer-precision hard range of the validator.
 */
void ewxNumericValidator::setHardRange(const int& min, const int& max)
{
   p_isInt = true;
   p_useHardRange = true;
   p_useHardMin = true;
   p_useHardMax = true;
   p_hardMin = min;
   p_hardMax = max;
   p_hardMinOpen = false;
   p_hardMaxOpen = false;
}


/**
 * Besides explicitly defining a hard range with numeric values, a string
 * can also be used.
 * ex) [1..10], (-12.13333, 2.321]
 */
void ewxNumericValidator::setHardRange(string range)
{
  parseRange(range, true);
}


/**
 * Set the double-precision soft range of the validator.
 */
void ewxNumericValidator::setSoftRange(const double& min, const double& max)
{
   p_isInt = false;
   p_useSoftRange = true;
   p_useSoftMin = true;
   p_useSoftMax = true;
   p_softMin = min;
   p_softMax = max;
   p_softMinOpen = false;
   p_softMaxOpen = false;
}


/**
 * Set the integer-precision soft range of the validator.
 */
void ewxNumericValidator::setSoftRange(const int& min, const int& max)
{
   p_isInt = true;
   p_useSoftRange = true;
   p_useSoftMin = true;
   p_useSoftMax = true;
   p_softMin = min;
   p_softMax = max;
   p_softMinOpen = false;
   p_softMaxOpen = false;
}


/**
 * Besides explicitly defining a soft range with numeric values, a string
 * can also be used.
 * ex) [1..10], (-12.13333, 2.321]
 */
void ewxNumericValidator::setSoftRange(string range)
{
  parseRange(range, false);
}


/**
 * Called when the value in the window must be validated.
 * This function can pop up an error message.
 */
bool ewxNumericValidator::Validate(wxWindow *parent)
{
    if( !CheckValidator() )
        return false;

    // If window is disabled, simply return
    if ( !m_validatorWindow->IsEnabled() )
        return true;

    // get the value in the window, return if failed
    wxString val = getControlValue();

    // NB: this format string should contian exactly one '%s'
    wxString errormsg = "'" + val + "' is invalid";

    bool isHard = true;
    try {
      if (val.empty() && !p_emptyOk) {
        throw InvalidException(errormsg, __FILE__, __LINE__);
      } else {
        if (p_useHardRange) {
          check(val, isHard, p_useHardMin, p_useHardMax,
                p_hardMinOpen, p_hardMaxOpen, p_hardMin, p_hardMax);
        }
        if (p_useSoftRange) {
          isHard = false;
          check(val, isHard, p_useSoftMin, p_useSoftMax,
                p_softMinOpen, p_softMaxOpen, p_softMin, p_softMax);
        }
      }
    } catch (InvalidException ex) {
      if (isHard) {
        wxBell();
        postMessage(errormsg.c_str());
        return afterValidation(false);
      }
      else {
        postMessage(errormsg.c_str());
        return afterValidation(true);
      }
    }

    return afterValidation(true);
}


void ewxNumericValidator::dump()
{
  cout<< "p_isInt:        "<<p_isInt<<endl;
  cout<< "p_emptyOk:      "<<p_emptyOk<<endl;
  cout<< "current Value:  "<<getValue()<<endl;

  cout<< "p_useHardRange: "<<p_useHardRange<<endl;
  cout<< "p_hardMin:      "<<p_hardMin<<endl;
  cout<< "p_hardMax:      "<<p_hardMax<<endl;
  cout<< "p_hardMinOpen:  "<<p_hardMinOpen<<endl;
  cout<< "p_hardMaxOpen:  "<<p_hardMaxOpen<<endl;
  cout<< "p_useHardMin:   "<<p_useHardMin<<endl;
  cout<< "p_useHardMax:   "<<p_useHardMax<<endl;

  cout<< "p_useSoftRange: "<<p_useSoftRange<<endl;
  cout<< "p_softMin:      "<<p_softMin<<endl;
  cout<< "p_softMax:      "<<p_softMax<<endl;
  cout<< "p_softMinOpen:  "<<p_softMinOpen<<endl;
  cout<< "p_softMaxOpen:  "<<p_softMaxOpen<<endl;
  cout<< "p_useSoftMin:   "<<p_useSoftMin<<endl;
  cout<< "p_useSoftMax:   "<<p_useSoftMax<<endl;
}


/**
 * Event handler for a user key stroke.
 * We validate the character and that the number is within range.
 */
void ewxNumericValidator::onChar(wxKeyEvent& event)
{
  int keyCode = event.GetKeyCode();

  if (isValidKey(keyCode)) {
    // do something with it?
  } else { 
     if ( !wxValidator::IsSilent() )
        wxBell();

     // eat message
     return;
  }

   event.Skip();
}


/**
 * Returns true if the given keycode is a digit, exponent (when double),
 * or sign.
 */
bool ewxNumericValidator::isValidKey(int keyCode) const
{
  bool ret = false;

  // if the key is a control key, just let it fall through
  if (keyCode == WXK_BACK || keyCode == WXK_TAB || keyCode == WXK_RETURN ||
      keyCode == WXK_ESCAPE || keyCode == WXK_SPACE || keyCode == WXK_DELETE ||
      keyCode == WXK_LEFT || keyCode == WXK_RIGHT || 
      keyCode == WXK_DOWN || keyCode == WXK_UP)
  {
    ret = true;
  } else {
    if (p_isInt) {
      ret = wxIsdigit(keyCode) || 
        keyCode == wxT('+') || keyCode == wxT('-');
    } else {
      ret = wxIsdigit(keyCode) || 
        keyCode == wxT('.') || 
        keyCode == wxT('E') || keyCode == wxT('e') || 
        keyCode == wxT('+') || keyCode == wxT('-');
    }
  }

  return ret;
}


/**
 *
 */
bool ewxNumericValidator::CheckValidator() const
{
  wxCHECK_MSG( m_validatorWindow, FALSE,
               _T("No window associated with validator") );
  /* This numeric validator should be able to be used on non-text fields
   * as well.
  wxCHECK_MSG( m_validatorWindow->IsKindOf(CLASSINFO(wxTextCtrl)), FALSE,
               _T("wxTextValidator is only for wxTextCtrl's") );
  wxCHECK_MSG( m_pString, FALSE,
               _T("No variable storage for validator") );
  */

  return TRUE;
}


void ewxNumericValidator::parseRange(string range, bool isHard)
{
  try {

    STLUtil::stripLeadingAndTrailingWhiteSpace(range);
    if (range.empty()) {
      throw InvalidException("empty range", __FILE__, __LINE__);
    }

    double min, max;
    int ival, size;
    bool isInt, minOpen, maxOpen, useMin, useMax;
    string sval;
    string msg = "bad range string";

    STLUtil::toLower(range);
    StringTokenizer tokenizer(range, ",");
    vector<string> tokens = tokenizer.tokenize(",");
    if (tokens.size() != 2) {
      throw InvalidException(msg, __FILE__, __LINE__);
    }

    string lower = tokens[0];
    string upper = tokens[1];

    // Figure out lower limit
    if (lower[0] == '[') {
      minOpen = false;
    } else if (lower[0] == '(') {
      minOpen = true;
    } else {
      throw InvalidException(msg, __FILE__, __LINE__);
    }
    lower = lower.substr(1);
    if (!lower.empty()) {
      useMin = true;
      if (StringConverter::toInt(lower, ival)) {
        isInt = true;
        min = ival;
      } else if (StringConverter::toDouble(lower, min)) {
        isInt = false;
      } else {
        throw InvalidException(msg, __FILE__, __LINE__);
      }
    } else {
      useMin = false;
    }

    // Figure out upper limit
    size = upper.size();
    if (upper[size-1] == ']') {
      maxOpen = false;
    } else if (upper[size-1] == ')') {
      maxOpen = true;
    } else {
      throw InvalidException(msg, __FILE__, __LINE__);
    }
    upper = upper.substr(0, size-1);
    if (!upper.empty()) {
      useMax = true;
      if (isInt) {
        if (StringConverter::toInt(upper, ival)) {
          max = ival;
        } else {
          throw InvalidException(msg, __FILE__, __LINE__);
        }
      } else if (!StringConverter::toDouble(upper, max)) {
        throw InvalidException(msg, __FILE__, __LINE__);
      }
    } else {
      useMax = false;
    }

    if (!useMin && !useMax) {
      isInt = false;
    }

    p_isInt = isInt;
    if (isHard) {
      p_useHardRange = true;
      p_hardMin = min;
      p_hardMax = max;
      p_hardMinOpen = minOpen;
      p_hardMaxOpen = maxOpen;
      p_useHardMin = useMin;
      p_useHardMax = useMax;
#ifdef DEBUG
      cout << "Setting Hard Min to "<<p_hardMin<<endl;
      cout << "Setting Hard Max to "<<p_hardMax<<endl;
      cout << "Hard min isOpen "<<p_hardMinOpen<<endl;
      cout << "Hard max isOpen "<<p_hardMaxOpen<<endl;
      cout << "Hard has no min "<<p_useHardMin<<endl;
      cout << "Hard has no max "<<p_useHardMax<<endl;
#endif
    } else {
      p_useSoftRange = true;
      p_softMin = min;
      p_softMax = max;
      p_softMinOpen = minOpen;
      p_softMaxOpen = maxOpen;
      p_useSoftMin = useMin;
      p_useSoftMax = useMax;
#ifdef DEBUG
      cout << "Setting Soft Min to "<<p_softMin<<endl;
      cout << "Setting Soft Max to "<<p_softMax<<endl;
      cout << "Soft min isOpen "<<p_softMinOpen<<endl;
      cout << "Soft max isOpen "<<p_softMaxOpen<<endl;
      cout << "Soft has no min "<<p_useSoftMin<<endl;
      cout << "Soft has no max "<<p_useSoftMax<<endl;
#endif
    }
  } catch (InvalidException ex) {
    if (isHard) {
      p_useHardRange = false;
    } else {
      p_useSoftRange = false;
    }
  }
}


/**
 * Check the given value against the validator's hard range.
 */
void ewxNumericValidator::check(const wxString& valString, bool isHard,
                                bool useMin, bool useMax,
                                bool minOpen, bool maxOpen,
                                double min, double max) throw (InvalidException)
{
  int tmp;
  double val;

  if (StringConverter::toInt(valString.c_str(), tmp)) {
    val = tmp;
  } else if (!StringConverter::toDouble(valString.c_str(), val)) {
    throw InvalidException("Invalid string entered", __FILE__, __LINE__);
  }

  string msg;
  if (isHard) {
    msg = "Value outside allowed range";
  } else {
    msg = "Value outside recommended range";
  }

  if (useMin) {
    if (minOpen) {
      if (val <= min) {
        throw InvalidException(msg, __FILE__, __LINE__);
      }
    } else {
      if (val < min) {
        throw InvalidException(msg, __FILE__, __LINE__);
      }
    }
  }
  if (useMax) {
    if (maxOpen) {
      if (val >= max) {
        throw InvalidException(msg, __FILE__, __LINE__);
      }
    } else {
      if (val > max) {
        throw InvalidException(msg, __FILE__, __LINE__);
      }
    }
  }
}


void ewxNumericValidator::initHardRange()
{
   p_useHardRange = true;
   p_hardMin = 0.;
   p_hardMax = 1.;
   p_hardMinOpen = false;
   p_hardMaxOpen = false;
   p_useHardMin = false;
   p_useHardMax = false;
}


void ewxNumericValidator::initSoftRange()
{
   p_useSoftRange = false;
   p_softMin = 0.0;
   p_softMax = 0.0;
   p_softMinOpen = false;
   p_softMaxOpen = false;
   p_useSoftMin = false;
   p_useSoftMax = false;
}


void ewxNumericValidator::postMessage(const string& msg)
{
     WxFeedback *feedback = WxFeedback::getFeedback(
                                           (wxWindow*)m_validatorWindow);
     if (feedback) feedback->setMessage(msg);
     // GDB 3/4/12  Added 0 arg to suppress compiler warning
     else wxLogMessage(msg.c_str(),0);
}

